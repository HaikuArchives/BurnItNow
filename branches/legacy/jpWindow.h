/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _JPWINDOW_H_
#define _JPWINDOW_H_


#include <Box.h>
#include <FilePanel.h>
#include <Path.h>
#include <Slider.h>
#include <StatusBar.h>
#include <TabView.h>
#include <Window.h>

#include "const.h"


// Forward declarations
class DataView;
class BurnView;
class AudioView;
class CopyCDView;
class PrefsView;
class IconLabel;
class LogView;
class CDRWView;
class RightList;
class LeftList;
class AskName;
class Prefs;
class StatusWindow;
class AboutWindow;


class jpWindow : public BWindow {
public:
	jpWindow(BRect frame);
	virtual void InitBurnIt();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage* message);
	virtual void CheckForDevices();
	virtual void FindCDRTools();
	virtual void SetISOFile(char* string);
	virtual void PutLog(const char*);
	virtual void MessageLog(const char*);
	virtual void MakeImageNOW(int, const char*);
	virtual void BurnNOW();
	virtual void BlankNOW();
	virtual void BurnWithCDRecord();
	virtual void UpdateStatus(float delta, char* str);
	virtual void SetButtons(bool);
	virtual void AWindow();
	virtual int  CheckMulti(char* str);
	virtual void GetTsize(char* tsize);
	virtual void SaveData();
	virtual void MakeBootImage();
	virtual void CalculateSize();
	virtual uint64 GetVRCDSize();

	BButton* fParentDirButton, *fMakeDirButton, *fNewVRCDButton, *fAddISOButton, *fCalcSizeButton;
	RightList* fRightList;
	Prefs* fBurnItPrefs;

	BSlider* fSpeedSlider;
	BMenuBar* fMenuBar;
	BBox* fAroundBox;
	AskName* fVolumeNameWindow;
	StatusWindow* fStatusWindow;
	AboutWindow* fAboutWindow;

	BFilePanel* fISOOpenPanel, *fISOSavePanel;
	BStatusBar* fStatusBar;
	BTabView* fTabView;
	BTab* fMiscTab;

	BurnView* fBurnView;
	DataView* fDataView;
	AudioView* fAudioView;
	PrefsView* fPrefsView;
	LogView* fLogView;
	CDRWView* fCDRWView;


	LeftList* fLeftList;


	struct cdrecorder fScsiDevices[100];
	int fRecorderCount;
	struct cdrecorder* fBurnDevice;
	
private:
	BPath		fPath;
	status_t	fStatus;	
};


#endif	// _JPWINDOW_H_
