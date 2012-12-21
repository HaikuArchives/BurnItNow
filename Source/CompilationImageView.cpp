/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationImageView.h"

#include "CommandThread.h"

#include <Box.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>


const float kControlPadding = 5;

// Message Constants
const int32 kChooseImageMessage = 'Chus';
const int32 kScannerMessage = 'Scan';


CompilationImageView::CompilationImageView()
	:
	BView("Image File(ISO/CUE)", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fImagePath(new BPath()),
	fImageScannerThread(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BBox* imageInfoBox = new BBox("ImageInfoBBox");
	imageInfoBox->SetLabel("Image Information");

	fImageInfoTextView = new BTextView("ImageInfoTextView");
	fImageInfoTextView->SetWordWrap(false);
	fImageInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("ImageInfoScrollView", fImageInfoTextView, 0, true, true);

	imageInfoBox->AddChild(infoScrollView);

	BButton* chooseImageButton = new BButton("ChooseImageButton", "Choose File", new BMessage(kChooseImageMessage));
	chooseImageButton->SetTarget(this);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(new BStringView("ImageFileStringView", "Image File: "))
			.Add(chooseImageButton)
			.End()
		.AddGroup(B_HORIZONTAL)
			.Add(imageInfoBox)
			.End();
}


CompilationImageView::~CompilationImageView()
{
	delete fImagePath;
	delete fImageScannerThread;
}


#pragma mark -- BView Overrides --


void CompilationImageView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* chooseImageButton = dynamic_cast<BButton*>(FindView("ChooseImageButton"));
	if (chooseImageButton != NULL)
		chooseImageButton->SetTarget(this);
}


void CompilationImageView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChooseImageMessage:
			_ChooseImage();
			break;
		case kScannerMessage:
			_ImageScannerParse(message);
			break;
		case B_REFS_RECEIVED:
			_OpenImage(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void CompilationImageView::_ChooseImage()
{
	// TODO Create a RefFilter for the panel?
	if (fOpenPanel == NULL)
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false, NULL, NULL, true);

	fOpenPanel->Show();
}


void CompilationImageView::_OpenImage(BMessage* message)
{
	entry_ref imageRef;

	if (message->FindRef("refs", &imageRef) != B_OK)
		return;

	BStringView* imageFileStringView = dynamic_cast<BStringView*>(FindView("ImageFileStringView"));
	if (imageFileStringView == NULL)
		return;

	BString imageFileString("Image File: ");

	fImagePath->SetTo(&imageRef);
	imageFileString << fImagePath->Path();
	imageFileStringView->SetText(imageFileString.String());

	fImageInfoTextView->SetText(NULL);

	// TODO Verify that the file is a supported image type

	if (fImageScannerThread != NULL)
		delete fImageScannerThread;

	fImageScannerThread = new CommandThread(NULL, new BInvoker(new BMessage(kScannerMessage), this));

	// TODO Search for 'isoinfo' in case it isn't in the $PATH

	fImageScannerThread->AddArgument("isoinfo")
		->AddArgument("-d")
		->AddArgument("-i")
		->AddArgument(fImagePath->Path())
		->Run();
}


void CompilationImageView::_ImageScannerParse(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fImageInfoTextView->Insert(data.String());
//	message->PrintToStream();
}
