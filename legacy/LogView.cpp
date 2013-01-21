/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow Maintainers
 * Distributed under the terms of the MIT License.
 */


#include "LogView.h"

#include "const.h"
#include "IconLabel.h"

#include <Box.h>
#include <ScrollView.h>


LogView::LogView(BRect size)
	:
	BView(size, "LogView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect r, r2;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	// fLogBox
	IconLabel* logLabel = new IconLabel(BRect(0, 0, 19 + be_bold_font->StringWidth(""), 19), "", "log_16.png");
	logLabel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	logLabel->SetDrawingMode(B_OP_ALPHA);
	r = Bounds();
	BBox* logBox = new BBox(r, "logBox", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	logBox->SetLabel(logLabel);
	AddChild(logBox);

	// This will be inside the fLogBox
	r = logBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.top += 15;
	r2 = r;
	r2.InsetBy(2.0, 2.0);
	fLogTextView = new BTextView(r, "fLogTextView", r2, B_FOLLOW_NONE, B_WILL_DRAW);
	fLogTextView->MakeEditable(false);
	fLogTextView->SetStylable(true);
	//BScrollView* 
	logScrollView = new BScrollView("logScrollView", fLogTextView, B_FOLLOW_NONE, 0, false, true, B_FANCY_BORDER);
	logBox->AddChild(logScrollView);
}
