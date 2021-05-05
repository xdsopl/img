/*
Decoder for lossy image compression based on tile prediction and the discrete wavelet transformation

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
	for (int j = 0, y = length*row; j < length && y < height; ++j, ++y)
		for (int i = 0, x = length*col; i < length && x < width; ++i, ++x)
			output[(width*y+x)*stride] = input[length*j+i];
}

void decode(struct bits *bits, float *values, int length)
{
	int pixels = length * length;
	for (int i = 0; i < pixels; ++i)
		values[i] = 0;
	for (int i = 0; i < pixels; ++i) {
		int val = get_vli(bits);
		if (val) {
			if (get_bit(bits))
				val = -val;
			int idx = hilbert(length, i);
			values[idx] = val;
		} else {
			i += get_vli(bits);
		}
	}
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s input.dwt output.ppm\n", argv[0]);
		return 1;
	}
	struct bits *bits = bits_reader(argv[1]);
	if (!bits)
		return 1;
	int wavelet = get_bit(bits);
	int rounding = get_bit(bits);
	int width = get_vli(bits);
	int height = get_vli(bits);
	int length = get_vli(bits);
	int quant[3];
	for (int i = 0; i < 3; ++i)
		quant[i] = get_vli(bits);
	int cols = (width + length - 1) / length;
	int rows = (height + length - 1) / length;
	int pixels = length * length;
	float *input = malloc(sizeof(float) * pixels);
	float *output = malloc(sizeof(float) * pixels);
	float *buffer = malloc(sizeof(float) * 3 * pixels);
	struct image *image = new_image(argv[2], width, height);
	for (int row = 0; row < rows; ++row) {
		for (int i = 0; i < 3 * pixels; ++i)
			buffer[i] = 0;
		for (int col = 0; col < cols; ++col) {
			for (int j = 0; j < 3; ++j) {
				float *prev = buffer + j * pixels;
				decode(bits, input, length);
				dequantize(input, length, quant[j], rounding);
				if (wavelet)
					idwt2d(icdf97, output, input, 2, length, 1, 1);
				else
					ihaar2d(output, input, 2, length, 1, 1);
				for (int i = 0; i < pixels; ++i)
					output[i] += prev[i];
				copy(image->buffer+j, output, width, height, length, col, row, 3);
				for (int i = 0; i < pixels; ++i)
					prev[i] = output[i];
			}
		}
	}
	close_reader(bits);
	for (int i = 0; i < width * height; ++i)
		image->buffer[3*i] += 0.5f;
	rgb_image(image);
	if (!write_ppm(image))
		return 1;
	return 0;
}

