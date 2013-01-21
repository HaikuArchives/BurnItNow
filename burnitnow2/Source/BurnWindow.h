/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BURNWINDOW_H_
#define _BURNWINDOW_H_


#include <MenuBar.h>
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


class BurnWindow : public BWindow {
public:
	BurnWindow(BRect frame, const char* title);

	virtual void MessageReceived(BMessage* message);
	
	void FindDevices(sdevice *array);
	sdevice GetSelectedDevice();
	bool GetSessionMode();

private:
	BMenuBar* _CreateMenuBar();
	BView* _CreateToolBar();
	BTabView* _CreateTabView();
	BView* _CreateDiskUsageView();

	void _BurnDisc();
	void _BuildImage();
	void _ClearCache();
	void _OpenSettings();
	void _OpenWebSite();
	void _OpenHelp();
	void _UpdateSpeedSlider(BMessage* message);
	
	BTabView* fTabView;
};

#endif	// _BURNWINDOW_H_
