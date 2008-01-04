CXXFLAGS = -DP_USE_PRAGMA -g -D_REENTRANT -O0 -Wall -fPIC -DPIC -DPTRACING
LDFLAGS = `ptlib-config --libs`

all: h223dump reverse h223read if2amr amr2if

h223read: h223read.o ../libh324m.a
	g++ -o h223read h223read.o ../libh324m.a $(LDFLAGS)

h223dump: h223dump.o ../libh324m.a
	g++ -o h223dump h223dump.o ../libh324m.a $(LDFLAGS)

reverse: reverse.o 
	g++ -o reverse reverse.o 

if2amr: if2amr.o 
	g++ -o if2amr if2amr.o 

amr2if: amr2if.o
	g++ -o amr2if amr2if.o

clean:
	rm -f *.o reverse h223read h223dump
