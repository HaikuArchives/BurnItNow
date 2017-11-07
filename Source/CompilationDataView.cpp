/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <stdio.h>
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
#include "CompilationDataView.h"
#include "CommandThread.h"
#include "CompilationShared.h"
#include "Constants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Compilation views"


CompilationDataView::CompilationDataView(BurnWindow& parent)
	:
	BView(B_TRANSLATE_COMMENT("Data disc", "Tab lable"), B_WILL_DRAW,
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
	fAction(IDLE),
	fRunner(NULL)
{
	fWindowParent = &parent;

//	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fInfoView->SetFont(be_bold_font);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Choose the folder to burn",
		"Status notification"));

	fPathView = new PathView("FolderStringView",
		B_TRANSLATE("Folder: <none>"));
	fPathView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fPathView->SetExplicitMinSize(BSize(
		be_plain_font->StringWidth("A really fairly long disc label goes here"),
		B_SIZE_UNSET));

	fDiscLabel = new BTextControl("disclabel", B_TRANSLATE("Disc label:"), "",
		NULL);
	fDiscLabel->TextView()->SetMaxBytes(32);
	fDiscLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fOutputView = new BTextView("DataInfoTextView");
	fOutputView->SetWordWrap(false);
	fOutputView->MakeEditable(false);
	BScrollView* fOutputScrollView = new BScrollView("DataInfoScrollView",
		fOutputView, B_WILL_DRAW, true, true);
	fOutputScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fChooseButton = new BButton("ChooseDirectoryButton",
		B_TRANSLATE_COMMENT("Choose folder", "Button label"),
		new BMessage(kChooseButton));
	fChooseButton->SetTarget(this);

	fBuildButton = new BButton("BuildImageButton", B_TRANSLATE_COMMENT(
		"Build image", "Button label"), new BMessage(kBuildButton));
	fBuildButton->SetTarget(this);

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE_COMMENT(
		"Burn disc", "Button label"), new BMessage(kBurnButton));
	fBurnButton->SetTarget(this);

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGrid(kControlPadding, 0, 0)
			.Add(fDiscLabel, 0, 0)
			.Add(fPathView, 0, 1)
			.Add(fChooseButton, 1, 0)
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


CompilationDataView::~CompilationDataView()
{
	delete fBurnerThread;
	delete fOpenPanel;
}


#pragma mark -- BView Overrides --


void
CompilationDataView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fChooseButton->SetTarget(this);
	fChooseButton->SetEnabled(true);

	fBuildButton->SetTarget(this);
	fBuildButton->SetEnabled(false);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationDataView::MessageReceived(BMessage* message)
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
CompilationDataView::InProgress()
{
	return fAction;
}


#pragma mark -- Private Methods --


void
CompilationDataView::_Build()
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
	buildProgress.SetTitle(B_TRANSLATE_COMMENT("Building data image",
		"Notification title"));
	buildProgress.SetContent(B_TRANSLATE_COMMENT(
		"Preparing the build" B_UTF8_ELLIPSIS, "Notification content"));
	buildProgress.SetProgress(0);

	char id[5];
	snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
	fNoteID = "BurnItNow_Data-";
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
	buildProgress.Send(10 * 1000000LL);

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

	discLabel.Truncate(32, false);	//mkisofs limits to 32char labels

	status_t ret = fImagePath->Append(kCacheFileData);
	if (ret == B_OK) {
		fBurnerThread->AddArgument("mkisofs")
			->AddArgument("-iso-level 3")
			->AddArgument("-J")
			->AddArgument("-joliet-long")
			->AddArgument("-rock")
			->AddArgument("-V")
			->AddArgument(discLabel)
			->AddArgument("-o")
			->AddArgument(fImagePath->Path())
			->AddArgument(fDirPath->Path())
			->Run();
	}
}


