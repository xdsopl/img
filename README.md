# ITW

### Playing with lossy image compression based on tile prediction and the discrete wavelet transformation

Quick start:

Encode [PNM](https://en.wikipedia.org/wiki/Netpbm) picture file to ```encoded.itw```:

```
./itwenc smpte.ppm encoded.itw
```

Decode ```encoded.itw``` file to ```decoded.ppm``` picture file:

```
./itwdec encoded.itw decoded.ppm
```

Watch ```decoded.ppm``` picture file in [feh](https://feh.finalrewind.org/):

```
feh decoded.ppm
```
