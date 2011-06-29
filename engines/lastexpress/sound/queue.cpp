/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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

#include "lastexpress/sound/queue.h"

#include "lastexpress/game/logic.h"
#include "lastexpress/game/state.h"

#include "lastexpress/sound/entry.h"

#include "lastexpress/helpers.h"
#include "lastexpress/lastexpress.h"

namespace LastExpress {

#define SOUNDCACHE_ENTRY_SIZE 92160
#define SOUNDCACHE_MAX_SIZE   6

SoundQueue::SoundQueue(LastExpressEngine *engine) : _engine(engine) {
	 _state = 0;
	 _currentType = kSoundType16;
	 _flag = 0;

	// Cache and filter buffers
	memset(&_buffer, 0, sizeof(_buffer));
	_soundCacheData = malloc(6 * SOUNDCACHE_ENTRY_SIZE);

	_subtitlesFlag = 0;
	_currentSubtitle = NULL;
}

SoundQueue::~SoundQueue() {
	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i)
		SAFE_DELETE(*i);
	_soundList.clear();

	// Entries in the cache are just pointers to sound list entries
	_soundCache.clear();

	for (Common::List<SubtitleEntry *>::iterator i = _subtitles.begin(); i != _subtitles.end(); ++i)
		SAFE_DELETE(*i);
	_subtitles.clear();

	_currentSubtitle = NULL;

	free(_soundCacheData);

	// Zero passed pointers
	_engine = NULL;
}

//////////////////////////////////////////////////////////////////////////
// Timer
//////////////////////////////////////////////////////////////////////////
void SoundQueue::handleTimer() {
	Common::StackLock locker(_mutex);

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		SoundEntry *entry = (*i);
		if (entry->getStream() == NULL) {
			SAFE_DELETE(*i);
			i = _soundList.reverse_erase(i);
			continue;
		} else if (!entry->getStreamedSound()) {
			// TODO: stream any sound in the queue after filtering
			entry->loadStream();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Sound queue management
//////////////////////////////////////////////////////////////////////////
void SoundQueue::addToQueue(SoundEntry *entry) {
	_soundList.push_back(entry);
}

void SoundQueue::removeFromQueue(EntityIndex entity) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(entity);
	if (entry)
		entry->reset();
}

void SoundQueue::removeFromQueue(Common::String filename) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(filename);
	if (entry)
		entry->reset();
}

void SoundQueue::updateQueue() {
	Common::StackLock locker(_mutex);

	warning("[Sound::updateQueue] Not implemented");
}

void SoundQueue::resetQueue() {
	Common::StackLock locker(_mutex);

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		if ((*i)->getType() == kSoundType1) {
			(*i)->reset();
			break;
		}
	}

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		if ((*i)->getType() == kSoundType2) {
			(*i)->reset();
			break;
		}
	}
}

void SoundQueue::resetQueue(SoundType type1, SoundType type2) {
	if (!type2)
		type2 = type1;

	Common::StackLock locker(_mutex);

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		if ((*i)->getType() != type1 && (*i)->getType() != type2)
			(*i)->reset();
	}
}

void SoundQueue::clearQueue() {
	_flag |= 4;

	// FIXME: Wait a while for a flag to be set
	//for (int i = 0; i < 3000000; i++)
	//	if (_flag & 8)
	//		break;

	_flag |= 8;

	Common::StackLock locker(_mutex);

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		SoundEntry *entry = (*i);

		// Delete entry
		entry->close();
		SAFE_DELETE(entry);

		i = _soundList.reverse_erase(i);
	}

	updateSubtitles();
}

//////////////////////////////////////////////////////////////////////////
// State
//////////////////////////////////////////////////////////////////////////
void SoundQueue::clearStatus() {
	Common::StackLock locker(_mutex);

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i)
		(*i)->setStatus((*i)->getStatus().status | kSoundStatusClear3);
}

//////////////////////////////////////////////////////////////////////////
// Entry management
//////////////////////////////////////////////////////////////////////////
void SoundQueue::setupEntry(SoundType type, EntityIndex index) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(type);
	if (entry)
		entry->setEntity(index);
}

void SoundQueue::processEntry(EntityIndex entity) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(entity);
	if (entry) {
		entry->update(0);
		entry->setEntity(kEntityPlayer);
	}
}

void SoundQueue::processEntry(SoundType type) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(type);
	if (entry)
		entry->update(0);
}

void SoundQueue::processEntry(Common::String filename) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(filename);
	if (entry) {
		entry->update(0);
		entry->setEntity(kEntityPlayer);
	}
}

void SoundQueue::processEntries() {
	_state = 0;

	processEntry(kSoundType1);
	processEntry(kSoundType2);
}

SoundEntry *SoundQueue::getEntry(EntityIndex index) {
	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		if ((*i)->getEntity() == index)
			return *i;
	}

	return NULL;
}

