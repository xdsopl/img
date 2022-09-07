CFLAGS = -std=c99 -W -Wall -O3 -D_GNU_SOURCE=1 -g
LDLIBS = -lm
RM = rm -f

all: itwenc itwdec

test: itwenc itwdec
	./itwenc input.ppm /dev/stdout | ./itwdec /dev/stdin output.ppm

itwenc: src/encode.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

itwdec: src/decode.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

clean:
	$(RM) itwenc itwdec
