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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "mohawk/cursors.h"
#include "mohawk/myst.h"
#include "mohawk/myst_areas.h"
#include "mohawk/myst_graphics.h"
#include "mohawk/myst_state.h"
#include "mohawk/sound.h"
#include "mohawk/video.h"
#include "mohawk/myst_stacks/mechanical.h"

#include "common/events.h"
#include "common/system.h"

namespace Mohawk {
namespace MystStacks {

Mechanical::Mechanical(MohawkEngine_Myst *vm) :
		MystScriptParser(vm), _state(vm->_gameState->_mechanical) {
	setupOpcodes();

	_elevatorGoingMiddle = false;
	_elevatorPosition = 0;

	_crystalLit = 0;

	_mystStaircaseState = false;
	_fortressPosition = 0;
	_fortressRotationSpeed = 0;
	_fortressSimulationSpeed = 0;
	_gearsWereRunning = false;

	_fortressRotationShortMovieWorkaround = false;
	_fortressRotationShortMovieCount = 0;
	_fortressRotationShortMovieLast = 0;
}

Mechanical::~Mechanical() {
}

#define OPCODE(op, x) _opcodes.push_back(new MystOpcode(op, (OpcodeProcMyst) &Mechanical::x, #x))

void Mechanical::setupOpcodes() {
	// "Stack-Specific" Opcodes
	OPCODE(100, o_throneEnablePassage);
	OPCODE(101, o_birdCrankStart);
	OPCODE(102, NOP);
	OPCODE(103, o_birdCrankStop);
	OPCODE(104, o_snakeBoxTrigger);
	OPCODE(105, o_fortressStaircaseMovie);
	OPCODE(106, o_elevatorRotationStart);
	OPCODE(107, o_elevatorRotationMove);
	OPCODE(108, o_elevatorRotationStop);
	OPCODE(109, o_fortressRotationSpeedStart);
	OPCODE(110, o_fortressRotationSpeedMove);
	OPCODE(111, o_fortressRotationSpeedStop);
	OPCODE(112, o_fortressRotationBrakeStart);
	OPCODE(113, o_fortressRotationBrakeMove);
	OPCODE(114, o_fortressRotationBrakeStop);
	OPCODE(115, o_fortressSimulationSpeedStart);
	OPCODE(116, o_fortressSimulationSpeedMove);
	OPCODE(117, o_fortressSimulationSpeedStop);
	OPCODE(118, o_fortressSimulationBrakeStart);
	OPCODE(119, o_fortressSimulationBrakeMove);
	OPCODE(120, o_fortressSimulationBrakeStop);
	OPCODE(121, o_elevatorWindowMovie);
	OPCODE(122, o_elevatorGoMiddle);
	OPCODE(123, o_elevatorTopMovie);
	OPCODE(124, o_fortressRotationSetPosition);
	OPCODE(125, o_mystStaircaseMovie);
	OPCODE(126, o_elevatorWaitTimeout);
	OPCODE(127, o_crystalEnterYellow);
	OPCODE(128, o_crystalLeaveYellow);
	OPCODE(129, o_crystalEnterGreen);
	OPCODE(130, o_crystalLeaveGreen);
	OPCODE(131, o_crystalEnterRed);
	OPCODE(132, o_crystalLeaveRed);

	// "Init" Opcodes
	OPCODE(200, o_throne_init);
	OPCODE(201, o_fortressStaircase_init);
	OPCODE(202, o_bird_init);
	OPCODE(203, o_snakeBox_init);
	OPCODE(204, o_elevatorRotation_init);
	OPCODE(205, o_fortressRotation_init);
	OPCODE(206, o_fortressSimulation_init);
	OPCODE(209, o_fortressSimulationStartup_init);

	// "Exit" Opcodes
	OPCODE(300, NOP);
}

#undef OPCODE

void Mechanical::disablePersistentScripts() {
	_fortressSimulationRunning = false;
	_elevatorRotationLeverMoving = false;
	_birdSinging = false;
	_fortressRotationRunning = false;
}

void Mechanical::runPersistentScripts() {
	if (_birdSinging)
		birdSing_run();

	if (_elevatorRotationLeverMoving)
		elevatorRotation_run();

	if (_elevatorGoingMiddle)
		elevatorGoMiddle_run();

	if (_fortressRotationRunning)
		fortressRotation_run();

	if (_fortressSimulationRunning)
		fortressSimulation_run();
}

uint16 Mechanical::getVar(uint16 var) {
	switch(var) {
	case 0: // Achenar's Secret Panel State
		return _state.achenarPanelState;
	case 1: // Sirrus's Secret Panel State
		return _state.sirrusPanelState;
	case 2: // Achenar's Secret Room Crate Lid Open and Blue Page Present
		if (_state.achenarCrateOpened) {
			if (_globals.bluePagesInBook & 4 || _globals.heldPage == 3)
				return 2;
			else
				return 3;
		} else {
			return _globals.bluePagesInBook & 4 || _globals.heldPage == 3;
		}
	case 3: // Achenar's Secret Room Crate State
		return _state.achenarCrateOpened;
	case 4: // Myst Book Room Staircase State
		return _mystStaircaseState;
	case 5: // Fortress Position
		return _fortressPosition;
	case 6: // Fortress Position - Big Cog Visible Through Doorway
		return _fortressPosition == 0;
	case 7: // Fortress Elevator Open
		if (_state.elevatorRotation == 4)
			return 1; // Open
		else
			return 0; // Closed
	case 10: // Fortress Staircase State
		return _state.staircaseState;
	case 11: // Fortress Elevator Rotation Position
		return _state.elevatorRotation;
	case 12: // Fortress Elevator Rotation Cog Position
		return 5 - (uint16)(_elevatorRotationGearPosition + 0.5) % 6;
	case 13: // Elevator position
		return _elevatorPosition;
	case 14: // Elevator going down when at top
		if (_elevatorGoingDown && _elevatorTooLate)
			return 2;
		else
			return _elevatorGoingDown;
	case 15: // Code Lock Execute Button Script
		if (_mystStaircaseState)
			return 0;
		else if (_state.codeShape[0] == 2 && _state.codeShape[1] == 8
				&& _state.codeShape[2] == 5 && _state.codeShape[3] == 1)
			return 1;
		else
			return 2;
	case 16: // Code Lock Shape #1 - Left
	case 17: // Code Lock Shape #2
	case 18: // Code Lock Shape #3
	case 19: // Code Lock Shape #4 - Right
		return _state.codeShape[var - 16];
	case 20: // Crystal Lit Flag - Yellow
		return _crystalLit == 3;
	case 21: // Crystal Lit Flag - Green
		return _crystalLit == 1;
	case 22: // Crystal Lit Flag - Red
		return _crystalLit == 2;
	case 102: // Red page
		return !(_globals.redPagesInBook & 4) && (_globals.heldPage != 9);
	case 103: // Blue page
		return !(_globals.bluePagesInBook & 4) && (_globals.heldPage != 3);
	default:
		return MystScriptParser::getVar(var);
	}
}

void Mechanical::toggleVar(uint16 var) {
	switch(var) {
	case 0: // Achenar's Secret Panel State
		_state.achenarPanelState ^= 1;
		break;
	case 1: // Sirrus's Secret Panel State
		_state.sirrusPanelState ^= 1;
		break;
	case 3: // Achenar's Secret Room Crate State
		_state.achenarCrateOpened ^= 1;
		break;
	case 4: // Myst Book Room Staircase State
		_mystStaircaseState ^= 1;
		break;
	case 10: // Fortress Staircase State
		_state.staircaseState ^= 1;
		break;
	case 16: // Code Lock Shape #1 - Left
	case 17: // Code Lock Shape #2
	case 18: // Code Lock Shape #3
	case 19: // Code Lock Shape #4 - Right
		_state.codeShape[var - 16] = (_state.codeShape[var - 16] + 1) % 10;
		break;
	case 23: // Elevator player is in cabin
		_elevatorInCabin = false;
		break;
	case 102: // Red page
		if (!(_globals.redPagesInBook & 4)) {
			if (_globals.heldPage == 9)
				_globals.heldPage = 0;
			else
				_globals.heldPage = 9;
		}
		break;
	case 103: // Blue page
		if (!(_globals.bluePagesInBook & 4)) {
			if (_globals.heldPage == 3)
				_globals.heldPage = 0;
			else
				_globals.heldPage = 3;
		}
		break;
	default:
		MystScriptParser::toggleVar(var);
		break;
	}
}

bool Mechanical::setVarValue(uint16 var, uint16 value) {
	bool refresh = false;

	switch (var) {
	case 13:
		_elevatorPosition = value;
		break;
	case 14: // Elevator going down when at top
		_elevatorGoingDown = value;
		break;
	default:
		refresh = MystScriptParser::setVarValue(var, value);
		break;
	}

	return refresh;
}

void Mechanical::o_throneEnablePassage(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Enable throne passage", op);

	_vm->_resources[argv[0]]->setEnabled(getVar(var));
}

void Mechanical::o_birdCrankStart(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Mechanical bird crank start", op);

	MystAreaDrag *crank = getInvokingResource<MystAreaDrag>();

	uint16 crankSoundId = crank->getList2(0);
	_vm->_sound->replaceSoundMyst(crankSoundId, Audio::Mixer::kMaxChannelVolume, true);

	_birdSingEndTime = 0;
	_birdCrankStartTime = _vm->_system->getMillis();

	MystAreaVideo *crankMovie = static_cast<MystAreaVideo *>(crank->getSubResource(0));
	crankMovie->playMovie();
}

void Mechanical::o_birdCrankStop(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Mechanical bird crank stop", op);

	MystAreaDrag *crank = getInvokingResource<MystAreaDrag>();

	MystAreaVideo *crankMovie = static_cast<MystAreaVideo *>(crank->getSubResource(0));
	crankMovie->pauseMovie(true);

	uint16 crankSoundId = crank->getList2(1);
	_vm->_sound->replaceSoundMyst(crankSoundId);

	_birdSingEndTime = 2 * _vm->_system->getMillis() - _birdCrankStartTime;
	_birdSinging = true;

	_bird->playMovie();
}

void Mechanical::o_snakeBoxTrigger(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Trigger Playing Of Snake Movie", op);

	// Used on Mechanical Card 6043 (Weapons Rack with Snake Box)
	_snakeBox->playMovie();
}

void Mechanical::o_fortressStaircaseMovie(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Play Stairs Movement Movie", op);

	VideoEntryPtr staircase = _vm->_video->playMovie(_vm->wrapMovieFilename("hhstairs", kMechanicalStack));
	if (!staircase)
		error("Failed to open hhstairs movie");

	staircase->moveTo(174, 222);

	if (_state.staircaseState) {
		staircase->setBounds(Audio::Timestamp(0, 840, 600), Audio::Timestamp(0, 1680, 600));
	} else {
		staircase->setBounds(Audio::Timestamp(0, 0, 600), Audio::Timestamp(0, 840, 600));
	}

	_vm->waitUntilMovieEnds(staircase);
}

void Mechanical::o_elevatorRotationStart(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Elevator rotation lever start", op);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(0);

	_elevatorRotationLeverMoving = true;
	_elevatorRotationSpeed = 0;

	_vm->_sound->stopBackgroundMyst();

	_vm->_cursor->setCursor(700);
}

void Mechanical::o_elevatorRotationMove(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Elevator rotation lever move", op);

	const Common::Point &mouse = _vm->_system->getEventManager()->getMousePos();
	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Make the handle follow the mouse
	int16 maxStep = lever->getNumFrames() - 1;
	Common::Rect rect = lever->getRect();
	int16 step = ((rect.bottom - mouse.y) * lever->getNumFrames()) / rect.height();
	step = CLIP<int16>(step, 0, maxStep);

	_elevatorRotationSpeed = step * 0.1f;

	// Draw current frame
	lever->drawFrame(step);
}

void Mechanical::o_elevatorRotationStop(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Elevator rotation lever stop", op);

	const Common::Point &mouse = _vm->_system->getEventManager()->getMousePos();
	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Get current lever frame
	int16 maxStep = lever->getNumFrames() - 1;
	Common::Rect rect = lever->getRect();
	int16 step = ((rect.bottom - mouse.y) * lever->getNumFrames()) / rect.height();
	step = CLIP<int16>(step, 0, maxStep);

	// Release lever
	for (int i = step; i >= 0; i--) {
		lever->drawFrame(i);
		_vm->_system->delayMillis(10);
	}

	// Stop persistent script
	_elevatorRotationLeverMoving = false;

	float speed = _elevatorRotationSpeed * 10;

	if (speed > 0) {

		// Decrease speed
		while (speed > 2) {
			speed -= 0.5f;

			_elevatorRotationGearPosition += speed * 0.1f;

			if (_elevatorRotationGearPosition > 12)
				break;

			_vm->redrawArea(12);
			_vm->wait(100);
		}

		// Increment position
		_state.elevatorRotation = (_state.elevatorRotation + 1) % 10;

		_vm->_sound->replaceSoundMyst(_elevatorRotationSoundId);
		_vm->redrawArea(11);
	}

	_vm->checkCursorHints();
}

void Mechanical::o_fortressRotationSpeedStart(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation speed lever start", op);

	_vm->_cursor->setCursor(700);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(0);
}

void Mechanical::o_fortressRotationSpeedMove(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation speed lever move", op);

	const Common::Point &mouse = _vm->_system->getEventManager()->getMousePos();
	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Make the handle follow the mouse
	int16 maxStep = lever->getNumFrames() - 1;
	Common::Rect rect = lever->getRect();
	int16 step = ((rect.top + 65 - mouse.y) * lever->getNumFrames()) / 65;
	step = CLIP<int16>(step, 0, maxStep);

	_fortressRotationSpeed = step;

	// Draw current frame
	lever->drawFrame(step);
}

void Mechanical::o_fortressRotationSpeedStop(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation speed lever stop", op);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Release lever
	for (int i = _fortressRotationSpeed; i >= 0; i--) {
		lever->drawFrame(i);
		_vm->_system->delayMillis(10);
	}

	_fortressRotationSpeed = 0;

	_vm->checkCursorHints();
}

void Mechanical::o_fortressRotationBrakeStart(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation brake lever start", op);

	_vm->_cursor->setCursor(700);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(_fortressRotationBrake);
}

void Mechanical::o_fortressRotationBrakeMove(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation brake lever move", op);

	const Common::Point &mouse = _vm->_system->getEventManager()->getMousePos();
	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Make the handle follow the mouse
	int16 maxStep = lever->getNumFrames() - 1;
	Common::Rect rect = lever->getRect();
	int16 step = ((rect.top + 65 - mouse.y) * lever->getNumFrames()) / 65;
	step = CLIP<int16>(step, 0, maxStep);

	_fortressRotationBrake = step;

	// Draw current frame
	lever->drawFrame(step);
}

void Mechanical::o_fortressRotationBrakeStop(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation brake lever stop", op);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(_fortressRotationBrake);

	_vm->checkCursorHints();
}

void Mechanical::o_fortressSimulationSpeedStart(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation simulator speed lever start", op);

	_vm->_cursor->setCursor(700);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(0);
}

void Mechanical::o_fortressSimulationSpeedMove(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation simulator speed lever move", op);

	const Common::Point &mouse = _vm->_system->getEventManager()->getMousePos();
	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Make the handle follow the mouse
	int16 maxStep = lever->getNumFrames() - 1;
	Common::Rect rect = lever->getRect();
	int16 step = ((rect.bottom - mouse.y) * lever->getNumFrames()) / rect.height();
	step = CLIP<int16>(step, 0, maxStep);

	_fortressSimulationSpeed = step;

	// Draw current frame
	lever->drawFrame(step);
}

void Mechanical::o_fortressSimulationSpeedStop(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation simulator speed lever stop", op);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Release lever
	for (int i = _fortressSimulationSpeed; i >= 0; i--) {
		lever->drawFrame(i);
		_vm->_system->delayMillis(10);
	}

	_fortressSimulationSpeed = 0;

	_vm->checkCursorHints();
}

void Mechanical::o_fortressSimulationBrakeStart(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation simulator brake lever start", op);

	_vm->_cursor->setCursor(700);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(_fortressSimulationBrake);
}

void Mechanical::o_fortressSimulationBrakeMove(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation simulator brake lever move", op);

	const Common::Point &mouse = _vm->_system->getEventManager()->getMousePos();
	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();

	// Make the handle follow the mouse
	int16 maxStep = lever->getNumFrames() - 1;
	Common::Rect rect = lever->getRect();
	int16 step = ((rect.bottom - mouse.y) * lever->getNumFrames()) / rect.height();
	step = CLIP<int16>(step, 0, maxStep);

	_fortressSimulationBrake = step;

	// Draw current frame
	lever->drawFrame(step);
}

void Mechanical::o_fortressSimulationBrakeStop(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d Fortress rotation simulator brake lever stop", op);

	MystVideoInfo *lever = getInvokingResource<MystVideoInfo>();
	lever->drawFrame(_fortressSimulationBrake);

	_vm->checkCursorHints();
}

void Mechanical::o_elevatorWindowMovie(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	uint16 startTime = argv[0];
	uint16 endTime = argv[1];

	debugC(kDebugScript, "Opcode %d Movie Time Index %d to %d", op, startTime, endTime);

	VideoEntryPtr window = _vm->_video->playMovie(_vm->wrapMovieFilename("ewindow", kMechanicalStack));
	if (!window)
		error("Failed to open ewindow movie");

	window->moveTo(253, 0);
	window->setBounds(Audio::Timestamp(0, startTime, 600), Audio::Timestamp(0, endTime, 600));
	_vm->waitUntilMovieEnds(window);
}

void Mechanical::o_elevatorGoMiddle(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Elevator go middle from top", op);

	_elevatorTooLate = false;
	_elevatorTopCounter = 5;
	_elevatorGoingMiddle = true;
	_elevatorInCabin = true;
	_elevatorNextTime = _vm->_system->getMillis() + 1000;
}

void Mechanical::elevatorGoMiddle_run() {
	uint32 time = _vm->_system->getMillis();
	if (_elevatorNextTime < time) {
		_elevatorNextTime = time + 1000;
		_elevatorTopCounter--;

		if (_elevatorTopCounter > 0) {
			// Draw button pressed
			if (_elevatorInCabin) {
				_vm->_gfx->copyImageSectionToScreen(6332, Common::Rect(0, 35, 51, 63), Common::Rect(10, 137, 61, 165));
				_vm->_system->updateScreen();
			}

			// Blip
			_vm->_sound->playSoundBlocking(14120);

			// Restore button
			if (_elevatorInCabin) {
				_vm->_gfx->copyBackBufferToScreen(Common::Rect(10, 137, 61, 165));
				_vm->_system->updateScreen();
			 }
		} else {
			_elevatorTooLate = true;
			_elevatorGoingMiddle = false;

			if (_elevatorInCabin) {

				// Elevator going to middle animation
				_vm->_cursor->hideCursor();
				_vm->_sound->playSoundBlocking(11120);
				_vm->_gfx->copyImageToBackBuffer(6118, Common::Rect(544, 333));
				_vm->_sound->replaceSoundMyst(12120);
				_vm->_gfx->runTransition(kTransitionSlideToLeft, Common::Rect(177, 0, 370, 333), 25, 0);
				_vm->_sound->playSoundBlocking(13120);
				_vm->_sound->replaceSoundMyst(8120);
				_vm->_gfx->copyImageToBackBuffer(6327, Common::Rect(544, 333));
				_vm->wait(500);
				_vm->_sound->replaceSoundMyst(9120);
				static uint16 moviePos[2] = { 3540, 5380 };
				o_elevatorWindowMovie(121, 0, 2, moviePos);
				_vm->_gfx->copyBackBufferToScreen(Common::Rect(544, 333));
				_vm->_sound->replaceSoundMyst(10120);
				_vm->_cursor->showCursor();

				_elevatorPosition = 1;

				_vm->changeToCard(6327, kTransitionRightToLeft);
			}
		}
	}
}

void Mechanical::o_elevatorTopMovie(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	uint16 startTime = argv[0];
	uint16 endTime = argv[1];

	debugC(kDebugScript, "Opcode %d Movie Time Index %d to %d", op, startTime, endTime);

	VideoEntryPtr window = _vm->_video->playMovie(_vm->wrapMovieFilename("hcelev", kMechanicalStack));
	if (!window)
		error("Failed to open hcelev movie");

	window->moveTo(206, 38);
	window->setBounds(Audio::Timestamp(0, startTime, 600), Audio::Timestamp(0, endTime, 600));
	_vm->waitUntilMovieEnds(window);
}

void Mechanical::o_fortressRotationSetPosition(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Set fortress position", op);

	VideoEntryPtr gears = _fortressRotationGears->getVideo();
	uint32 moviePosition = Audio::Timestamp(gears->getTime(), 600).totalNumberOfFrames();

	// Myst ME short movie workaround, explained in o_fortressRotation_init
	if (_fortressRotationShortMovieWorkaround) {
		moviePosition += 3600 * _fortressRotationShortMovieCount;
	}

	_fortressPosition = (moviePosition + 900) / 1800 % 4;
}

void Mechanical::o_mystStaircaseMovie(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Myst book staircase video", op);

	_vm->playMovieBlocking(_vm->wrapMovieFilename("sstairs", kMechanicalStack), 199, 108);
}

void Mechanical::o_elevatorWaitTimeout(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Wait for the elevator to go middle", op);

	// Wait while the elevator times out
	while (_elevatorGoingMiddle) {
		runPersistentScripts();
		_vm->_system->delayMillis(10);
	}
}

void Mechanical::o_crystalEnterYellow(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Crystal enter", op);

	_crystalLit = 3;
	_vm->redrawArea(20);
}

void Mechanical::o_crystalEnterGreen(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Crystal enter", op);

	_crystalLit = 1;
	_vm->redrawArea(21);
}

void Mechanical::o_crystalEnterRed(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Crystal enter", op);

	_crystalLit = 2;
	_vm->redrawArea(22);
}

void Mechanical::o_crystalLeaveYellow(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Crystal leave", op);

	_crystalLit = 0;
	_vm->redrawArea(20);
}

void Mechanical::o_crystalLeaveGreen(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Crystal leave", op);

	_crystalLit = 0;
	_vm->redrawArea(21);
}

void Mechanical::o_crystalLeaveRed(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Crystal leave", op);

	_crystalLit = 0;
	_vm->redrawArea(22);
}

void Mechanical::o_throne_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	// Used on Card 6238 (Sirrus' Throne) and Card 6027 (Achenar's Throne)
	debugC(kDebugScript, "Opcode %d: Brother throne init", op);

