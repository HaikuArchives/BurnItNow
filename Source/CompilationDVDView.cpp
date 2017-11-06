/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
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
#include <StringView.h>

#include "BurnApplication.h"
#include "CompilationDVDView.h"
#include "CommandThread.h"
#include "CompilationShared.h"
#include "Constants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Compilation views"


CompilationDVDView::CompilationDVDView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Audio/Video DVD"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBurnerThread(NULL),
	fOpenPanel(NULL),
	fDirPath(new BPath()),
	fImagePath(new BPath()),
	fFolderSize(0),
	fNoteID(""),
	fID(0),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime),
	fAbort(0),
	fAction(IDLE)
{
	fWindowParent = &parent;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fInfoView->SetFont(be_bold_font);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Choose the DVD folder to burn", "Status notification"));

	fPathView = new PathView("FolderStringView",
		B_TRANSLATE("Folder: <none>"));
	fPathView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fDiscLabel = new BTextControl("disclabel", B_TRANSLATE("Disc label:"), "",
		NULL);
	fDiscLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fOutputView = new BTextView("OutputView");
	fOutputView->SetWordWrap(false);
	fOutputView->MakeEditable(false);
	BScrollView* fOutputScrollView = new BScrollView("OutputScroller",
		fOutputView, B_WILL_DRAW, true, true);
	fOutputScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fDVDButton = new BButton("ChooseDVDButton", B_TRANSLATE_COMMENT(
		"Choose DVD folder", "Button label"), new BMessage(kChooseButton));
	fDVDButton->SetTarget(this);
	fDVDButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));
		
	fBuildButton = new BButton("BuildImageButton", B_TRANSLATE_COMMENT(
		"Build image", "Button label"), new BMessage(kBuildButton));
	fBuildButton->SetTarget(this);
	fBuildButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE_COMMENT(
		"Burn disc", "Button label"), new BMessage(kBurnButton));
	fBurnButton->SetTarget(this);
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGrid(kControlPadding, 0, 0)
			.Add(fDiscLabel, 0, 0)
			.Add(fPathView, 0, 1)
			.Add(fDVDButton, 1, 0)
			.Add(fBuildButton, 2, 0)
			.Add(fBurnButton, 3, 0)
			.SetColumnWeight(0, 10.f)
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fInfoView)
			.Add(fOutputScrollView)
			.End()
		.Add(fSizeView);

	_UpdateSizeBar();
}


CompilationDVDView::~CompilationDVDView()
{
	delete fBurnerThread;
	delete fOpenPanel;
}


#pragma mark -- BView Overrides --


void
CompilationDVDView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fDVDButton->SetTarget(this);
	fDVDButton->SetEnabled(true);
	
	fBuildButton->SetTarget(this);
	fBuildButton->SetEnabled(false);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationDVDView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChooseButton:
			_ChooseDirectory();
			break;
		case kBuildButton:
			_Build();
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
		case B_REFS_RECEIVED:
			_OpenDirectory(message);
			break;
		case kSetFolderSize:
		{
			message->FindInt64("foldersize", &fFolderSize);
			_UpdateSizeBar();
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Public Methods --


int32
CompilationDVDView::InProgress()
{
	return fAction;
}


#pragma mark -- Private Methods --


void
CompilationDVDView::_Build()
{
	if (fDirPath->InitCheck() != B_OK)
		return;

	BFile testFile;
	entry_ref testRef;
	get_ref_for_path(fDirPath->Path(), &testRef);

	testFile.SetTo(&testRef, B_READ_ONLY);
	status_t result = testFile.InitCheck();

	if (result != B_OK) {
		BString text(B_TRANSLATE_COMMENT(
			"The chosen folder '%foldername%' seems to have disappeared. "
			"Was it perhaps moved or renamed?", "Alert text"));
		text.ReplaceFirst("%foldername%", fDirPath->Path());
		(new BAlert("FolderNotFound", text,
			B_TRANSLATE("OK")))->Go();
		return;
	}

	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(*fImagePath);
		settings->Unlock();
	}
	if (fImagePath->InitCheck() != B_OK)
		return;

	BNotification buildProgress(B_PROGRESS_NOTIFICATION);
	buildProgress.SetGroup("BurnItNow");
	buildProgress.SetTitle(B_TRANSLATE_COMMENT("Building DVD image",
		"Notification title"));
	buildProgress.SetContent(B_TRANSLATE_COMMENT(
		"Preparing the build" B_UTF8_ELLIPSIS, "Notification content"));
	buildProgress.SetProgress(0);

	char id[5];
	snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
	fNoteID = "BurnItNow_DVD-";
	fNoteID.Append(id);

	buildProgress.SetMessageID(fNoteID);
	buildProgress.Send();

	fAction = BUILDING;	// flag we're building ISO

	// still getting folder size?
	if (fFolderSize == 0) {
		BMessage message(kBuildButton);
		fRunner	= new BMessageRunner(this, &message, 1000000, 1); // 1 Hz
		return;
	}

	if (!CheckFreeSpace(fFolderSize * 1024, fImagePath->Path())) {
		fAction = IDLE;
		return;
	}

	 // It may take a while for the building to start...
	buildProgress.Send(60 * 1000000LL);

	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Building in progress" B_UTF8_ELLIPSIS, "Status notification"));

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBuildOutput), this));

	BString discLabel;
	if (fDiscLabel->TextView()->TextLength() == 0)
		discLabel = fDirPath->Leaf();
	else
		discLabel = fDiscLabel->Text();

	status_t ret = fImagePath->Append(kCacheFileDVD);
	if (ret == B_OK) {
		fBurnerThread->AddArgument("mkisofs")
			->AddArgument("-V")
			->AddArgument(discLabel)
			->AddArgument(fDVDMode)
			->AddArgument("-o")
			->AddArgument(fImagePath->Path())
			->AddArgument(fDirPath->Path())
			->Run();
	}
}


