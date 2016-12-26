/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "CommandThread.h"
#include "CompilationImageView.h"

#include <Alert.h>
#include <Button.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Message constants
const int32 kBurnImageMessage = 'Burn';
const int32 kChooseImageMessage = 'Chus';
const int32 kParserMessage = 'Scan';


CompilationImageView::CompilationImageView(BurnWindow& parent)
	:
	BView("Image file", B_WILL_DRAW, new BGroupLayout(B_VERTICAL,
		kControlPadding)),
	fOpenPanel(NULL),
	fImagePath(new BPath()),
	fImageParserThread(NULL)
{
	windowParent = &parent;
	step = 0;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fImageInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fImageInfoBox->SetFont(be_bold_font);
	fImageInfoBox->SetLabel("Choose the image to burn");

	fImageInfoTextView = new BTextView("ImageInfoTextView");
	fImageInfoTextView->SetWordWrap(false);
	fImageInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("ImageInfoScrollView",
		fImageInfoTextView, 0, true, true);

	BButton* chooseImageButton = new BButton("ChooseImageButton",
		"Choose image", new BMessage(kChooseImageMessage));
	chooseImageButton->SetTarget(this);
	
	BButton* burnImageButton = new BButton("BurnImageButton",
		"Burn disc", new BMessage(kBurnImageMessage));
	burnImageButton->SetTarget(this);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(new BStringView("ImageFileStringView", "Image: <none>"))
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(chooseImageButton)
				.Add(burnImageButton)
				.End()
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fImageInfoBox)
			.Add(infoScrollView)
			.End();
}


CompilationImageView::~CompilationImageView()
{
	delete fImagePath;
	delete fImageParserThread;
}


#pragma mark -- BView Overrides --


void
CompilationImageView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* chooseImageButton
		= dynamic_cast<BButton*>(FindView("ChooseImageButton"));
	if (chooseImageButton != NULL) {
		chooseImageButton->SetTarget(this);
		chooseImageButton->SetEnabled(true);
	}
	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL) {
		burnImageButton->SetTarget(this);
		burnImageButton->SetEnabled(false);
	}
}

void
CompilationImageView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChooseImageMessage:
			_ChooseImage();
			break;
		case kBurnImageMessage:
			_BurnImage();
			break;
		case kParserMessage:
			_ImageParserOutput(message);
			break;
		case B_REFS_RECEIVED:
			_OpenImage(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationImageView::_ChooseImage()
{
	// TODO Create a RefFilter for the panel?
	if (fOpenPanel == NULL)
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
			NULL, B_FILE_NODE, false, NULL, NULL, true);

	fOpenPanel->Show();
}


void
CompilationImageView::_OpenImage(BMessage* message)
{
	entry_ref imageRef;

	if (message->FindRef("refs", &imageRef) != B_OK)
		return;

	BStringView* imageFileStringView
		= dynamic_cast<BStringView*>(FindView("ImageFileStringView"));
	if (imageFileStringView == NULL)
		return;

	fImagePath->SetTo(&imageRef);

	imageFileStringView->SetText(fImagePath->Path());

	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetEnabled(true);

	fImageInfoTextView->SetText(NULL);

	// TODO Verify that the file is a supported image type

	if (fImageParserThread != NULL)
		delete fImageParserThread;

	step = 1;	// flag we're opening ISO

	fImageParserThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kParserMessage), this));
	fImageParserThread->AddArgument("isoinfo")
		->AddArgument("-d")
		->AddArgument("-i")
		->AddArgument(fImagePath->Path())
		->Run();

}

void
CompilationImageView::_BurnImage()
{
	BString imageFile = fImagePath->Path();

	if (fImagePath->Path() == NULL) {
		(new BAlert("ChooseImageFirstAlert",
			"First choose an image to burn.", "OK"))->Go();
		return;
	}

	if (fImageParserThread != NULL)
		delete fImageParserThread;

	step = 2;	// flag we're burning

	fImageInfoTextView->SetText(NULL);
	fImageInfoBox->SetLabel("Burning in progress" B_UTF8_ELLIPSIS);
	BButton* chooseImageButton
		= dynamic_cast<BButton*>(FindView("ChooseImageButton"));
	if (chooseImageButton != NULL)
		chooseImageButton->SetEnabled(false);

	BButton* burnImageButton
		= dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetEnabled(false);

	BString device("dev=");
	device.Append(windowParent->GetSelectedDevice().number.String());
	sessionConfig config = windowParent->GetSessionConfig();

	fImageParserThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kParserMessage), this));

	fImageParserThread->AddArgument("cdrecord");

	if (config.simulation)
		fImageParserThread->AddArgument("-dummy");
	if (config.eject)
		fImageParserThread->AddArgument("-eject");
	if (config.speed != "")
		fImageParserThread->AddArgument(config.speed);
	
	fImageParserThread->AddArgument(config.mode)
		->AddArgument(device)
		->AddArgument(fImagePath->Path())
		->Run();

}


void
CompilationImageView::_ImageParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fImageInfoTextView->Insert(data.String());
	}
	int32 code = -1;
	if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 1)) {
		fImageInfoBox->SetLabel("Burn the disc");
		BButton* chooseImageButton
			= dynamic_cast<BButton*>(FindView("ChooseImageButton"));
		if (chooseImageButton != NULL)
			chooseImageButton->SetEnabled(true);

		BButton* burnImageButton
			= dynamic_cast<BButton*>(FindView("BurnImageButton"));
		if (burnImageButton != NULL)
			burnImageButton->SetEnabled(true);

		step = 0;
	} else if ((message->FindInt32("thread_exit", &code) == B_OK) && (step == 2)) {
		fImageInfoBox->SetLabel("Burning complete. Burn another disc?");
		BButton* chooseImageButton
			= dynamic_cast<BButton*>(FindView("ChooseImageButton"));
		if (chooseImageButton != NULL)
			chooseImageButton->SetEnabled(true);

		BButton* burnImageButton
			= dynamic_cast<BButton*>(FindView("BurnImageButton"));
		if (burnImageButton != NULL)
			burnImageButton->SetEnabled(true);

		step = 0;
	}
}
