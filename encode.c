/*
Encoder for lossless image compression

Copyright 2024 Ahmet Inan <xdsopl@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

FILE *open_pnm(const char *name, int *width, int *height, int *channels)
{
	if (name[0] == '-' && !name[1])
		name =  "/dev/stdin";
	FILE *file = fopen(name, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file for reading\n", name);
		return 0;
	}
	int letter = fgetc(file);
	int number = fgetc(file);
	if ('P' != letter || ('5' != number && '6' != number)) {
		fprintf(stderr, "file \"%s\" neither P5 nor P6 image\n", name);
		fclose(file);
		return 0;
	}
	*channels = number == '5' ? 1 : 3;
	int integer[3];
	int byte = fgetc(file);
	if (EOF == byte)
		goto eof;
	for (int i = 0; i < 3; i++) {
		while ('#' == (byte = fgetc(file)))
			while ('\n' != (byte = fgetc(file)))
				if (EOF == byte)
					goto eof;
		while ((byte < '0') || ('9' < byte))
			if (EOF == (byte = fgetc(file)))
				goto eof;
		char str[16];
		for (int n = 0; n < 16; n++) {
			if (('0' <= byte) && (byte <= '9') && n < 15) {
				str[n] = byte;
				if (EOF == (byte = fgetc(file)))
					goto eof;
			} else {
				str[n] = 0;
				break;
			}
		}
		integer[i] = atoi(str);
	}
	if (!(integer[0] && integer[1] && integer[2])) {
		fprintf(stderr, "could not read image file \"%s\"\n", name);
		fclose(file);
		return 0;
	}
	if (integer[2] != 255) {
		fprintf(stderr, "cant read \"%s\", only 8 bit per channel SRGB supported\n", name);
		fclose(file);
		return 0;
	}
	*width = integer[0];
	*height = integer[1];
	return file;
eof:
	fclose(file);
	fprintf(stderr, "EOF while reading from \"%s\"\n", name);
	return 0;
}

FILE *open_img(const char *name, int width, int height, int channels)
{
	if (name[0] == '-' && !name[1])
		name =  "/dev/stdout";
	FILE *file = fopen(name, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file for writing\n", name);
		return 0;
	}
	uint16_t dimensions[2] = { width - 1, height - 1};
	if (3 != fwrite("IMG", 1, 3, file) || channels != fputc(channels, file) || 2 != fwrite(dimensions, 2, 2, file)) {
		fprintf(stderr, "could not write to file \"%s\"\n", name);
		fclose(file);
	}
	return file;
}

void leb128(FILE *file, int value)
{
	while (value >= 128) {
		fputc((value & 127) | 128, file);
		value >>= 7;
	}
	fputc(value, file);
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 1;
	int width, height, channels;
	FILE *ifile = open_pnm(argv[1], &width, &height, &channels);
	if (!ifile)
		return 1;
	if (width > 65536 || height > 65536) {
		fprintf(stderr, "max supported width or height is 65536\n");
		fclose(ifile);
		return 1;
	}
	FILE *ofile = open_img(argv[2], width, height, channels);
	if (!ofile) {
		fclose(ifile);
		return 1;
	}
	unsigned *line = calloc(width, sizeof(unsigned));
	unsigned prev;
	int count = 0;
	for (int i = 0; i < width * height; ++i) {
		unsigned value = 0;
		if (channels != (int)fread(&value, 1, channels, ifile))
			goto eof;
		unsigned diff = value - line[i%width];
		line[i%width] = value;
		if (!i) {
			prev = diff;
		} else if (prev == diff) {
			++count;
		} else {
			fwrite(&prev, channels, 1, ofile);
			leb128(ofile, count);
			prev = diff;
			count = 0;
		}
	}
	fwrite(&prev, channels, 1, ofile);
	leb128(ofile, count);
	free(line);
	fclose(ifile);
	fclose(ofile);
	return 0;
eof:
	fprintf(stderr, "EOF while reading from \"%s\"\n", argv[1]);
	free(line);
	fclose(ifile);
	fclose(ofile);
	return 1;
}

