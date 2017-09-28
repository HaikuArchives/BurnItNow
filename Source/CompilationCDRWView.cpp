/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CommandThread.h"
#include "CompilationCDRWView.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Blank view"

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Message constants
const int32 kBlankMessage = 'Blnk';
const int32 kBlankerMessage = 'Blkr';

CompilationCDRWView::CompilationCDRWView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Blank CD-RW"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fBlankerThread(NULL)
{
	windowParent = &parent;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBlankerInfoBox = new BSeparatorView("ImageInfoBBox");
	fBlankerInfoBox->SetFont(be_bold_font);
	fBlankerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
	"Insert the disc and blank it", "Status notification"));

	fBlankerInfoTextView = new BTextView("ImageInfoTextView");
	fBlankerInfoTextView->SetWordWrap(false);
	fBlankerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("ImageInfoScrollView",
		fBlankerInfoTextView, B_WILL_DRAW, true, true);
	infoScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fBlankModeMenu = new BMenu("BlankModeMenu");
	fBlankModeMenu->SetLabelFromMarked(true);

	BMenuItem* blankModeMenuItem = new BMenuItem(B_TRANSLATE_COMMENT("All",
		"Blanking mode. From the man page: 'Blank the entire disk. This may "
		"take a long time.'"), new BMessage());
	blankModeMenuItem->SetMarked(true);
	fBlankModeMenu->AddItem(blankModeMenuItem);
	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Fast",
		"Blanking mode. From the man page: 'Minimally blank the disk. This "
		"results in erasing the PMA, the TOC and the pregap.'"),
		new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Session",
		"Blanking mode. From the man page: 'Blank the last session.'"),
		new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Track",
		"Blanking mode. From the man page: 'Blank a track.'"),
		new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Track tail",
		"Blanking mode. From the man page: 'Blank the tail of a track.'"),
		new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Unreserve",
		"Blanking mode. From the man page: 'Unreserve a reserved track.'"),
		new BMessage()));
	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Unclose",
		"Blanking mode. From the man page: 'Unclose last session.'"),
		new BMessage()));

	BMenuField* blankModeMenuField = new BMenuField("BlankModeMenuField",
		"Type:", fBlankModeMenu);

	BString toolTip(B_TRANSLATE("Blanking types:\n\n"
		"All\t\t\tBlank the entire disk. This may take a long time.\n"
		"Fast\t\t\tMinimally blank the disk.\n"
		"\t\t\tThis erases the PMA, the TOC and the pregap.\n"
		"Session\t\tBlank the last session.\n"
		"Track\t\tBlank a track.\n"
		"Track tail\t\tBlank the tail of a track.\n"
		"Unreserve\tUnreserve a reserved track.\n"
		"Unclose\t\tUnclose last session."));

	blankModeMenuField->SetToolTip(toolTip.String());

	BButton* blankButton = new BButton("BlankButton",
		B_TRANSLATE("Blank disc"), new BMessage(kBlankMessage));
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
	delete fOpenPanel;
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

	mode.Prepend("blank=");

	fBlankerInfoTextView->SetText(NULL);
	fBlankerInfoBox->SetLabel(B_TRANSLATE_COMMENT("Blanking in progress"
		B_UTF8_ELLIPSIS, "Status notification"));

	BString device("dev=");
	device.Append(windowParent->GetSelectedDevice().number.String());
	sessionConfig config = windowParent->GetSessionConfig();

	fBlankerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBlankerMessage), this));

	fBlankerThread->AddArgument("cdrecord")
		->AddArgument(mode);

	if (config.simulation)
		fBlankerThread->AddArgument("-dummy");
	if (config.eject)
		fBlankerThread->AddArgument("-eject");

	fBlankerThread->AddArgument(device);
	fBlankerThread->Run();
}


void
CompilationCDRWView::_BlankerParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fBlankerInfoTextView->Insert(data.String());
		fBlankerInfoTextView->ScrollTo(0.0, 2000.0);
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK)
		fBlankerInfoBox->SetLabel(B_TRANSLATE_COMMENT("Blanking complete",
			"Status notification"));
}
