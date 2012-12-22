/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DataView.h"

#include "const.h"
#include "IconLabel.h"

#include <string.h>

#include <Box.h>


extern char DATA_STRING[100];
extern bool BOOTABLE;
extern bool VRCD;
extern int16 IMAGE_TYPE;


DataView::DataView(BRect size)
	:
	BView(size, "DataView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect r;
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom = 90;
	IconLabel* fileSystemLabel = new IconLabel(BRect(0, 0, 28 + be_bold_font->StringWidth(" Filesystem"), 19), " Filesystem", "datacd_16.png");
	fileSystemLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fileSystemLabel->SetDrawingMode(B_OP_ALPHA);
	BBox* fileSystemBox = new BBox(r, "fFileSystemBox", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	fileSystemBox->SetLabel(fileSystemLabel);
	AddChild(fileSystemBox);
	r = fileSystemBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.right = 220;
	r.bottom = 25;
	BRadioButton* ISO9660Radio = new BRadioButton(r, "ISO-9660", "ISO-9660", new BMessage(DATA_ISO9660));
	if (!strcmp(DATA_STRING, " ") && IMAGE_TYPE == 0)
		ISO9660Radio->SetValue(B_CONTROL_ON);
	fileSystemBox->AddChild(ISO9660Radio);


	r.bottom += 15;
	r.top += 15;
	BRadioButton* windowsRadio = new BRadioButton(r, "ISO-9660 with long filenames", "ISO-9660 with long filenames", new BMessage(DATA_WINDOWS));
	if (!strcmp(DATA_STRING, "-D -l") && IMAGE_TYPE == 0)
		windowsRadio->SetValue(B_CONTROL_ON);
	fileSystemBox->AddChild(windowsRadio);

	r.bottom += 15;
	r.top += 15;
	BRadioButton* jolietRadio = new BRadioButton(r, "fJolietRadio", "Windows (Joliet)", new BMessage(DATA_JOLIET));
	if (!strcmp(DATA_STRING, "-l -D -J") && IMAGE_TYPE == 0)
		jolietRadio->SetValue(B_CONTROL_ON);
	fileSystemBox->AddChild(jolietRadio);

	r.bottom += 15;
	r.top += 15;
	fBeOSRadio = new BRadioButton(r, "Haiku", "Haiku (bfs)", new BMessage(DATA_BFS));
	if (IMAGE_TYPE == 1)
		fBeOSRadio->SetValue(B_CONTROL_ON);

	fileSystemBox->AddChild(fBeOSRadio);


	r = fileSystemBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.bottom = 25;
	r.left = 230;
	BRadioButton* rockRadio = new BRadioButton(r, "RockRidge (UNIX Multiuser)", "RockRidge (UNIX Singeluser)", new BMessage(DATA_ROCK));
	if (!strcmp(DATA_STRING, "-l -L -r") && IMAGE_TYPE == 0)
		rockRadio->SetValue(B_CONTROL_ON);
	fileSystemBox->AddChild(rockRadio);

	r.bottom += 15;
	r.top += 15;
	BRadioButton* realRockRadio = new BRadioButton(r, "Real RockRidge (UNIX Multiuser)", "Real RockRidge (UNIX Multiuser)", new BMessage(DATA_REALROCK));
	if (!strcmp(DATA_STRING, "-l -L -R") && IMAGE_TYPE == 0)
		realRockRadio->SetValue(B_CONTROL_ON);
	fileSystemBox->AddChild(realRockRadio);

	r.bottom += 15;
	r.top += 15;
	BRadioButton* macRadio = new BRadioButton(r, "fMacRadio", "Mac (hfs)", new BMessage(DATA_HFS));
	if (!strcmp(DATA_STRING, "-hfs") && IMAGE_TYPE == 0)
		macRadio->SetValue(B_CONTROL_ON);
	fileSystemBox->AddChild(macRadio);

	r.bottom += 15;
	r.top += 15;
	BRadioButton* ownRadio = new BRadioButton(r, "own", "Own (choose mkisofs options)", new BMessage(DATA_HFS));
//	if(!strcmp(DATA_STRING,"-hfs"))
//		fMacRadio->SetValue(B_CONTROL_ON);
	ownRadio->SetEnabled(false);
	fileSystemBox->AddChild(ownRadio);

	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 95;
	r.right = r.right - 160;

	IconLabel* bootLabel = new IconLabel(BRect(0, 0, 19 + 
	                       be_bold_font->StringWidth(" El Torito Bootable CD"), 19), " El Torito Bootable CD", "bootcd_16.png");
	bootLabel->SetDrawingMode(B_OP_ALPHA);                       
	bootLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BBox* bootBox = new BBox(r, "BootBox", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	bootBox->SetLabel(bootLabel);
	AddChild(bootBox);

	r = bootBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.bottom = r.top + 15;
	fBootableCDCheckBox = new BCheckBox(r, "fBootableCDCheckBox", "Enable BootableCD", new BMessage(BOOT_CHECKED));
	if (BOOTABLE)
		fBootableCDCheckBox->SetValue(B_CONTROL_ON);
	if (!VRCD)
		fBootableCDCheckBox->SetEnabled(false);

	bootBox->AddChild(fBootableCDCheckBox);

	r = bootBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 35;
	fChooseBootImageButton = new BButton(r, "fChooseBootImageButton", "Choose boot image", new BMessage(BOOT_FILE_PANEL));
	if (!BOOTABLE || !VRCD)
		fChooseBootImageButton->SetEnabled(false);
	bootBox->AddChild(fChooseBootImageButton);

	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 135;
	r.left = r.right - 150;
	r.bottom = 165;

	AddChild(new BButton(r, "Change Volume Name", "Change Volume Name", new BMessage(CHANGE_VOL_NAME)));

	fFilePanel = new BFilePanel(B_OPEN_PANEL);
	fFilePanel->SetMessage(new BMessage(BOOT_CHANGE_IMAGE_NAME));
}
