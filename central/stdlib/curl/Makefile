all: build

SUBDIRS = examples

include $(shell git rev-parse --show-toplevel)/common.mk

$(call GEN_MEC_RULE)

CODE := $(wildcard *.me)
BYTECODE := $(patsubst %.me,%.mec,$(CODE))

DEFS := curl.defs
DCODE := $(patsubst %.defs,%_prim.c,$(DEFS))
DOBJ := $(patsubst %.c,%.o,$(DCODE))
%_prim.c: %.defs; $(call RUN_BINDGEN_CMD,$^) > $@ || rm $@

%.o: %.c; g++ -I$(VM_DIR) -I$(MM_DIR)/stdlib/bindgen -c -o $@ $^
build: $(BYTECODE) $(DCODE) $(DOBJ) $(SUBDIRS)
clean:; -rm $(BYTECODE) $(DOBJ) $(DCODE)

$(SUBDIRS):; $(MAKE) -C $@

.PHONY: $(SUBDIRS)
