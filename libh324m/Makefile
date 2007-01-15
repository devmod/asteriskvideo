

all: libh324m test

libh324m: h324m.o
	make -C src/ all
	g++ -o libh324m.so -shared h324m.o src/libh324m.a ../pwlib_v1_11_0/lib/libpt_linux_x86_r_s.a -lldap -llber -lldap_r -lpthread -lsasl2 -lssl -lcrypto -lexpat -lresolv -ldl 

test:	test.o	
	gcc -o test test.o -lh324m -L./ -lh324m

clean:
	make -C src/ clean
	rm -f h324m.o test.o libh324m.so