	getInvokingResource<MystArea>()->setEnabled(getVar(var));
}

void Mechanical::o_fortressStaircase_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Staircase init", op);

	_vm->_resources[argv[0]]->setEnabled(!_state.staircaseState);
	_vm->_resources[argv[1]]->setEnabled(!_state.staircaseState);
	_vm->_resources[argv[2]]->setEnabled(_state.staircaseState);
}

void Mechanical::birdSing_run() {
	// Used for Card 6220 (Sirrus' Mechanical Bird)
	uint32 time = _vm->_system->getMillis();
	if (_birdSingEndTime < time) {
		_bird->pauseMovie(true);
		_vm->_sound->stopSound();
		_birdSinging = false;
	}
}

void Mechanical::o_bird_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Mechanical bird init", op);

	_birdSinging = false;
	_birdSingEndTime = 0;
	_bird = getInvokingResource<MystAreaVideo>();
}

void Mechanical::o_snakeBox_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Snake box init", op);

	_snakeBox = getInvokingResource<MystAreaVideo>();
}

void Mechanical::elevatorRotation_run() {
	_vm->redrawArea(12);

	_elevatorRotationGearPosition += _elevatorRotationSpeed;

	if (_elevatorRotationGearPosition > 12) {
		uint16 position = (uint16)_elevatorRotationGearPosition;
		_elevatorRotationGearPosition = _elevatorRotationGearPosition - position + position % 6;

		_state.elevatorRotation = (_state.elevatorRotation + 1) % 10;

		_vm->_sound->replaceSoundMyst(_elevatorRotationSoundId);
		_vm->redrawArea(11);
		_vm->wait(100);
	}
}

