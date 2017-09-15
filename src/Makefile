include $(shell git rev-parse --show-toplevel)/common.mk

BIN = $(MEME)

CPP_FILES = log.cpp mec_image.cpp core_image.cpp vm.cpp	\
	utils.cpp mmobj.cpp process.cpp prims.cpp mec_fun.cpp qsc.cpp	\
	ctrl.cpp main.cpp

HPP_FILES = log.hpp mec_image.hpp core_image.hpp vm.hpp	\
	utils.hpp mmobj.hpp process.hpp prims.hpp mec_fun.hpp qsc.hpp	\
	ctrl.hpp defs.hpp

OBJS = $(CPP_FILES:%.cpp=%.o)

CXXFLAGS = -Wall -fPIC $(QT_CXXFLAGS)

LIBS = -lboost_system -lboost_iostreams -lboost_filesystem -lgc -ldl $(QT_LIBS)

# Main targets
all: release
cleanvm: clean
clean:; rm -f $(BIN) $(OBJS) qsc.moc.cpp
build: $(BIN)

debug: CXXFLAGS += -g
debug: build

release: CXXFLAGS += -DMM_NO_DEBUG -O2 -g
release: build

# Qsc file
qsc.moc.cpp: qsc.hpp; moc $< > $@

# Output binary
%.o: %.cpp; g++ $(CXXFLAGS) -c -o $@ $^
$(BIN): qsc.moc.cpp $(OBJS); g++ $(OBJS) $(LIBS) -Wl,-export-dynamic -o $@
