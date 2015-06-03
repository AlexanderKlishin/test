
CXX ?= g++
CFLAGS+=--std=gnu++11 -Wall -Werror
LDFLAGS=
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

PROG=test

$(PROG):$(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $< -o $@

$(OBJS): %.o: %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(PROG)


