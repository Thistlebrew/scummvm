/* ScummVM - Scumm Interpreter
 * Copyright (C) 2004 Ivan Dubrov
 * Copyright (C) 2004-2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "common/stdafx.h"
#include "common/endian.h"

#include "gob/gob.h"
#include "gob/video.h"
#include "gob/anim.h"
#include "gob/draw.h"
#include "gob/scenery.h"
#include "gob/mult.h"
#include "gob/util.h"
#include "gob/inter.h"
#include "gob/parse.h"
#include "gob/global.h"
#include "gob/sound.h"
#include "gob/palanim.h"
#include "gob/game.h"

namespace Gob {

Mult::Mult(GobEngine *vm) : _vm(vm) {
	_objects = 0;
	_renderData = 0;
	_renderData2 = 0;
	_objCount = 0;
	_underAnimSurf = 0;
	_multData = 0;
	_frame = 0;
	_doPalSubst = 0;
	_counter = 0;
	_frameRate = 0;

	_animArrayX = 0;
	_animArrayY = 0;
	_animArrayData = 0;

	_index = 0;

	_staticKeysCount = 0;
	_staticKeys = 0;
	int i;
	for (i = 0; i < 10; i++)
		_staticIndices[i] = 0;

	for (i = 0; i < 4; i++) {
		_animKeys[i] = 0;
		_animKeysCount[i] = 0;
	}
	_animLayer = 0;
	for (i = 0; i < 10; i++)
		_animIndices[i] = 0;

	_textKeysCount = 0;
	_textKeys = 0;

	_frameStart = 0;

	_palKeyIndex = 0;
	_palKeysCount = 0;
	_palKeys = 0;
	_oldPalette = 0;
	_palAnimKey = 0;
	for (i = 0; i < 256; i++) {
		_palAnimPalette[i].red = 0;
		_palAnimPalette[i].green = 0;
		_palAnimPalette[i].blue = 0;
	}
	for (i = 0; i < 4; i++) {
		_palAnimIndices[i] = 0;
		_palAnimRed[i] = 0;
		_palAnimGreen[i] = 0;
		_palAnimBlue[i] = 0;
	}

	_palFadeKeys = 0;
	_palFadeKeysCount = 0;
	_palFadingRed = 0;
	_palFadingGreen = 0;
	_palFadingBlue = 0;

	_animDataAllocated = 0;

	_dataPtr = 0;
	for (i = 0; i < 10; i++) {
		_staticLoaded[i] = 0;
		_animLoaded[i] = 0;
	}
	_sndSlotsCount = 0;

	_sndKeysCount = 0;
	_sndKeys = 0;

	for (i = 0; i < 5; i++)
		for (int j = 0; j < 16; j++) {
			_fadePal[i][j].red = 0;
			_fadePal[i][j].green = 0;
			_fadePal[i][j].blue = 0;
		}

	_orderArray = 0;
}

void Mult::interGetObjAnimSize(void) {
	Mult_AnimData *pAnimData;
	int16 objIndex;

	_vm->_inter->evalExpr(&objIndex);
	pAnimData = _objects[objIndex].pAnimData;
	if (pAnimData->isStatic == 0) {
		_vm->_scenery->updateAnim(pAnimData->layer, pAnimData->frame,
		    pAnimData->animation, 0, *(_objects[objIndex].pPosX),
		    *(_objects[objIndex].pPosY), 0);
	}
	WRITE_VAR_OFFSET(_vm->_parse->parseVarIndex(), _vm->_scenery->_toRedrawLeft);
	WRITE_VAR_OFFSET(_vm->_parse->parseVarIndex(), _vm->_scenery->_toRedrawTop);
	WRITE_VAR_OFFSET(_vm->_parse->parseVarIndex(), _vm->_scenery->_toRedrawRight);
	WRITE_VAR_OFFSET(_vm->_parse->parseVarIndex(), _vm->_scenery->_toRedrawBottom);
}

void Mult::freeMult(void) {
	if (_vm->_anim->_animSurf != 0)
		_vm->_video->freeSurfDesc(_vm->_anim->_animSurf);

	delete[] _objects;
	delete[] _renderData;

	_objects = 0;
	_renderData = 0;
	_vm->_anim->_animSurf = 0;
}

void Mult::interLoadMult(void) {
	int16 val;
	int16 objIndex;
	int16 i;
	char *lmultData;

	debugC(4, DEBUG_GAMEFLOW, "interLoadMult: Loading...");

	_vm->_inter->evalExpr(&objIndex);
	_vm->_inter->evalExpr(&val);
	*_objects[objIndex].pPosX = val;
	_vm->_inter->evalExpr(&val);
	*_objects[objIndex].pPosY = val;

	lmultData = (char *)_objects[objIndex].pAnimData;
	for (i = 0; i < 11; i++) {
		if ((char)READ_LE_UINT16(_vm->_global->_inter_execPtr) == (char)99) {
			_vm->_inter->evalExpr(&val);
			lmultData[i] = val;
		} else {
			_vm->_global->_inter_execPtr++;
		}
	}
}

void Mult::freeAll(void) {
	int16 i;

	freeMult();
	for (i = 0; i < 10; i++)
		_vm->_scenery->freeAnim(i);

	for (i = 0; i < 10; i++)
		_vm->_scenery->freeStatic(i);
}

void Mult::initAll(void) {
	int16 i;

	_objects = 0;
	_vm->_anim->_animSurf = 0;
	_renderData = 0;

	for (i = 0; i < 10; i++)
		_vm->_scenery->_animPictCount[i] = 0;

	for (i = 0; i < 20; i++) {
		_vm->_scenery->_spriteRefs[i] = 0;
		_vm->_scenery->_spriteResId[i] = -1;
	}

	for (i = 0; i < 10; i++)
		_vm->_scenery->_staticPictCount[i] = -1;

	_vm->_scenery->_curStaticLayer = -1;
	_vm->_scenery->_curStatic = -1;
}

void Mult::playSound(Snd::SoundDesc * soundDesc, int16 repCount, int16 freq,
	    int16 channel) {
	_vm->_snd->playSample(soundDesc, repCount, freq);
}

void Mult::zeroMultData(void) {
	_multData = 0;
}

void Mult::freeMultKeys(void) {
	int i;
	char animCount;
	char staticCount;

	_dataPtr = _multData;
	staticCount = _dataPtr[0];
	animCount = _dataPtr[1];

	delete[] _dataPtr;

	staticCount++;
	animCount++;
	for (i = 0; i < staticCount; i++) {

		if (_staticLoaded[i] != 0)
			_vm->_scenery->freeStatic(_staticIndices[i]);
	}

	for (i = 0; i < animCount; i++) {
		if (_animLoaded[i] != 0)
			_vm->_scenery->freeAnim(_animIndices[i]);
	}

	delete[] _staticKeys;

	for (i = 0; i < 4; i++)
		delete[] _animKeys[i];

	delete[] _palFadeKeys;
	delete[] _palKeys;
	delete[] _textKeys;

	for (i = 0; i < _sndSlotsCount; i++) {
		_vm->_game->freeSoundSlot(19 - i);
	}

	delete[] _sndKeys;

	_multData = 0;

	if (_animDataAllocated != 0) {
		delete[] _objects;
		_objects = 0;

		delete[] _renderData;
		_renderData = 0;

		delete[] _animArrayX;
		_animArrayX = 0;

		delete[] _animArrayY;
		_animArrayY = 0;

		delete[] _animArrayData;
		_animArrayData = 0;

		if (_vm->_anim->_animSurf)
			_vm->_video->freeSurfDesc(_vm->_anim->_animSurf);
		_vm->_anim->_animSurf = 0;

		_animDataAllocated = 0;
	}
}

void Mult::checkFreeMult(void) {
	if (_multData != 0)
		freeMultKeys();
}

}				// End of namespace Gob
