CFLAGS=-g
LDFLAGS=`ptlib-config --libs`

all: libh324m test

libh324m: h324m.o
	make -C src/ all
	g++ -o libh324m.so -shared h324m.o src/libh324m.a $(LDFLAGS)

test:	test.o	
	gcc -o test test.o -lh324m -L./ -lh324m

clean:
	make -C src/ clean
	rm -f h324m.o test.o libh324m.so
