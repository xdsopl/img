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

int leb128(FILE *file)
{
	int byte, shift = 0, value = 0;
	while ((byte = fgetc(file)) >= 128) {
		value |= (byte & 127) << shift;
		shift += 7;
	}
	if (byte < 0)
		return byte;
	return value | (byte << shift);
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
	uint8_t *line[3] = { calloc(width, channels) };
	for (int i = 1; i < channels; ++i)
		line[i] = line[i-1] + width;
	for (int j = 0; j < height; ++j) {
		for (int c = 0; c < channels; ++c) {
			int mode = fgetc(ifile);
			if (mode < 0)
				goto eof;
			if (mode == 1) {
				if (width != (int)fread(line[c], 1, width, ifile))
					goto eof;
			} else if (mode == 2) {
				for (int i = 0; i < width;) {
					uint8_t diff = 0;
					if (1 != fread(&diff, 1, 1, ifile))
						goto eof;
					int count = leb128(ifile);
					if (count < 0)
						goto eof;
					for (++count; count--; ++i) {
						uint8_t value = diff + line[c][i];
						line[c][i] = value;
					}
				}
			}
		}
		for (int i = 0; i < width; ++i)
			for (int c = 0; c < channels; ++c)
				fputc(line[c][i], ofile);
	}
	free(*line);
	fclose(ifile);
	fclose(ofile);
	return 0;
eof:
	fprintf(stderr, "EOF while reading from \"%s\"\n", argv[1]);
	free(*line);
	fclose(ifile);
	fclose(ofile);
	return 1;
}

