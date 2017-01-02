/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BURNWINDOW_H_
#define _BURNWINDOW_H_


#include <CheckBox.h>
#include <MenuBar.h>
#include <Slider.h>
#include <String.h>
#include <TabView.h>
#include <View.h>
#include <Window.h>

#define MAX_DEVICES 5


typedef struct sdevice {
	BString number;
	BString manufacturer;
	BString model;
} sdevice;


typedef struct sessionConfig {
	int32 multisession;
	int32 onthefly;
	int32 simulation;
	int32 eject;
	BString speed;
	BString mode;
} sessionConfig;


class BurnWindow : public BWindow {
public:
					BurnWindow(BRect frame, const char* title);

	bool			QuitRequested();
	virtual void	MessageReceived(BMessage* message);

	void			FindDevices(sdevice* array);
	sdevice			GetSelectedDevice();
	sessionConfig	GetSessionConfig();

private:
	BMenuBar*		_CreateMenuBar();
	BView*			_CreateToolBar();
	BTabView*		_CreateTabView();
	BView*			_CreateDiskUsageView();

	void 			_ClearCache();
	void 			_OpenSettings();
	void 			_OpenWebSite();
	void 			_OpenHelp();
	void 			_UpdateSpeedSlider(BMessage* message);

	BTabView* 		fTabView;

	BMenu* 			fSessionMenu;
	BMenu* 			fDeviceMenu;
	BMenuItem*		fCacheQuitItem;
//	BCheckBox* 		fMultiCheck;
//	BCheckBox* 		fOntheflyCheck;
	BCheckBox* 		fSimulationCheck;
	BCheckBox* 		fEjectCheck;
	BSlider* 		fSpeedSlider;

	sessionConfig	fConfig;
};

#endif	// _BURNWINDOW_H_
