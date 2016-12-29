/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CommandThread.h"
#include "CompilationCloneView.h"

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
const int32 kCreateImageMessage = 'Crat';
const int32 kBurnImageMessage = 'Wrte';
const int32 kClonerMessage = 'Clnr';

const uint32 kDeviceChangeMessage[MAX_DEVICES]
	= { 'CVC0', 'CVC1', 'CVC2', 'CVC3', 'CVC4' };

// Misc variables
int selectedSrcDevice;


CompilationCloneView::CompilationCloneView(BurnWindow& parent)
	:
	BView("Clone", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fClonerThread(NULL)
{
	windowParent = &parent;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fClonerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fClonerInfoBox->SetFont(be_bold_font);
	fClonerInfoBox->SetLabel("Insert the disc and create an image");

	fClonerInfoTextView = new BTextView("CloneInfoTextView");
	fClonerInfoTextView->SetWordWrap(false);
	fClonerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("CloneInfoScrollView",
		fClonerInfoTextView, B_WILL_DRAW, true, true);
	infoScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fImageButton = new BButton("CreateImageButton", "Create image",
		new BMessage(kCreateImageMessage));
	fImageButton->SetTarget(this);
	fImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	
	fBurnButton = new BButton("BurnImageButton", "Burn image",
		new BMessage(kBurnImageMessage));
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

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
			.Add(fClonerInfoBox)
			.Add(infoScrollView)
			.End();
}


CompilationCloneView::~CompilationCloneView()
{
	delete fClonerThread;
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
		case kCreateImageMessage:
			_CreateImage();
			break;
		case kBurnImageMessage:
			_BurnImage();
			break;
		case kClonerMessage:
			_ClonerOutput(message);
			break;
		default:
		if (kDeviceChangeMessage[0] == message->what) {
			selectedSrcDevice = 0;
			break;
		} else if (kDeviceChangeMessage[1] == message->what) {
			selectedSrcDevice = 1;
			break;
		} else if (kDeviceChangeMessage[2] == message->what) {
			selectedSrcDevice = 2;
			break;
		} else if (kDeviceChangeMessage[3] == message->what) {
			selectedSrcDevice = 3;
			break;
		} else if (kDeviceChangeMessage[4] == message->what) {
			selectedSrcDevice = 4;
			break;
		}
		BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationCloneView::_CreateImage()
{
	fClonerInfoTextView->SetText(NULL);
	fClonerInfoBox->SetLabel("Image creating in progress" B_UTF8_ELLIPSIS);
	fImageButton->SetEnabled(false);

	BPath path;
	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, &path) != B_OK)
		return;

	status_t ret = path.Append("burnitnow_cache.iso");
	if (ret == B_OK) {
		step = 1;

		BString file = "f=";
		file.Append(path.Path());
		BString device("dev=");
		device.Append(windowParent->GetSelectedDevice().number.String());
		sessionConfig config = windowParent->GetSessionConfig();
		
		fClonerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kClonerMessage), this));
		fClonerThread->AddArgument("readcd")
			->AddArgument(device)
			->AddArgument("-s")
			->AddArgument("speed=10")	// for max compatibility
			->AddArgument(file)
			->Run();
	}
}


void
CompilationCloneView::_BurnImage()
{
	BPath path;
	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, &path) != B_OK)
		return;

	status_t ret = path.Append("burnitnow_cache.iso");
	if (ret == B_OK) {
		step = 2;

		fClonerInfoTextView->SetText(NULL);
		fClonerInfoBox->SetLabel("Burning in progress" B_UTF8_ELLIPSIS);

		BString device("dev=");
		device.Append(windowParent->GetSelectedDevice().number.String());
		sessionConfig config = windowParent->GetSessionConfig();

		fClonerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kClonerMessage), this));
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
			->AddArgument("-pad")
			->AddArgument("padsize=63s")
			->AddArgument(path.Path())
			->Run();
	}
}


void
CompilationCloneView::_ClonerOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fClonerInfoTextView->Insert(data.String());
		fClonerInfoTextView->ScrollTo(0.0, 2000.0);
	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 1)) {
		BString result(fClonerInfoTextView->Text());

		// Last output line always (except error) contains speed statistics
		if (result.FindFirst(" kB/sec.") != B_ERROR) {
			step = 0;

			fClonerInfoBox->SetLabel("Insert a blank disc and burn the image");
			BString device("dev=");
			device.Append(windowParent->GetSelectedDevice().number.String());

			fClonerThread = new CommandThread(NULL,
				new BInvoker(new BMessage(), this));	// no need for notification
			fClonerThread->AddArgument("cdrecord")
				->AddArgument("-eject")
				->AddArgument(device)
				->Run();
		} else {
			step = 0;
			fClonerInfoBox->SetLabel("Failed to create image");
			return;
		}

		fImageButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		step = 0;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 2)) {
		fClonerInfoBox->SetLabel("Burning complete. Burn another disc?");
		fImageButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		step = 0;
	}
}
