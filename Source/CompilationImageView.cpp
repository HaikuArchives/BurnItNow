/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationImageView.h"

#include "CommandThread.h"

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

const float kControlPadding = 5;

// Message constants
const int32 kBurnImageMessage = 'Burn';
const int32 kChooseImageMessage = 'Chus';
const int32 kParserMessage = 'Scan';

CompilationImageView::CompilationImageView(BurnWindow &parent)
	:
	BView("Image File (ISO/CUE)", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fImagePath(new BPath()),
	fImageParserThread(NULL)
{
	windowParent = &parent;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fImageInfoBox = new BBox("ImageInfoBBox");
	fImageInfoBox->SetLabel("Image information");

	fImageInfoTextView = new BTextView("ImageInfoTextView");
	fImageInfoTextView->SetWordWrap(false);
	fImageInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("ImageInfoScrollView", fImageInfoTextView, 0, true, true);

	fImageInfoBox->AddChild(infoScrollView);

	BButton* chooseImageButton = new BButton("ChooseImageButton", "Choose file", new BMessage(kChooseImageMessage));
	chooseImageButton->SetTarget(this);
	
	BButton* burnImageButton = new BButton("BurnImageButton", "Burn image", new BMessage(kBurnImageMessage));
	burnImageButton->SetTarget(this);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(new BStringView("ImageFileStringView", "Image file: "))
			.Add(chooseImageButton)
			.Add(burnImageButton)
			.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fImageInfoBox)
			.End();
}


CompilationImageView::~CompilationImageView()
{
	delete fImagePath;
	delete fImageParserThread;
}


#pragma mark -- BView Overrides --


void CompilationImageView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* chooseImageButton = dynamic_cast<BButton*>(FindView("ChooseImageButton"));
	if (chooseImageButton != NULL)
		chooseImageButton->SetTarget(this);
		
	BButton* burnImageButton = dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetTarget(this);
}


void CompilationImageView::MessageReceived(BMessage* message)
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


void CompilationImageView::_ChooseImage()
{
	// TODO Create a RefFilter for the panel?
	if (fOpenPanel == NULL)
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false, NULL, NULL, true);

	fOpenPanel->Show();
}

void CompilationImageView::_BurnImage()
{
	BString imageFile = fImagePath->Path();
	
	if (fImageParserThread != NULL)
		delete fImageParserThread;
		
	fImageInfoTextView->SetText(NULL);
	fImageInfoBox->SetLabel("Burning in progress...");
		
	fImageParserThread = new CommandThread(NULL, new BInvoker(new BMessage(kParserMessage), this));
	
	fImageParserThread->AddArgument("cdrecord")
		->AddArgument("-dev=")
		->AddArgument(windowParent->GetSelectedDevice().number.String());
	
	if (windowParent->GetSessionMode())
		fImageParserThread->AddArgument("-sao")->AddArgument(fImagePath->Path())->Run();
	else
		fImageParserThread->AddArgument("-tao")->AddArgument(fImagePath->Path())->Run();
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

	if (fImageParserThread != NULL)
		delete fImageParserThread;

	fImageParserThread = new CommandThread(NULL, new BInvoker(new BMessage(kParserMessage), this));

	// TODO Search for 'isoinfo' in case it isn't in the $PATH

	fImageParserThread->AddArgument("isoinfo")
		->AddArgument("-d")
		->AddArgument("-i")
		->AddArgument(fImagePath->Path())
		->Run();
	
	fImageInfoBox->SetLabel("Image information");
}


void CompilationImageView::_ImageParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fImageInfoTextView->Insert(data.String());
	
	if (!fImageParserThread->IsRunning())
		fImageInfoBox->SetLabel("Ready.");
}
