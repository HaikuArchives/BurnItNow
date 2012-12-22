/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow maintainers
 * Distributed under the terms of the MIT License.
 */


#include "BurnView.h"

#include "const.h"
#include "IconLabel.h"

#include <stdio.h>
#include <string.h>

#include <Box.h>
#include <RadioButton.h>


extern int16 BURN_SPD;
extern int16 BURN_TYPE;
extern char MULTISESSION[10];
extern char DUMMYMODE[10];
extern char EJECT[10];
extern bool ONTHEFLY;


BurnView::BurnView(BRect size)
	:
	BView(size, "BurnView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect r, r2;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	char temp_char[100];
	// burnBox
	IconLabel* burnLabel = new IconLabel(BRect(0, 0, 19 + be_bold_font->StringWidth(" Burn"), 19), " Burn", "cd_16.png");
	burnLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	burnLabel->SetDrawingMode(B_OP_ALPHA);
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	BBox* burnBox = new BBox(r, "burnBox", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	burnBox->SetLabel(burnLabel);
	AddChild(burnBox);
	r = burnBox->Bounds();
	r.left = 150;
	r.right = 278;
	r.top = 50;
	r.bottom = 120;
	IconLabel* BGLabel = new IconLabel(r, "", "lighter.png");
	BGLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BGLabel->SetDrawingMode(B_OP_ALPHA);
	burnBox->AddChild(BGLabel);
	// RadioButtons
	r = burnBox->Bounds();
	r2 = r;
	r.InsetBy(5.0, 5.0);
	r.left += 25;
	r.top += 15;
	r.bottom = 30;
	r.right = 125;
	r2.InsetBy(5.0, 5.0);
	r2.left += 6;
	r2.right = 27;
	r2.top += 14;
	r2.bottom = 34;
	IconLabel* dataCDLabel = new IconLabel(r2, "", "datacd_16.png");
	dataCDLabel->SetDrawingMode(B_OP_ALPHA);
	dataCDLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	burnBox->AddChild(dataCDLabel);

	r2.top += 21;
	r2.bottom += 21;
	IconLabel* audioCDLabel = new IconLabel(r2, "", "audiocd_16.png");
	audioCDLabel->SetDrawingMode(B_OP_ALPHA);
	audioCDLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	burnBox->AddChild(audioCDLabel);

	r2.top += 21;
	r2.bottom += 21;
	IconLabel* mixCDLabel = new IconLabel(r2, "", "mix-cd_16.png");
	mixCDLabel->SetDrawingMode(B_OP_ALPHA);
	mixCDLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	burnBox->AddChild(mixCDLabel);

	BRadioButton* dataCDRadio = new BRadioButton(r, "DataCD", "DataCD", new BMessage(BURN_DATA_CD));
	if (BURN_TYPE == 0)
		dataCDRadio->SetValue(B_CONTROL_ON);
	burnBox->AddChild(dataCDRadio);

	r.top += 20;
	r.bottom += 20;
	BRadioButton* audioCDRadio = new BRadioButton(r, "AudioCD", "AudioCD", new BMessage(BURN_AUDIO_CD));
	if (BURN_TYPE == 1)
		audioCDRadio->SetValue(B_CONTROL_ON);
	burnBox->AddChild(audioCDRadio);

	r.top += 20;
	r.bottom += 20;
	BRadioButton* mixCDRadio = new BRadioButton(r, "MixCD", "MixCD", new BMessage(BURN_MIX_CD));
	if (BURN_TYPE == 2)
		mixCDRadio->SetValue(B_CONTROL_ON);
	burnBox->AddChild(mixCDRadio);
	// CheckBoxes
	r.top += 25;
	r.bottom += 25;
	r.left = 13;
	r.right = 125;
	fMultiCheckBox = new BCheckBox(r, "fMultiCheckBox", "MultiSession", new BMessage(BURN_MULTI));
	if (!strncmp(MULTISESSION, "-multi", 6))
		fMultiCheckBox->SetValue(B_CONTROL_ON);

	burnBox->AddChild(fMultiCheckBox);

	r.top += 15;
	r.bottom += 15;
	fOnTheFlyCheckBox = new BCheckBox(r, "fOnTheFlyCheckBox", "On the fly", new BMessage(BURN_ONTHEFLY));
	if (ONTHEFLY == true) {
		fOnTheFlyCheckBox->SetValue(B_CONTROL_ON);
		fMultiCheckBox->SetEnabled(false);
	}
	burnBox->AddChild(fOnTheFlyCheckBox);

	r.top += 15;
	r.bottom += 15;
	fDummyModeCheckBox = new BCheckBox(r, "fDummyModeCheckBox", "Dummy mode", new BMessage(BURN_DUMMY_MODE));
	if (!strncmp(DUMMYMODE, "-dummy", 6))
		fDummyModeCheckBox->SetValue(B_CONTROL_ON);

	burnBox->AddChild(fDummyModeCheckBox);

	r.top += 15;
	r.bottom += 15;
	fEjectCheckBox = new BCheckBox(r, "fEjectCheckBox", "Eject", new BMessage(BURN_EJECT));
	if (!strncmp(EJECT, "-eject", 6))
		fEjectCheckBox->SetValue(B_CONTROL_ON);

	burnBox->AddChild(fEjectCheckBox);

	// fBurnButton
	r = burnBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 120;
	r.bottom -= 5;
	r.left = 370;
	r.right = 460;
	fBurnButton = new BButton(r, "fBurnButton", "Burn!", new BMessage(MAKE_IMAGE));
	burnBox->AddChild(fBurnButton);

	r = burnBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 120;
	r.bottom -= 5;
	r.left = 270;
	r.right = 360;
	fMakeImageButton = new BButton(r, "fMakeImageButton", "Make Image", new BMessage(MAKE_AND_SAVE_IMAGE));
	burnBox->AddChild(fMakeImageButton);
	// BurningSpeed
	r = burnBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.left = 225;
	r.right = 370;
	r.top += 10;
	sprintf(temp_char, "Burning Speed [%dx]", BURN_SPD);
	fSpeedSlider = new BSlider(r, "Speed", temp_char, new BMessage(SPEED_CHANGE), 0, 5, B_BLOCK_THUMB, B_FOLLOW_NONE);
	fSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSpeedSlider->SetHashMarkCount(6);
	burnBox->AddChild(fSpeedSlider);
	fSpeedSlider->SetValue((int32)(BURN_SPD / 2) - 1);
}


void BurnView::SetButton(bool what)
{
	fBurnButton->SetEnabled(what);
	fMakeImageButton->SetEnabled(what);
}


void BurnView::SetLabel(char* label)
{
	fBurnButton->SetLabel(label);
}
