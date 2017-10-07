/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "CompilationDVDView.h"
#include "CommandThread.h"
#include "Constants.h"
#include "DirRefFilter.h"
#include "FolderSizeCount.h"

#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DVD view"


CompilationDVDView::CompilationDVDView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Audio/Video DVD"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fBurnerThread(NULL),
	fDirPath(new BPath()),
	fImagePath(new BPath())
{
	windowParent = &parent;
	step = NONE;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBurnerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fBurnerInfoBox->SetFont(be_bold_font);
	fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
		"Choose DVD folder to burn", "Status notification"));

	fPathView = new PathView("FolderStringView",
		B_TRANSLATE("Folder: <none>"));
	fPathView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fDiscLabel = new BTextControl("disclabel", B_TRANSLATE("Disc label:"), "",
		NULL);
	fDiscLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnerInfoTextView = new BTextView("DVDInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("DVDInfoScrollView",
		fBurnerInfoTextView, B_WILL_DRAW, true, true);
	infoScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fDVDButton = new BButton("ChooseDVDButton",
		B_TRANSLATE("Choose DVD folder"),
		new BMessage(kChooseMessage));
	fDVDButton->SetTarget(this);
	fDVDButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));
		
	fImageButton = new BButton("BuildImageButton", B_TRANSLATE("Build image"),
	    new BMessage(kBuildImageMessage));
	fImageButton->SetTarget(this);
	fImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE("Burn disc"),
		new BMessage(kBurnDiscMessage));
	fBurnButton->SetTarget(this);
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGrid(kControlPadding, 0, 0)
			.Add(fDiscLabel, 0, 0)
			.Add(fPathView, 0, 1)
			.Add(fDVDButton, 1, 0)
			.Add(fImageButton, 2, 0)
			.Add(fBurnButton, 3, 0)
			.SetColumnWeight(0, 10.f)
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fBurnerInfoBox)
			.Add(infoScrollView)
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
	
	fImageButton->SetTarget(this);
	fImageButton->SetEnabled(false);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationDVDView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChooseMessage:
			_ChooseDirectory();
			break;
		case kBurnDiscMessage:
			BurnDisc();
			break;
		case kBuildImageMessage:
			BuildISO();
			break;
		case B_REFS_RECEIVED:
		{
			_OpenDirectory(message);
			_GetFolderSize();
			break;
		}
		case kSetFolderSize:
		{
			message->FindInt64("foldersize", &fFolderSize);
			_UpdateSizeBar();
		}
		case kBurnerMessage:
			_BurnerOutput(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationDVDView::_ChooseDirectory()
{
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, NULL, new DirRefFilter(), true);
		fOpenPanel->Window()->SetTitle(B_TRANSLATE("Choose DVD folder"));
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

	fSizeView->ShowInfoText("calculating" B_UTF8_ELLIPSIS);
}


void
CompilationDVDView::_OpenDirectory(BMessage* message)
{
	BString status(B_TRANSLATE_COMMENT(
			"Didn't find valid files needed for a Audio or Video DVD",
			"Status notification"));

	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK) {
		fBurnerInfoBox->SetLabel(status);
		return;
	}

	BEntry entry(&ref, true);	// also accept symlinks
	BNode node(&entry);
	if ((node.InitCheck() != B_OK) || !node.IsDirectory()) {
		fBurnerInfoBox->SetLabel(status);
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
		fBurnerInfoBox->SetLabel(status);
		return;
	}

	fPathView->SetText(fDirPath->Path());

	if (fDiscLabel->TextView()->TextLength() == 0) {
		fDiscLabel->SetText(fDirPath->Leaf());
		fDiscLabel->MakeFocus(true);
	}

	fImageButton->SetEnabled(true);
	fBurnButton->SetEnabled(false);
	fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT("Build the DVD image",
		"Status notification"));
}


void
CompilationDVDView::_BurnerOutput(BMessage* message)
{
	BString DVD;

	if (message->FindString("line", &DVD) == B_OK) {
		DVD << "\n";
		fBurnerInfoTextView->Insert(DVD.String());
		fBurnerInfoTextView->ScrollBy(0.0, 50.0);
	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (step == BUILDING)) {
		BString infoText(fBurnerInfoTextView->Text());
		// mkisofs has same errors for dvd-video and dvd-hybrid, but
		// no error checking for dvd-audio, apparently
		if (infoText.FindFirst(
			"mkisofs: Unable to make a DVD-Video image.\n") != B_ERROR) {
			fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
				"Unable to create a DVD image",
				"Status notification"));
			fBurnButton->SetEnabled(false);
		} else {
			fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT("Burn the disc",
				"Status notification"));
			fImageButton->SetEnabled(false);
			fBurnButton->SetEnabled(true);
		}
		step = NONE;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (step == BURNING)) {
		fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
			"Burning complete. Burn another disc?", "Status notification"));
		fDVDButton->SetEnabled(true);
		fImageButton->SetEnabled(false);
		fBurnButton->SetEnabled(true);

		step = NONE;
	}
}


void
CompilationDVDView::_UpdateSizeBar()
{
	fSizeView->UpdateSizeDisplay(fFolderSize, DATA, DVD_ONLY); // size in KiB
}


#pragma mark -- Public Methods --


void
CompilationDVDView::BuildISO()
{
	if (fDirPath->Path() == NULL) {
		(new BAlert("ChooseDirectoryFirstAlert",
			B_TRANSLATE("First choose DVD folder to burn."),
			B_TRANSLATE("OK")))->Go();
		return;
	}
	if (fDirPath->InitCheck() != B_OK)
		return;

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
		"Building in progress" B_UTF8_ELLIPSIS, "Status notification"));
	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnerMessage), this));

	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(*fImagePath);
		settings->Unlock();
	}
	if (fImagePath->InitCheck() != B_OK)
		return;

	BString discLabel;
	if (fDiscLabel->TextView()->TextLength() == 0)
		discLabel = fDirPath->Leaf();
	else
		discLabel = fDiscLabel->Text();

	status_t ret = fImagePath->Append(kCacheFileDVD);
	if (ret == B_OK) {
		step = BUILDING;	// flag we're building ISO

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
CompilationDVDView::BurnDisc()
{
	if (fImagePath->Path() == NULL) {
		(new BAlert("ChooseDirectoryFirstAlert", B_TRANSLATE(
			"First build an image to burn."), B_TRANSLATE("OK")))->Go();
		return;
	}
	if (fImagePath->InitCheck() != B_OK)
		return;

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	step = BURNING;	// flag we're burning

	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS,"Status notification"));
	fDVDButton->SetEnabled(false);
	fImageButton->SetEnabled(false);
	fBurnButton->SetEnabled(false);

	BString device("dev=");
	device.Append(windowParent->GetSelectedDevice().number.String());
	sessionConfig config = windowParent->GetSessionConfig();

	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnerMessage), this));
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
		->AddArgument("-pad")
		->AddArgument("padsize=63s")
		->AddArgument(fImagePath->Path())
		->Run();
}


int32
CompilationDVDView::InProgress()
{
	return step;
}
