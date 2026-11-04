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

#ifndef GRAPHICS_BLIT_RESIZE_H
#define GRAPHICS_BLIT_RESIZE_H

#include "common/scummsys.h"

namespace Graphics {

class CResizeImage {
public:
	CResizeImage(int width, int height, int maxWidth, int maxHeight);
	CResizeImage(byte *buffer, int width, int height, int bpp, int srcPitch,
	             byte *newBuffer, int newWidth, int newHeight, int newBpp, int dstPitch);

	void determineNewSize(int &outWidth, int &outHeight);
	void resize();
	static void freeGamma();

private:
	int _width;
	int _newWidth;
	int _height;
	int _newHeight;
	int _bpp;
	int _newBpp;
	int _maxWidth;
	int _maxHeight;

	float _fx;
	float _fy;
	int _rowSize;
	int _newRowSize;

	byte *_buffer;
	byte *_newBuffer;

	void scaleDown();
	void scaleDownPreCalculate(int *&ixA, float *&dxA, int *&nrxA, float *&dxB);
	void scaleUp();
	void scaleUpPreCalculate(int *&startX, int *&endX, int *&startY, int *&endY, 
	                         float **&hWeights, float **&vWeights, float *&hDensity, float *&vDensity);

	float lanczos(float x);

	static float *_toGamma;
	static byte *_fromGamma;
	static void initGamma();
};

} // End of namespace Graphics

#endif
