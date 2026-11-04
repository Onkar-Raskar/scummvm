/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This Lanczos image scaling algorithm is an adapted ScummVM port 
 * of the CResizeImage class originally developed by tmiddelkoop.
 * 
 * Original source code (Commit: 558b52ac4f158ecc30005553090daba764770134):
 * https://github.com/tmiddelkoop/ImageResize/blob/558b52ac4f158ecc30005553090daba764770134/ResizeImage/ResizeImage.cpp
 */

#include "common/util.h"

#include "graphics/blit/blit-resize.h"

#define CAP(x) ((x) < 0.0f ? 0.0f : (x) > 1.0f ? 1.0f : (x))
#define ROUND(x) (floor(x + 0.5))

namespace Graphics {

const int LANCZOS_WINDOW = 2;
const float LANCZOS_BLUR = 1.25;
const int GAMMASIZE = 200000;
const int MAX_UPSCALE = 100; 

CResizeImage::CResizeImage(int width, int height, int maxWidth, int maxHeight)
	: _width(width), _height(height), _maxWidth(maxWidth), _maxHeight(maxHeight) {
	initGamma();
}

CResizeImage::CResizeImage(byte *buffer, int width, int height, int bpp, int srcPitch,
                           byte *newBuffer, int newWidth, int newHeight, int newBpp, int dstPitch)
	: _width(width), _height(height), _bpp(bpp), _newWidth(newWidth), _newHeight(newHeight), 
	  _newBpp(newBpp), _buffer(buffer), _newBuffer(newBuffer),
	  _rowSize(srcPitch), _newRowSize(dstPitch) { 
	initGamma();
}

void CResizeImage::determineNewSize(int &outWidth, int &outHeight) {
	_fx = (float)_maxWidth / _width;
	_fy = (float)_maxHeight / _height;

	if (_width < _maxWidth && _height < _maxHeight && MAX_UPSCALE > 100) {
		float upscale = (float)MAX_UPSCALE / 100;
		if (_fx < _fy) {
			_fx = _fx > upscale ? upscale : _fx;
			_fy = _fx;
		} else {
			_fy = _fy > upscale ? upscale : _fy;
			_fx = _fy;
		}
	} else if (_width > _maxWidth || _height > _maxHeight) {
		if (_fx < _fy) {
			_fy = _fx;
		} else {
			_fx = _fy;
		}
	} else {
		_fx = 1.0f;
		_fy = 1.0f;
	}

	outWidth = ROUND(_fx * _width);
	outHeight = ROUND(_fy * _height);
}
	
void CResizeImage::resize() {
	if (_newWidth <= 0 || _newHeight <= 0 || _width <= 0 || _height <= 0) {
		return;
	}

	_fx = (float)_width / _newWidth;
	_fy = (float)_height / _newHeight;

	if (_fx > 1.0 || _fy > 1.0) {
		scaleDown();
	} else {
		scaleUp();
	}
}

void CResizeImage::scaleDown() {
	int nrRows = (int)ceil(_fy + 1.0f);
	byte **rows = new byte *[nrRows];
	float *dxA;
	float *dxB;
	float *dyA = new float[nrRows];
	int *ixA;
	int *iyA = new int[nrRows];
	int *nrxA;
	byte *newLine = _newBuffer;
	byte *c1;
	byte *c2;
	float startY;
	float endY = 0.0f;
	float dy;
	float diffY;
	int pos;
	int nrx;
	int nrx1;
	int nry;
	int t3;
	int t6;
	float sum1;
	float sum2;
	float sum3;
	float sum4;
	float f;
	float area;
	float rawArea;

	scaleDownPreCalculate(ixA, dxA, nrxA, dxB);

	for (int t1 = 0; t1 < _newHeight; t1++) {
		pos = t1 * _fy;
		t3 = pos + nrRows < _height + 1 ? 0 : pos - _height + 1;

		for (int t2 = 0; t2 < nrRows + t3; t2++) {
			rows[t2] = _buffer + (pos + t2) * _rowSize;
		}

		startY = t1 * _fy - pos;
		endY = (t1 + 1) * _fy - pos + t3;
		diffY = endY - startY;
		nry = 0;

		for (float y = startY; y < endY; y = floor(y + 1.0f)) {
			iyA[nry] = (int)y;
			if (endY - y > 1.0f) {
				dyA[nry] = floor(y + 1.0f) - y;
			} else {
				dyA[nry] = endY - y;
			}
			nry++;
		}

		nrx = 0;
		t6 = 0;

		for (int t2 = 0; t2 < _newWidth; t2++) {
			t3 = nrxA[t2];
			rawArea = 1.0f / (dxB[t2] * diffY);
			area = rawArea * GAMMASIZE;
			sum1 = 0.0f;
			sum2 = 0.0f;
			sum3 = 0.0f;
			sum4 = 0.0f;

			for (int t4 = 0; t4 < nry; t4++) {
				c1 = rows[iyA[t4]];
				dy = dyA[t4];
				nrx1 = nrx;

				for (int t5 = 0; t5 < t3; t5++) {
					f = dxA[nrx1] * dy;
					c2 = c1 + ixA[nrx1];

					sum1 += _toGamma[c2[0]] * f;
					sum2 += _toGamma[c2[1]] * f;
					sum3 += _toGamma[c2[2]] * f;
					sum4 += c2[3] * f;
					nrx1++;
				}
			}
			
			newLine[t6 + 0] = _fromGamma[(int)(sum1 * area)];
			newLine[t6 + 1] = _fromGamma[(int)(sum2 * area)];
			newLine[t6 + 2] = _fromGamma[(int)(sum3 * area)];
			newLine[t6 + 3] = (byte)(CAP((sum4 * rawArea) / 255.0f) * 255.0f);

			nrx += t3;
			t6  += _newBpp;
		}
		newLine += _newRowSize;
	}

	delete[] rows;
	delete[] nrxA;
	delete[] ixA;
	delete[] iyA;
	delete[] dxA;
	delete[] dyA;
	delete[] dxB;
}

void CResizeImage::scaleDownPreCalculate(int *&ixA, float *&dxA, int *&nrxA, float *&dxB) {
	int nrCols = (int)ceil(_fx + 1.0f);
	int nrx = 0;
	int nrx1;
	float startX = 0.0f;
	float endX = _fx;

	dxA = new float[nrCols * _newWidth];
	dxB = new float[_newWidth];
	ixA = new int[nrCols * _newWidth];
	nrxA = new int[_newWidth];

	for (int t1 = 0; t1 < _newWidth; t1++) {
		endX = endX < _width + 1 ? endX : endX - _width + 1;

		for (float x = startX; x < endX; x = floor(x + 1.0f)) {
			ixA[nrx] = (int)x * _bpp;

			if (endX - x > 1.0f) {
				dxA[nrx] = floor(x + 1.0f) - x;
			} else {
				dxA[nrx] = endX - x;
			}
			nrx++;
		}

		if (t1 > 0) {
			nrxA[t1] = nrx - nrx1;
		} else {
			nrxA[t1] = nrx;
		}

		nrx1 = nrx;
		dxB[t1] = endX - startX;
		startX = endX;
		endX += _fx;
	}
}

void CResizeImage::scaleUp() {
	int *startX;
	int *endX;
	int *startY;
	int *endY;
	float **hWeights;
	float **vWeights;
	float *hDensity;
	float *vDensity;
	float *vWeight;
	float *hWeight;
	float sumh1;
	float sumh2;
	float sumh3;
	float sumh4;
	float sumv1;
	float sumv2;
	float sumv3;
	float sumv4;

	scaleUpPreCalculate(startX, endX, startY, endY, hWeights, vWeights, hDensity, vDensity);

	for (int t1 = 0; t1 < _newHeight; t1++) {
		int nmaxv = endY[t1] - startY[t1];
		int nmaxh;
		byte *line = _newBuffer + t1 * _newRowSize;

		for (int t2 = 0; t2 < _newWidth; t2++) {
			nmaxh = endX[t2] - startX[t2];
			byte *yPos = _buffer + startY[t1] * _rowSize;

			sumv1 = 0.0f;
			sumv2 = 0.0f;
			sumv3 = 0.0f;
			sumv4 = 0.0f;
			vWeight = vWeights[t1];

			for (int t3 = 0; t3 < nmaxv; t3++) {
				byte *xPos = yPos + startX[t2] * _bpp;

				sumh1 = 0.0f;
				sumh2 = 0.0f;
				sumh3 = 0.0f;
				sumh4 = 0.0f;
				hWeight = hWeights[t2];

				for (int t4 = 0; t4 < nmaxh; t4++) {
					sumh1 += *hWeight * _toGamma[xPos[0]];
					sumh2 += *hWeight * _toGamma[xPos[1]];
					sumh3 += *hWeight * _toGamma[xPos[2]];
					sumh4 += *hWeight * xPos[3];

					xPos += _bpp;
					hWeight++;
				}

				sumv1 += *vWeight * sumh1;
				sumv2 += *vWeight * sumh2;
				sumv3 += *vWeight * sumh3;
				sumv4 += *vWeight * sumh4;

				yPos += _rowSize;
				vWeight++;
			}

			line[0] = _fromGamma[(int)(CAP(sumv1) * GAMMASIZE)];
			line[1] = _fromGamma[(int)(CAP(sumv2) * GAMMASIZE)];
			line[2] = _fromGamma[(int)(CAP(sumv3) * GAMMASIZE)];
			line[3] = (byte)(CAP(sumv4 / 255.0f) * 255.0f);

			line += _newBpp;
		}
	}

	delete[] startX;
	delete[] endX;
	delete[] startY;
	delete[] endY;
	for (int t1 = 0; t1 < _newWidth; t1++) {
		delete[] hWeights[t1];
	}
	delete[] hWeights;
	for (int t1 = 0; t1 < _newHeight; t1++) {
		delete[] vWeights[t1];
	}
	delete[] vWeights;
	delete[] hDensity;
	delete[] vDensity;
}

void CResizeImage::scaleUpPreCalculate(int *&startX, int *&endX, int *&startY, int *&endY, 
                                       float **&hWeights, float **&vWeights, float *&hDensity, float *&vDensity) {
	float center;
	float start;
	int nmax;

	startX = new int[_newWidth];
	endX = new int[_newWidth];
	startY = new int[_newHeight];
	endY = new int[_newHeight];
	hWeights = new float *[_newWidth];
	vWeights = new float *[_newHeight];
	hDensity = new float[_newWidth];
	vDensity = new float[_newHeight];

	for (int t1 = 0; t1 < _newWidth; t1++) {
		center = ((float)t1 + 0.5f) * _fx;
		startX[t1] = MAX<int>(center - LANCZOS_WINDOW, 0);
		endX[t1] = MIN<int>((int)ceil(center + LANCZOS_WINDOW), _width - 1);
		nmax = endX[t1] - startX[t1];
		start = (float)startX[t1] + 0.5f - center;

		hWeights[t1] = new float[nmax];
		hDensity[t1] = 0.0f;

		for (int t2 = 0; t2 < nmax; t2++) {
			hWeights[t1][t2] = lanczos((start + t2) / LANCZOS_BLUR);
			hDensity[t1] += hWeights[t1][t2];
		}

		for (int t2 = 0; t2 < nmax; t2++) {
			hWeights[t1][t2] /= hDensity[t1];
		}
	}

	for (int t1 = 0; t1 < _newHeight; t1++) {
		center = ((float)t1 + 0.5f) * _fy;
		startY[t1] = MAX<int>(center - LANCZOS_WINDOW, 0);
		endY[t1] = MIN<int>((int)ceil(center + LANCZOS_WINDOW), _height - 1);
		nmax = endY[t1] - startY[t1];
		start = (float)startY[t1] + 0.5f - center;

		vWeights[t1] = new float[nmax];
		vDensity[t1] = 0.0f;

		for (int t2 = 0; t2 < nmax; t2++) {
			vWeights[t1][t2] = lanczos((start + t2) / LANCZOS_BLUR);
			vDensity[t1] += vWeights[t1][t2];
		}

		for (int t2 = 0; t2 < nmax; t2++) {
			vWeights[t1][t2] /= vDensity[t1];
		}
	}
}

float CResizeImage::lanczos(float x) {
	if (x == 0.0f) {
		return 1.0f;
	} else {
		float xpi = x * 3.141593f;
		return LANCZOS_WINDOW * sin(xpi) * sin(xpi / LANCZOS_WINDOW) / (xpi * xpi);
	}
}

float *CResizeImage::_toGamma = nullptr;
byte *CResizeImage::_fromGamma = nullptr;

void CResizeImage::initGamma() {
	if (_toGamma != nullptr && _fromGamma != nullptr) {
		return;
	}

	_toGamma = new float[256];
	for (int i1 = 0; i1 < 256; i1++) {
		_toGamma[i1] = (float)pow(i1 / 255.0, 2.2);
	}

	_fromGamma = new byte[GAMMASIZE + 1];
	for (int i2 = 0; i2 <= GAMMASIZE; i2++) {
		_fromGamma[i2] = (byte)(pow((double)i2 / GAMMASIZE, 1 / 2.2) * 255);
	}

	for (int i3 = GAMMASIZE + 1; i3 <= GAMMASIZE; i3++) {
		_fromGamma[i3] = 255;
	}
}

void CResizeImage::freeGamma() {
	if (_toGamma != nullptr) {
		delete[] _toGamma;
		_toGamma = nullptr;
	}
	if (_fromGamma != nullptr) {
		delete[] _fromGamma;
		_fromGamma = nullptr;
	}
}

} // End of namespace Graphics
