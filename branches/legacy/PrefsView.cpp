/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PrefsView.h"

#include "const.h"
#include "IconLabel.h"

#include <string.h>

#include <Box.h>
#include <MenuField.h>


extern char DAO[10];
extern char BURNPROOF[30];


PrefsView::PrefsView(BRect size)
	:
	BView(size, "PrefsView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect r;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom = r.top + 20;
	fRecordersMenu = new BMenu("Select");
	fRecordersMenu->SetLabelFromMarked(true);
	BMenuField* recorderMenuField = new BMenuField(r, "recorder", "Devices:", fRecordersMenu);
	recorderMenuField->SetDivider(be_plain_font->StringWidth("Devices:  "));
	AddChild(recorderMenuField);

	IconLabel* miscLabel = new IconLabel(BRect(0, 0, 19 + 
	                       be_bold_font->StringWidth(" Misc Options (see help before change options)"), 19), 
	                       " Misc Options (see help before change options)", "cdprefs_16.png");
	miscLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	miscLabel->SetDrawingMode(B_OP_ALPHA);
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = r.top + 25;
	BBox* miscOptBox = new BBox(r, "fMiscOptBoxions", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	miscOptBox->SetLabel(miscLabel);
	AddChild(miscOptBox);

	r = miscOptBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 15;
	r.right = 175;
	r.bottom = 30;
	fDAOCheckBox = new BCheckBox(r, "DAO", "DAO (Disc At Once)", new BMessage(MISC_DAO));
	if (!strncmp(DAO, "-dao", 4))
		fDAOCheckBox->SetValue(B_CONTROL_ON);

	miscOptBox->AddChild(fDAOCheckBox);

	r.top += 15;
	r.bottom += 15;
	fBurnProofCheckBox = new BCheckBox(r, "burnproof", "BurnProof (read help)", new BMessage(MISC_BURNPROOF));
	if (!strncmp(BURNPROOF, "driveropts = burnproof", 22))
		fBurnProofCheckBox->SetValue(B_CONTROL_ON);

	miscOptBox->AddChild(fBurnProofCheckBox);
}
