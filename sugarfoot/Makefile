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

# brew install boost155
LDFLAGS =  -L/usr/local/opt/boost155/lib
CPPFLAGS = -I/usr/local/opt/boost155/include

LIB_PATHS = -L/usr/local/Cellar/boost/1.54.0/lib $(LDFLAGS)
INC_PATHS = -I/usr/local/Cellar/qscintilla2/2.7.1/include/Qsci $(CPPFLAGS)

CPP_FILES =  mmc_image.cpp core_image.cpp vm.cpp log.cpp utils.cpp mmobj.cpp main.cpp process.cpp prims.cpp mmc_fun.cpp ctrl.cpp qt_prims.cpp qsc.cpp mmjit.cpp

HPP_FILES = mmc_image.hpp core_image.hpp vm.hpp log.hpp utils.hpp defs.hpp mmobj.hpp process.hpp prims.hpp mmc_fun.hpp ctrl.hpp qt_prims.hpp qsc.hpp mmjit.hpp

LIBS = /usr/local/lib/x86_64/libjit.a  -lboost_system-mt -lboost_iostreams-mt -lboost_filesystem-mt -lqscintilla2

QT_FLAGS = -pipe -g -arch x86_64 -W \
	-DQT_NO_DEBUG -DQT_WEBKIT_LIB -DQT_SCRIPT_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED \
	-I/usr/local/Cellar/qt/4.8.5/mkspecs/unsupported/macx-clang \
	-I. -I/usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/Headers \
	-I/usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/Headers \
	-I/usr/local/Cellar/qt/4.8.5/lib/QtGui.framework/Versions/4/Headers \
	-I/usr/local/Cellar/qt/4.8.5/lib/QtScript.framework/Versions/4/Headers \
	-I/usr/local/Cellar/qt/4.8.5/lib/QtWebKit.framework/Versions/4/Headers \
	-I/usr/local/Cellar/qt/4.8.5/lib/QtNetwork.framework/Versions/4/Headers \
	-I/usr/local/Cellar/qt/4.8.5/include \
	-I. \
	-F/usr/local/Cellar/qt/4.8.5/lib \
	-headerpad_max_install_names -arch x86_64 \
	-F/usr/local/Cellar/qt/4.8.5/lib -L/usr/local/Cellar/qt/4.8.5/lib \
	-framework QtScript -L/opt/X11/lib -L/usr/local/Cellar/qt/4.8.5/lib \
	-F/usr/local/Cellar/qt/4.8.5/lib -framework QtCore -framework QtGui -framework QtWebKit -framework QtNetwork

sf-vm: $(CPP_FILES) $(HPP_FILES)
	moc qsc.hpp > qsc.moc.cpp
	g++ -Wall -g $(CPP_FILES) $(LIBS) $(LIB_PATHS) $(INC_PATHS)  -o sf-vm $(QT_FLAGS)

clean:
	rm -f sf-vm
	rm mm/*.mmc
	rm tests/*.mmc

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

test: $(TEST_DEPS) $(MMC_TESTER) $(MMC_TESTS) $(MEMETALK)
	MEME_PATH=./mm/ ./sf-vm mm/memetest.mmc
