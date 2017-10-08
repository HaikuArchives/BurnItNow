/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "CommandThread.h"
#include "CompilationCloneView.h"
#include "Constants.h"
#include "OutputParser.h"

#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Clone view"


// Misc variables
int selectedSrcDevice;


CompilationCloneView::CompilationCloneView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Clone disc"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fClonerThread(NULL)
{
	windowParent = &parent;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fClonerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fClonerInfoBox->SetFont(be_bold_font);
	fClonerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
		"Insert the disc and create an image",
		"Status notification"));

	fClonerInfoTextView = new BTextView("CloneInfoTextView");
	fClonerInfoTextView->SetWordWrap(false);
	fClonerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("CloneInfoScrollView",
		fClonerInfoTextView, B_WILL_DRAW, true, true);
	infoScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fImageButton = new BButton("CreateImageButton",
		B_TRANSLATE("Create image"), new BMessage(kBuildImageMessage));
	fImageButton->SetTarget(this);
	fImageButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE("Burn image"),
		new BMessage(kBurnDiscMessage));
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fSizeView = new SizeView();

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
			.End()
		.Add(fSizeView);

	_UpdateSizeBar();
}


CompilationCloneView::~CompilationCloneView()
{
	delete fClonerThread;
	delete fOpenPanel;
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
		case kBuildImageMessage:
			_CreateImage();
			_UpdateSizeBar();
			break;
		case kBurnDiscMessage:
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
	fClonerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
		"Image creating in progress" B_UTF8_ELLIPSIS, "Status notification"));
	fImageButton->SetEnabled(false);

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
		step = BUILDING;

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
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(path);
		settings->Unlock();
	}
	if (path.InitCheck() != B_OK)
		return;

	status_t ret = path.Append(kCacheFileClone);
	if (ret == B_OK) {
		step = BURNING;

		fClonerInfoTextView->SetText(NULL);
		fClonerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS, "Status notification"));

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
		BString text = fClonerInfoTextView->Text();
		bool modified = OutputParser(text, data);
		if (modified) {
			fClonerInfoTextView->SetText(text);
			fClonerInfoTextView->ScrollTo(0.0, 1000000.0);
		} else {
			data << "\n";
			fClonerInfoTextView->Insert(data.String());
			fClonerInfoTextView->ScrollBy(0.0, 50.0);
		}
	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (step == BUILDING)) {
		BString result(fClonerInfoTextView->Text());

		// Last output line always (except error) contains speed statistics
		if (result.FindFirst(" kB/sec.") != B_ERROR) {
			step = NONE;

			fClonerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
				"Insert a blank disc and burn the image",
				"Status notification"));
			BString device("dev=");
			device.Append(windowParent->GetSelectedDevice().number.String());

			fClonerThread = new CommandThread(NULL,
				new BInvoker(new BMessage(), this)); // no need for notification
			fClonerThread->AddArgument("cdrecord")
				->AddArgument("-eject")
				->AddArgument(device)
				->Run();
		} else {
			step = NONE;
			fClonerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
				"Failed to create image", "Status notification"));
			return;
		}

		fImageButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		step = NONE;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (step == BURNING)) {
		fClonerInfoBox->SetLabel(B_TRANSLATE_COMMENT(
			"Burning complete. Burn another disc?", "Status notification"));
		fImageButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		step = NONE;
	}
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


int32
CompilationCloneView::InProgress()
{
	return step;
}
