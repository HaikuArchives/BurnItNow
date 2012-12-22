/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow maintainers
 * Distributed under the terms of the MIT License.
 */


#include "CDRWView.h"

#include "const.h"
#include "IconLabel.h"

#include <stdio.h>

#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>


extern int16 BLANK_SPD;


CDRWView::CDRWView(BRect size)
	:
	BView(size, "CDRWView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect r;
	char temp_char[100];
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	// CDRW BOX
	IconLabel* CDRWLabel = new IconLabel(BRect(0, 0, 19 + be_bold_font->StringWidth(""), 19), "", "cd_16.png");
	CDRWLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	CDRWLabel->SetDrawingMode(B_OP_ALPHA);
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom = r.top + 160;
	BBox* CDRWBox = new BBox(r, "CDRWBox", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	CDRWBox->SetLabel(CDRWLabel);
	AddChild(CDRWBox);

	// BlankMenu (CDRWBOX)
	r = CDRWBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 25;
	fBlankMenu = new BMenu("Select");
	fBlankMenu->SetLabelFromMarked(true);
	BMenuField* blankMenuField = new BMenuField(r, "blank", "Blank:", fBlankMenu);
	blankMenuField->SetDivider(be_plain_font->StringWidth("Blank:  "));
	CDRWBox->AddChild(blankMenuField);

	fBlankMenu->AddItem(new BMenuItem("Full", new BMessage(BLANK_FULL)));
	fBlankMenu->AddItem(new BMenuItem("Fast", new BMessage(BLANK_FAST)));
	fBlankMenu->AddItem(new BMenuItem("Session", new BMessage(BLANK_SESSION)));
	fBlankMenu->AddItem(new BMenuItem("Track", new BMessage(BLANK_TRACK)));
	fBlankMenu->AddItem(new BMenuItem("Track tail", new BMessage(BLANK_TRACK_TAIL)));
	fBlankMenu->AddItem(new BMenuItem("Unreserve", new BMessage(BLANK_UNRES)));
	fBlankMenu->AddItem(new BMenuItem("Unclose", new BMessage(BLANK_UNCLOSE)));

	r = CDRWBox->Bounds();
	r.left = 150;
	r.right = 278;
	r.top = 60;
	r.bottom = 128;
	IconLabel* BGLabel1 = new IconLabel(r, "", "cdrw_64.png");  //cdrw_64.png
	BGLabel1->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BGLabel1->SetDrawingMode(B_OP_BLEND);
	CDRWBox->AddChild(BGLabel1);
	IconLabel* BGLabel2 = new IconLabel(r, "", "erase_64.png"); //erase_64.png
	BGLabel2->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BGLabel2->SetDrawingMode(B_OP_BLEND);
	CDRWBox->AddChild(BGLabel2);


	// BlankSpeed
	r = CDRWBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 15;
	r.left = 155;
	r.right = 270;
	sprintf(temp_char,"Blank Speed [%dx]", BLANK_SPD);
	fBlankSpeedSlider = new BSlider(r, "BlankSpeed", temp_char, new BMessage(BLANK_SPEED_CHANGE), 0, 5, B_BLOCK_THUMB, B_FOLLOW_NONE);
	fBlankSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBlankSpeedSlider->SetHashMarkCount(6);
	fBlankSpeedSlider->SetValue((int32)(BLANK_SPD / 2) - 1);
	CDRWBox->AddChild(fBlankSpeedSlider);

	// BlankButton
	r = CDRWBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.bottom = r.top + 50;
	r.left = 330;
	r.right = 415;
	fBlankButton = new BButton(r, "BlankButton", "Blank!", new BMessage(BLANK_IT_NOW));
	CDRWBox->AddChild(fBlankButton);
}
