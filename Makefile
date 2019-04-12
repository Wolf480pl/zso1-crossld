CFLAGS := -O3 -g -DDEBUG
export CFLAGS

DIRS := include
INCLUDES := crossld.h
LIBS := libcrossld.so libcrossld.o libcrossld.fake.so libcrossld.fake.o

INCLUDES_SRCDIR := $(patsubst %,src/%,$(INCLUDES))
INCLUDES_INCLUDEDIR := $(patsubst %,include/%,$(INCLUDES))
LIBS_SRCDIR := $(patsubst %,src/%,$(LIBS))

FILES_SRCDIR := $(LIBS_SRCDIR) $(INCLUDES_SRCDIR)

all: $(LIBS_SRCDIR) $(INCLUDES_INCLUDEDIR)

.PHONY: all clean $(LIBS_SRCDIR)

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
