/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include "CommandThread.h"
#include "CompilationBlankView.h"
#include "Constants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Blank view"


CompilationBlankView::CompilationBlankView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Blank RW-disc"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBlankerThread(NULL),
	fNotification(B_INFORMATION_NOTIFICATION),
	fAction(IDLE)
{
	fWindowParent = &parent;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoView = new BSeparatorView("InfoView");
	fInfoView->SetFont(be_bold_font);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
	"Insert the disc and blank it", "Status notification"));

	fOutputView = new BTextView("OutputView");
	fOutputView->SetWordWrap(false);
	fOutputView->MakeEditable(false);
	BScrollView* fOutputScrollView = new BScrollView("OutputScroller",
		fOutputView, B_WILL_DRAW, true, true);
	fOutputScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fBlankModeMenu = new BMenu("BlankModeMenu");
	fBlankModeMenu->SetLabelFromMarked(true);

	fBlankModeMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("All",
		"Blanking mode. From the man page: 'Blank the entire disk. This may "
		"take a long time.'"), new BMessage()));
	BMenuItem* blankModeMenuItem = new BMenuItem(B_TRANSLATE_COMMENT("Fast",
		"Blanking mode. From the man page: 'Minimally blank the disk. This "
		"results in erasing the PMA, the TOC and the pregap.'"),
		new BMessage());
	blankModeMenuItem->SetMarked(true);
	fBlankModeMenu->AddItem(blankModeMenuItem);
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

	fBlankButton = new BButton("BlankButton",
		B_TRANSLATE("Blank disc"), new BMessage(kBlankButton));
	fBlankButton->SetTarget(this);

	fBlankModeMenu->SetExplicitMinSize(BSize(200, B_SIZE_UNLIMITED));

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(blankModeMenuField)
			.Add(fBlankButton)
			.AddGlue()
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fInfoView)
			.Add(fOutputScrollView)
			.End();
}


CompilationBlankView::~CompilationBlankView()
{
	delete fBlankerThread;
}


#pragma mark -- BView Overrides --


void
CompilationBlankView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fBlankButton->SetTarget(this);
}


void
CompilationBlankView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBlankButton:
			_Blank();
			break;
		case kBlankOutput:
			_BlankOutput(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Public Methods --


int32
CompilationBlankView::InProgress()
{
	return fAction;
}


#pragma mark -- Private Methods --


void
CompilationBlankView::_Blank()
{
	fAction = BLANKING;

	BString mode = fBlankModeMenu->FindMarked()->Label();
	mode.ToLower();

	if (mode == "track tail")
		mode = "trtail";

	mode.Prepend("blank=");

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Blanking in progress"
		B_UTF8_ELLIPSIS, "Status notification"));
	fBlankButton->SetEnabled(false);

	fNotification.SetGroup("BurnItNow");
	fNotification.SetMessageID("BurnItNow_Blank");
	fNotification.SetTitle(B_TRANSLATE("Blanking disc"));
	if (mode == "All")
		fNotification.SetContent(B_TRANSLATE(
		"This may take over 30 minutes..."));
	else
		fNotification.SetContent(B_TRANSLATE(
		"This may take a while..."));

	fNotification.Send();

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fBlankerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBlankOutput), this));

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
CompilationBlankView::_BlankOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fOutputView->Insert(data.String());
		fOutputView->ScrollBy(0.0, 50.0);
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		fInfoView->SetLabel(B_TRANSLATE_COMMENT("Blanking finished",
			"Status notification"));

		fNotification.SetMessageID("BurnItNow_Blank");
		fNotification.SetContent(B_TRANSLATE("Blanking finished!"));
		fNotification.Send();

		fBlankButton->SetEnabled(true);

		fAction = IDLE;
	}
}