void
CompilationDVDView::_BuildOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseMkisofsLine(text, data);
		if (modified == NOCHANGE) {
			data << "\n";
			fOutputView->Insert(data.String());
			fOutputView->ScrollBy(0.0, 50.0);
		} else {
			if (modified == PERCENT)
				_UpdateProgress(B_TRANSLATE_COMMENT("Building DVD image",
				"Notification title"));
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		BString infoText(fOutputView->Text());
		// mkisofs has same errors for dvd-video and dvd-hybrid, but
		// no error checking for dvd-audio, apparently
		if (infoText.FindFirst(
			"mkisofs: Unable to make a DVD-Video image.\n") != B_ERROR) {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Unable to create a DVD image",
				"Status notification"));
			fBurnButton->SetEnabled(false);

			BNotification buildAbort(B_IMPORTANT_NOTIFICATION);
			buildAbort.SetGroup("BurnItNow");
			buildAbort.SetTitle(B_TRANSLATE_COMMENT("Building aborted",
				"Notification title"));
			buildAbort.SetContent(B_TRANSLATE_COMMENT(
				"Unable to create DVD image", "Notification content"));
			buildAbort.SetMessageID(fNoteID);
			buildAbort.Send();

		} else {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT("Burn the disc",
				"Status notification"));
			fBuildButton->SetEnabled(false);
			fBurnButton->SetEnabled(true);

			BNotification buildSuccess(B_INFORMATION_NOTIFICATION);
			buildSuccess.SetGroup("BurnItNow");
			buildSuccess.SetTitle(B_TRANSLATE_COMMENT("Building DVD image",
				"Notification title"));
			buildSuccess.SetContent(B_TRANSLATE_COMMENT("Building finished!",
				"Notification content"));
			buildSuccess.SetMessageID(fNoteID);
			buildSuccess.Send();

			BEntry entry(fImagePath->Path());
			if (entry.InitCheck() == B_OK) {
				off_t fileSize = 0;
				entry.GetSize(&fileSize);
				fFolderSize = fileSize / 1024;
				_UpdateSizeBar();
			}
		}
		fAction = IDLE;
	}
}


void
CompilationDVDView::_Burn()
{
	BFile testFile;
	entry_ref testRef;
	get_ref_for_path(fImagePath->Path(), &testRef);

	testFile.SetTo(&testRef, B_READ_ONLY);
	status_t result = testFile.InitCheck();

	if (result != B_OK) {
		BString text(B_TRANSLATE_COMMENT(
			"There isn't an image '%filename%' in the cache folder. "
			"Was it perhaps moved or renamed?", "Alert text"));
		text.ReplaceFirst("%filename%", kCacheFileDVD);
		(new BAlert("ImageNotFound", text,
			B_TRANSLATE("OK")))->Go();
		return;
	}

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	fAction = BURNING;	// flag we're burning

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS,"Status notification"));
	fDVDButton->SetEnabled(false);
	fBuildButton->SetEnabled(false);
	fBurnButton->SetEnabled(false);

	BNotification burnProgress(B_PROGRESS_NOTIFICATION);
	burnProgress.SetGroup("BurnItNow");
	burnProgress.SetTitle(B_TRANSLATE_COMMENT("Burning DVD",
		"Notification title"));
	burnProgress.SetProgress(0);

	char id[5];
	snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
	fNoteID = "BurnItNow_DVD-";
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

	fBurnerThread->AddArgument(config.mode)
		->AddArgument("fs=16m")
		->AddArgument(device)
		->AddArgument("-v")	// to get progress output
		->AddArgument("-gracetime=2")
		->AddArgument("-pad")
		->AddArgument("padsize=63s")
		->AddArgument(fImagePath->Path())
		->Run();

	fParser.Reset();
}