SoundEntry *SoundQueue::getEntry(Common::String name) {
	if (!name.contains('.'))
		name += ".SND";

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		if ((*i)->getName2() == name)
			return *i;
	}

	return NULL;
}

SoundEntry *SoundQueue::getEntry(SoundType type) {
	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i) {
		if ((*i)->getType() == type)
			return *i;
	}

	return NULL;
}

uint32 SoundQueue::getEntryTime(EntityIndex index) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(index);
	if (entry)
		return entry->getTime();

	return 0;
}

bool SoundQueue::isBuffered(EntityIndex entity) {
	Common::StackLock locker(_mutex);

	return (getEntry(entity) != NULL);
}

bool SoundQueue::isBuffered(Common::String filename, bool testForEntity) {
	Common::StackLock locker(_mutex);

	SoundEntry *entry = getEntry(filename);

	if (testForEntity)
		return entry != NULL && entry->getEntity() != kEntityPlayer;

	return (entry != NULL);
}

//////////////////////////////////////////////////////////////////////////
// Subtitles
//////////////////////////////////////////////////////////////////////////
void SoundQueue::updateSubtitles() {
	Common::StackLock locker(_mutex);

	uint32 index = 0;
	SubtitleEntry *subtitle = NULL;

	for (Common::List<SubtitleEntry *>::iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {
		uint32 current_index = 0;
		SoundEntry *soundEntry = (*i)->getSoundEntry();
		SoundStatus status = (SoundStatus)soundEntry->getStatus().status;

		if (!(status & kSoundStatus_40)
		 || status & kSoundStatus_180
		 || soundEntry->getTime() == 0
		 || (status & kSoundStatusClear1) < 6
		 || ((getFlags()->nis & 0x8000) && soundEntry->getPriority() < 90)) {
			 current_index = 0;
		} else {
			current_index = soundEntry->getPriority() + (status & kSoundStatusClear1);

			if (_currentSubtitle == (*i))
				current_index += 4;
		}

		if (index < current_index) {
			index = current_index;
			subtitle = (*i);
		}
	}

	if (_currentSubtitle == subtitle) {
		if (subtitle)
			subtitle->setupAndDraw();

		return;
	}

	if (_subtitlesFlag & 1)
		subtitle->drawOnScreen();

	if (subtitle) {
		subtitle->loadData();
		subtitle->setupAndDraw();
	}
}

//////////////////////////////////////////////////////////////////////////
// Cache
//////////////////////////////////////////////////////////////////////////
bool SoundQueue::setupCache(SoundEntry *entry) {
	if (entry->_soundData)
		return true;

	if (_soundCache.size() >= SOUNDCACHE_MAX_SIZE) {

		SoundEntry *cacheEntry = NULL;
		uint32 size = 1000;

		for (Common::List<SoundEntry *>::iterator i = _soundCache.begin(); i != _soundCache.end(); ++i) {
			if (!((*i)->getStatus().status & kSoundStatus_180)) {
				uint32 newSize = (*i)->getPriority() + ((*i)->getStatus().status & kSoundStatusClear1);

				if (newSize < size) {
					cacheEntry = (*i);
					size = newSize;
				}
			}
		}

		if (entry->getPriority() <= size)
			return false;

		if (!cacheEntry)
			error("[SoundManager::setupCache] Cannot find a valid entry");

		cacheEntry->setInCache();

		// TODO: Wait until the cache entry is ready to be removed
		while (!(cacheEntry->getStatus().status1 & 1))
			;

		if (cacheEntry->_soundData)
			removeFromCache(cacheEntry);

		_soundCache.push_back(entry);
		entry->_soundData = (char *)_soundCacheData + SOUNDCACHE_ENTRY_SIZE * (_soundCache.size() - 1);
	} else {
		_soundCache.push_back(entry);
		entry->_soundData = (char *)_soundCacheData + SOUNDCACHE_ENTRY_SIZE * (_soundCache.size() - 1);
	}

	return true;
}

void SoundQueue::removeFromCache(SoundEntry *entry) {
	for (Common::List<SoundEntry *>::iterator i = _soundCache.begin(); i != _soundCache.end(); ++i) {
		if ((*i) == entry) {
			// Remove sound buffer
			entry->_soundData = NULL;

			// Remove entry from sound cache
			i = _soundCache.reverse_erase(i);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Savegame
//////////////////////////////////////////////////////////////////////////
void SoundQueue::saveLoadWithSerializer(Common::Serializer &s) {
	Common::StackLock locker(_mutex);

	s.syncAsUint32LE(_state);
	s.syncAsUint32LE(_currentType);

	// Compute the number of entries to save
	uint32 numEntries = count();
	s.syncAsUint32LE(numEntries);

	// Save or load each entry data
	if (s.isSaving()) {
		for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i)
			(*i)->saveLoadWithSerializer(s);
	} else {
		warning("[Sound::saveLoadWithSerializer] Loading not implemented");
		s.skip(numEntries * 64);
	}
}


// FIXME: We probably need another mutex here to protect during the whole savegame process
// as we could have removed an entry between the time we check the count and the time we
// save the entries
uint32 SoundQueue::count() {
	Common::StackLock locker(_mutex);

	uint32 numEntries = 0;
	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i)
		if ((*i)->getName2().matchString("NISSND?"))
			++numEntries;

	return numEntries;
}

//////////////////////////////////////////////////////////////////////////
// Sound filters
//////////////////////////////////////////////////////////////////////////
static const int filterData[1424] = {
	0, 0, 0, 0, 128, 256, 384, 512, 0, 0, 0, 0, 128, 256,
	384, 512, 0, 0, 0, 0, 192, 320, 448, 576, 0, 0, 0, 0,
	192, 320, 448, 576, 64, 64, 64, 64, 256, 384, 512, 640,
	64, 64, 64, 64, 256, 384, 512, 640, 128, 128, 128, 128,
	320, 448, 576, 704, 128, 128, 128, 128, 320, 448, 576,
	704, 192, 192, 192, 192, 384, 512, 640, 768, 192, 192,
	192, 192, 384, 512, 640, 768, 256, 256, 256, 256, 448,
	576, 704, 832, 256, 256, 256, 256, 448, 576, 704, 832,
	320, 320, 320, 320, 512, 640, 768, 896, 320, 320, 320,
	320, 512, 640, 768, 896, 384, 384, 384, 384, 576, 704,
	832, 960, 384, 384, 384, 384, 576, 704, 832, 960, 448,
	448, 448, 448, 640, 768, 896, 1024, 448, 448, 448, 448,
	640, 768, 896, 1024, 512, 512, 512, 512, 704, 832, 960,
	1088, 512, 512, 512, 512, 704, 832, 960, 1088, 576,
	576, 576, 576, 768, 896, 1024, 1152, 576, 576, 576,
	576, 768, 896, 1024, 1152, 640, 640, 640, 640, 832,
	960, 1088, 1216, 640, 640, 640, 640, 832, 960, 1088,
	1216, 704, 704, 704, 704, 896, 1024, 1152, 1280, 704,
	704, 704, 704, 896, 1024, 1152, 1280, 768, 768, 768,
	768, 960, 1088, 1216, 1344, 768, 768, 768, 768, 960,
	1088, 1216, 1344, 832, 832, 832, 832, 1024, 1152, 1280,
	1408, 832, 832, 832, 832, 1024, 1152, 1280, 1408, 896,
	896, 896, 896, 1088, 1216, 1344, 1472, 896, 896, 896,
	896, 1088, 1216, 1344, 1472, 960, 960, 960, 960, 1152,
	1280, 1408, 1536, 960, 960, 960, 960, 1152, 1280, 1408,
	1536, 1024, 1024, 1024, 1024, 1216, 1344, 1472, 1600,
	1024, 1024, 1024, 1024, 1216, 1344, 1472, 1600, 1088,
	1088, 1088, 1088, 1280, 1408, 1536, 1664, 1088, 1088,
	1088, 1088, 1280, 1408, 1536, 1664, 1152, 1152, 1152,
	1152, 1344, 1472, 1600, 1728, 1152, 1152, 1152, 1152,
	1344, 1472, 1600, 1728, 1216, 1216, 1216, 1216, 1408,
	1536, 1664, 1792, 1216, 1216, 1216, 1216, 1408, 1536,
	1664, 1792, 1280, 1280, 1280, 1280, 1472, 1600, 1728,
	1856, 1280, 1280, 1280, 1280, 1472, 1600, 1728, 1856,
	1344, 1344, 1344, 1344, 1536, 1664, 1792, 1920, 1344,
	1344, 1344, 1344, 1536, 1664, 1792, 1920, 1408, 1408,
	1408, 1408, 1600, 1728, 1856, 1984, 1408, 1408, 1408,
	1408, 1600, 1728, 1856, 1984, 1472, 1472, 1472, 1472,
	1664, 1792, 1920, 2048, 1472, 1472, 1472, 1472, 1664,
	1792, 1920, 2048, 1536, 1536, 1536, 1536, 1728, 1856,
	1984, 2112, 1536, 1536, 1536, 1536, 1728, 1856, 1984,
	2112, 1600, 1600, 1600, 1600, 1792, 1920, 2048, 2176,
	1600, 1600, 1600, 1600, 1792, 1920, 2048, 2176, 1664,
	1664, 1664, 1664, 1856, 1984, 2112, 2240, 1664, 1664,
	1664, 1664, 1856, 1984, 2112, 2240, 1728, 1728, 1728,
	1728, 1920, 2048, 2176, 2304, 1728, 1728, 1728, 1728,
	1920, 2048, 2176, 2304, 1792, 1792, 1792, 1792, 1984,
	2112, 2240, 2368, 1792, 1792, 1792, 1792, 1984, 2112,
	2240, 2368, 1856, 1856, 1856, 1856, 2048, 2176, 2304,
	2432, 1856, 1856, 1856, 1856, 2048, 2176, 2304, 2432,
	1920, 1920, 1920, 1920, 2112, 2240, 2368, 2496, 1920,
	1920, 1920, 1920, 2112, 2240, 2368, 2496, 1984, 1984,
	1984, 1984, 2176, 2304, 2432, 2560, 1984, 1984, 1984,
	1984, 2176, 2304, 2432, 2560, 2048, 2048, 2048, 2048,
	2240, 2368, 2496, 2624, 2048, 2048, 2048, 2048, 2240,
	2368, 2496, 2624, 2112, 2112, 2112, 2112, 2304, 2432,
	2560, 2688, 2112, 2112, 2112, 2112, 2304, 2432, 2560,
	2688, 2176, 2176, 2176, 2176, 2368, 2496, 2624, 2752,
	2176, 2176, 2176, 2176, 2368, 2496, 2624, 2752, 2240,
	2240, 2240, 2240, 2432, 2560, 2688, 2816, 2240, 2240,
	2240, 2240, 2432, 2560, 2688, 2816, 2304, 2304, 2304,
	2304, 2496, 2624, 2752, 2880, 2304, 2304, 2304, 2304,
	2496, 2624, 2752, 2880, 2368, 2368, 2368, 2368, 2560,
	2688, 2816, 2944, 2368, 2368, 2368, 2368, 2560, 2688,
	2816, 2944, 2432, 2432, 2432, 2432, 2624, 2752, 2880,
	3008, 2432, 2432, 2432, 2432, 2624, 2752, 2880, 3008,
	2496, 2496, 2496, 2496, 2688, 2816, 2944, 3072, 2496,
	2496, 2496, 2496, 2688, 2816, 2944, 3072, 2560, 2560,
	2560, 2560, 2752, 2880, 3008, 3136, 2560, 2560, 2560,
	2560, 2752, 2880, 3008, 3136, 2624, 2624, 2624, 2624,
	2816, 2944, 3072, 3200, 2624, 2624, 2624, 2624, 2816,
	2944, 3072, 3200, 2688, 2688, 2688, 2688, 2880, 3008,
	3136, 3264, 2688, 2688, 2688, 2688, 2880, 3008, 3136,
	3264, 2752, 2752, 2752, 2752, 2944, 3072, 3200, 3328,
	2752, 2752, 2752, 2752, 2944, 3072, 3200, 3328, 2816,
	2816, 2816, 2816, 3008, 3136, 3264, 3392, 2816, 2816,
	2816, 2816, 3008, 3136, 3264, 3392, 2880, 2880, 2880,
	2880, 3072, 3200, 3328, 3456, 2880, 2880, 2880, 2880,
	3072, 3200, 3328, 3456, 2944, 2944, 2944, 2944, 3136,
	3264, 3392, 3520, 2944, 2944, 2944, 2944, 3136, 3264,
	3392, 3520, 3008, 3008, 3008, 3008, 3200, 3328, 3456,
	3584, 3008, 3008, 3008, 3008, 3200, 3328, 3456, 3584,
	3072, 3072, 3072, 3072, 3264, 3392, 3520, 3648, 3072,
	3072, 3072, 3072, 3264, 3392, 3520, 3648, 3136, 3136,
	3136, 3136, 3328, 3456, 3584, 3712, 3136, 3136, 3136,
	3136, 3328, 3456, 3584, 3712, 3200, 3200, 3200, 3200,
	3392, 3520, 3648, 3776, 3200, 3200, 3200, 3200, 3392,
	3520, 3648, 3776, 3264, 3264, 3264, 3264, 3456, 3584,
	3712, 3840, 3264, 3264, 3264, 3264, 3456, 3584, 3712,
	3840, 3328, 3328, 3328, 3328, 3520, 3648, 3776, 3904,
	3328, 3328, 3328, 3328, 3520, 3648, 3776, 3904, 3392,
	3392, 3392, 3392, 3584, 3712, 3840, 3968, 3392, 3392,
	3392, 3392, 3584, 3712, 3840, 3968, 3456, 3456, 3456,
	3456, 3648, 3776, 3904, 4032, 3456, 3456, 3456, 3456,
	3648, 3776, 3904, 4032, 3520, 3520, 3520, 3520, 3712,
	3840, 3968, 4096, 3520, 3520, 3520, 3520, 3712, 3840,
	3968, 4096, 3584, 3584, 3584, 3584, 3776, 3904, 4032,
	4160, 3584, 3584, 3584, 3584, 3776, 3904, 4032, 4160,
	3648, 3648, 3648, 3648, 3840, 3968, 4096, 4224, 3648,
	3648, 3648, 3648, 3840, 3968, 4096, 4224, 3712, 3712,
	3712, 3712, 3904, 4032, 4160, 4288, 3712, 3712, 3712,
	3712, 3904, 4032, 4160, 4288, 3776, 3776, 3776, 3776,
	3968, 4096, 4224, 4352, 3776, 3776, 3776, 3776, 3968,
	4096, 4224, 4352, 3840, 3840, 3840, 3840, 4032, 4160,
	4288, 4416, 3840, 3840, 3840, 3840, 4032, 4160, 4288,
	4416, 3904, 3904, 3904, 3904, 4096, 4224, 4352, 4480,
	3904, 3904, 3904, 3904, 4096, 4224, 4352, 4480, 3968,
	3968, 3968, 3968, 4160, 4288, 4416, 4544, 3968, 3968,
	3968, 3968, 4160, 4288, 4416, 4544, 4032, 4032, 4032,
	4032, 4224, 4352, 4480, 4608, 4032, 4032, 4032, 4032,
	4224, 4352, 4480, 4608, 4096, 4096, 4096, 4096, 4288,
	4416, 4544, 4672, 4096, 4096, 4096, 4096, 4288, 4416,
	4544, 4672, 4160, 4160, 4160, 4160, 4352, 4480, 4608,
	4736, 4160, 4160, 4160, 4160, 4352, 4480, 4608, 4736,
	4224, 4224, 4224, 4224, 4416, 4544, 4672, 4800, 4224,
	4224, 4224, 4224, 4416, 4544, 4672, 4800, 4288, 4288,
	4288, 4288, 4480, 4608, 4736, 4864, 4288, 4288, 4288,
	4288, 4480, 4608, 4736, 4864, 4352, 4352, 4352, 4352,
	4544, 4672, 4800, 4928, 4352, 4352, 4352, 4352, 4544,
	4672, 4800, 4928, 4416, 4416, 4416, 4416, 4608, 4736,
	4864, 4992, 4416, 4416, 4416, 4416, 4608, 4736, 4864,
	4992, 4480, 4480, 4480, 4480, 4672, 4800, 4928, 5056,
	4480, 4480, 4480, 4480, 4672, 4800, 4928, 5056, 4544,
	4544, 4544, 4544, 4736, 4864, 4992, 5120, 4544, 4544,
	4544, 4544, 4736, 4864, 4992, 5120, 4608, 4608, 4608,
	4608, 4800, 4928, 5056, 5184, 4608, 4608, 4608, 4608,
	4800, 4928, 5056, 5184, 4672, 4672, 4672, 4672, 4864,
	4992, 5120, 5248, 4672, 4672, 4672, 4672, 4864, 4992,
	5120, 5248, 4736, 4736, 4736, 4736, 4928, 5056, 5184,
	5312, 4736, 4736, 4736, 4736, 4928, 5056, 5184, 5312,
	4800, 4800, 4800, 4800, 4992, 5120, 5248, 5376, 4800,
	4800, 4800, 4800, 4992, 5120, 5248, 5376, 4864, 4864,
	4864, 4864, 5056, 5184, 5312, 5440, 4864, 4864, 4864,
	4864, 5056, 5184, 5312, 5440, 4928, 4928, 4928, 4928,
	5120, 5248, 5376, 5504, 4928, 4928, 4928, 4928, 5120,
	5248, 5376, 5504, 4992, 4992, 4992, 4992, 5184, 5312,
	5440, 5568, 4992, 4992, 4992, 4992, 5184, 5312, 5440,
	5568, 5056, 5056, 5056, 5056, 5248, 5376, 5504, 5632,
	5056, 5056, 5056, 5056, 5248, 5376, 5504, 5632, 5120,
	5120, 5120, 5120, 5312, 5440, 5568, 5632, 5120, 5120,
	5120, 5120, 5312, 5440, 5568, 5632, 5184, 5184, 5184,
	5184, 5376, 5504, 5632, 5632, 5184, 5184, 5184, 5184,
	5376, 5504, 5632, 5632, 5248, 5248, 5248, 5248, 5440,
	5568, 5632, 5632, 5248, 5248, 5248, 5248, 5440, 5568,
	5632, 5632, 5312, 5312, 5312, 5312, 5504, 5632, 5632,
	5632, 5312, 5312, 5312, 5312, 5504, 5632, 5632, 5632,
	5376, 5376, 5376, 5376, 5568, 5632, 5632, 5632, 5376,
	5376, 5376, 5376, 5568, 5632, 5632, 5632, 5440, 5440,
	5440, 5440, 5632, 5632, 5632, 5632, 5440, 5440, 5440,
	5440, 5632, 5632, 5632, 5632, 5504, 5504, 5504, 5504,
	5632, 5632, 5632, 5632, 5504, 5504, 5504, 5504, 5632,
	5632, 5632, 5632, 5568, 5568, 5568, 5568, 5632, 5632,
	5632, 5632, 5568, 5568, 5568, 5568, 5632, 5632, 5632,
	5632
};

static const int filterData2[1424] = {
	0, 2, 4, 6, 7, 9, 11, 13, 0, -2, -4, -6, -7, -9, -11,
	-13, 1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9,
	-11, -13, -15, 1, 3, 5, 7, 10, 12, 14, 16, -1, -3, -5,
	-7, -10, -12, -14, -16, 1, 3, 6, 8, 11, 13, 16, 18,
	-1, -3, -6, -8, -11, -13, -16, -18, 1, 4, 6, 9, 12,
	15, 17, 20, -1, -4, -6, -9, -12, -15, -17, -20, 1, 4,
	7, 10, 13, 16, 19, 22, -1, -4, -7, -10, -13, -16, -19,
	-22, 1, 4, 8, 11, 14, 17, 21, 24, -1, -4, -8, -11, -14,
	-17, -21, -24, 1, 5, 8, 12, 15, 19, 22, 26, -1, -5,
	-8, -12, -15, -19, -22, -26, 2, 6, 10, 14, 18, 22, 26,
	30, -2, -6, -10, -14, -18, -22, -26, -30, 2, 6, 10,
	14, 19, 23, 27, 31, -2, -6, -10, -14, -19, -23, -27,
	-31, 2, 7, 11, 16, 21, 26, 30, 35, -2, -7, -11, -16,
	-21, -26, -30, -35, 2, 7, 13, 18, 23, 28, 34, 39, -2,
	-7, -13, -18, -23, -28, -34, -39, 2, 8, 14, 20, 25,
	31, 37, 43, -2, -8, -14, -20, -25, -31, -37, -43, 3,
	9, 15, 21, 28, 34, 40, 46, -3, -9, -15, -21, -28, -34,
	-40, -46, 3, 10, 17, 24, 31, 38, 45, 52, -3, -10, -17,
	-24, -31, -38, -45, -52, 3, 11, 19, 27, 34, 42, 50,
	58, -3, -11, -19, -27, -34, -42, -50, -58, 4, 12, 21,
	29, 38, 46, 55, 63, -4, -12, -21, -29, -38, -46, -55,
	-63, 4, 13, 23, 32, 41, 50, 60, 69, -4, -13, -23, -32,
	-41, -50, -60, -69, 5, 15, 25, 35, 46, 56, 66, 76, -5,
	-15, -25, -35, -46, -56, -66, -76, 5, 16, 28, 39, 50,
	61, 73, 84, -5, -16, -28, -39, -50, -61, -73, -84, 6,
	18, 31, 43, 56, 68, 81, 93, -6, -18, -31, -43, -56,
	-68, -81, -93, 6, 20, 34, 48, 61, 75, 89, 103, -6, -20,
	-34, -48, -61, -75, -89, -103, 7, 22, 37, 52, 67, 82,
	97, 112, -7, -22, -37, -52, -67, -82, -97, -112, 8,
	24, 41, 57, 74, 90, 107, 123, -8, -24, -41, -57, -74,
	-90, -107, -123, 9, 27, 45, 63, 82, 100, 118, 136, -9,
	-27, -45, -63, -82, -100, -118, -136, 10, 30, 50, 70,
	90, 110, 130, 150, -10, -30, -50, -70, -90, -110, -130,
	-150, 11, 33, 55, 77, 99, 121, 143, 165, -11, -33, -55,
	-77, -99, -121, -143, -165, 12, 36, 60, 84, 109, 133,
	157, 181, -12, -36, -60, -84, -109, -133, -157, -181,
	13, 40, 66, 93, 120, 147, 173, 200, -13, -40, -66, -93,
	-120, -147, -173, -200, 14, 44, 73, 103, 132, 162, 191,
	221, -14, -44, -73, -103, -132, -162, -191, -221, 16,
	48, 81, 113, 146, 178, 211, 243, -16, -48, -81, -113,
	-146, -178, -211, -243, 17, 53, 89, 125, 160, 196, 232,
	268, -17, -53, -89, -125, -160, -196, -232, -268, 19,
	58, 98, 137, 176, 215, 255, 294, -19, -58, -98, -137,
	-176, -215, -255, -294, 21, 64, 108, 151, 194, 237,
	281, 324, -21, -64, -108, -151, -194, -237, -281, -324,
	23, 71, 118, 166, 213, 261, 308, 356, -23, -71, -118,
	-166, -213, -261, -308, -356, 26, 78, 130, 182, 235,
	287, 339, 391, -26, -78, -130, -182, -235, -287, -339,
	-391, 28, 86, 143, 201, 258, 316, 373, 431, -28, -86,
	-143, -201, -258, -316, -373, -431, 31, 94, 158, 221,
	284, 347, 411, 474, -31, -94, -158, -221, -284, -347,
	-411, -474, 34, 104, 174, 244, 313, 383, 453, 523, -34,
	-104, -174, -244, -313, -383, -453, -523, 38, 115, 191,
	268, 345, 422, 498, 575, -38, -115, -191, -268, -345,
	-422, -498, -575, 42, 126, 210, 294, 379, 463, 547,
	631, -42, -126, -210, -294, -379, -463, -547, -631,
	46, 139, 231, 324, 417, 510, 602, 695, -46, -139, -231,
	-324, -417, -510, -602, -695, 51, 153, 255, 357, 459,
	561, 663, 765, -51, -153, -255, -357, -459, -561, -663,
	-765, 56, 168, 280, 392, 505, 617, 729, 841, -56, -168,
	-280, -392, -505, -617, -729, -841, 61, 185, 308, 432,
	555, 679, 802, 926, -61, -185, -308, -432, -555, -679,
	-802, -926, 68, 204, 340, 476, 612, 748, 884, 1020,
	-68, -204, -340, -476, -612, -748, -884, -1020, 74,
	224, 373, 523, 672, 822, 971, 1121, -74, -224, -373,
	-523, -672, -822, -971, -1121, 82, 246, 411, 575, 740,
	904, 1069, 1233, -82, -246, -411, -575, -740, -904,
	-1069, -1233, 90, 271, 452, 633, 814, 995, 1176, 1357,
	-90, -271, -452, -633, -814, -995, -1176, -1357, 99,
	298, 497, 696, 895, 1094, 1293, 1492, -99, -298, -497,
	-696, -895, -1094, -1293, -1492, 109, 328, 547, 766,
	985, 1204, 1423, 1642, -109, -328, -547, -766, -985,
	-1204, -1423, -1642, 120, 361, 601, 842, 1083, 1324,
	1564, 1805, -120, -361, -601, -842, -1083, -1324, -1564,
	-1805, 132, 397, 662, 927, 1192, 1457, 1722, 1987, -132,
	-397, -662, -927, -1192, -1457, -1722, -1987, 145, 437,
	728, 1020, 1311, 1603, 1894, 2186, -145, -437, -728,
	-1020, -1311, -1603, -1894, -2186, 160, 480, 801, 1121,
	1442, 1762, 2083, 2403, -160, -480, -801, -1121, -1442,
	-1762, -2083, -2403, 176, 529, 881, 1234, 1587, 1940,
	2292, 2645, -176, -529, -881, -1234, -1587, -1940, -2292,
	-2645, 194, 582, 970, 1358, 1746, 2134, 2522, 2910,
	-194, -582, -970, -1358, -1746, -2134, -2522, -2910,
	213, 640, 1066, 1493, 1920, 2347, 2773, 3200, -213,
	-640, -1066, -1493, -1920, -2347, -2773, -3200, 234,
	704, 1173, 1643, 2112, 2582, 3051, 3521, -234, -704,
	-1173, -1643, -2112, -2582, -3051, -3521, 258, 774,
	1291, 1807, 2324, 2840, 3357, 3873, -258, -774, -1291,
	-1807, -2324, -2840, -3357, -3873, 284, 852, 1420, 1988,
	2556, 3124, 3692, 4260, -284, -852, -1420, -1988, -2556,
	-3124, -3692, -4260, 312, 937, 1561, 2186, 2811, 3436,
	4060, 4685, -312, -937, -1561, -2186, -2811, -3436,
	-4060, -4685, 343, 1030, 1718, 2405, 3092, 3779, 4467,
	5154, -343, -1030, -1718, -2405, -3092, -3779, -4467,
	-5154, 378, 1134, 1890, 2646, 3402, 4158, 4914, 5670,
	-378, -1134, -1890, -2646, -3402, -4158, -4914, -5670,
	415, 1247, 2079, 2911, 3742, 4574, 5406, 6238, -415,
	-1247, -2079, -2911, -3742, -4574, -5406, -6238, 457,
	1372, 2287, 3202, 4117, 5032, 5947, 6862, -457, -1372,
	-2287, -3202, -4117, -5032, -5947, -6862, 503, 1509,
	2516, 3522, 4529, 5535, 6542, 7548, -503, -1509, -2516,
	-3522, -4529, -5535, -6542, -7548, 553, 1660, 2767,
	3874, 4981, 6088, 7195, 8302, -553, -1660, -2767, -3874,
	-4981, -6088, -7195, -8302, 608, 1826, 3044, 4262, 5479,
	6697, 7915, 9133, -608, -1826, -3044, -4262, -5479,
	-6697, -7915, -9133, 669, 2009, 3348, 4688, 6027, 7367,
	8706, 10046, -669, -2009, -3348, -4688, -6027, -7367,
	-8706, -10046, 736, 2210, 3683, 5157, 6630, 8104, 9577,
	11051, -736, -2210, -3683, -5157, -6630, -8104, -9577,
	-11051, 810, 2431, 4052, 5673, 7294, 8915, 10536, 12157,
	-810, -2431, -4052, -5673, -7294, -8915, -10536, -12157,
	891, 2674, 4457, 6240, 8023, 9806, 11589, 13372, -891,
	-2674, -4457, -6240, -8023, -9806, -11589, -13372, 980,
	2941, 4903, 6864, 8825, 10786, 12748, 14709, -980, -2941,
	-4903, -6864, -8825, -10786, -12748, -14709, 1078, 3236,
	5393, 7551, 9708, 11866, 14023, 16181, -1078, -3236,
	-5393, -7551, -9708, -11866, -14023, -16181, 1186, 3559,
	5933, 8306, 10679, 13052, 15426, 17799, -1186, -3559,
	-5933, -8306, -10679, -13052, -15426, -17799, 1305,
	3915, 6526, 9136, 11747, 14357, 16968, 19578, -1305,
	-3915, -6526, -9136, -11747, -14357, -16968, -19578,
	1435, 4307, 7179, 10051, 12922, 15794, 18666, 21538,
	-1435, -4307, -7179, -10051, -12922, -15794, -18666,
	-21538, 1579, 4738, 7896, 11055, 14214, 17373, 20531,
	23690, -1579, -4738, -7896, -11055, -14214, -17373,
	-20531, -23690, 1737, 5212, 8686, 12161, 15636, 19111,
	22585, 26060, -1737, -5212, -8686, -12161, -15636, -19111,
	-22585, -26060, 1911, 5733, 9555, 13377, 17200, 21022,
	24844, 28666, -1911, -5733, -9555, -13377, -17200, -21022,
	-24844, -28666, 2102, 6306, 10511, 14715, 18920, 23124,
	27329, 31533, -2102, -6306, -10511, -14715, -18920,
	-23124, -27329, -31533, 2312, 6937, 11562, 16187, 20812,
	25437, 30062, 32767, -2312, -6937, -11562, -16187, -20812,
	-25437, -30062, -32767, 2543, 7631, 12718, 17806, 22893,
	27981, 32767, 32767, -2543, -7631, -12718, -17806, -22893,
	-27981, -32767, -32767, 2798, 8394, 13990, 19586, 25183,
	30779, 32767, 32767, -2798, -8394, -13990, -19586, -25183,
	-30779, -32767, -32767, 3077, 9233, 15389, 21545, 27700,
	32767, 32767, 32767, -3077, -9233, -15389, -21545, -27700,
	-32767, -32767, -32767, 3385, 10157, 16928, 23700, 30471,
	32767, 32767, 32767, -3385, -10157, -16928, -23700,
	-30471, -32767, -32767, -32767, 3724, 11172, 18621,
	26069, 32767, 32767, 32767, 32767, -3724, -11172, -18621,
	-26069, -32767, -32767, -32767, -32767, 4095, 12287,
	20479, 28671, 32767, 32767, 32767, 32767, -4095, -12287,
	-20479, -28671, -32767, -32767, -32767, -32767
};

static const int p1s[17] = { 0, 4, 3, 4, 2, 4, 3, 4, 1, 4, 3, 4,  2, 4,  3, 4,  0 };
static const int p2s[17] = { 0, 1, 1, 3, 1, 5, 3, 7, 1, 9, 5, 11, 3, 13, 7, 15, 1 };

static void soundFilter(byte *data, int16 *buffer, int p1, int p2);

void SoundQueue::applyFilter(SoundEntry *entry, int16 *buffer) {
	if ((((byte *)entry->_soundData)[1] << 6) > 0x1600) {
		entry->setStatus(entry->getStatus().status | kSoundStatus_20000000);
	} else {
		int variant = entry->getStatus().status & 0x1f;

		soundFilter((byte *)entry->_soundData, buffer, p1s[variant], p2s[variant]);
	}
}


static void soundFilter(byte *data, int16 *buffer, int p1, int p2) {
	int data1, data2, data1p, data2p;
	byte idx;

	data2 = data[0];
	data1 = data[1] << 6;

	data += 2;

	for (int count = 0; count < 735; count++) {
		idx = data[count] >> 4;
		data1p = filterData[idx + data1];
		data2p = CLIP(filterData2[idx + data1] + data2, -32767, 32767);

		buffer[2 * count] = (p2 * data2p) >> p1;

		idx = data[count] & 0xF;

		data1 = filterData[idx + data1p];
		data2 = CLIP(filterData2[idx + data1p] + data2p, -32767, 32767);
		buffer[2 * count + 1] = (p2 * data2) >> p1;
	}
}

//////////////////////////////////////////////////////////////////////////
// Debug
//////////////////////////////////////////////////////////////////////////
void SoundQueue::stopAllSound() {
	Common::StackLock locker(_mutex);

	for (Common::List<SoundEntry *>::iterator i = _soundList.begin(); i != _soundList.end(); ++i)
		(*i)->getStreamedSound()->stop();
}

} // End of namespace LastExpress
