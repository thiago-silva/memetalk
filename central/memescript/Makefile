all: build

include $(shell git rev-parse --show-toplevel)/common.mk

# $(call GEN_TRS_RULE)
$(call GEN_MEC_RULE)

CODE := bits.me comp_vmemory.me meme.me meme_tr.me mec.me opcode.me compiler.me	\
	entries.me ometa.me ometa_tr.me ometa_base.me vmemory.me \
	gen_parser.me
BYTECODE := $(patsubst %.me,%.mec,$(CODE))

#GRAMMARS := ometa.g ometa_tr.g meme.g meme_tr.g
# GCODE := $(patsubst %.g,%.me,$(GRAMMARS))
# GBCODE := $(patsubst %.g,%.mec,$(GRAMMARS))

build: $(BYTECODE)
clean:; -rm -f $(BYTECODE) $(GBCODE)
