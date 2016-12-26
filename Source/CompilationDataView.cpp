/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationDataView.h"

#include "CommandThread.h"
#include <Alert.h>
#include <Button.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Message constants
const int32 kChooseDirectoryMessage = 'Cusd';
const int32 kBurnerMessage = 'Brnr';
const int32 kBuildImageMessage = 'IMAG';
const int32 kBurnDiscMessage = 'BURN';


CompilationDataView::CompilationDataView(BurnWindow& parent)
	:
	BView("Data", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fBurnerThread(NULL),
	fDirPath(new BPath()),
	fImagePath(new BPath())
{
	windowParent = &parent;
	step = 0;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBurnerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fBurnerInfoBox->SetFont(be_bold_font);
	fBurnerInfoBox->SetLabel("Choose the folder to burn");

	fBurnerInfoTextView = new BTextView("DataInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("DataInfoScrollView",
		fBurnerInfoTextView, 0, true, true);

	BButton* chooseDirectoryButton = new BButton("ChooseDirectoryButton",
		"Choose folder", new BMessage(kChooseDirectoryMessage));
	chooseDirectoryButton->SetTarget(this);
	chooseDirectoryButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));

	BButton* buildImageButton = new BButton("BuildImageButton",
		"Build image", new BMessage(kBuildImageMessage));
	buildImageButton->SetTarget(this);
	buildImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BButton* burnImageButton = new BButton("BurnImageButton",
		"Burn disc", new BMessage(kBurnDiscMessage));
	burnImageButton->SetTarget(this);
	burnImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(new BStringView("FolderStringView", "Folder: <none>"))
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.Add(chooseDirectoryButton)
				.Add(buildImageButton)
				.Add(burnImageButton)
				.End()
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fBurnerInfoBox)
			.Add(infoScrollView)
			.End();
}


CompilationDataView::~CompilationDataView()
{
	delete fBurnerThread;
}


#pragma mark -- BView Overrides --


void
CompilationDataView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* chooseDirectoryButton
		= dynamic_cast<BButton*>(FindView("ChooseDirectoryButton"));
	if (chooseDirectoryButton != NULL) {
		chooseDirectoryButton->SetTarget(this);
		chooseDirectoryButton->SetEnabled(true);
	}

	BButton* buildImageButton
		= dynamic_cast<BButton*>(FindView("BuildImageButton"));
	if (buildImageButton != NULL) {
		buildImageButton->SetTarget(this);
		buildImageButton->SetEnabled(false);
	}
	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL) {
		burnImageButton->SetTarget(this);
		burnImageButton->SetEnabled(false);
	}
}


void
CompilationDataView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChooseDirectoryMessage:
			_ChooseDirectory();
			break;
		case kBurnDiscMessage:
			BurnDisc();
			break;
		case kBuildImageMessage:
			BuildISO();
			break;
		case B_REFS_RECEIVED:
			_OpenDirectory(message);
			break;
		case kBurnerMessage:
			_BurnerOutput(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationDataView::_ChooseDirectory()
{
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, NULL, NULL, true);
	}
	fOpenPanel->Show();
}


void
CompilationDataView::_OpenDirectory(BMessage* message)
{
	entry_ref dirRef;

	if (message->FindRef("refs", &dirRef) != B_OK)
		return;

	BStringView* folderStringView
		= dynamic_cast<BStringView*>(FindView("FolderStringView"));
	if (folderStringView == NULL)
		return;

	fDirPath->SetTo(&dirRef);

	folderStringView->SetText(fDirPath->Path());

	BButton* buildImageButton
		= dynamic_cast<BButton*>(FindView("BuildImageButton"));
	if (buildImageButton != NULL)
		buildImageButton->SetEnabled(true);

	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetEnabled(false);

	fBurnerInfoBox->SetLabel("Build the image");
}


void
CompilationDataView::_BurnerOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fBurnerInfoTextView->Insert(data.String());
	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 1)) {
		fBurnerInfoBox->SetLabel("Burn the disc");
		BButton* buildImageButton
			= dynamic_cast<BButton*>(FindView("BuildImageButton"));
		if (buildImageButton != NULL)
			buildImageButton->SetEnabled(false);

		BButton* burnImageButton
			= dynamic_cast<BButton*>(FindView("BurnImageButton"));
		if (burnImageButton != NULL)
			burnImageButton->SetEnabled(true);

		step = 0;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 2)) {
		fBurnerInfoBox->SetLabel("Burning complete. Burn another disc?");
		BButton* chooseDirectoryButton
			= dynamic_cast<BButton*>(FindView("ChooseDirectoryButton"));
		if (chooseDirectoryButton != NULL)
			chooseDirectoryButton->SetEnabled(true);

		BButton* buildImageButton
			= dynamic_cast<BButton*>(FindView("BuildImageButton"));
		if (buildImageButton != NULL)
			buildImageButton->SetEnabled(false);

		BButton* burnImageButton
			= dynamic_cast<BButton*>(FindView("BurnImageButton"));
		if (burnImageButton != NULL)
			burnImageButton->SetEnabled(true);

		step = 0;
	}
}


#pragma mark -- Public Methods --


void
CompilationDataView::BuildISO()
{
	if (fDirPath->Path() == NULL) {
		(new BAlert("ChooseDirectoryFirstAlert",
			"First choose the folder to burn.", "OK"))->Go();
		return;
	}

	if (fBurnerThread != NULL)
		delete fBurnerThread;
		
	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel("Building in progress" B_UTF8_ELLIPSIS);
	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnerMessage), this));

	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, fImagePath) != B_OK)
		return;

	status_t ret = fImagePath->Append("burnitnow_iso.iso");
	if (ret == B_OK) {
		step = 1;	// flag we're building ISO

		fBurnerThread->AddArgument("mkisofs")
		->AddArgument("-iso-level 3")
		->AddArgument("-J")
		->AddArgument("-joliet-long")
		->AddArgument("-rock")
		->AddArgument("-V")
		->AddArgument(fDirPath->Leaf())
		->AddArgument("-o")
		->AddArgument(fImagePath->Path())
		->AddArgument(fDirPath->Path())
		->Run();
	}
}


void
CompilationDataView::BurnDisc()
{
	if (fImagePath->Path() == NULL) {
		(new BAlert("ChooseDirectoryFirstAlert",
			"First build an image to burn.", "OK"))->Go();
		return;
	}

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	step = 2;	// flag we're burning

	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel("Burning in progress" B_UTF8_ELLIPSIS);
	BButton* chooseDirectoryButton
		= dynamic_cast<BButton*>(FindView("ChooseDirectoryButton"));
	if (chooseDirectoryButton != NULL)
		chooseDirectoryButton->SetEnabled(false);

	BButton* buildImageButton
		= dynamic_cast<BButton*>(FindView("BuildImageButton"));
	if (buildImageButton != NULL)
		buildImageButton->SetEnabled(false);

	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetEnabled(false);

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