void
CompilationDVDView::_BurnOutput(BMessage* message)
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
				_UpdateProgress(B_TRANSLATE_COMMENT("Burning DVD",
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
			burnSuccess.SetTitle(B_TRANSLATE_COMMENT("Burning DVD",
				"Notification title"));
			burnSuccess.SetContent(B_TRANSLATE_COMMENT("Burning finished!",
				"Notification content"));
			burnSuccess.SetMessageID(fNoteID);
			burnSuccess.Send();
		}

		fDVDButton->SetEnabled(true);
		fBuildButton->SetEnabled(false);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;
		fAbort = 0;
		fParser.Reset();
	}
}


void
CompilationDVDView::_ChooseDirectory()
{
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, NULL, new DirRefFilter(), true);
		fOpenPanel->Window()->SetTitle(B_TRANSLATE_COMMENT("Choose DVD folder",
			"File panel title"));
	}
	fOpenPanel->Show();
}


void
CompilationDVDView::_GetFolderSize()
{
	BMessage* msg = new BMessage('NULL');
	msg->AddString("path", fDirPath->Path());
	msg->AddMessenger("from", this);

	thread_id sizecount = spawn_thread(FolderSizeCount,
		"Folder size counter", B_LOW_PRIORITY, msg);

	if (sizecount >= B_OK)
		resume_thread(sizecount);

	fSizeView->ShowInfoText(B_TRANSLATE_COMMENT("calculating" B_UTF8_ELLIPSIS,
		"In size view, as short as possible!"));}


void
CompilationDVDView::_OpenDirectory(BMessage* message)
{
	BString status(B_TRANSLATE_COMMENT(
		"Didn't find valid files needed for an Audio or Video DVD",
		"Status notification"));

	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK) {
		fInfoView->SetLabel(status);
		return;
	}

	BEntry entry(&ref, true);	// also accept symlinks
	BNode node(&entry);
	if ((node.InitCheck() != B_OK) || !node.IsDirectory()) {
		fInfoView->SetLabel(status);
		return;
	}

	// get parent folder if user chose subfolder
	fDirPath->SetTo(&entry);
	const char* name(fDirPath->Leaf());
	if ((strcmp("VIDEO_TS", name) == 0) || (strcmp("AUDIO_TS", name) == 0))
		fDirPath->GetParent(fDirPath);

	// make sure there's a VIDEO_TS and AUDIO_TS folder
	BDirectory folder(fDirPath->Path());

	if ((folder.Contains("VIDEO_TS", B_DIRECTORY_NODE))
			|| (folder.Contains("AUDIO_TS", B_DIRECTORY_NODE))) {
		folder.CreateDirectory("VIDEO_TS", NULL);
		folder.CreateDirectory("AUDIO_TS", NULL);
	}

	// check for Video/Audio/Hybrid DVD
	BPath path(fDirPath->Path());
	path.Append("VIDEO_TS");
	folder.SetTo(path.Path());
	bool hasVTS = folder.Contains("VIDEO_TS.IFO", B_FILE_NODE);

	path.GetParent(&path);
	path.Append("AUDIO_TS");
	folder.SetTo(path.Path());
	bool hasATS = folder.Contains("AUDIO_TS.IFO", B_FILE_NODE);

	if (hasATS) {
		if (hasVTS)
			fDVDMode = "-dvd-hybrid";
		else
			fDVDMode = "-dvd-audio";
	} else if (hasVTS)
		fDVDMode = "-dvd-video";
	else {
		fInfoView->SetLabel(status);
		return;
	}

	fFolderSize = 0;
	fOutputView->SetText(NULL);

	fPathView->SetText(fDirPath->Path());

	if (fDiscLabel->TextView()->TextLength() == 0) {
		fDiscLabel->SetText(fDirPath->Leaf());
		fDiscLabel->MakeFocus(true);
	}

	fBuildButton->SetEnabled(true);
	fBurnButton->SetEnabled(false);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Build the DVD image",
		"Status notification"));

	_GetFolderSize();
}



void
CompilationDVDView::_UpdateProgress(const char* title)
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
CompilationDVDView::_UpdateSizeBar()
{
	fSizeView->UpdateSizeDisplay(fFolderSize, DATA, DVD_ONLY); // size in KiB
}
