/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STATUSWINDOW_H_
#define _STATUSWINDOW_H_


#include <StatusBar.h>
#include <Window.h>


class StatusView : public BView {
public:
	StatusView(BRect r, const char* name);
	virtual void Draw(BRect updateRect);
	virtual void SetAngles(float* ang, int tracks);

	BFont* fViewFont;
	float fAngles[100];
	int fNumberOfTracks;
};


class StatusWindow : public BWindow {
public:
	StatusWindow(const char* title);
	virtual void UpdateStatus(float delta, const char* str);
	virtual void MessageReceived(BMessage* msg);
	virtual void StatusSetMax(float t1);
	virtual void StatusSetText(const char* str);
	virtual void StatusSetColor(rgb_color color);
	virtual void StatusUpdateReset();
	virtual void SetAngles(float* ang, int tracks);
	virtual void SendMessage(BMessage* msg);
	virtual void Ready();

	StatusView* fStatusView;
	BButton* fCloseButton, *fMoreButton;
	BStatusBar* fStatusBar;
	bool fFullView;
};


#endif	// _STATUSWINDOW_H_
