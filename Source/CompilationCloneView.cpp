/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <string>

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
#include "CompilationShared.h"
#include "Constants.h"
#include "OutputParser.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Compilation views"


// Misc variables
int selectedSrcDevice;


CompilationCloneView::CompilationCloneView(BurnWindow& parent)
	:
	BView(B_TRANSLATE_COMMENT("Clone disc", "Tab label"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBurnerThread(NULL),
	fOpenPanel(NULL),
	fImageSize(0),
	fNotification(B_PROGRESS_NOTIFICATION),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime),
	fAbort(0),
	fAction(IDLE)
{
	fWindowParent = &parent;

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

	fBuildButton = new BButton("CreateImageButton",
		B_TRANSLATE_COMMENT("Create image", "Button label"),
		new BMessage(kBuildButton));
	fBuildButton->SetTarget(this);
	fBuildButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE_COMMENT(
	"Burn disc", "Button label"), new BMessage(kBurnButton));
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fBuildButton)
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
	delete fBurnerThread;
	delete fOpenPanel;
}


#pragma mark -- BView Overrides --


void
CompilationCloneView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fBuildButton->SetTarget(this);
	fBuildButton->SetEnabled(true);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationCloneView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBuildButton:
			_GetImageSize();
			break;
		case kBuildOutput:
			_BuildOutput(message);
			break;
		case kBurnButton:
			_Burn();
			break;
		case kBurnOutput:
			_BurnOutput(message);
			break;
		case kGetImageSizeOutput:
			_GetImageSizeOutput(message);
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
CompilationCloneView::_Build()
{
	BPath path;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(path);
		settings->Unlock();
	}
	if (path.InitCheck() != B_OK)
		return;

	if (!CheckFreeSpace(fImageSize * 1024, path.Path())) {
		fBuildButton->SetEnabled(true);
		return;
	}

	status_t ret = path.Append(kCacheFileClone);
	if (ret == B_OK) {
		fOutputView->SetText(NULL);
		fInfoView->SetLabel(B_TRANSLATE_COMMENT(
			"Reading in disc" B_UTF8_ELLIPSIS, "Status notification"));
		fBuildButton->SetEnabled(false);
	
		fNotification.SetGroup("BurnItNow");
		fNotification.SetMessageID("BurnItNow_Clone");
		fNotification.SetTitle(B_TRANSLATE_COMMENT("Building clone image",
			"Notification title"));
		fNotification.SetContent(B_TRANSLATE_COMMENT(
			"Reading in disc" B_UTF8_ELLIPSIS, "Notification content"));
		fNotification.SetProgress(0);
		fNotification.Send();

		fAction = BUILDING;

		BString file = "f=";
		file.Append(path.Path());
		BString device("dev=");
		device.Append(fWindowParent->GetSelectedDevice().number.String());
		sessionConfig config = fWindowParent->GetSessionConfig();

		fBurnerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kBuildOutput), this));
		fBurnerThread->AddArgument("readcd")
			->AddArgument(device)
			->AddArgument("-s")
			->AddArgument("speed=10")	// for max compatibility
			->AddArgument(file)
			->Run();
	}
}


void
CompilationCloneView::_BuildOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseReadcdLine(text, data);
		if (modified == NOCHANGE) {
			data << "\n";
			fOutputView->Insert(data.String());
			fOutputView->ScrollBy(0.0, 50.0);
		} else {
			if (modified == PERCENT) {
				_UpdateProgress();
				_UpdateSizeBar();
			}
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		fInfoView->SetLabel(B_TRANSLATE_COMMENT(
			"Insert a blank disc and burn it",
			"Status notification"));

		fNotification.SetMessageID("BurnItNow_Clone");
		fNotification.SetProgress(100);
		fNotification.SetContent(B_TRANSLATE_COMMENT("Building finished!",
			"Notification content"));
		fNotification.Send();

		BString device("dev=");
		device.Append(fWindowParent->GetSelectedDevice().number.String());

		fBurnerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(), this)); // no need for notification
		fBurnerThread->AddArgument("cdrecord")
			->AddArgument("-eject")
			->AddArgument(device)
			->Run();

		fBuildButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;
	}
}


void
CompilationCloneView::_Burn()
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
	if (ret != B_OK) {
		BString text(B_TRANSLATE_COMMENT(
			"There isn't an image '%filename%' in the cache folder. "
			"Was it perhaps moved or renamed?", "Alert text"));
		text.ReplaceFirst("%filename%", kCacheFileClone);
		(new BAlert("ImageNotFound", text,
			B_TRANSLATE("OK")))->Go();
		return;
	}

	fAction = BURNING;

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
	"Burning in progress" B_UTF8_ELLIPSIS, "Status notification"));
	fBuildButton->SetEnabled(false);
	fBurnButton->SetEnabled(false);

	fNotification.SetGroup("BurnItNow");
	fNotification.SetMessageID("BurnItNow_Clone");
	fNotification.SetTitle(B_TRANSLATE_COMMENT("Burning cloned disc",
		"Notification title"));
	fNotification.SetProgress(0);
	fNotification.Send(60 * 1000000LL);

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnOutput), this));
	fBurnerThread->AddArgument("cdrecord");

	if (config.simulation)
		fBurnerThread->AddArgument("-dummy");
	if (config.eject)
		fBurnerThread->AddArgument("-eject");
	if (config.speed != "")
		fBurnerThread->AddArgument(config.speed);

	fBurnerThread->AddArgument(config.mode)
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


void
CompilationCloneView::_BurnOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseCdrecordLine(text, data);
		if (modified < 0)
			fAbort = modified;
		if (modified <= 0) {
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
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		if (fAbort == SMALLDISC) {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning aborted: The data doesn't fit on the disc.",
				"Status notification"));
			fNotification.SetTitle(B_TRANSLATE_COMMENT("Burning aborted",
				"Notification title"));
			fNotification.SetContent(B_TRANSLATE_COMMENT(
				"The data doesn't fit on the disc.", "Notification content"));
		} else {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning complete. Burn another disc?",
				"Status notification"));
			fNotification.SetProgress(100);
			fNotification.SetContent(B_TRANSLATE_COMMENT("Burning finished!",
				"Notification content"));
		}
		fNotification.SetMessageID("BurnItNow_Clone");
		fNotification.Send();

		fBuildButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;
		fAbort = 0;
		fParser.Reset();
	}
}


void
CompilationCloneView::_GetImageSize()
{
	fImageSize = 0;
	fOutputView->SetText(NULL);
	fBuildButton->SetEnabled(false);

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kGetImageSizeOutput), this));
	fBurnerThread->AddArgument("cdrecord")
		->AddArgument("-media-info")
		->AddArgument(device)
		->Run();
}


void
CompilationCloneView::_GetImageSizeOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		fParser.ParseMediainfoLine(fImageSize, data);
		printf("Image size forecast: %" PRId64 " KiB, %" PRId64 " MiB\n",
			fImageSize, fImageSize / 1024);

		if (fImageSize != 0)
			fSizeView->UpdateSizeDisplay(fImageSize, DATA, CD_OR_DVD);

		data << "\n";
		fOutputView->Insert(data.String());
		fOutputView->ScrollBy(0.0, 50.0);
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK)
		_Build();
}


void
CompilationCloneView::_UpdateProgress()
{
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
