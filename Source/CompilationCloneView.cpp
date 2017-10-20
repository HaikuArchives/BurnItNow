/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include "BurnApplication.h"
#include "CommandThread.h"
#include "CompilationCloneView.h"
#include "Constants.h"
#include "OutputParser.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Clone view"


// Misc variables
int selectedSrcDevice;


CompilationCloneView::CompilationCloneView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Clone disc"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fClonerThread(NULL),
	fOpenPanel(NULL),
	fNotification(B_PROGRESS_NOTIFICATION),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime)
{
	fWindowParent = &parent;
	fAction = IDLE;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoView = new BSeparatorView("InfoView");
	fInfoView->SetFont(be_bold_font);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Insert the disc and create an image",
		"Status notification"));

	fOutputView = new BTextView("OutputView");
	fOutputView->SetWordWrap(false);
	fOutputView->MakeEditable(false);
	BScrollView* fOutputScrollView = new BScrollView("OutputScroller",
		fOutputView, B_WILL_DRAW, true, true);
	fOutputScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fImageButton = new BButton("CreateImageButton",
		B_TRANSLATE("Create image"), new BMessage(kBuildImage));
	fImageButton->SetTarget(this);
	fImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE("Burn image"),
		new BMessage(kBurnDisc));
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fImageButton)
				.Add(fBurnButton)
				.AddGlue()
				.End()
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fInfoView)
			.Add(fOutputScrollView)
			.End()
		.Add(fSizeView);

	_UpdateSizeBar();
}


CompilationCloneView::~CompilationCloneView()
{
	delete fClonerThread;
	delete fOpenPanel;
}


#pragma mark -- BView Overrides --


void
CompilationCloneView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fImageButton->SetTarget(this);
	fImageButton->SetEnabled(true);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationCloneView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBuildImage:
			_CreateImage();
			break;
		case kBurnDisc:
			_BurnImage();
			break;
		case kCloner:
			_CloneOutput(message);
			break;
		default:
		if (kDeviceChange[0] == message->what) {
			selectedSrcDevice = 0;
			break;
		} else if (kDeviceChange[1] == message->what) {
			selectedSrcDevice = 1;
			break;
		} else if (kDeviceChange[2] == message->what) {
			selectedSrcDevice = 2;
			break;
		} else if (kDeviceChange[3] == message->what) {
			selectedSrcDevice = 3;
			break;
		} else if (kDeviceChange[4] == message->what) {
			selectedSrcDevice = 4;
			break;
		}
		BView::MessageReceived(message);
	}
}


#pragma mark -- Public Methods --


int32
CompilationCloneView::InProgress()
{
	return fAction;
}


#pragma mark -- Private Methods --


void
CompilationCloneView::_BurnImage()
{
	BPath path;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(path);
		settings->Unlock();
	}
	if (path.InitCheck() != B_OK)
		return;

	status_t ret = path.Append(kCacheFileClone);
	if (ret == B_OK) {
		fAction = BURNING;

		fOutputView->SetText(NULL);
		fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS, "Status notification"));

		fNotification.SetGroup("BurnItNow");
		fNotification.SetMessageID("BurnItNow_Clone");
		fNotification.SetTitle(B_TRANSLATE("Burning cloned disc"));
		fNotification.SetProgress(0);
		fNotification.Send(60 * 1000000LL);

		BString device("dev=");
		device.Append(fWindowParent->GetSelectedDevice().number.String());
		sessionConfig config = fWindowParent->GetSessionConfig();

		fClonerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kCloner), this));
		fClonerThread->AddArgument("cdrecord");

		if (config.simulation)
			fClonerThread->AddArgument("-dummy");
		if (config.eject)
			fClonerThread->AddArgument("-eject");
		if (config.speed != "")
			fClonerThread->AddArgument(config.speed);

		fClonerThread->AddArgument(config.mode)
			->AddArgument("fs=16m")
			->AddArgument(device)
			->AddArgument("-v")	// to get progress output
			->AddArgument("-gracetime=2")
			->AddArgument("-pad")
			->AddArgument("padsize=63s")
			->AddArgument(path.Path())
			->Run();

		fParser.Reset();
	}
}