void Mechanical::o_elevatorRotation_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Elevator rotation init", op);

	_elevatorRotationSoundId = argv[0];
	_elevatorRotationGearPosition = 0;
	_elevatorRotationLeverMoving = false;
}

void Mechanical::fortressRotation_run() {
	VideoEntryPtr gears = _fortressRotationGears->getVideo();

	double oldRate = gears->getRate().toDouble();
	uint32 moviePosition = Audio::Timestamp(gears->getTime(), 600).totalNumberOfFrames();

	// Myst ME short movie workaround, explained in o_fortressRotation_init
	if (_fortressRotationShortMovieWorkaround) {
		// Detect if we just looped
		if (ABS<int32>(_fortressRotationShortMovieLast - 3680) < 50
				&& ABS<int32>(moviePosition) < 50) {
			_fortressRotationShortMovieCount++;
		}

		_fortressRotationShortMovieLast = moviePosition;

		// Simulate longer movie
		moviePosition += 3600 * _fortressRotationShortMovieCount;
	}

	int32 positionInQuarter = 900 - (moviePosition + 900) % 1800;

	// Are the gears moving?
	if (oldRate >= 0.1 || ABS<int32>(positionInQuarter) >= 30 || _fortressRotationBrake) {

		double newRate = oldRate;
		if (_fortressRotationBrake && (double)_fortressRotationBrake * 0.2 > oldRate) {
			newRate += 0.1;
		}

		// Don't let the gears get stuck between two fortress positions
		if (ABS<double>(oldRate) <= 0.05) {
			if (oldRate <= 0.0) {
				newRate += oldRate;
			} else {
				newRate -= oldRate;
			}
		} else {
			if (oldRate <= 0.0) {
				newRate += 0.05;
			} else {
				newRate -= 0.05;
			}
		}

		// Adjust speed accordingly to acceleration lever
		newRate +=  (double) (positionInQuarter / 1500.0)
				* (double) (9 - _fortressRotationSpeed) / 9.0;

		newRate = CLIP<double>(newRate, -2.5, 2.5);

		gears->setRate(Common::Rational((int)(newRate * 1000.0), 1000));

		_gearsWereRunning = true;
	} else if (_gearsWereRunning) {
		// The fortress has stopped. Set its new position
		_fortressPosition = (moviePosition + 900) / 1800 % 4;

		gears->setRate(0);

		if (!_fortressRotationShortMovieWorkaround) {
			gears->seek(Audio::Timestamp(0, 1800 * _fortressPosition, 600));
		} else {
			gears->seek(Audio::Timestamp(0, 1800 * (_fortressPosition % 2), 600));
		}

		_vm->_sound->playSoundBlocking(_fortressRotationSounds[_fortressPosition]);

		_gearsWereRunning = false;
	}
}

