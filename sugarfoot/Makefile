# Entry points:
#
#   -sf-vm [default]: builds the VM
#   -clean: deletes the VM binary and mmc files
#
#   -core: builds the core.img file
#   -memetalk: build the compiler, VM and core image
#   -test: runs all memetalk tests
#

############ VM Section ############

BIN = sf-vm

INC_DIRS = /usr/include /usr/local/include

QSCI_INC ?= $(dir $(foreach i,$(INC_DIRS),$(shell find -L $(i) -name qscicommandset.h)))

PKGLIBS = QtCore QtGui QtScript QtWebKit QtNetwork

CXXFLAGS = -Wall $(shell pkg-config --cflags $(PKGLIBS)) -I$(QSCI_INC)

#CXXFLAGS = -Wall  -O2 -DMM_NO_DEBUG $(shell pkg-config --cflags $(PKGLIBS)) -I$(QSCI_INC)
#CXXFLAGS = -Wall  -g $(shell pkg-config --cflags $(PKGLIBS)) -I$(QSCI_INC)

LDFLAGS = $(shell pkg-config --libs $(PKGLIBS)) -lqscintilla2	\
	-lboost_system -lboost_iostreams -lboost_filesystem

CPP_FILES = log.cpp mmc_image.cpp core_image.cpp vm.cpp utils.cpp	\
	mmobj.cpp main.cpp process.cpp prims.cpp mmc_fun.cpp ctrl.cpp	\
	qt_prims.cpp qsc.cpp remote_repl.cpp

HPP_FILES = mmc_image.hpp core_image.hpp vm.hpp log.hpp utils.hpp	\
	defs.hpp mmobj.hpp process.hpp prims.hpp mmc_fun.hpp ctrl.hpp	\
	qt_prims.hpp qsc.hpp remote_repl.hpp

OBJS = $(CPP_FILES:%.cpp=%.o)

all: release

debug: CXXFLAGS += -g
debug: build

release: CXXFLAGS += -DMM_NO_DEBUG -O2
release: build

build: $(BIN)

qsc.moc.cpp: qsc.hpp
	moc $< > $@

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

$(OBJS): qsc.moc.cpp $(CPP_FILES) $(HPP_FILES)

$(BIN): $(OBJS)
	g++ $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(BIN) $(OBJS) qsc.moc.cpp

############ Compiler Section ############


GRAMMARS = pyparsers/*.g
PY_PARSER_FILES = pyparsers/memetr.py pyparsers/parser.py
PY_COMPILER_DEPENDENCIES = $(GRAMMARS) pycompiler/*py pycore/*.py pyutils/*.py
MM_CORE = mm/core.mm
MEMETALK = core.img $(PY_PARSER_FILES) sf-vm

core: core.img
memetalk: $(MEMETALK)

$(PY_PARSER_FILES): $(GRAMMARS)
	python -m pyparsers.gen_parsers

core.img: $(PY_PARSER_FILES) $(MM_CORE) $(PY_COMPILER_DEPENDENCIES)
	python -m pycore.compiler

############ Memetalk Tests Section ############



MM_TESTS = tests/*.mm
MMC_TESTS = tests/*.mmc

TEST_DEPS = mm/io.mmc
MM_TESTER = mm/memetest.mm
MMC_TESTER = mm/memetest.mmc

# compile_tester: $(MMC_TESTER)
# compile_all_tests: $(MMC_TESTS)

$(TEST_DEPS): mm/io.mm
	python -m pycompiler.compiler mm/io.mm

$(MMC_TESTER): $(MM_TESTER) $(MEMETALK)
	python -m pycompiler.compiler $(MM_TESTER)


$(MMC_TESTS): $(MM_TESTS) $(MEMETALK)
	python -m pycompiler.compiler $(MM_TESTS)

mm/qt.mmc: mm/qt.mm # debugger tests need qt
	python -m pycompiler.compiler mm/qt.mm

test: core.img $(TEST_DEPS) $(MMC_TESTER) $(MMC_TESTS) $(MEMETALK) mm/qt.mmc
	MEME_PATH=./mm/ ./sf-vm mm/memetest.mmc


memeclean:
	rm -f mm/*.mmc
	rm -f tests/*.mmc

cleanall: memeclean clean
	rm -f core.img
	rm -f $(PY_PARSER_FILES)
	rm -f pyparsers/*pyc
