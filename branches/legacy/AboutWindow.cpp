/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2010-2011 BurnItNow maintainers 
 * Distributed under the terms of the MIT License.
 */


#include "AboutWindow.h"
#include "copyright.h"

#include "const.h"

#include <Button.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>


BBitmap* GetBitmapResource(type_code type, const char* name);


AboutView::AboutView(BRect r, const char* title)
	:
	BView(r, title, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fViewFont = new BFont(be_plain_font);
	SetFont(fViewFont);
	fBurnBitmap = BTranslationUtils::GetBitmap('PNG ', "about.png");
	fBurnProofBitmap = BTranslationUtils::GetBitmap('PNG ', "burn-proof.png");
	fCDRecordBitmap = BTranslationUtils::GetBitmap('PNG ', "cdrecord.png");
}


AboutView::~AboutView()
{
	delete fViewFont;
	delete fBurnBitmap;
	delete fBurnProofBitmap;
	delete fCDRecordBitmap;
}


void AboutView::Draw(BRect updateRect)
{
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect r, r2;
	BPoint p1;
	r = Bounds();
	FillRect(r);
	p1.x = r.left;
	p1.y = r.top;
	DrawBitmap(fBurnBitmap, p1);
	fViewFont->SetSize(18);
	fViewFont->SetFace(B_BOLD_FACE);
	SetFont(fViewFont);
	p1.x = r.left + 252;
	p1.y = r.top + 67;
	MovePenTo(p1);
	SetHighColor(255,120,255);
	SetDrawingMode(B_OP_ALPHA);
	DrawString(VERSION);
	MovePenTo(p1);
	SetHighColor(0,0,0);
	SetDrawingMode(B_OP_ALPHA);
	DrawString(VERSION);
	fViewFont->SetSize(20);
	fViewFont->SetFace(B_REGULAR_FACE);
	SetFont(fViewFont);
	p1.x = r.left + 10;
	p1.y = r.top + 110;
	MovePenTo(p1);
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_ALPHA);
	DrawString(COPYRIGHT1);
	p1.x = r.left + 10;
	p1.y = r.top +140;
	MovePenTo(p1);
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_ALPHA);
	DrawString(COPYRIGHT2);
	p1.x = r.left + 205;
	p1.y = r.bottom - 37;
	DrawBitmap(fBurnProofBitmap, p1);
	r2.left = p1.x;
	r2.top = p1.y;
	r2.right = p1.x + 180;
	r2.bottom = p1.y + 32;
	StrokeRect(r2);
	p1.x = r.left + 5;
	p1.y = r.bottom - 65;
	DrawBitmap(fCDRecordBitmap, p1);
	r2.left = p1.x;
	r2.top = p1.y;
	r2.right = p1.x + 180;
	r2.bottom = p1.y + 60;
	StrokeRect(r2);
	StrokeRect(r);
}


AboutWindow::AboutWindow()
	:
	BWindow(BRect(250, 200, 650, 500), "AboutWindow", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	BRect r;
	SetTitle("About");

	r = Bounds();
	BView* aroundView = new BView(r, "AroundView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	aroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(aroundView);

	r = aroundView->Bounds();
	r.InsetBy(5.0, 5.0);
	aroundView->AddChild(new BButton(r, "close", "Close", new BMessage('ClWi')));
	aroundView->AddChild(new AboutView(r, "AboutView"));
}


void AboutWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'ClWi':
			Quit();
			break;
	}
}
