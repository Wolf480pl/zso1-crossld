CFLAGS := -O3 -g -DDEBUG -std=gnu11 -Wall
export CFLAGS

DIRS := include pkg
INCLUDES := crossld.h
LIBS := libcrossld.so libcrossld.o libcrossld.fake.so libcrossld.fake.o
PKGNAME := wd371280
PKGDIR := pkg/$(PKGNAME)

INCLUDES_SRCDIR := $(patsubst %,src/%,$(INCLUDES))
INCLUDES_INCLUDEDIR := $(patsubst %,include/%,$(INCLUDES))
LIBS_SRCDIR := $(patsubst %,src/%,$(LIBS))

FILES_SRCDIR := $(LIBS_SRCDIR) $(INCLUDES_SRCDIR)

all: $(LIBS_SRCDIR) $(INCLUDES_INCLUDEDIR)

.PHONY: all clean package $(LIBS_SRCDIR)

$(FILES_SRCDIR): src/%:
	$(MAKE) -C src $*

$(DIRS): %:
	mkdir -p $@

$(INCLUDES_INCLUDEDIR): include/%: src/% | include
	cp $< $@

test: all
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	rm -rf include
	rm -rf pkg

package: clean pkg
	mkdir -p $(PKGDIR)/
	cp src/* $(PKGDIR)/
	cp README.txt $(PKGDIR)/
	tar -czvf pkg/$(PKGNAME).tar.gz -C pkg $(PKGNAME)
