CXXFLAGS = -DP_USE_PRAGMA -g -D_REENTRANT -O0 -Wall -fPIC -DPIC -DPTRACING
LDFLAGS = `ptlib-config --libs`

all: h223dump reverse h223read h245test

h223read: h223read.o ../libh324m.a
	g++ -o h223read h223read.o ../libh324m.a $(LDFLAGS)

h223dump: h223dump.o ../libh324m.a
	g++ -o h223dump h223dump.o ../libh324m.a $(LDFLAGS)

reverse: reverse.o ../bits.o
	g++ -o reverse reverse.o ../bits.o

h245test: h245test.o
	g++ -o h245test h245test.o ../libh324m.a $(LDFLAGS)

clean:
	rm -f *.o reverse h223read h223dump
