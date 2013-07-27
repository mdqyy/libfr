CXX = g++
CC = gcc
# RELEASE OPTIONS
CXXFLAGS = -Wall -march=i686 -msse3 -mfpmath=both -O3 -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG
CFLAGS = -Wall -std=c99 -march=i686 -msse3 -mfpmath=both -O3 -ffast-math -fomit-frame-pointer -finline-functions -D NDEBUG
RM = rm

LIBS = `pkg-config --libs opencv libxml-2.0 argtable2` -L../lib -labr -lm
INCS = `pkg-config --cflags opencv libxml-2.0 argtable2` -I../include


.PHONY: all clean

all: process_image xml2h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@
        
%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@
        
process_image: process_image.o
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

process_image2: process_image2.o
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

xml2h: xml2h.o
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

stats: stats.o
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

face_detect: frontal-face-lrd2x2-a20.o face_detect.o 
	$(CXX) $^ -o $@ $(CXXFLAGS) $(INCS) $(LIBS)

frontal-face-lrd2x2-a20.c: xml2h ../data/frontal-face-lrd2x2-a20.xml
	./xml2h -i ../data/frontal-face-lrd2x2-a20.xml -o frontal-face-lrd2x2-a20 -n frontal_face_lrd2x2_a20

clean:
	$(RM) *.o
	$(RM) face_detect process_image xml2h
