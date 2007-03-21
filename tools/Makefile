LDFLAGS = -lmp4 -lmp4v2
CFLAGS = 

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ $<

all: pcm2mp4

pcm2mp4: pcm2mp4.o
	g++ -o pcm2mp4 pcm2mp4.o $(LDFLAGS) 

clean:
	rm -f pcm2mp4 *.o
