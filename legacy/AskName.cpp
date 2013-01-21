/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow Maintainers
 * Distributed under the terms of the MIT License.
 */


#include "AskName.h"

#include "const.h"

#include <Application.h>
#include <Button.h>
#include <ScrollView.h>


AskName::AskName(BRect frame, const char* title, uint32 mess, const char* what)
	:
	BWindow(frame, "AskName", B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	BRect r, r2;
	if (title != NULL)
		SetTitle(title);
	else
		SetTitle("BurnItNow");

	fMessageWhat = mess;

	r = Bounds();
	BView* aroundView = new BView(r, "aroundView", B_FOLLOW_NONE, B_WILL_DRAW);
	aroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(aroundView);

	r = aroundView->Bounds();
	r.InsetBy(5.0, 5.0);
	r.right -= 70;
	r.bottom = 26;
	fNameTextView = new BTextView(r, "Name_", r, B_FOLLOW_NONE, B_WILL_DRAW);
	fNameTextView->SetWordWrap(false);
	fNameTextView->DisallowChar('\n');
	aroundView->AddChild(new BScrollView("NameScroll", fNameTextView, B_FOLLOW_NONE, 0, false, false, B_FANCY_BORDER));
	r2 = aroundView->Bounds();
	r2.InsetBy(5.0, 5.0);
	r2.left = r.right + 5;
	r2.top -= 3;
	r2.bottom = 25;
	aroundView->AddChild(new BButton(r2, "Ok", "Ok", new BMessage('STxt')));

	fNameTextView->MakeFocus();
	if (what != NULL) {
		fNameTextView->Insert(what);
		fNameTextView->SelectAll();
	}
}


void AskName::SendText()
{
	BMessage* temp_msg;
	temp_msg = new BMessage(fMessageWhat);
	temp_msg->AddString("DirName", fNameTextView->Text());
	be_app->PostMessage(temp_msg);
	Quit();
}


void AskName::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'STxt' :
			SendText();
			break;
		default :
			BWindow::MessageReceived(msg);
	}
}
