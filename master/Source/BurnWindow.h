/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BURNWINDOW_H_
#define _BURNWINDOW_H_


#include <MenuBar.h>
#include <View.h>
#include <Window.h>


class BurnWindow : public BWindow {
public:
	BurnWindow(BRect frame, const char* title);

	virtual void MessageReceived(BMessage* message);

private:
	BMenuBar* _CreateMenuBar();
	BView* _CreateToolBar();
	BView* _CreateTabView();
	BView* _CreateDiskUsageView();

	void _BurnDisc();
	void _BuildImage();
	void _OpenWebSite();
	void _OpenHelp();
	void _UpdateSpeedSlider(BMessage* message);
};


#endif	// _BURNWINDOW_H_