void Mechanical::o_fortressRotation_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Fortress rotation init", op);

	_fortressRotationGears = getInvokingResource<MystAreaVideo>();

	VideoEntryPtr gears = _fortressRotationGears->playMovie();
	gears->setLooping(true);
	gears->seek(Audio::Timestamp(0, 1800 * _fortressPosition, 600));
	gears->setRate(0);

	_fortressRotationSounds[0] = argv[0];
	_fortressRotationSounds[1] = argv[1];
	_fortressRotationSounds[2] = argv[2];
	_fortressRotationSounds[3] = argv[3];

	_fortressRotationBrake = 0;

	// WORKAROUND for the tower rotation bug in Myst ME.
	// The original engine only allowed to visit two out of the three small islands,
	// preventing the game from being fully completable.
	// The fortress rotation is computed from the current position in the movie
	// hcgears.mov. The version of this movie that shipped with the ME edition is
	// too short to allow to visit all the islands.
	// ScummVM simulates a longer movie by counting the number of times the movie
	// looped and adding that time to the current movie position.
	// Hence allowing the fortress position to be properly computed.
	uint32 movieDuration = gears->getDuration().convertToFramerate(600).totalNumberOfFrames();
	if (movieDuration == 3680) {
		_fortressRotationShortMovieWorkaround = true;
		_fortressRotationShortMovieCount = 0;
		_fortressRotationShortMovieLast = 0;
	}

	_fortressRotationRunning = true;
	_gearsWereRunning = false;
}

