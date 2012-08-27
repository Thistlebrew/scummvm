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
 */

#include "common/algorithm.h"
#include "common/bufferedstream.h"
#include "common/EventRecorder.h"
#include "common/savefile.h"
#include "common/system.h"
#include "graphics/colormasks.h"
#include "graphics/palette.h"
#include "graphics/scaler.h"
#include "graphics/thumbnail.h"
#include "common/translation.h"
#include "gui/widgets/list.h"
#include "gui/message.h"
#include "gui/saveload.h"
#include "common/system.h"
#include "gui/ThemeEval.h"
#include "gui/gui-manager.h"
#include "recorderdialog.h"

#define MAX_RECORDS_NAMES 0xFF

namespace GUI {

enum {
	kRecordCmd = 'RCRD',
	kPlaybackCmd = 'PBCK',
	kDeleteCmd = 'DEL ',
	kNextScreenshotCmd = 'NEXT',
	kPrevScreenshotCmd = 'PREV',
	kEditRecordCmd = 'EDIT'
};

RecorderDialog::RecorderDialog() : Dialog("RecorderDialog"), _list(0), _currentScreenshot(0) {
	_backgroundType = ThemeEngine::kDialogBackgroundSpecial;
	ButtonWidget *recordButton;
	ButtonWidget *playbackButton;
	_list = new GUI::ListWidget(this, "RecorderDialog.List");
	_list->setNumberingMode(GUI::kListNumberingOff);
	new GUI::ButtonWidget(this, "RecorderDialog.Delete", _("Delete"), 0, kDeleteCmd);
	new GUI::ButtonWidget(this, "RecorderDialog.Cancel", _("Cancel"), 0, kCloseCmd);
	recordButton = new GUI::ButtonWidget(this, "RecorderDialog.Record", _("Record"), 0, kRecordCmd);
	playbackButton = new GUI::ButtonWidget(this, "RecorderDialog.Playback", _("Playback"), 0, kPlaybackCmd);
	_gfxWidget = new GUI::GraphicsWidget(this, 0, 0, 10, 10);
	_container = new GUI::ContainerWidget(this, 0, 0, 10, 10);
	new GUI::ButtonWidget(this,"RecorderDialog.NextScreenShotButton", "<", 0, kPrevScreenshotCmd);
	new GUI::ButtonWidget(this, "RecorderDialog.PreviousScreenShotButton", ">", 0, kNextScreenshotCmd);	
	_currentScreenshotText = new StaticTextWidget(this, "RecorderDialog.currentScreenshot", "0/0");
	if (_gfxWidget)
		_gfxWidget->setGfx(0);
}


void RecorderDialog::reflowLayout() {
	if (g_gui.xmlEval()->getVar("Globals.RecorderDialog.ExtInfo.Visible") == 1) {
		int16 x, y;
		uint16 w, h;

		if (!g_gui.xmlEval()->getWidgetData("RecorderDialog.Thumbnail", x, y, w, h)) {
			error("Error when loading position data for Recorder Thumbnails");
		}

		int thumbW = kThumbnailWidth;
		int thumbH = kThumbnailHeight2;
		int thumbX = x + (w >> 1) - (thumbW >> 1);
		int thumbY = y + kLineHeight;

		_container->resize(x, y, w, h);
		_gfxWidget->resize(thumbX, thumbY, thumbW, thumbH);

		_container->setVisible(true);
		_gfxWidget->setVisible(true);
		updateSelection(false);
	} else {
		_container->setVisible(false);
		_gfxWidget->setVisible(false);
	}
	Dialog::reflowLayout();
}



void RecorderDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch(cmd) {
	case kEditRecordCmd: {
		EditRecordDialog editDlg(_playbackFile.getHeader().author, _playbackFile.getHeader().name, _playbackFile.getHeader().notes);
		if (editDlg.runModal() != kOKCmd) {
			return;
		}
		_playbackFile.getHeader().author = editDlg.getAuthor();
		_playbackFile.getHeader().name = editDlg.getName();
		_playbackFile.getHeader().notes = editDlg.getNotes();
		_playbackFile.updateHeader();
	}
		break;
	case kNextScreenshotCmd:
		++_currentScreenshot;
		updateScreenshot();
		break;
	case kPrevScreenshotCmd:
		--_currentScreenshot;
		updateScreenshot();
		break;
	case kDeleteCmd:
		if (_list->getSelected() >= 0) {
			MessageDialog alert(_("Do you really want to delete this record?"),
				_("Delete"), _("Cancel"));
			if (alert.runModal() == GUI::kMessageOK) {
				_playbackFile.close();
				g_eventRec.deleteRecord(_list->getSelectedString());
				_list->setSelected(-1);
				updateList();
			}
		}
		break;
	case GUI::kListSelectionChangedCmd:
		updateSelection(true);
		break;
	case kRecordCmd: {
		const EnginePlugin *plugin = 0;
		TimeDate t;
		Common::String gameId = ConfMan.get("gameid", _target);
		GameDescriptor desc = EngineMan.findGame(gameId, &plugin);
		g_system->getTimeAndDate(t);
		EditRecordDialog editDlg("Unknown Author", Common::String::format("%.2d.%.2d.%.4d ", t.tm_mday, t.tm_mon, 1900 + t.tm_year) + desc.description(), "");
		if (editDlg.runModal() != kOKCmd) {
			return;
		}
		_author = editDlg.getAuthor();
		_name = editDlg.getName();
		_notes = editDlg.getNotes();
		_filename = generateRecordFileName();
		setResult(kRecordDialogRecord);
		close();
		}
		break;
	case kPlaybackCmd:
		if (_list->getSelected() >= 0) {
			_filename = _list->getSelectedString();
			setResult(kRecordDialogPlayback);
			close();
		}
		break;
	case kCloseCmd:
		setResult(kRecordDialogClose);
	default:
		Dialog::handleCommand(sender, cmd, data);
		}
	}

