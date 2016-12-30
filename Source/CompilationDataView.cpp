/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationDataView.h"
#include "CommandThread.h"

#include <Alert.h>
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
const int32 kNoPathMessage = 'Noph'; // defined in PathView


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

	fPathView = new PathView("FolderStringView", "Folder: <none>");

	fBurnerInfoTextView = new BTextView("DataInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("DataInfoScrollView",
		fBurnerInfoTextView, B_WILL_DRAW, true, true);
	infoScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fChooseButton = new BButton("ChooseDirectoryButton", "Choose folder",
		new BMessage(kChooseDirectoryMessage));
	fChooseButton->SetTarget(this);
	fChooseButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));

	fImageButton = new BButton("BuildImageButton", "Build image",
		new BMessage(kBuildImageMessage));
	fImageButton->SetTarget(this);
	fImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnButton = new BButton("BurnImageButton", "Burn disc",
		new BMessage(kBurnDiscMessage));
	fBurnButton->SetTarget(this);
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(fPathView)
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.Add(fChooseButton)
				.Add(fImageButton)
				.Add(fBurnButton)
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

	fChooseButton->SetTarget(this);
	fChooseButton->SetEnabled(true);

	fImageButton->SetTarget(this);
	fImageButton->SetEnabled(false);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationDataView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kNoPathMessage:
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


bool
DataRefFilter::Filter(const entry_ref* ref, BNode* node,
	struct stat_beos* stat, const char* filetype)
{
	if (node->IsDirectory())
		return true;

	return false;
}


void
CompilationDataView::_ChooseDirectory()
{
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, NULL, new DataRefFilter(), true);
	}
	fOpenPanel->Show();
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
	fImageButton->SetEnabled(true);
	fBurnButton->SetEnabled(false);
	fBurnerInfoBox->SetLabel("Build the image");
}


void
CompilationDataView::_BurnerOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fBurnerInfoTextView->Insert(data.String());
		fBurnerInfoTextView->ScrollTo(0.0, 2000.0);

	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 1)) {
		fBurnerInfoBox->SetLabel("Burn the disc");
		fImageButton->SetEnabled(false);
		fBurnButton->SetEnabled(true);

		step = 0;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 2)) {
		fBurnerInfoBox->SetLabel("Burning complete. Burn another disc?");
		fChooseButton->SetEnabled(true);
		fImageButton->SetEnabled(false);
		fBurnButton->SetEnabled(true);

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

	status_t ret = fImagePath->Append("burnitnow_data.iso");
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
	fChooseButton->SetEnabled(false);
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
