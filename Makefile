CFLAGS = -std=c99 -W -Wall -Ofast
# CFLAGS += -g -fsanitize=address

all: encode decode

test: encode decode
	./encode input.pnm putput.img
	du -b putput.img
	./decode putput.img output.pnm
	compare -verbose -metric PSNR input.pnm output.pnm /dev/null ; true

%: %.c *.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f encode decode