void Mechanical::fortressSimulation_run() {
	if (_fortressSimulationInit) {
		// Init sequence
		_vm->_sound->replaceBackgroundMyst(_fortressSimulationStartSound1, 65535);
		_vm->wait(5000, true);
		_vm->_sound->replaceSoundMyst(_fortressSimulationStartSound2);

		// Update movie while the sound is playing
		VideoEntryPtr startup = _fortressSimulationStartup->playMovie();
		while (_vm->_sound->isPlaying(_fortressSimulationStartSound2)) {
			if (_vm->_video->updateMovies())
				_vm->_system->updateScreen();

			_vm->_system->delayMillis(10);
		}
		_vm->_sound->replaceBackgroundMyst(_fortressSimulationStartSound1, 65535);
		_vm->waitUntilMovieEnds(startup);
		_vm->_sound->stopBackgroundMyst();
		_vm->_sound->replaceSoundMyst(_fortressSimulationStartSound2);


		Common::Rect src = Common::Rect(0, 0, 176, 176);
		Common::Rect dst = Common::Rect(187, 3, 363, 179);
		_vm->_gfx->copyImageSectionToBackBuffer(6046, src, dst);
		_vm->_gfx->copyBackBufferToScreen(dst);
		_vm->_system->updateScreen();

		_fortressSimulationStartup->pauseMovie(true);
		VideoEntryPtr holo = _fortressSimulationHolo->playMovie();
		holo->setLooping(true);
		holo->setRate(0);

		// HACK: Support negative rates with edit lists
		_fortressSimulationHoloRate = 0;
		// END HACK

		_vm->_cursor->showCursor();

		_fortressSimulationInit = false;
	} else {
		VideoEntryPtr holo = _fortressSimulationHolo->getVideo();

		double oldRate = holo->getRate().toDouble();

		// HACK: Support negative rates with edit lists
		oldRate = _fortressSimulationHoloRate;
		// END HACK

		uint32 moviePosition = Audio::Timestamp(holo->getTime(), 600).totalNumberOfFrames();

		int32 positionInQuarter = 900 - (moviePosition + 900) % 1800;

		// Are the gears moving?
		if (oldRate >= 0.1 || ABS<int32>(positionInQuarter) >= 30 || _fortressSimulationBrake) {

			double newRate = oldRate;
			if (_fortressSimulationBrake && (double)_fortressSimulationBrake * 0.2 > oldRate) {
				newRate += 0.1;
			}

			// Don't let the gears get stuck between two fortress positions
			if (ABS<double>(oldRate) <= 0.05) {
				if (oldRate <= 0.0) {
					newRate += oldRate;
				} else {
					newRate -= oldRate;
				}
			} else {
				if (oldRate <= 0.0) {
					newRate += 0.05;
				} else {
					newRate -= 0.05;
				}
			}

			// Adjust speed accordingly to acceleration lever
			newRate +=  (double) (positionInQuarter / 1500.0)
					* (double) (9 - _fortressSimulationSpeed) / 9.0;

			newRate = CLIP<double>(newRate, -2.5, 2.5);

			// HACK: Support negative rates with edit lists

			// Our current QuickTime implementation does not support negative
			// playback rates for movies using edit lists.
			// The fortress rotation simulator movie this code handles is the
			// only movie in the game requiring that feature.

			// This hack approximates the next frame to display when the rate
			// is negative, and seeks to it. It's not intended to be precise.

			_fortressSimulationHoloRate = newRate;

			if (_fortressSimulationHoloRate < 0) {
				double newMoviePosition = moviePosition + _fortressSimulationHoloRate * 10;
				holo->setRate(0);
				holo->seek(Audio::Timestamp(0, (uint)newMoviePosition, 600));
			} else {
				holo->setRate(Common::Rational((int)(newRate * 1000.0), 1000));
			}
			// END HACK

			_gearsWereRunning = true;
		} else if (_gearsWereRunning) {
			// The fortress has stopped. Set its new position
			uint16 simulationPosition = (moviePosition + 900) / 1800 % 4;

			holo->setRate(0);

			// HACK: Support negative rates with edit lists
			_fortressSimulationHoloRate = 0;
			// END HACK

			holo->seek(Audio::Timestamp(0, 1800 * simulationPosition, 600));
			_vm->_sound->playSoundBlocking(	_fortressRotationSounds[simulationPosition]);

			_gearsWereRunning = false;
		}
	}
}

void Mechanical::o_fortressSimulation_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Fortress rotation simulator init", op);

	_fortressSimulationHolo = getInvokingResource<MystAreaVideo>();

	_fortressSimulationStartSound1 = argv[0];
	_fortressSimulationStartSound2 = argv[1];

	_fortressRotationSounds[0] = argv[2];
	_fortressRotationSounds[1] = argv[3];
	_fortressRotationSounds[2] = argv[4];
	_fortressRotationSounds[3] = argv[5];

	_fortressSimulationBrake = 0;

	_fortressSimulationRunning = true;
	_gearsWereRunning = false;
	_fortressSimulationInit = true;

	_vm->_cursor->hideCursor();
}

void Mechanical::o_fortressSimulationStartup_init(uint16 op, uint16 var, uint16 argc, uint16 *argv) {
	debugC(kDebugScript, "Opcode %d: Fortress rotation simulator startup init", op);

	_fortressSimulationStartup = getInvokingResource<MystAreaVideo>();
}

} // End of namespace MystStacks
} // End of namespace Mohawk
