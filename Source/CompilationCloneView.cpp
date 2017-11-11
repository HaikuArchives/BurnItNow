/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <string>
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Notification.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringList.h>
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
	fNoteID(""),
	fID(0),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime),
	fAbort(0),
	fAction(IDLE),
	fAudioMode(false)
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
			_GetImageInfo();
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
		case kGetImageInfoOutput:
			_GetImageInfoOutput(message);
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

	if (fAudioMode == true) {
		BDirectory folder(path.Path());
		status_t ret = folder.CreateDirectory(kCacheFolderAudioClone, NULL);
		if (!(ret == B_FILE_EXISTS || ret == B_OK))
			return;

		ret = path.Append(kCacheFolderAudioClone);
		if (ret == B_OK) {
			fAction = BUILDING;
			fBuildButton->SetEnabled(false);

			fOutputView->SetText(NULL);
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Reading in WAV files" B_UTF8_ELLIPSIS, "Status notification"));

			BNotification buildProgress(B_PROGRESS_NOTIFICATION);
			buildProgress.SetGroup("BurnItNow");
			buildProgress.SetTitle(B_TRANSLATE_COMMENT("Cloning Audio CD",
				"Notification title"));
			buildProgress.SetContent(B_TRANSLATE_COMMENT(
				"Reading in WAV files" B_UTF8_ELLIPSIS, "Notification content"));
			buildProgress.SetProgress(0);

			char id[5];
			snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
			fNoteID = "BurnItNow_Clone-";
			fNoteID.Append(id);

			buildProgress.SetMessageID(fNoteID);
			buildProgress.Send();

			BString wavPath(path.Path());
			wavPath.Append("/");

			BString device("dev=");
			device.Append(fWindowParent->GetSelectedDevice().number.String());
			sessionConfig config = fWindowParent->GetSessionConfig();

			fBurnerThread = new CommandThread(NULL,
				new BInvoker(new BMessage(kBuildOutput), this));
			fBurnerThread->AddArgument("cdda2wav")
				->AddArgument(device)
				->AddArgument("paraopts=proof")
				->AddArgument("-vall")
				->AddArgument("cddb=0")
				->AddArgument("-B")
				->AddArgument("-Owav")
				->AddArgument(wavPath)
				->Run();
		}
	} else {
		status_t ret = path.Append(kCacheFileClone);
		if (ret == B_OK) {
			fAction = BUILDING;
			fBuildButton->SetEnabled(false);

			fOutputView->SetText(NULL);
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Reading in disc" B_UTF8_ELLIPSIS, "Status notification"));

			BNotification buildProgress(B_PROGRESS_NOTIFICATION);
			buildProgress.SetGroup("BurnItNow");
			buildProgress.SetTitle(B_TRANSLATE_COMMENT("Building clone image",
				"Notification title"));
			buildProgress.SetContent(B_TRANSLATE_COMMENT(
				"Reading in disc" B_UTF8_ELLIPSIS, "Notification content"));
			buildProgress.SetProgress(0);

			char id[5];
			snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
			fNoteID = "BurnItNow_Clone-";
			fNoteID.Append(id);

			buildProgress.SetMessageID(fNoteID);
			buildProgress.Send();

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
				_UpdateProgress(B_TRANSLATE_COMMENT("Building clone image",
					"Notification title"));
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

		BNotification buildSuccess(B_INFORMATION_NOTIFICATION);
		buildSuccess.SetGroup("BurnItNow");
		buildSuccess.SetTitle(B_TRANSLATE_COMMENT("Building clone image",
			"Notification title"));
		buildSuccess.SetContent(B_TRANSLATE_COMMENT("Building finished!",
			"Notification content"));
		buildSuccess.SetMessageID(fNoteID);
		buildSuccess.Send();

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

	if (fAudioMode == true) {
		path.Append(kCacheFolderAudioClone);
		BFile testFile(path.Path(), B_READ_ONLY);
		status_t result = testFile.InitCheck();

		if (result != B_OK) {
			BString text(B_TRANSLATE_COMMENT(
				"There is no folder '%foldername%' in the cache folder. "
				"Was it perhaps moved or renamed?", "Alert text"));
			text.ReplaceFirst("%foldername%", kCacheFolderAudioClone);
			(new BAlert("ImageNotFound", text,
				B_TRANSLATE("OK")))->Go();

			return;
		}
	} else {
		path.Append(kCacheFileClone);
		BFile testFile(path.Path(), B_READ_ONLY);
		status_t result = testFile.InitCheck();

		if (result != B_OK) {
			BString text(B_TRANSLATE_COMMENT(
				"There isn't an image '%filename%' in the cache folder. "
				"Was it perhaps moved or renamed?", "Alert text"));
			text.ReplaceFirst("%filename%", kCacheFileClone);
			(new BAlert("ImageNotFound", text,
				B_TRANSLATE("OK")))->Go();

			return;
		}
	}

	fAction = BURNING;
	fBuildButton->SetEnabled(false);
	fBurnButton->SetEnabled(false);

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS, "Status notification"));

	BNotification burnProgress(B_PROGRESS_NOTIFICATION);
	burnProgress.SetGroup("BurnItNow");
	burnProgress.SetTitle(B_TRANSLATE_COMMENT("Burning clone disc",
		"Notification title"));
	burnProgress.SetProgress(0);

	char id[5];
	snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
	fNoteID = "BurnItNow_Clone-";
	fNoteID.Append(id);

	burnProgress.SetMessageID(fNoteID);
	burnProgress.Send(60 * 1000000LL);

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

	if (fAudioMode == true) {
		BString files(path.Path());
		files.Append("/*.wav");
		fBurnerThread->AddArgument(config.mode)
			->AddArgument(device)
			->AddArgument("gracetime=2")
			->AddArgument("-v")	// to get progress output
			->AddArgument("-dao")
			->AddArgument("-useinfo")
			->AddArgument("-text")
			->AddArgument(files)
			->Run();

	} else {
		fBurnerThread->AddArgument(config.mode)
			->AddArgument("fs=16m")
			->AddArgument(device)
			->AddArgument("-v")	// to get progress output
			->AddArgument("gracetime=2")
			->AddArgument("-pad")
			->AddArgument("padsize=63s")
			->AddArgument(path.Path())
			->Run();
	}
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
				_UpdateProgress(B_TRANSLATE_COMMENT("Burning clone disc",
				"Notification title"));
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		if (fAbort == SMALLDISC) {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning aborted: The data doesn't fit on the disc",
				"Status notification"));

			BNotification burnAbort(B_IMPORTANT_NOTIFICATION);
			burnAbort.SetGroup("BurnItNow");
			burnAbort.SetTitle(B_TRANSLATE_COMMENT("Burning aborted",
				"Notification title"));
			burnAbort.SetContent(B_TRANSLATE_COMMENT(
				"The data doesn't fit on the disc.", "Notification content"));
			burnAbort.SetMessageID(fNoteID);
			burnAbort.Send();
		} else {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning complete. Burn another disc?",
				"Status notification"));

			BNotification burnSuccess(B_INFORMATION_NOTIFICATION);
			burnSuccess.SetGroup("BurnItNow");
			burnSuccess.SetTitle(B_TRANSLATE_COMMENT("Burning clone disc",
				"Notification title"));
			burnSuccess.SetContent(B_TRANSLATE_COMMENT("Burning finished!",
				"Notification content"));
			burnSuccess.SetMessageID(fNoteID);
			burnSuccess.Send();
		}

		fBuildButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;
		fAbort = 0;
		fParser.Reset();
	}
}


