/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AudioView.h"

#include "const.h"
#include "IconLabel.h"

#include <string.h>

#include <Box.h>
#include <ScrollView.h>
#include <TextView.h>


extern char PAD[10]; //-pad (audio)
extern char NOFIX[10]; // -nofix (audio)
extern char PREEMP[10]; // -preemp (audio)
extern char SWAB[10]; // -swab


AudioView::AudioView(BRect size)
	:
	BView(size, "AudioView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect r, r2;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	IconLabel* advLabel = new IconLabel(BRect(0, 0, 19 + be_bold_font->StringWidth(" Audio Options (see help before change options)"), 19), 
							" Audio Options (see help before change options)", "audiocd_16.png");
	advLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	advLabel->SetDrawingMode(B_OP_ALPHA);
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom = 80;
	BBox* advOptionsBox = new BBox(r, "fAdvOptionsBox", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	advOptionsBox->SetLabel(advLabel);
	AddChild(advOptionsBox);


	r = advOptionsBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.right = 120;
	r.bottom = 25;
	fPadCheckBox = new BCheckBox(r, "pad", "Padding", new BMessage(AUDIO_PAD));
	if (!strncmp(PAD, "-pad", 4))
		fPadCheckBox->SetValue(B_CONTROL_ON);

	advOptionsBox->AddChild(fPadCheckBox);

	r.bottom += 15;
	r.top += 15;
	r.right = 120;
	fNoFixCheckBox = new BCheckBox(r, "nofix", "No fixate", new BMessage(AUDIO_NOFIX));
	if (!strncmp(NOFIX, "-nofix", 6))
		fNoFixCheckBox->SetValue(B_CONTROL_ON);

	advOptionsBox->AddChild(fNoFixCheckBox);

	r.bottom += 15;
	r.top += 15;
	fPreEmpCheckBox = new BCheckBox(r, "preemp", "Preemp", new BMessage(AUDIO_PREEMP));
	if (!strncmp(PREEMP, "-preemp", 7))
		fPreEmpCheckBox->SetValue(B_CONTROL_ON);

	advOptionsBox->AddChild(fPreEmpCheckBox);

	r = advOptionsBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.left = 125;
	r.bottom = 25;

	fSwabCheckBox = new BCheckBox(r, "swab", "Swab", new BMessage(AUDIO_SWAB));
	if (!strncmp(SWAB, "-swab", 5))
		fSwabCheckBox->SetValue(B_CONTROL_ON);

	advOptionsBox->AddChild(fSwabCheckBox);


	// Info box
	r = Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 85;
	BBox* infoBox = new BBox(r, "Info", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	infoBox->SetLabel("Audio Info");
	AddChild(infoBox);

	r = infoBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top += 10;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r2 = r;
	r2.InsetBy(2.0, 2.0);
	BTextView* infoTextView = new BTextView(r, "infoTextView", r2, B_FOLLOW_NONE, B_WILL_DRAW);
	infoTextView->MakeEditable(false);
	infoTextView->SetStylable(true);
	BScrollView* infoScrollView = new BScrollView("infoScrollView", infoTextView, B_FOLLOW_NONE, 0, false, true, B_FANCY_BORDER);
	infoBox->AddChild(infoScrollView);
	infoTextView->SetFontAndColor(be_fixed_font, B_FONT_ALL, &darkblue);
	infoTextView->Insert("Here will be some AudioInfo in the future like what codecs there are on the system.");
}
