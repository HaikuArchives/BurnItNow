/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationCloneView.h"

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
const int32 kCreateImageMessage = 'Crat';
const int32 kBurnImageMessage = 'Wrte';
const int32 kClonerMessage = 'Clnr';

constexpr int32 kDeviceChangeMessage[MAX_DEVICES] = { 'CVC0', 'CVC1', 'CVC2', 'CVC3', 'CVC4' };

// Misc variables
sdevice srcDevices[MAX_DEVICES];
int selectedSrcDevice;

int step = 0;

CompilationCloneView::CompilationCloneView(BurnWindow &parent)
	:
	BView("Clone", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fOpenPanel(NULL),
	fClonerThread(NULL)
{
	windowParent = &parent;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fClonerInfoBox = new BBox("CloneInfoBBox");
	fClonerInfoBox->SetLabel("Ready.");

	fClonerInfoTextView = new BTextView("CloneInfoTextView");
	fClonerInfoTextView->SetWordWrap(false);
	fClonerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("CloneInfoScrollView", fClonerInfoTextView, 0, true, true);

	fClonerInfoBox->AddChild(infoScrollView);

	fSourceDeviceMenu = new BMenu("CloneModeMenu");
	fSourceDeviceMenu->SetLabelFromMarked(true);
	
	windowParent->FindDevices(srcDevices);
	
	for (unsigned int ix=0; ix<MAX_DEVICES; ++ix) {
		if (srcDevices[ix].number.IsEmpty())
			break;
		BString deviceString("");
		deviceString << srcDevices[ix].manufacturer << srcDevices[ix].model << "(" << srcDevices[ix].number << ")";
		BMenuItem* deviceItem = new BMenuItem(deviceString, new BMessage(kDeviceChangeMessage[ix]));
		deviceItem->SetEnabled(true);
		if (ix == 0)
			deviceItem->SetMarked(true);
		fSourceDeviceMenu->AddItem(deviceItem);
	}
	
	BMenuField* sourceDeviceMenuField = new BMenuField("SourceDeviceMenuField", "Source:", fSourceDeviceMenu);

	BButton* createImageButton = new BButton("CreateImageButton", "Step 1: Create disc ISO", new BMessage(kCreateImageMessage));
	createImageButton->SetTarget(this);
	
	BButton* burnImageButton = new BButton("BurnImageButton", "Step 2: Burn ISO", new BMessage(kBurnImageMessage));
	burnImageButton->SetTarget(this);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(sourceDeviceMenuField)
			.Add(createImageButton)
			.Add(burnImageButton)
			.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fClonerInfoBox)
			.End();
}


CompilationCloneView::~CompilationCloneView()
{
	delete fClonerThread;
}


#pragma mark -- BView Overrides --


void CompilationCloneView::AttachedToWindow()
{
	BView::AttachedToWindow();

	BButton* createImageButton = dynamic_cast<BButton*>(FindView("CreateImageButton"));
	if (createImageButton != NULL)
		createImageButton->SetTarget(this);
		
	BButton* burnImageButton = dynamic_cast<BButton*>(FindView("BurnImageButton"));
	if (burnImageButton != NULL)
		burnImageButton->SetTarget(this);
}


void CompilationCloneView::MessageReceived(BMessage* message)
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
		case kDeviceChangeMessage[0]:
			selectedSrcDevice=0;
			break;
		case kDeviceChangeMessage[1]:
			selectedSrcDevice=1;
			break;
		case kDeviceChangeMessage[2]:
			selectedSrcDevice=2;
			break;
		case kDeviceChangeMessage[3]:
			selectedSrcDevice=3;
			break;
		case kDeviceChangeMessage[4]:
			selectedSrcDevice=4;
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void CompilationCloneView::_CreateImage()
{
	const char* device = srcDevices[selectedSrcDevice].number.String();

	fClonerInfoTextView->SetText(NULL);
	fClonerInfoBox->SetLabel("Image creating in progress...");
	
	fClonerThread = new CommandThread(NULL, new BInvoker(new BMessage(kClonerMessage), this));
	fClonerThread->AddArgument("readcd")
		->AddArgument("-s")
		->AddArgument("f=/boot/common/cache/burnitnow_cache.iso")
		->AddArgument("dev=")
		->AddArgument(device)
		->Run();
		
	step = 1;
}


void CompilationCloneView::_BurnImage()
{
	fClonerInfoTextView->SetText(NULL);
	fClonerInfoBox->SetLabel("Image burning in progress...");
	
	fClonerThread = new CommandThread(NULL, new BInvoker(new BMessage(kClonerMessage), this));
	
	fClonerThread->AddArgument("cdrecord")
		->AddArgument("-dev=")
		->AddArgument(windowParent->GetSelectedDevice().number.String());
	
	if (windowParent->GetSessionMode())
		fClonerThread->AddArgument("-sao")->AddArgument("/boot/common/cache/burnitnow_cache.iso")->Run();
	else
		fClonerThread->AddArgument("-tao")->AddArgument("/boot/common/cache/burnitnow_cache.iso")->Run();
		
	step = 2;
}


void CompilationCloneView::_ClonerOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) != B_OK)
		return;

	data << "\n";

	fClonerInfoTextView->Insert(data.String());
	
	if (!fClonerThread->IsRunning() && step == 1)
	{
		fClonerInfoBox->SetLabel("Ready.");
		BString result(fClonerInfoTextView->Text());
		// Last output line always (expect error) contains speed statistics
		if (result.FindFirst(" kB/sec.") != B_ERROR)
		{
			BAlert* finishAlert = new BAlert("CreateImageFinishAlert", "Image file has been created. "
				"Would you like to open destination folder?", "Yes", "No");
			int resp = finishAlert->Go();
			if (resp == 0)
			{
				CommandThread* command = new CommandThread(NULL, new BInvoker(new BMessage(), this));
				command->AddArgument("open")->AddArgument("/boot/common/cache/")->Run();
			}
		}
	}
	else if (!fClonerThread->IsRunning() && step == 2)
		fClonerInfoBox->SetLabel("Ready.");
}
