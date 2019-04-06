TESTS := hello norel
CLEANTESTS := $(patsubst %,%.clean,$(TESTS))

all: $(TESTS)

clean: $(CLEANTESTS)

.PHONY: all clean $(TESTS) $(CLEANTESTS)

$(TESTS): %:
	make -C $* test

$(CLEANTESTS): %.clean:
	make -C $* clean
