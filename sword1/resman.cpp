/* ScummVM - Scumm Interpreter
 * Copyright (C) 2003-2004 The ScummVM project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "scummsys.h"
#include "memman.h"
#include "resman.h"
#include "sworddefs.h"
#include "base/engine.h"
#include "common/util.h"
#include "swordres.h"

namespace Sword1 {

#define MAX_PATH_LEN 260

ResMan::ResMan(const char *resFile, MemMan *pMemoMan) {
	_memMan = pMemoMan;
	loadCluDescript(resFile);
}

ResMan::~ResMan(void) {
	freeCluDescript();
}

void ResMan::loadCluDescript(const char *fileName) {
	File resFile;
	resFile.open(fileName);
	if (!resFile.isOpen())
		error("ResMan::loadCluDescript(): File %s not found!", fileName);
	
	_prj.noClu = resFile.readUint32LE();
	_prj.clu = new Clu*[_prj.noClu];

	uint32 *cluIndex = (uint32*)malloc(_prj.noClu * 4);
	resFile.read(cluIndex, _prj.noClu * 4);

	for (uint32 clusCnt = 0; clusCnt < _prj.noClu; clusCnt++)
		if (cluIndex[clusCnt]) {
			Clu *cluster = _prj.clu[clusCnt] = new Clu;
			resFile.read(cluster->label, MAX_LABEL_SIZE);

			cluster->noGrp = resFile.readUint32LE();
			cluster->grp = new Grp*[cluster->noGrp];

			uint32 *grpIndex = (uint32*)malloc(cluster->noGrp * 4);
			resFile.read(grpIndex, cluster->noGrp * 4);

			for (uint32 grpCnt = 0; grpCnt < cluster->noGrp; grpCnt++)
				if (grpIndex[grpCnt]) {
					Grp *group = cluster->grp[grpCnt] = new Grp;
					group->noRes = resFile.readUint32LE();
					group->resHandle = new MemHandle[group->noRes];
					group->offset = new uint32[group->noRes];
					group->length = new uint32[group->noRes];
					uint32 *resIdIdx = (uint32*)malloc(group->noRes * 4);
					resFile.read(resIdIdx, group->noRes * 4);

					for (uint32 resCnt = 0; resCnt < group->noRes; resCnt++) {
						if (resIdIdx[resCnt]) {
							group->offset[resCnt] = resFile.readUint32LE();
							group->length[resCnt] = resFile.readUint32LE();
							_memMan->initHandle(group->resHandle + resCnt);
						} else {
							group->offset[resCnt] = 0xFFFFFFFF;
							group->length[resCnt] = 0;
							_memMan->initHandle(group->resHandle + resCnt);
						}
					}
					free(resIdIdx);
				} else
					cluster->grp[grpCnt] = NULL;
			free(grpIndex);
		} else
			_prj.clu[clusCnt] = NULL;
	free(cluIndex);
	
	if (_prj.clu[3]->grp[5]->noRes == 29)
		for (uint8 cnt = 0; cnt < 29; cnt++)
			_srIdList[cnt] = 0x04050000 | cnt;
}

void ResMan::freeCluDescript(void) {
	
	for (uint32 clusCnt = 0; clusCnt < _prj.noClu; clusCnt++)
		if (Clu *cluster = _prj.clu[clusCnt]) {
			for (uint32 grpCnt = 0; grpCnt < cluster->noGrp; grpCnt++)
				if (Grp *group = cluster->grp[grpCnt]) {
					for (uint32 resCnt = 0; resCnt < group->noRes; resCnt++)
						_memMan->freeNow(group->resHandle + resCnt);
					delete[] group->resHandle;
					delete[] group->offset;
					delete[] group->length;
					delete group;
				}
			delete[] cluster->grp;
			delete cluster;
		}
	delete[] _prj.clu;
}

void ResMan::flush(void) {
	for (uint32 clusCnt = 0; clusCnt < _prj.noClu; clusCnt++)
		if (Clu *cluster = _prj.clu[clusCnt])
			for (uint32 grpCnt = 0; grpCnt < cluster->noGrp; grpCnt++)
				if (Grp *group = cluster->grp[grpCnt])
					for (uint32 resCnt = 0; resCnt < group->noRes; resCnt++)
						if (group->resHandle[resCnt].cond != MEM_FREED) {
							_memMan->setCondition(group->resHandle + resCnt, MEM_CAN_FREE);
							group->resHandle[resCnt].refCount = 0;
						}
}

void *ResMan::fetchRes(uint32 id) {
	MemHandle *memHandle = resHandle(id);
	if (!memHandle->data)
		error("fetchRes:: resource %d is not open!", id);
	return memHandle->data;
}

void *ResMan::openFetchRes(uint32 id) {
	resOpen(id);
	return fetchRes(id);
}

void ResMan::dumpRes(uint32 id) {
	char outn[30];
	sprintf(outn, "DUMP%08X.BIN", id);
	File outf;
	if (outf.open(outn, File::kFileWriteMode, "")) {
		resOpen(id);
		MemHandle *memHandle = resHandle(id);
		outf.write(memHandle->data, memHandle->size);
		outf.close();
		resClose(id);
	}
}

Header *ResMan::lockScript(uint32 scrID) {
	if (!_scriptList[scrID / ITM_PER_SEC])
		error("Script id %d not found.\n", scrID);
	scrID = _scriptList[scrID / ITM_PER_SEC];
#ifdef SCUMM_BIG_ENDIAN
	MemHandle *memHandle = resHandle(scrID);
	if (memHandle->cond == MEM_FREED)
		openScriptResourceBigEndian(scrID);
	else
		resOpen(scrID);
#else
	resOpen(scrID);
#endif
	return (Header*)resHandle(scrID)->data;
}

void ResMan::unlockScript(uint32 scrID) {
	resClose(_scriptList[scrID / ITM_PER_SEC]);
}

void *ResMan::cptResOpen(uint32 id) {
#ifdef SCUMM_BIG_ENDIAN
	MemHandle *memHandle = resHandle(id);
	if (memHandle->cond == MEM_FREED)
		openCptResourceBigEndian(id);
	else
		resOpen(id);
#else
	resOpen(id);
#endif
	return resHandle(id)->data;
}

void ResMan::resOpen(uint32 id) {  // load resource ID into memory
	MemHandle *memHandle = resHandle(id);
	if (memHandle->cond == MEM_FREED) { // memory has been freed
		uint32 size = resLength(id);
		_memMan->alloc(memHandle, size);
		File *clusFile = openClusterFile(id);
		clusFile->seek( resOffset(id) );
		clusFile->read( memHandle->data, size);
		if (clusFile->ioFailed())
			error("Can't read %d bytes from cluster %d\n", size, id);
		clusFile->close();
		delete clusFile;
	} else
		_memMan->setCondition(memHandle, MEM_DONT_FREE);
	memHandle->refCount++;
	if (memHandle->refCount > 20) {
		debug(1, "%d references to id %d. Guess there's something wrong.", memHandle->refCount, id);
	}
}

void ResMan::resClose(uint32 id) {
	MemHandle *handle = resHandle(id);
	if (!handle->refCount) {
		warning("Resource Manager fail: unlocking object with refCount 0. Id: %d\n", id);		
	} else {
		handle->refCount--;
		if (!handle->refCount)
			_memMan->setCondition( handle, MEM_CAN_FREE);
	}
}

FrameHeader *ResMan::fetchFrame(void *resourceData, uint32 frameNo) {
	uint8 *frameFile = (uint8*)resourceData;
	uint8 *idxData = frameFile + sizeof(Header);
	if (frameNo >= READ_LE_UINT32(idxData))
		error("fetchFrame:: frame %d doesn't exist in resource.", frameNo);
	frameFile += READ_LE_UINT32(idxData + (frameNo+1) * 4);
	return (FrameHeader*)frameFile;
}

File *ResMan::openClusterFile(uint32 id) {
	File *clusFile = new File();
	char fileName[15];
	sprintf(fileName, "%s.CLU", _prj.clu[(id >> 24)-1]->label);
	clusFile->open(fileName);
	if (!clusFile->isOpen())
		error("Can't open cluster file %s", fileName);
	return clusFile;
}

MemHandle *ResMan::resHandle(uint32 id) {
	if ((id >> 16) == 0x0405)
		id = _srIdList[id & 0xFFFF];
	uint8 cluster = (uint8)((id >> 24) - 1);
	uint8 group = (uint8)(id >> 16);

	return &(_prj.clu[cluster]->grp[group]->resHandle[id & 0xFFFF]);
}

uint32 ResMan::resLength(uint32 id) {
	if ((id >> 16) == 0x0405)
		id = _srIdList[id & 0xFFFF];
	uint8 cluster = (uint8)((id >> 24) - 1);
	uint8 group = (uint8)(id >> 16);

	return _prj.clu[cluster]->grp[group]->length[id & 0xFFFF];
}

uint32 ResMan::resOffset(uint32 id) {
	if ((id >> 16) == 0x0405)
		id = _srIdList[id & 0xFFFF];
	uint8 cluster = (uint8)((id >> 24) - 1);
	uint8 group = (uint8)(id >> 16);

	return _prj.clu[cluster]->grp[group]->offset[id & 0xFFFF];
}

void ResMan::openCptResourceBigEndian(uint32 id) {
	resOpen(id);
	MemHandle *handle = resHandle(id);
	uint32 totSize = handle->size;
	uint32 *data = (uint32*)((uint8*)handle->data + sizeof(Header));
	totSize -= sizeof(Header);
	if (totSize & 3)
		error("Illegal compact size for id %d: %d", id, totSize);
	totSize /= 4;
	for (uint32 cnt = 0; cnt < totSize; cnt++) {
		*data = READ_LE_UINT32(data);
		data++;
	}
}

void ResMan::openScriptResourceBigEndian(uint32 id) {
	resOpen(id);
	MemHandle *handle = resHandle(id);
	// uint32 totSize = handle->size;
	Header *head = (Header*)handle->data;
	head->comp_length = FROM_LE_32(head->comp_length);
	head->decomp_length = FROM_LE_32(head->decomp_length);
	head->version = FROM_LE_16(head->version);
	uint32 *data = (uint32*)((uint8*)handle->data + sizeof(Header));
	uint32 size = handle->size - sizeof(Header);
	if (size & 3)
		error("Odd size during script endian conversion. Resource ID =%d, size = %d", id, size);
	size >>= 2;
	for (uint32 cnt = 0; cnt < size; cnt++) {
		*data = READ_LE_UINT32(data);
		data++;
	}
}

uint32 ResMan::_srIdList[29] = { // the file numbers differ for the control panel file IDs, so we need this array
	OTHER_SR_FONT,		// SR_FONT
	0x04050000,			// SR_BUTTON
	OTHER_SR_REDFONT,	// SR_REDFONT
	0x04050001,			// SR_PALETTE
	0x04050002,			// SR_PANEL_ENGLISH
	0x04050003,			// SR_PANEL_FRENCH
	0x04050004,			// SR_PANEL_GERMAN
	0x04050005,			// SR_PANEL_ITALIAN
	0x04050006,			// SR_PANEL_SPANISH
	0x04050007,			// SR_PANEL_AMERICAN
	0x04050008,			// SR_TEXT_BUTTON
	0x04050009,			// SR_SPEED
	0x0405000A,			// SR_SCROLL1
	0x0405000B,			// SR_SCROLL2
	0x0405000C,			// SR_CONFIRM
	0x0405000D,			// SR_VOLUME
	0x0405000E,			// SR_VLIGHT
	0x0405000F,			// SR_VKNOB
	0x04050010,			// SR_WINDOW
	0x04050011,			// SR_SLAB1
	0x04050012,			// SR_SLAB2
	0x04050013,			// SR_SLAB3
	0x04050014,			// SR_SLAB4
	0x04050015,			// SR_BUTUF
	0x04050016,			// SR_BUTUS
	0x04050017,			// SR_BUTDS
	0x04050018,			// SR_BUTDF
	0x04050019,			// SR_DEATHPANEL
	0,
};

} // End of namespace Sword1