void
CompilationCloneView::_CreateImage()
{
	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Image creating in progress" B_UTF8_ELLIPSIS, "Status notification"));
	fImageButton->SetEnabled(false);

	fNotification.SetGroup("BurnItNow");
	fNotification.SetMessageID("BurnItNow_Clone");
	fNotification.SetTitle(B_TRANSLATE("Building clone image"));
	fNotification.SetContent(B_TRANSLATE("Preparing the build" B_UTF8_ELLIPSIS));
	fNotification.SetProgress(0);
	 // It may take a while for the building to start...
	fNotification.Send(60 * 1000000LL);

	BPath path;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(path);
		settings->Unlock();
	}
	if (path.InitCheck() != B_OK)
		return;

	status_t ret = path.Append(kCacheFileClone);
	if (ret == B_OK) {
		fAction = BUILDING;

		BString file = "f=";
		file.Append(path.Path());
		BString device("dev=");
		device.Append(fWindowParent->GetSelectedDevice().number.String());
		sessionConfig config = fWindowParent->GetSessionConfig();

		fClonerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kCloner), this));
		fClonerThread->AddArgument("readcd")
			->AddArgument(device)
			->AddArgument("-s")
			->AddArgument("speed=10")	// for max compatibility
			->AddArgument(file)
			->Run();
	}
}


void
CompilationCloneView::_CloneOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseLine(text, data);
		if (modified == NOCHANGE) {
			data << "\n";
			fOutputView->Insert(data.String());
			fOutputView->ScrollBy(0.0, 50.0);
		} else {
			if (modified == PERCENT)
				_UpdateProgress();
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (fAction == BUILDING)) {
		BString result(fOutputView->Text());

		// Last output line always (except error) contains speed statistics
		if (result.FindFirst(" kB/sec.") != B_ERROR) {
			fAction = IDLE;

			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Insert a blank disc and burn the image",
				"Status notification"));

			_UpdateSizeBar();

			BString device("dev=");
			device.Append(fWindowParent->GetSelectedDevice().number.String());

			fClonerThread = new CommandThread(NULL,
				new BInvoker(new BMessage(), this)); // no need for notification
			fClonerThread->AddArgument("cdrecord")
				->AddArgument("-eject")
				->AddArgument(device)
				->Run();
		} else {
			fAction = IDLE;
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Failed to create image", "Status notification"));
			return;
		}

		fImageButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (fAction == BURNING)) {
		fInfoView->SetLabel(B_TRANSLATE_COMMENT(
			"Burning complete. Burn another disc?", "Status notification"));
		fImageButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fNotification.SetMessageID("BurnItNow_Clone");
		fNotification.SetProgress(100);
		fNotification.SetContent(B_TRANSLATE("Burning finished!"));
		fNotification.Send();

		fAction = IDLE;
		fParser.Reset();
	}
}


void
CompilationCloneView::_UpdateProgress()
{
	if (fProgress == 0 || fProgress == 1.0)
		fNotification.SetContent(" ");
	else
		fNotification.SetContent(fETAtime);
	fNotification.SetMessageID("BurnItNow_Clone");
	fNotification.SetProgress(fProgress);
	fNotification.Send();
}


void
CompilationCloneView::_UpdateSizeBar()
{
	off_t fileSize = 0;

	BPath path;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(path);
		settings->Unlock();
	}
	if (path.InitCheck() != B_OK)
		return;

	status_t ret = path.Append(kCacheFileClone);
	if (ret == B_OK) {
		BEntry entry(path.Path());
		entry.GetSize(&fileSize);
	}
	fSizeView->UpdateSizeDisplay(fileSize / 1024, DATA, CD_OR_DVD);
	// size in KiB
}
