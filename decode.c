/*
Decoder for lossless image compression

Copyright 2024 Ahmet Inan <xdsopl@gmail.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

FILE *open_img(const char *name, int *width, int *height, int *channels)
{
	if (name[0] == '-' && !name[1])
		name =  "/dev/stdin";
	FILE *file = fopen(name, "r");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file for reading\n", name);
		return 0;
	}
	char magic[3];
	uint16_t dimensions[2];
	if (3 != fread(magic, 1, 3, file) || EOF == (*channels = fgetc(file)) || 2 != fread(dimensions, 2, 2, file)) {
		fprintf(stderr, "EOF while reading from \"%s\"\n", name);
		fclose(file);
		return 0;
	}
	if (*channels != 1 && *channels != 3) {
		fprintf(stderr, "only one or three channels supported\n");
		fclose(file);
		return 0;
	}
	*width = dimensions[0] + 1;
	*height = dimensions[1] + 1;
	return file;
}

FILE *open_pnm(const char *name, int width, int height, int channels)
{
	if (name[0] == '-' && !name[1])
		name =  "/dev/stdout";
	FILE *file = fopen(name, "w");
	if (!file) {
		fprintf(stderr, "could not open \"%s\" file for writing\n", name);
		return 0;
	}
	int number = channels == 1 ? 5 : 6;
	if (!fprintf(file, "P%d %d %d 255\n", number, width, height)) {
		fprintf(stderr, "could not write to file \"%s\"\n", name);
		fclose(file);
	}
	return file;
}

long leb128(FILE *file)
{
	long byte, shift = 0, value = 0;
	while ((byte = fgetc(file)) >= 128) {
		value |= (byte & 127) << shift;
		shift += 7;
	}
	if (byte < 0)
		return byte;
	return value | (byte << shift);
}

void deinterleave(long mixed, int *values, int count)
{
	for (int j = 0; j < count; ++j) {
		values[j] = 0;
		long m = mixed >> j;
		for (int i = 0; m; ++i, m >>= count)
			values[j] |= (m & 1L) << i;
	}
}

int sgn_int(int x)
{
	return (x & 1) ? -(x >> 1) : (x >> 1);
}

int main(int argc, char **argv)
{
	if (argc != 3)
		return 1;
	int width, height, channels;
	FILE *ifile = open_img(argv[1], &width, &height, &channels);
	if (!ifile)
		return 1;
	FILE *ofile = open_pnm(argv[2], width, height, channels);
	if (!ofile) {
		fclose(ifile);
		return 1;
	}
	uint8_t *line = calloc(width, channels);
	long limit = channels == 3 ? 255 : 4095;
	long total = width * height;
	for (long i = 0; i < total;) {
		long mixed = leb128(ifile);
		if (mixed < 0)
			goto eof;
		int diff[4];
		deinterleave(mixed, diff, channels + 1);
		for (int c = 0; c < channels; ++c)
			diff[c] = sgn_int(diff[c]);
		long count = diff[channels];
		if (count == limit)
			count += leb128(ifile);
		for (++count; count--; ++i) {
			for (int c = 0; c < channels; ++c) {
				int value = diff[c] + line[(i%width)*channels+c];
				line[(i%width)*channels+c] = value;
				fputc(value, ofile);
			}
		}
	}
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

