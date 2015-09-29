CXX ?= g++

CFLAGS+=--std=c++11 -Wall -Werror -Igzstream
LDFLAGS=-L/usr/lib64/llvm -lclang -ldb_cxx -lz

OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))
OBJS+=gzstream/gzstream.o

PROG=clang4vim-index

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@

%.o: %.C
	$(CXX) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(PROG) $(OBJS)


