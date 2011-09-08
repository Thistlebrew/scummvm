/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * Additional copyright for this file:
 * Copyright (C) 1995-1997 Presto Studios, Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/stream.h"

#include "pegasus/pegasus.h"
#include "pegasus/items/inventory/inventoryitem.h"

namespace Pegasus {

InventoryItem::InventoryItem(const tItemID id, const tNeighborhoodID neighborhood, const tRoomID room, const tDirectionConstant direction) :
		Item(id, neighborhood, room, direction) {

	PegasusEngine *vm = (PegasusEngine *)g_engine;

	Common::SeekableReadStream *leftInfo = vm->_resFork->getResource(MKTAG('L', 'e', 'f', 't'), kItemBaseResID + id);
	if (leftInfo) {
		_leftAreaInfo = readItemState(leftInfo);
		delete leftInfo;
	} else {
		_leftAreaInfo.numEntries = 0;
		_leftAreaInfo.entries = 0;
	}

	Common::SeekableReadStream *inventoryInfo = vm->_resFork->getResource(MKTAG('I', 'n', 'v', 'I'), kItemBaseResID + id);
	if (inventoryInfo) {
		_inventoryInfo.panelStart = inventoryInfo->readUint32BE();
		_inventoryInfo.panelStop = inventoryInfo->readUint32BE();
		delete inventoryInfo;
	} else {
		_inventoryInfo.panelStart = _inventoryInfo.panelStop = 0;
	}

	_itemAnimationTime = 0;
}

InventoryItem::~InventoryItem() {
	delete[] _leftAreaInfo.entries;
}

tItemType InventoryItem::getItemType() {
	return kInventoryItemType;
}

TimeValue InventoryItem::getLeftAreaTime() const {
	if (!_leftAreaInfo.entries)
		return 0xffffffff;

	TimeValue time;
	tItemState state;

	findItemStateEntryByState(_leftAreaInfo, _itemState, time);
	if (time == 0xffffffff)
		getItemStateEntry(_leftAreaInfo, 0, state, time);

	return time;
}

void InventoryItem::setAnimationTime(const TimeValue time) {
	_itemAnimationTime = time;
}

TimeValue InventoryItem::getAnimationTime() const {
	return _itemAnimationTime;
}

//	Must affect images in left area.
void InventoryItem::select() {
	Item::select();

	// TODO: AI
}

void InventoryItem::deselect() {
	Item::deselect();

	// TODO: AI
}

void InventoryItem::getPanelTimes(TimeValue &start, TimeValue &stop) const {
	start = _inventoryInfo.panelStart;
	stop = _inventoryInfo.panelStop;
}

} // End of namespace Pegasus
