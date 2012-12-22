/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "StatusWindow.h"

#include "const.h"

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Button.h>


extern char VOL_NAME[25];
int32 TRACK_FIN;


StatusView::StatusView(BRect r, const char* title)
	:
	BView(r, title, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fViewFont = new BFont(be_plain_font);
	SetFont(fViewFont);
	fNumberOfTracks = 0;
	fAngles[0] = 0;
	TRACK_FIN = 0;
}


void StatusView::SetAngles(float ang[100], int tracks)
{
	int i;
	fNumberOfTracks = tracks;
	for (i = 0; i < fNumberOfTracks; i++)
		fAngles[i] = ang[i];
}


void StatusView::Draw(BRect updateRect)
{
	char temp[5];
	float oldangle;
	int i;
	BPoint center, p1;
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fViewFont->SetSize(13);
	center.x = 70;
	center.y = Bounds().bottom / 2;
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());
	SetHighColor(190, 190, 190);
	SetPenSize(2.0);
	FillEllipse(center, 60.0, 60.0);
	SetHighColor(50, 50, 50);
	SetPenSize(2.0);
	StrokeEllipse(center, 60.0, 60.0);

	SetHighColor(105, 105, 105);

	oldangle = 0;
	SetLowColor(190, 190, 190);
	if (fNumberOfTracks > 1) {
		for (i = 0; i < fNumberOfTracks; i++) {
			p1 = center;
			p1.y += 60 * sin((3.14 / 180) * (fAngles[i] - 90));
			p1.x += 60 * cos((3.14 / 180) * (fAngles[i] - 90));
			StrokeLine(center, p1);
			p1 = center;
			p1.y += 40 * sin((3.14 / 180) * ((fAngles[i] - (fAngles[i] - oldangle) / 2) - 85));
			p1.x += 40 * cos((3.14 / 180) * ((fAngles[i] - (fAngles[i] - oldangle) / 2) - 85));
			MovePenTo(p1);
			fViewFont->SetRotation(-(fAngles[i] - (fAngles[i] - oldangle) / 2) - 180 - 89);
			if (i < TRACK_FIN) {
				fViewFont->SetFace(B_BOLD_FACE);
				SetFont(fViewFont);

				if (TRACK_FIN - 1 == i)
					SetHighColor(255, 0, 0);
				else
					SetHighColor(0, 0, 0);

				sprintf(temp, "[%d]", i + 1);
			} else {
				fViewFont->SetFace(B_REGULAR_FACE);
				SetFont(fViewFont);
				SetHighColor(0, 0, 0);
				sprintf(temp, "%d", i + 1);
			}
			DrawString(temp);
			SetHighColor(105, 105, 105);
			oldangle = fAngles[i];
		}
	} else {
		if (fNumberOfTracks > 0) {
			SetHighColor(0, 0, 0);
			SetFont(fViewFont);
			MovePenTo(center);
			DrawString("1");
		}
	}
	SetHighColor(50, 50, 50);
	p1 = center;
	p1.y -= 60;
}


StatusWindow::StatusWindow(const char* title)
	:
	BWindow(BRect(220, 200, 500, 270), "StatusWindow", B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	BRect r;
	if (title != NULL)
		SetTitle(title);
	else
		SetTitle("BurnItNow");

	r = Bounds();
	BView* aroundView = new BView(r, "AroundView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	aroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(aroundView);

	r = aroundView->Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom = r.top + 30;
	fStatusBar = new BStatusBar(r, "MyStatus");
	aroundView->AddChild(fStatusBar);
	fStatusBar->SetText("");

	fFullView = false;

	r = aroundView->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = r.bottom - 20;
	r.left = r.right - 55;
	fCloseButton = new BButton(r, "close", "Close", new BMessage('ClWi'));
	aroundView->AddChild(fCloseButton);
	fCloseButton->SetEnabled(false);
	r.right = r.left - 5;
	r.left = r.right - 55;
	fMoreButton = new BButton(r, "more", "More..", new BMessage('More'));
	aroundView->AddChild(fMoreButton);
	r = aroundView->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 70;
	r.bottom = 210;
	fStatusView = new StatusView(r, "StatusView");
	aroundView->AddChild(fStatusView);
}


void StatusWindow::SendMessage(BMessage* msg)
{
	be_app->PostMessage(msg);
}


void StatusWindow::StatusSetMax(float t1)
{
	fStatusBar->SetMaxValue(t1);
}


void StatusWindow::UpdateStatus(float delta, const char* str)
{
	char temp[1024];
	char temp_char[5];
	int temp_int;
	memset(temp_char, 0, 5);

	strncpy(temp_char, &str[6], 2);
	temp_int = atol(temp_char);
	if (temp_int != TRACK_FIN) {
		TRACK_FIN = temp_int;
		fStatusView->Invalidate();
	}
	fStatusBar->Update(delta - fStatusBar->CurrentValue(), str);
	sprintf(temp, "%s - [%s]", VOL_NAME, str);
	SetTitle(temp);
}


void StatusWindow::StatusSetText(const char* str)
{
	char temp[1024];
	char temp_char[5];

	strncpy(temp_char, &str[7], 2);
	TRACK_FIN = atol(temp_char);
	if (!strcmp("Fixating...", str))
		TRACK_FIN = 1000;
	fStatusView->Invalidate();
	sprintf(temp, "%s - [%s]", VOL_NAME, str);
	fStatusBar->SetText(str);
	SetTitle(temp);
}


void StatusWindow::StatusSetColor(rgb_color color)
{
	fStatusBar->SetBarColor(color);
}


void StatusWindow::StatusUpdateReset()
{
	fStatusBar->Reset();
	fStatusBar->Update(fStatusBar->MaxValue());
}


void StatusWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'ClWi':
			Quit();
			break;
		case 'More':
			if (!fFullView) {
				fFullView = !fFullView;
				ResizeBy(0.0, 150.0);
				fMoreButton->SetLabel("Less..");
			} else {
				fFullView = !fFullView;
				ResizeBy(0.0, -150.0);
				fMoreButton->SetLabel("More..");
			}
			break;
	}
}


void StatusWindow::Ready()
{
	char temp[1024];
	sprintf(temp, "Done!");
	StatusSetColor(blue);
	StatusUpdateReset();
	StatusSetText(temp);
	fCloseButton->SetEnabled(true);
	TRACK_FIN = 1000;
	fStatusView->Invalidate();
}


void StatusWindow::SetAngles(float ang[100], int tracks)
{
	fStatusView->SetAngles(ang, tracks);
	fStatusView->Invalidate();
}
