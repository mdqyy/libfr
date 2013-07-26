CXX = g++
CC = gcc
RM = rm

ifeq ($(TARGET), DEBUG)
CXXFLAGS = -Wall -ggdb3 -msse4 -D DEBUG -D _OPENCV
CFLAGS=-Wall -std=c99 -ggdb3 -msse4 -D DEBUG -D _OPENCV
else
CXXFLAGS = -Wall -msse4 -mfpmath=both -O3 -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG -D _OPENCV
CFLAGS = -Wall -std=c99 -msse4 -mfpmath=both -O3 -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG -D _OPENCV
endif

LIBS = `pkg-config --libs opencv libxml-2.0` -ml
INCS = `pkg-config --cflags opencv libxml-2.0`

.PHONY: all clean lib

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@
        
%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@
        
%.s: %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -S $< -o $@

%.s: %.c
	$(CC) $(CFLAGS) $(INCS) -S $< -o $@

all: lib bin/test

LIB_SRC=$(addprefix src/, classifier.cpp const.cpp core.cpp core_simple.cpp core_sse.cpp lbp.cpp preprocess.cpp simplexml.cpp)

LIB_OBJ=$(LIB_SRC:.cpp=.o)

# Dependencies

src/classifier.o: src/classifier.cpp src/classifier.h src/core.h src/simplexml.h

src/const.o: src/const.c src/const.h

src/core.o: src/core.cpp src/core.h src/const.h src/preprocess.h src/structures.h

src/core_simple.o: src/core_simple.cpp src/core_simple.h src/core.h src/const.h src/preprocess.h src/structures.h

src/core_sse.o: src/core_sse.cpp src/core_sse.h src/core.h src/const.h src/preprocess.h src/structures.h

src/lbp.o: src/lbp.c src/lbp.h src/const.h

src/preprocess.o: src/preprocess.cpp src/preprocess.h src/lbp.h

src/simplexml.o: src/simplexml.cpp src/simplexml.h src/lbp.h

# Build rules

lib: lib/libabr.so

bin/test: src/test.o $(LIB_OBJ)
	$(CXX) -o $@ $(CXXFLAGS) $(INCS) $(LIBS) $^

lib/libabr.so: $(LIB_OBJ)
	$(CXX) -o $@ -shared -fPIC $(CXXFLAGS) $(INCS) $(LIBS) $^

clean:
	@echo "Cleaning"
	@$(RM) $(LIB_OBJ)
	@$(RM) lib/libabr.so
	@$(RM) bin/test