void
CompilationCloneView::_GetImageInfo()
{
	fImageSize = 0;
	fOutputView->SetText(NULL);
	fBuildButton->SetEnabled(false);

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kGetImageInfoOutput), this));
	fBurnerThread->AddArgument("cdrecord")
		->AddArgument("-media-info")
		->AddArgument(device)
		->Run();
}


void
CompilationCloneView::_GetImageInfoOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		fParser.ParseMediainfoLine(fImageSize, data);
//		printf("Image size forecast: %" PRId64 " KiB, %" PRId64 " MiB\n",
//			fImageSize, fImageSize / 1024);

		if (fImageSize != 0)
			fSizeView->UpdateSizeDisplay(fImageSize, DATA, CD_OR_DVD);

		data << "\n";
		fOutputView->Insert(data.String());
		fOutputView->ScrollBy(0.0, 50.0);
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		BString text = fOutputView->Text();
		BStringList output;
		BStringList wordList;
		text.Split("==============================================", true, 
			output);
		output.StringAt(1).Split(" ", true, wordList);
		if (wordList.StringAt(3) == "Audio")
			fAudioMode = true;
		else
			fAudioMode = false;

		_Build();
	}
}


void
CompilationCloneView::_UpdateProgress(const char* title)
{
	BNotification burnProgress(B_PROGRESS_NOTIFICATION);
	burnProgress.SetGroup("BurnItNow");
	burnProgress.SetTitle(title);
	burnProgress.SetContent(fETAtime);
	burnProgress.SetProgress(fProgress);
	burnProgress.SetMessageID(fNoteID);
	burnProgress.Send();
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
