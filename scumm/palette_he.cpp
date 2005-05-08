/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001  Ludvig Strigeus
 * Copyright (C) 2001-2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "common/system.h"
#include "common/util.h"
#include "scumm/scumm.h"
#include "scumm/intern.h"
#include "scumm/resource.h"
#include "scumm/util.h"


namespace Scumm {

void ScummEngine_v72he::remapHEPalette(const uint8 *src, uint8 *dst) {
	int r, g, b, sum, bestitem, bestsum;
	int ar, ag, ab;
	uint8 *palPtr;
	src += 30;

	if (_heversion >= 99) {
		palPtr = _hePalettes + 1024 + 30;
	} else {
		palPtr = _currentPalette + 30;
	}

	for (int j = 10; j < 246; j++) {
		bestitem = 0xFFFF;
		bestsum = 0xFFFF;

		r = *src++;
		g = *src++;
		b = *src++;

		uint8 *curPal = palPtr;

		for (int k = 10; k < 246; k++) {
			ar = r - *curPal++;
			ag = g - *curPal++;
			ab = b - *curPal++;

			sum = (ar * ar) + (ag * ag) + (ab * ab);

			if (bestitem == 0xFFFF || sum <= bestsum) {
				bestitem = k;
				bestsum = sum;
			}
		}

		dst[j] = bestitem;
	}
}

uint8 *ScummEngine_v90he::getHEPaletteIndex(int palSlot) {
	if (palSlot) {
		assert(palSlot >= 1 && palSlot <= _numPalettes);
		return _hePalettes + palSlot * 1024;
	} else {
		return _hePalettes + 1024;
	}
}

int ScummEngine_v90he::getHEPaletteSimilarColor(int palSlot, int red, int green, int start, int end) {
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	checkRange(255, 0, start, "Invalid palette slot %d");
	checkRange(255, 0, end, "Invalid palette slot %d");

	uint8 *pal = _hePalettes + palSlot * 1024 + start * 3;

	int bestsum = 0xFFFFFFFF;
	int bestitem = start;

	for (int i = start; i <= end; i++) {
		int dr = red - pal[0];
		int dg = green - pal[1];
		int sum = dr * dr + dg * dg * 2;
		if (sum == 0) {
			return i;
		}
		if (sum < bestsum) {
			bestsum = sum;
			bestitem = i;
		}
		pal += 3;
	}
	return bestitem;
}

int ScummEngine_v90he::getHEPaletteColorComponent(int palSlot, int color, int component) {
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	checkRange(255, 0, color, "Invalid palette slot %d");

	return _hePalettes[palSlot * 1024 + color * 3 + component % 3];
}

int ScummEngine_v90he::getHEPaletteColor(int palSlot, int color) {
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	checkRange(255, 0, color, "Invalid palette slot %d");

	return _hePalettes[palSlot * 1024 + 768 + color];
}

void ScummEngine_v90he::setHEPaletteColor(int palSlot, uint8 color, uint8 r, uint8 g, uint8 b) {
	debug(7, "setHEPaletteColor(%d, %d, %d, %d, %d)", palSlot, color, r, g, b);
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	uint8 *p = _hePalettes + palSlot * 1024 + color * 3;
	*(p + 0) = r;
	*(p + 1) = g;
	*(p + 2) = b;
	_hePalettes[palSlot * 1024 + 768 + color] = color;
}

void ScummEngine_v90he::setHEPaletteFromPtr(int palSlot, const uint8 *palData) {
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	uint8 *pc = _hePalettes + palSlot * 1024;
	uint8 *pi = pc + 768;
	for (int i = 0; i < 256; ++i) {
		*pc++ = *palData++;
		*pc++ = *palData++;
		*pc++ = *palData++;
		*pi++ = i;
	}
}

void ScummEngine_v90he::setHEPaletteFromCostume(int palSlot, int resId) {
	debug(7, "setHEPaletteFromCostume(%d, %d)", palSlot, resId);
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	const uint8 *data = getResourceAddress(rtCostume, resId);
	assert(data);
	const uint8 *rgbs = findResourceData(MKID('RGBS'), data);
	assert(rgbs);
	setHEPaletteFromPtr(palSlot, rgbs);
}

void ScummEngine_v90he::setHEPaletteFromImage(int palSlot, int resId, int state) {
	debug(7, "setHEPaletteFromImage(%d, %d, %d)", palSlot, resId, state);
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	uint8 *data = getResourceAddress(rtImage, resId);
	assert(data);
	const uint8 *rgbs = findWrappedBlock(MKID('RGBS'), data, state, 0);
	assert(rgbs);
	setHEPaletteFromPtr(palSlot, rgbs);
}

void ScummEngine_v90he::setHEPaletteFromRoom(int palSlot, int resId, int state) {
	debug(7, "setHEPaletteFromRoom(%d, %d, %d)", palSlot, resId, state);
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	const uint8 *data = getResourceAddress(rtRoom, resId);
	assert(data);
	const uint8 *pals = findResourceData(MKID('PALS'), data);
	assert(pals);
	const uint8 *rgbs = findPalInPals(pals, state);
	assert(rgbs);
	setHEPaletteFromPtr(palSlot, rgbs);
}

void ScummEngine_v90he::restoreHEPalette(int palSlot) {
	debug(7, "restoreHEPalette(%d)", palSlot);
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	if (palSlot != 1) {
		memcpy(_hePalettes + palSlot * 1024, _hePalettes + 1024, 1024);
	}
}

void ScummEngine_v90he::copyHEPalette(int dstPalSlot, int srcPalSlot) {
	debug(7, "copyHEPalette(%d, %d)", dstPalSlot, srcPalSlot);
	assert(dstPalSlot >= 1 && dstPalSlot <= _numPalettes);
	assert(srcPalSlot >= 1 && srcPalSlot <= _numPalettes);
	if (dstPalSlot != srcPalSlot) {
		memcpy(_hePalettes + dstPalSlot * 1024, _hePalettes + srcPalSlot * 1024, 1024);
	}
}

void ScummEngine_v90he::copyHEPaletteColor(int palSlot, uint8 dstColor, uint8 srcColor) {
	debug(7, "copyHEPaletteColor(%d, %d, %d)", palSlot, dstColor, srcColor);
	checkRange(_numPalettes, 1, palSlot, "Invalid palette %d");
	uint8 *dstPal = _hePalettes + palSlot * 1024 + dstColor * 3;
	uint8 *srcPal = _hePalettes + 1024 + srcColor * 3;
	memcpy(dstPal, srcPal, 3);
	_hePalettes[palSlot * 1024 + 768 + dstColor] = srcColor;
}

void ScummEngine_v99he::setPaletteFromPtr(const byte *ptr, int numcolor) {
	int i;
	byte *dest, r, g, b;

	if (numcolor < 0) {
		numcolor = getResourceDataSize(ptr) / 3;
	}

	checkRange(256, 0, numcolor, "Too many colors (%d) in Palette");

	dest = _hePalettes + 1024;

	for (i = 0; i < numcolor; i++) {
		r = *ptr++;
		g = *ptr++;
		b = *ptr++;

		if (i == 15 || r < 252 || g < 252 || b < 252) {
			*dest++ = r;
			*dest++ = g;
			*dest++ = b;
			_hePalettes[1792 + i] = i;
		} else {
			dest += 3;
		}
	}

	memcpy(_hePalettes, _hePalettes + 1024, 768);

	for (i = 0; i < 10; ++i)
		_hePalettes[1792 + i] = i;
	for (i = 246; i < 256; ++i)
		_hePalettes[1792 + i] = i;

	setDirtyColors(0, numcolor - 1);
}

void ScummEngine_v99he::darkenPalette(int redScale, int greenScale, int blueScale, int startColor, int endColor) {
	uint8 *src, *dst;
	int color, j;

	src = _hePalettes + startColor * 3;
	dst = _hePalettes + 1024 + startColor * 3;
	for (j = startColor; j <= endColor; j++) {
		color = *src++;
		color = color * redScale / 0xFF;
		if (color > 255)
			color = 255;
		*dst++ = color;

		color = *src++;
		color = color * greenScale / 0xFF;
		if (color > 255)
			color = 255;
		*dst++ = color;

		color = *src++;
		color = color * blueScale / 0xFF;
		if (color > 255)
			color = 255;
		*dst++ = color;

		_hePalettes[1792 + j] = j;
		setDirtyColors(j, endColor);
	}
}

void ScummEngine_v99he::copyPalColor(int dst, int src) {
	byte *dp, *sp;

	if ((uint) dst >= 256 || (uint) src >= 256)
		error("copyPalColor: invalid values, %d, %d", dst, src);

	dp = &_hePalettes[1024 + dst * 3];
	sp = &_hePalettes[1024 + src * 3];

	dp[0] = sp[0];
	dp[1] = sp[1];
	dp[2] = sp[2];
	_hePalettes[1792 + dst] = dst;

	setDirtyColors(dst, dst);
}

void ScummEngine_v99he::setPalColor(int idx, int r, int g, int b) {
	_hePalettes[1024 + idx * 3 + 0] = r;
	_hePalettes[1024 + idx * 3 + 1] = g;
	_hePalettes[1024 + idx * 3 + 2] = b;
	_hePalettes[1792 + idx] = idx;
	setDirtyColors(idx, idx);
}

void ScummEngine_v99he::updatePalette() {
	if (_palDirtyMax == -1)
		return;

	int num = _palDirtyMax - _palDirtyMin + 1;
	int i;

	byte palette_colors[1024];
	byte *p = palette_colors;

	for (i = _palDirtyMin; i <= _palDirtyMax; i++) {
		byte *data = _hePalettes + 1024 + i * 3;

		*p++ = data[0];
		*p++ = data[1];
		*p++ = data[2];
		*p++ = 0;
	}
	
	_system->setPalette(palette_colors, _palDirtyMin, num);

	_palDirtyMax = -1;
	_palDirtyMin = 256;
}

} // End of namespace Scumm
