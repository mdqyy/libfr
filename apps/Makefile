CXX = g++
CC = gcc
CXXFLAGS = -Wall -O3
RM = rm

LIBS = `pkg-config --libs opencv libxml-2.0` -largtable2 -lm
INCS = `pkg-config --cflags opencv libxml-2.0`

.PHONY: all clean

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@
        
%.o: %.c %.h
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@
        
%.s: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(INCS) -S $< -o $@

all: bin/demo.exe

bin/wbdetect: src/wbdetect.cpp src/lrd_engine.o src/faces.h src/classifier.o src/simplexml.o src/image.o
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

bin/demo.exe: src/demo.cpp src/lrd_engine.o src/faces.h src/classifier.o src/simplexml.o src/image.o
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

clean:
	$(RM) src/*.o src/*.s
