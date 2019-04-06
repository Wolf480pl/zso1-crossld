#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

#include "crossld.h"
#include "trampolines.h"

const size_t PAGE_SIZE = 4096;

int crossld_start_fun(char *start, const struct function *funcs, int nfuncs,
                      uint32_t *patch_ptr, size_t funidx) {
    void* trampolines[nfuncs];

    void* common_hunks = crossld_generate_trampolines(trampolines, funcs, nfuncs);
    if (common_hunks == NULL) {
        return -1;
    }

    size_t hunk_ptr = (size_t) trampolines[funidx];

    printf("putting: %x as trampoline ptr at %zx\n", (uint32_t) hunk_ptr, patch_ptr);
    *patch_ptr = (uint32_t) hunk_ptr;

    return crossld_enter(start, common_hunks);
}

const char valid_elf_ident[EI_NIDENT] = {
    ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
    ELFCLASS32,
    ELFDATA2LSB,
    EV_CURRENT,
//    ELFOSABI_SYSV,
//    0, // ABI version
};

static void *load_elf(const char *fname, void * const *trampolines,
                      const struct function *funcs, int nfuncs) {

    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    Elf32_Ehdr elfhdr;
    if (pread(fd, &elfhdr, sizeof(elfhdr), 0) < 0) {
        perror("pread");
        return NULL;
    }

    if (memcmp(&elfhdr.e_ident, valid_elf_ident, sizeof(valid_elf_ident)) != 0) {
        fprintf(stderr, "bad ELF header\n");
        return NULL;
    }

    if (elfhdr.e_type != ET_EXEC && elfhdr.e_type != ET_DYN) {
        fprintf(stderr, "bad ELF type - not ET_EXEC or ET_DYN");
        return NULL;
    }

    if (elfhdr.e_machine != EM_386) {
        fprintf(stderr, "bad ELF machine - not i386");
        return NULL;
    }

    if (elfhdr.e_phoff == 0 || elfhdr.e_phnum == 0) {
        fprintf(stderr, "no program headers in ELF");
        return NULL;        
    }

    if (elfhdr.e_phnum == PN_XNUM) {
        fprintf(stderr, "too many headers in ELF");
        return NULL;                
    }

    if (elfhdr.e_phentsize != sizeof(Elf32_Phdr)) {
        fprintf(stderr, "weird ELF program header size: %hu, expected %zu\n",
                         elfhdr.e_phentsize, sizeof(Elf32_Phdr));
        return NULL;                        
    }

    Elf32_Phdr phdrs[elfhdr.e_phnum];
    if (pread(fd, phdrs, sizeof(phdrs), elfhdr.e_phoff) < 0) {
        perror("pread");
        return NULL;
    }

#ifdef FAKE
    uint32_t* crossld_call64_in_fake_ptr_ptr;
#endif

    for (size_t i = 0; i < elfhdr.e_phnum; ++i) {
        printf("hdr %zu\n", i);
        int prot = 0;
        Elf32_Phdr* hdr = &phdrs[i];
        switch (hdr->p_type) {
        case PT_LOAD:
            if (hdr->p_flags & PF_R) {
                prot |= PROT_READ;
            }
            if (hdr->p_flags & PF_W) {
                prot |= PROT_WRITE;
            }
            if (hdr->p_flags & PF_X) {
                prot |= PROT_EXEC;
            }
            if (hdr->p_filesz > hdr->p_memsz) {
                fprintf(stderr, "segment with filesize greater than memsize:"
                                "%u > %u\n", hdr->p_filesz, hdr->p_memsz);
                return NULL;
            }
            if (hdr->p_filesz < hdr->p_memsz) {
                fprintf(stderr, "segment with trailing zeros (unimplemented,"
                                " skipping):"
                                "%u > %u\n", hdr->p_filesz, hdr->p_memsz);
                continue;
            }
            printf("LOAD %zx %zx %zx %zx %zu\n", hdr->p_vaddr, hdr->p_memsz,
                    hdr->p_offset, hdr->p_filesz, prot);
            size_t page_offset = hdr->p_vaddr & (PAGE_SIZE - 1);
            size_t vaddr = hdr->p_vaddr - page_offset;
            size_t offset = hdr->p_offset - page_offset;
            size_t size = hdr->p_filesz + page_offset;
            size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            printf("mmap %zx %zx %zx %zx %zu\n", vaddr, size,
                    offset, size, prot);
            void *addr = mmap((void*) vaddr, size, prot,
                              MAP_PRIVATE | MAP_32BIT, fd, offset);
            if (addr == MAP_FAILED) {
                perror("mmap");
                return NULL;
            }
            if ((size_t) addr != vaddr) {
                perror("mmap put us somewhere else");
                printf("ERROR: mmap moved us to %zx\n", addr);
                if (munmap(addr, size) < 0) {
                    perror("munmap");
                }
                return NULL;
            }
#ifdef FAKE
            //TMPHACK
            crossld_call64_in_fake_ptr_ptr = addr + page_offset;
#endif
            break;

        case PT_DYNAMIC: {
            size_t dyncnt = hdr->p_filesz / sizeof(Elf32_Dyn);
            Elf32_Dyn dyns[dyncnt];
            if (pread(fd, &dyns, sizeof(dyns), hdr->p_offset) < 0) {
                perror("pread dynamic");
                return NULL;
            }

            char* strtab = NULL;
            Elf32_Sym* symtab = NULL;
            void* jmprel = NULL;
            size_t pltrelsz = 0;

            for (size_t j = 0; j < dyncnt; ++j) {
                Elf32_Dyn* dyn = &dyns[j];
                switch (dyn->d_tag) {
                    case DT_STRTAB:
                        strtab = (void*) (uint64_t) dyn->d_un.d_ptr;
                        break;
                    case DT_SYMTAB:
                        symtab = (void*) (uint64_t) dyn->d_un.d_ptr;
                        break;
                    case DT_PLTRELSZ:
                        pltrelsz = dyn->d_un.d_val;
                        break;
                    case DT_JMPREL:
                        jmprel = (void*) (uint64_t) dyn->d_un.d_ptr;
                        break;
                    case DT_PLTREL:
                        if (dyn->d_un.d_val != DT_REL) {
                            fprintf(stderr, "Unsupported PLT relocation type %u (should be %u)\n",
                                    dyn->d_un.d_val, DT_REL);
                            return NULL;
                        }
                        break;
                }
            }
            printf("DYNAMIC str %zx sym %zx plt %zx size %zx\n",
                    strtab, symtab, jmprel, pltrelsz);

            /*
             * We don't need to check if these pointers point inside
             * mapped segments. We're gonna give code exec to this
             * ELF anyway, so we don't mind if the ELF tries to pwn us.
             */

            if (!strtab || !symtab || !jmprel || !pltrelsz) {
                printf("nothing dynamic here, skipping\n");
                break;
            }

            Elf32_Rel* rels = jmprel;
            //size_t relcnt = pltrelsz / sizeof(Elf32_Rel);

            for (Elf32_Rel* rel = rels; rel < rels + pltrelsz; ++rel) {
                if (ELF32_R_TYPE(rel->r_info) == R_386_NONE) {
                    continue;
                }
                if (ELF32_R_TYPE(rel->r_info) != R_386_JMP_SLOT) {
                    fprintf(stderr, "weird relocation type: %d - skipping\n",
                        ELF32_R_TYPE(rel->r_info));
                    continue;
                }
                Elf32_Sym* sym = &symtab[ELF32_R_SYM(rel->r_info)];
                char* symname = &strtab[sym->st_name];
                size_t funidx;
                for (funidx = 0; funidx < nfuncs; ++funidx) {
                    if (!strcmp(symname, funcs[funidx].name)) {
                        break;
                    }
                }
                if (funidx == nfuncs) {
                    fprintf(stderr, "unknown function %s\n", symname);
                    return NULL;
                }
                uint32_t value = (uint32_t) (size_t) trampolines[funidx];
                uint32_t* target = (uint32_t*) (uint64_t) rel->r_offset;
                printf("writing symbol %s value %zx at %zx\n", symname, value, target);
                *target = value;
            }

            break;}
        }
    }

    uint32_t entrypoint = elfhdr.e_entry;

#ifdef FAKE
    size_t hunk_ptr = (size_t) trampolines[1];

    //TMPHACK
    //crossld_call64_in_fake_ptr_ptr = (uint32_t*) 0x0804b010UL;

    printf("putting: %x as trampoline ptr at %zx\n", (uint32_t) hunk_ptr, crossld_call64_in_fake_ptr_ptr);
    *crossld_call64_in_fake_ptr_ptr = (uint32_t) hunk_ptr;
#endif

    return (void*) (uint64_t) entrypoint;
}

int crossld_start(const char *fname, const struct function *funcs, int nfuncs) {
    struct function allfuncs[nfuncs+1];

    memcpy(allfuncs, funcs, nfuncs * sizeof(struct function));
    allfuncs[nfuncs] = crossld_exit_fun;
    ++nfuncs;

    void* trampolines[nfuncs];

    void* common_hunks = crossld_generate_trampolines(trampolines, allfuncs, nfuncs);
    if (common_hunks == NULL) {
        return -1;
    }

    void *start = load_elf(fname, trampolines, allfuncs, nfuncs);
    if (start == NULL) {
        return -1;
    }

    return crossld_enter(start, common_hunks);
}
