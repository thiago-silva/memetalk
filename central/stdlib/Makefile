all: build

include $(shell git rev-parse --show-toplevel)/common.mk

$(call GEN_MEC_RULE)

CXXFLAGS = -Wall -fPIC $(QT_CXXFLAGS) -I$(VM_DIR)

LIBS = $(QT_LIBS)

PRIM_QT_CODE := qt_prims.cpp
PRIM_QT_COBJ := qt_prims.o
PRIM_QT_DYNL := qt_prims.so

$(PRIM_QT_COBJ): $(PRIM_QT_CODE)
$(PRIM_QT_DYNL): $(PRIM_QT_COBJ); g++ -shared $(PRIM_QT_COBJ) $(LIBS) -o $@

CODE := io.me qt.me remote_repl.me memetest.me
BYTECODE := $(patsubst %.me,%.mec,$(CODE))

%.o: %.cpp; g++ $(CXXFLAGS) -c -o $@ $^
build: $(BYTECODE) $(PRIM_QT_DYNL)
clean:; -rm -f $(BYTECODE)
