/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CommandThread.h"
#include "CompilationCDRWView.h"

#include <Alert.h>
#include <Button.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Message constants
const int32 kBlankMessage = 'Blnk';
const int32 kBlankerMessage = 'Blkr';

CompilationCDRWView::CompilationCDRWView(BurnWindow& parent)
	:
	BView("CDRW", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fBlankerThread(NULL)
{
	windowParent = &parent;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBlankerInfoBox = new BSeparatorView("ImageInfoBBox");
	fBlankerInfoBox->SetFont(be_bold_font);
	fBlankerInfoBox->SetLabel("Ready");

	fBlankerInfoTextView = new BTextView("ImageInfoTextView");
	fBlankerInfoTextView->SetWordWrap(false);
	fBlankerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("ImageInfoScrollView",
		fBlankerInfoTextView, 0, true, true);

	fBlankModeMenu = new BMenu("BlankModeMenu");
	fBlankModeMenu->SetLabelFromMarked(true);

	BMenuItem* blankModeMenuItem = new BMenuItem("All", new BMessage());
	blankModeMenuItem->SetMarked(true);
	fBlankModeMenu->AddItem(blankModeMenuItem);
	fBlankModeMenu->AddItem(new BMenuItem("Fast", new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem("Session", new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem("Track", new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem("Track tail", new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem("Unreserve", new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem("Unclose", new BMessage()));
	
	BMenuField* blankModeMenuField = new BMenuField("BlankModeMenuField",
		"Blank:", fBlankModeMenu);

	BButton* blankButton = new BButton("BlankButton", "Blank!",
		new BMessage(kBlankMessage));
	blankButton->SetTarget(this);
	
	fBlankModeMenu->SetExplicitMinSize(BSize(200, B_SIZE_UNLIMITED));

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(blankModeMenuField)
			.Add(blankButton)
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fBlankerInfoBox)
			.Add(infoScrollView)
			.End();
}


CompilationCDRWView::~CompilationCDRWView()
{
	delete fBlankerThread;
}


#pragma mark -- BView Overrides --


void
CompilationCDRWView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* blankButton = dynamic_cast<BButton*>(FindView("BlankButton"));
	if (blankButton != NULL)
		blankButton->SetTarget(this);
}


void
CompilationCDRWView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBlankMessage:
			_Blank();
			break;
		case kBlankerMessage:
			_BlankerParserOutput(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationCDRWView::_Blank()
{
	BString mode = fBlankModeMenu->FindMarked()->Label();
	mode.ToLower();
	
	if (mode == "track tail")
		mode = "trtail";		
		
	fBlankerInfoTextView->SetText(NULL);
	fBlankerInfoBox->SetLabel("Blanking in progress" B_UTF8_ELLIPSIS);
	
	fBlankerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBlankerMessage), this));
	fBlankerThread->AddArgument("cdrecord")
		->AddArgument("blank=")
		->AddArgument(mode)
		->AddArgument("dev=")
		->AddArgument(windowParent->GetSelectedDevice().number.String())
		->Run();
}


void
CompilationCDRWView::_BlankerParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fBlankerInfoTextView->Insert(data.String());
	
	if (!fBlankerThread->IsRunning())
		fBlankerInfoBox->SetLabel("Ready");
}
