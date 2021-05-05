/*
Encoder for lossy image compression based on tile prediction and the discrete wavelet transformation

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

#include "hilbert.h"
#include "haar.h"
#include "cdf97.h"
#include "dwt.h"
#include "ppm.h"
#include "vli.h"
#include "bits.h"
#include "img.h"

void copy(float *output, float *input, int width, int height, int length, int col, int row, int stride)
{
	for (int j = 0, y = length*row; j < length; ++j, ++y)
		for (int i = 0, x = length*col; i < length; ++i, ++x)
			if (x < width && y < height)
				output[length*j+i] = input[(width*y+x)*stride];
			else
				output[length*j+i] = 0;
}

void encode(struct bits *bits, float *values, int length)
{
	int last = 0, pixels = length * length;
	for (int i = 0; i < pixels; ++i) {
		int idx = hilbert(length, i);
		float value = values[idx];
		if (value) {
			if (i - last) {
				put_vli(bits, 0);
				put_vli(bits, i - last - 1);
			}
			last = i + 1;
			put_vli(bits, fabsf(value));
			put_bit(bits, value < 0.f);
		}
	}
	if (last < pixels) {
		put_vli(bits, 0);
		put_vli(bits, pixels - last - 1);
	}
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 6 && argc != 7 && argc != 8) {
		fprintf(stderr, "usage: %s input.ppm output.img [Q0 Q1 Q2] [WAVELET] [ROUNDING]\n", argv[0]);
		return 1;
	}
	struct image *image = read_ppm(argv[1]);
	if (!image)
		return 1;
	int width = image->width;
	int height = image->height;
	int length = 16;
	int cols = (width + length - 1) / length;
	int rows = (height + length - 1) / length;
	int pixels = length * length;
	int quant[3] = { 128, 32, 32 };
	if (argc >= 6)
		for (int i = 0; i < 3; ++i)
			quant[i] = atoi(argv[3+i]);
	for (int i = 0; i < 3; ++i)
		if (!quant[i])
			return 1;
	int wavelet = 1;
	if (argc >= 7)
		wavelet = atoi(argv[6]);
	int rounding = 1;
	if (argc >= 8)
		rounding = atoi(argv[7]);
	ycbcr_image(image);
	for (int i = 0; i < width * height; ++i)
		image->buffer[3*i] -= 0.5f;
	struct bits *bits = bits_writer(argv[2]);
	if (!bits)
		return 1;
	put_bit(bits, wavelet);
	put_bit(bits, rounding);
	put_vli(bits, width);
	put_vli(bits, height);
	put_vli(bits, length);
	for (int i = 0; i < 3; ++i)
		put_vli(bits, quant[i]);
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	float *buffer = malloc(sizeof(float) * 3 * pixels);
	for (int row = 0; row < rows; ++row) {
		for (int i = 0; i < 3 * pixels; ++i)
			buffer[i] = 0;
		for (int col = 0; col < cols; ++col) {
			for (int j = 0; j < 3; ++j) {
				float *prev = buffer + j * pixels;
				copy(input, image->buffer+j, width, height, length, col, row, 3);
				for (int i = 0; i < pixels; ++i)
					input[i] -= prev[i];
				if (wavelet)
					dwt2d(cdf97, output, input, 2, length, 1, 1);
				else
					haar2d(output, input, 2, length, 1, 1);
				quantize(output, length, quant[j], rounding);
				encode(bits, output, length);
				dequantize(output, length, quant[j], rounding);
				if (wavelet)
					idwt2d(icdf97, input, output, 2, length, 1, 1);
				else
					ihaar2d(input, output, 2, length, 1, 1);
				for (int i = 0; i < pixels; ++i)
					prev[i] += input[i];
			}
		}
	}
	fprintf(stderr, "%d bits encoded\n", bits_count(bits));
	close_writer(bits);
	return 0;
}

