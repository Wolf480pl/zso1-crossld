#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <elf.h>

#include "crossld.h"
#include "trampolines.h"
#include "debug.h"

const size_t PAGE_SIZE = 4096;

struct tounmap {
    void *addr;
    size_t len;
};

#if 0
// We could have a struct like this, but how do you calloc it?
struct unmap_vec {
    size_t count;
    struct tounmap elems[];
}
#endif

static void unmapall(struct tounmap *vec) {
    for (struct tounmap *elem = vec; elem->len != 0; ++elem) {
        if (munmap(elem->addr, elem->len) < 0) {
            perror("munmap");
            // keep going, unmap what we can
        } else {
            DBG("freed\n");
        }
    }
}

static struct tounmap mmap_exact(void *addr, size_t length, int prot, int flags,
                        int fd, off_t offset) {
    void* actual_addr = mmap(addr, length, prot, flags, fd, offset);

    struct tounmap res;
    res.addr = MAP_FAILED;
    res.len = 0;

    if (actual_addr == MAP_FAILED) {
        perror("mmap");
        return res;
    }
    if (actual_addr != addr) {
        DBG("ERROR: mmap moved us to %p\n", actual_addr);
        if (munmap(actual_addr, length) < 0) {
            perror("munmap");
        }
        return res;
    }
    res.addr = actual_addr;
    res.len = length;
    return res;
}

// a wrapper around pread where short read only happens at EOF
ssize_t preadall(int fd, void *vbuf, size_t nbyte, off_t offset) {
    char* buf = vbuf;
    ssize_t res;
    ssize_t left = nbyte;
    do {
        res = pread(fd, buf, left, offset);
        if (res < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("pread");
                return res;
            }
        }
        buf += res;
        offset += res;
        left -= res;
    } while (res > 0);
    if (left != 0) {
        fprintf(stderr, "pread: End of file - expected %zd more bytes\n", left);
    }
    return nbyte - left;
}

const char valid_elf_ident[EI_NIDENT] = {
    ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
    ELFCLASS32,
    ELFDATA2LSB,
    EV_CURRENT,
};

static void *load_elf(const int fd, void * const *trampolines,
                      struct tounmap** map_vec_ptr,
                      const struct function *funcs, int nfuncs) {

    Elf32_Ehdr elfhdr;
    if (preadall(fd, &elfhdr, sizeof(elfhdr), 0) != sizeof(elfhdr)) {
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
    if (preadall(fd, phdrs, sizeof(phdrs), elfhdr.e_phoff) != sizeof(phdrs)) {
        return NULL;
    }

    // if each header is LOAD, and each has a BSS, we'll use 2*e_phnum mappings
    // plus one for null terminator
    struct tounmap* map_vec = calloc(elfhdr.e_phnum * 2 + 1, sizeof(struct tounmap));
    *map_vec_ptr = map_vec;
    struct tounmap* map_next = map_vec;

    for (size_t i = 0; i < elfhdr.e_phnum; ++i) {
        DBG("hdr %zu\n", i);
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
            DBG("LOAD %x %x %x %x %d\n", hdr->p_vaddr, hdr->p_memsz,
                    hdr->p_offset, hdr->p_filesz, prot);
            size_t page_offset = hdr->p_vaddr & (PAGE_SIZE - 1);
            size_t vaddr = hdr->p_vaddr - page_offset;
            size_t offset = hdr->p_offset - page_offset;
            size_t size = hdr->p_filesz + page_offset;
            size_t mapsize = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            DBG("mmap %zx %zx %zx %zx %d\n", vaddr, mapsize,
                    offset, size, prot);
            *map_next = mmap_exact((void*) vaddr, mapsize, prot,
                                    MAP_PRIVATE | MAP_32BIT, fd, offset);
            char *addr = map_next->addr;
            if (addr == MAP_FAILED) {
                return NULL;
            }
            ++map_next;

            if (hdr->p_filesz < hdr->p_memsz) {
                DBG("segment with trailing zeros:"
                       "%u < %u\n", hdr->p_filesz, hdr->p_memsz);
                char* bss_start = addr + size;
                char* bss_end = addr + page_offset + hdr->p_memsz;
                size_t bss_size = bss_end - bss_start;
                size_t bss_page_offset = ((size_t) bss_start) & (PAGE_SIZE - 1);
                size_t bzero_size = PAGE_SIZE - bss_page_offset;
                size_t anon_map_size = 0;
                if (bzero_size > bss_size) {
                    bzero_size = bss_size;
                } else {
                    anon_map_size = bss_size - bzero_size;
                }
                char* anon_start = bss_start + bzero_size;

                DBG("bzero at %p, size %zx, map anon from %p, size %zx\n",
                       bss_start, bzero_size, anon_start, anon_map_size);

                bzero(bss_start, bzero_size);
                if (anon_map_size > 0) {
                    *map_next = mmap_exact(anon_start, anon_map_size,
                        prot, MAP_PRIVATE | MAP_32BIT | MAP_ANONYMOUS, -1, 0);
                    if (map_next->addr == MAP_FAILED) {
                        return NULL;
                    }
                    ++map_next;
                }
            }
            break;

        case PT_DYNAMIC: {
            size_t dyncnt = hdr->p_filesz / sizeof(Elf32_Dyn);
            Elf32_Dyn dyns[dyncnt];
            if (preadall(fd, &dyns, sizeof(dyns), hdr->p_offset) != sizeof(dyns)) {
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
            DBG("DYNAMIC str %p sym %p plt %p size %zx\n",
                    strtab, symtab, jmprel, pltrelsz);

            /*
             * We don't need to check if these pointers point inside
             * mapped segments. We're gonna give code exec to this
             * ELF anyway, so we don't mind if the ELF tries to pwn us.
             */

            if (!strtab || !symtab || !jmprel || !pltrelsz) {
                DBG("nothing dynamic here, skipping\n");
                break;
            }

            Elf32_Rel* rels = jmprel;

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
                int funidx;
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
                DBG("writing symbol %s value %x at %p\n", symname, value, target);
                *target = value;
            }

            break;}
        }
    }

    uint32_t entrypoint = elfhdr.e_entry;

    return (void*) (uint64_t) entrypoint;
}

__attribute__((visibility("default")))
int crossld_start(const char *fname, const struct function *funcs, int nfuncs) {
    struct function allfuncs[nfuncs+1];

    memcpy(allfuncs, funcs, nfuncs * sizeof(struct function));
    struct function* exit_fun = &allfuncs[nfuncs];
    ++nfuncs;

    void* trampolines[nfuncs];

    struct crossld_ctx* ctx = crossld_generate_trampolines(trampolines, allfuncs, nfuncs, exit_fun);
    if (ctx == NULL) {
        return -1;
    }

    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        perror("open");
        crossld_free_trampolines(ctx);
        return -1;
    }

    struct tounmap *map_vec = NULL;
    void *start = load_elf(fd, trampolines, &map_vec, allfuncs, nfuncs);

    // we no longer need the FD, as mmaps will keep their own references
    if (close(fd) < 0) {
        perror("close");
        // keep going, we can live with an extra unneeded FD
    }

    int res = -1;
    if (start != NULL) {
        // call the entrypoint
        res = crossld_enter(start, ctx);
    }

    if (map_vec) {
        unmapall(map_vec);
    }
    free(map_vec);
    crossld_free_trampolines(ctx);
    return res;
}
