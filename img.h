/*
Lossy image compression based on prediction using the Hilbert curve and the Haar wavelet transform

Copyright 2021 Ahmet Inan <xdsopl@gmail.com>
*/

void quantize(float *values, int length, int quant, int rounding)
{
	for (int i = 0; i < length * length; ++i) {
		float v = values[i];
		v *= quant;
		if (rounding)
			v = truncf(v);
		else
			v = nearbyintf(v);
		values[i] = v;
	}
}

void dequantize(float *values, int length, int quant, int rounding)
{
	for (int i = 0; i < length * length; ++i) {
		float v = values[i];
		if (rounding) {
			float bias = 0.375f;
			if (v < 0.f)
				v -= bias;
			else if (v > 0.f)
				v += bias;
		}
		if (quant)
			v /= quant;
		else
			v = 0;
		values[i] = v;
	}
}

