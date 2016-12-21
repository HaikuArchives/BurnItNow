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
const int32 kFromScratchMessage = 'Scrh';
const int32 kBurnerMessage = 'Brnr';
const int32 kBuildImageMessage = 'IMAG';
const int32 kBurnDiscMessage = 'BURN';

// Misc variable(s)
int mode = 0;

CompilationDataView::CompilationDataView(BurnWindow &parent)
	:
	BView("Data", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fBurnerThread(NULL),
	fDirPath(new BPath()),
	fImagePath(new BPath())
{
	windowParent = &parent;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBurnerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fBurnerInfoBox->SetFont(be_bold_font);
	fBurnerInfoBox->SetLabel("Ready");

	fBurnerInfoTextView = new BTextView("DataInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("DataInfoScrollView",
		fBurnerInfoTextView, 0, true, true);

	BButton* chooseDirectoryButton = new BButton("ChooseDirectoryButton",
		"Choose folder to burn", new BMessage(kChooseDirectoryMessage));
	chooseDirectoryButton->SetTarget(this);
	chooseDirectoryButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BButton* fromScratchButton = new BButton("FromScratchButton",
		"Prepare compilation", new BMessage(kFromScratchMessage));
	fromScratchButton->SetTarget(this);
	fromScratchButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

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
		.AddGrid(kControlPadding * 2, kControlPadding / 2)
			.Add(new BStringView("", "Step 1:"), 0, 0)
			.Add(chooseDirectoryButton, 1, 0)
			.Add(new BStringView("", "or"), 2, 0)
			.Add(fromScratchButton, 3, 0)
			.Add(new BStringView("", "Step 2:"), 0, 1)
			.Add(buildImageButton, 1, 1)
			.Add(new BStringView("", "or"), 2, 1)
			.Add(burnImageButton, 3, 1)
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


void CompilationDataView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* chooseDirectoryButton = dynamic_cast<BButton*>(FindView("ChooseDirectoryButton"));
	if (chooseDirectoryButton != NULL)
		chooseDirectoryButton->SetTarget(this);
	
	BButton* fromScratchButton = dynamic_cast<BButton*>(FindView("FromScratchButton"));
	if (fromScratchButton != NULL)
		fromScratchButton->SetTarget(this);

	BButton* buildImageButton = dynamic_cast<BButton*>(FindView("BuildImageButton"));
	if (buildImageButton != NULL)
		buildImageButton->SetTarget(this);

	BButton* burnImageButton = dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetTarget(this);
}


void CompilationDataView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChooseDirectoryMessage:
			_ChooseDirectory();
			break;
		case kFromScratchMessage:
			_FromScratch();
			break;
		case kBurnDiscMessage:
			BurnDisc();
			break;
		case kBuildImageMessage:
			BuildISO();
			break;
		case B_REFS_RECEIVED:
			_OpenDirectory(message);
		case kBurnerMessage:
			_BurnerOutput(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void CompilationDataView::_ChooseDirectory()
{
	if (fOpenPanel == NULL)
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, NULL, NULL, true);

	fOpenPanel->Show();
}

void CompilationDataView::_FromScratch()
{
	// Create cache directory
	BPath path;
	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, &path) != B_OK)
		return;

	status_t ret = path.Append("burnitnow_cache");
	if (ret == B_OK) {
		BDirectory* dir = new BDirectory();
		dir->CreateDirectory(path.Path(), NULL);
		fDirPath->SetTo(path.Path());

		(new BAlert("DirectoryOpenedAlert",
			"Prepare the compilation in the opened folder. Then close it and "
			"build the ISO or burn the disc.", "OK"))->Go();

		// Display cache directory
		CommandThread* command = new CommandThread(NULL, new BInvoker(new BMessage(), this));
		command->AddArgument("open")->AddArgument(path.Path())->Run();
	}
}

void CompilationDataView::_OpenDirectory(BMessage* message)
{
	entry_ref dirRef;

	if (message->FindRef("refs", &dirRef) != B_OK)
		return;

	fDirPath->SetTo(&dirRef);
	
	(new BAlert("DirectoryOpenedAlert",
		"Now you can build the image or burn the disc.", "OK"))->Go();
}


void CompilationDataView::_BurnerOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fBurnerInfoTextView->Insert(data.String());
	
	if (!fBurnerThread->IsRunning() && mode == 1)
	{
		BPath path;
		if (find_directory(B_SYSTEM_CACHE_DIRECTORY, &path) != B_OK)
			return;
		fBurnerInfoBox->SetLabel("Ready");
		CommandThread* command = new CommandThread(NULL, new BInvoker(new BMessage(), this));
		command->AddArgument("open")->AddArgument(path.Path())->Run();
		mode = 0;
	}
	else if (!fBurnerThread->IsRunning() && mode == 2)
		fBurnerInfoBox->SetLabel("Ready");
}

#pragma mark -- Public Methods --

void CompilationDataView::BuildISO()
{
	mode = 1;
	
	if (fDirPath->Path() == NULL)
	{
		(new BAlert("ChooseDirectoryFirstAlert",
			"First choose the folder to burn.", "OK"))->Go();
		return;
	}

	if (fBurnerThread != NULL)
		delete fBurnerThread;
		
	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel("Building in progress" B_UTF8_ELLIPSIS);
	
	fBurnerThread = new CommandThread(NULL, new BInvoker(new BMessage(kBurnerMessage), this));

	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, fImagePath) != B_OK)
		return;

	status_t ret = fImagePath->Append("burnitnow_iso.iso");
	if (ret == B_OK) {
		fBurnerThread->AddArgument("mkisofs")
		->AddArgument("-o")
		->AddArgument(fImagePath->Path())
		->AddArgument(fDirPath->Path())
		->Run();
	}
}


void CompilationDataView::BurnDisc()
{
	mode = 1;

	if (fImagePath->Path() == NULL)
	{
		(new BAlert("ChooseDirectoryFirstAlert",
			"First choose the folder to burn.", "OK"))->Go();
		return;
	}

	if (fBurnerThread != NULL)
		delete fBurnerThread;

	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel("Burning in progress" B_UTF8_ELLIPSIS);
	BString device = windowParent->GetSelectedDevice().number.String();

	fBurnerThread = new CommandThread(NULL, new BInvoker(new BMessage(kBurnerMessage), this));
	fBurnerThread->AddArgument("cdrecord")
		->AddArgument("dev=")
		->AddArgument(device);
	
	if (windowParent->GetSessionMode())
		fBurnerThread->AddArgument("-sao")->AddArgument(fImagePath->Path())->Run();
	else
		fBurnerThread->AddArgument("-tao")->AddArgument(fImagePath->Path())->Run();
}