void
CompilationDataView::_BuildOutput(BMessage* message)
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
				_UpdateProgress(B_TRANSLATE_COMMENT("Building data image",
				"Notification title"));
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		fInfoView->SetLabel(B_TRANSLATE_COMMENT("Burn the disc",
			"Status notification"));
		fBuildButton->SetEnabled(false);
		fBurnButton->SetEnabled(true);

		BNotification buildSuccess(B_INFORMATION_NOTIFICATION);
		buildSuccess.SetGroup("BurnItNow");
		buildSuccess.SetTitle(B_TRANSLATE_COMMENT("Building data image",
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
		fAction = IDLE;
	}
}


void
CompilationDataView::_Burn()
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
		text.ReplaceFirst("%filename%", kCacheFileData);
		(new BAlert("ImageNotFound", text,
			B_TRANSLATE("OK")))->Go();
		return;
	}

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	fAction = BURNING;	// flag we're burning
	fChooseButton->SetEnabled(false);
	fBuildButton->SetEnabled(false);
	fBurnButton->SetEnabled(false);

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS,"Status notification"));

	BNotification burnProgress(B_PROGRESS_NOTIFICATION);
	burnProgress.SetGroup("BurnItNow");
	burnProgress.SetTitle(B_TRANSLATE_COMMENT("Burning data disc",
		"Notification title"));
	burnProgress.SetProgress(0);

	char id[5];
	snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
	fNoteID = "BurnItNow_Audio-";
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
CompilationDataView::_BurnOutput(BMessage* message)
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
				_UpdateProgress(B_TRANSLATE_COMMENT("Burning data disc",
				"Notification title"));
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK){
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
			burnSuccess.SetTitle(B_TRANSLATE_COMMENT("Burning data disc",
				"Notification title"));
			burnSuccess.SetContent(B_TRANSLATE_COMMENT("Burning finished!",
				"Notification content"));
			burnSuccess.SetMessageID(fNoteID);
			burnSuccess.Send();
		}

		fChooseButton->SetEnabled(true);
		fBuildButton->SetEnabled(false);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;
		fAbort = 0;
		fParser.Reset();
	}
}


void
CompilationDataView::_ChooseDirectory()
{
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, NULL, new DirRefFilter(), true);
		fOpenPanel->Window()->SetTitle(B_TRANSLATE_COMMENT("Choose data folder",
			"File panel title"));
	}
	fOpenPanel->Show();
}


void
CompilationDataView::_GetFolderSize()
{
	BMessage* msg = new BMessage('NULL');
	msg->AddString("path", fDirPath->Path());
	msg->AddMessenger("from", this);

	thread_id sizecount = spawn_thread(FolderSizeCount,
		"Folder size counter", B_LOW_PRIORITY, msg);

	if (sizecount >= B_OK)
		resume_thread(sizecount);

	fSizeView->ShowInfoText(B_TRANSLATE_COMMENT("calculating" B_UTF8_ELLIPSIS,
		"In size view, as short as possible!"));
}


void
CompilationDataView::_OpenDirectory(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK)
		return;

	BEntry entry(&ref, true);	// also accept symlinks
	BNode node(&entry);
	if ((node.InitCheck() != B_OK) || !node.IsDirectory())
		return;

	fDirPath->SetTo(&entry);
	fPathView->SetText(fDirPath->Path());

	if (fDiscLabel->TextView()->TextLength() == 0) {
		BString discLabel(fDirPath->Leaf());
		discLabel.Truncate(32, false);	//mkisofs limits to 32char labels
		fDiscLabel->SetText(discLabel);
		fDiscLabel->MakeFocus(true);
	}

	fFolderSize = 0;
	fOutputView->SetText(NULL);

	fBuildButton->SetEnabled(true);
	fBurnButton->SetEnabled(false);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Build the image",
		"Status notification"));

	_GetFolderSize();
}


void
CompilationDataView::_UpdateProgress(const char* title)
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
CompilationDataView::_UpdateSizeBar()
{
	fSizeView->UpdateSizeDisplay(fFolderSize, DATA, CD_OR_DVD); // size in KiB
}
