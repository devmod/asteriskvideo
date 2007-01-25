CXXFLAGS = -DP_USE_PRAGMA -g -D_REENTRANT -O0 -Wall -fPIC -DPIC -DPTRACING
LDFLAGS = `ptlib-config --libs`

all: h223dump reverse

h223dump: h223dump.o
	g++ -o h223dump h223dump.o ../libh324m.a $(LDFLAGS)

reverse: reverse.o
	g++ -o reverse reverse.o ../bits.o
