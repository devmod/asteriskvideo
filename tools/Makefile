LDFLAGS = -lmp4 -lmp4v2
CFLAGS =  -I/usr/local/include

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ $<

all: pcm2mp4 mp4band

pcm2mp4: pcm2mp4.o
	g++ -o pcm2mp4 pcm2mp4.o $(LDFLAGS) 

mp4band: mp4band.o
	g++ -o mp4band mp4band.o $(LDFLAGS) 

clean:
	rm -f pcm2mp4 *.o
