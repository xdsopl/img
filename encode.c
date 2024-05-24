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

void leb128(FILE *file, long value)
{
	while (value >= 128) {
		fputc((value & 127) | 128, file);
		value >>= 7;
	}
	fputc(value, file);
}

void pair(long *y, long x)
{
	*y = (x * x + x + 2 * x * *y + 3 * *y + *y * *y) / 2;
}

int abs_sgn(int x)
{
	return (abs(x) << 1) | (x < 0);
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
	uint8_t *line = calloc(width, channels);
	long previous = 0, counter = 0;
	for (long i = 0; i < width * height; ++i) {
		long packed = 0;
		for (int c = 0; c < channels; ++c) {
			int value = fgetc(ifile);
			if (value < 0)
				goto eof;
			int diff = abs_sgn(value - line[(i%width)*channels+c]);
			line[(i%width)*channels+c] = value;
			if (c)
				pair(&packed, diff);
			else
				packed = diff;
		}
		if (!i) {
			previous = packed;
		} else if (previous == packed) {
			++counter;
		} else {
			leb128(ofile, counter);
			leb128(ofile, previous);
			previous = packed;
			counter = 0;
		}
	}
	leb128(ofile, counter);
	leb128(ofile, previous);
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