void RecorderDialog::updateList() {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::String pattern(_target+".r??");
	Common::StringArray files = saveFileMan->listSavefiles(pattern);
	_list->setList(files);
	_list->draw();
}

int RecorderDialog::runModal(Common::String &target) {
	_target = target;
	updateList();
	return Dialog::runModal();
}

RecorderDialog::~RecorderDialog() {
}

void RecorderDialog::updateSelection(bool redraw) {
	_gfxWidget->setGfx(-1, -1, 0, 0, 0);
	_screenShotsCount = 0;
	_currentScreenshot = 0;
	updateScreenShotsText();
	if (_list->getSelected() >= 0) {
		_playbackFile.openRead(_list->getSelectedString());
		_screenShotsCount = _playbackFile.getScreensCount();
		if ((_screenShotsCount) > 0) {
			_currentScreenshot = 1;
		}
		updateScreenshot();
	}
}

Common::String RecorderDialog::generateRecordFileName() {
	ConfMan.getActiveDomainName();
	GUI::ListWidget::StringArray recordsList = _list->getList();
	for (int i = 0; i < MAX_RECORDS_NAMES; ++i) {
		Common::String recordName = Common::String::format("%s.r%02x", _target.c_str(), i);
		if (Common::find(_list->getList().begin(), _list->getList().end(), recordName) != _list->getList().end()) {
			continue;
		}
		return recordName;
	}
	return "";
}

void RecorderDialog::updateScreenshot() {
	if (_currentScreenshot < 1) {
		_currentScreenshot = _screenShotsCount;
	}
	if (_currentScreenshot > _screenShotsCount) {
		_currentScreenshot = 1;
	}
	Graphics::Surface *srcsf = _playbackFile.getScreenShot(_currentScreenshot);
	if (srcsf != NULL) {
		Graphics::Surface *destsf = Graphics::scale(*srcsf, _gfxWidget->getWidth(), _gfxWidget->getHeight());
		_gfxWidget->setGfx(destsf);
		updateScreenShotsText();
		delete destsf;
		delete srcsf;
	} else {
		_gfxWidget->setGfx(-1, -1, 0, 0, 0);
	}
	_gfxWidget->draw();
}

void RecorderDialog::updateScreenShotsText() {
	_currentScreenshotText->setLabel(Common::String::format("%d / %d", _currentScreenshot, _screenShotsCount));
}

} // End of namespace GUI
