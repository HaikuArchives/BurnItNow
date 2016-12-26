/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CommandThread.h"
#include "CompilationCloneView.h"

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
		fClonerInfoTextView, 0, true, true);

	BButton* createImageButton = new BButton("CreateImageButton",
		"Create image", new BMessage(kCreateImageMessage));
	createImageButton->SetTarget(this);
	
	BButton* burnImageButton = new BButton("BurnImageButton",
		"Burn image", new BMessage(kBurnImageMessage));
	burnImageButton->SetTarget(this);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(new BStringView("ImageFileStringView", "Image: <none>"))
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.Add(createImageButton)
				.Add(burnImageButton)
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

	BButton* createImageButton
		= dynamic_cast<BButton*>(FindView("CreateImageButton"));
	if (createImageButton != NULL) {
		createImageButton->SetTarget(this);
		createImageButton->SetEnabled(true);
	}
	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL) {
		burnImageButton->SetTarget(this);
		burnImageButton->SetEnabled(false);
	}
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

	BButton* createImageButton
		= dynamic_cast<BButton*>(FindView("CreateImageButton"));
	if (createImageButton != NULL)
		createImageButton->SetEnabled(false);

	BPath path;
	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, &path) != B_OK)
		return;

	status_t ret = path.Append("burnitnow_cache.iso");
	if (ret == B_OK) {
		BString parameter = "f=";
		parameter.Append(path.Path());
		BString device("dev=");
		device.Append(windowParent->GetSelectedDevice().number.String());
		
		fClonerThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kClonerMessage), this));
		fClonerThread->AddArgument("readcd")
			->AddArgument("-s")
			->AddArgument(parameter)
			->AddArgument(device)
			->Run();

		step = 1;
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
			->AddArgument(device)
			->AddArgument(path.Path())
			->Run();

		step = 2;
	}
}


void
CompilationCloneView::_ClonerOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fClonerInfoTextView->Insert(data.String());
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
		BButton* burnImageButton
			= dynamic_cast<BButton*>(FindView("BurnImageButton"));
		if (burnImageButton != NULL)
			burnImageButton->SetEnabled(true);

		BButton* createImageButton
			= dynamic_cast<BButton*>(FindView("CreateImageButton"));
		if (createImageButton != NULL)
			createImageButton->SetEnabled(true);

		step = 0;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 2)) {
		fClonerInfoBox->SetLabel("Burning complete. Burn another disc?");
		BButton* burnImageButton
			= dynamic_cast<BButton*>(FindView("BurnImageButton"));
		if (burnImageButton != NULL)
			burnImageButton->SetEnabled(true);

		BButton* createImageButton
			= dynamic_cast<BButton*>(FindView("CreateImageButton"));
		if (createImageButton != NULL)
			createImageButton->SetEnabled(true);

		step = 0;
	}
}
