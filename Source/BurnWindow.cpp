/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnWindow.h"

#include "CompilationDataView.h"
#include "CompilationAudioView.h"
#include "CompilationImageView.h"
#include "CompilationCDRWView.h"
#include "CompilationCloneView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <RadioButton.h>
#include <Slider.h>
#include <StatusBar.h>


// Message constants
const int32 kOpenHelpMessage = 'Help';
const int32 kOpenWebsiteMessage = 'Site';

const int32 kSpeedSliderMessage = 'Sped';
const int32 kBurnDiscMessage = 'BURN';
const int32 kBuildImageMessage = 'IMAG';

constexpr int32 kDeviceChangeMessage[MAX_DEVICES] = { 'DVC0', 'DVC1', 'DVC2', 'DVC3', 'DVC4' };

// Misc constants
const int32 kMinBurnSpeed = 2;
const int32 kMaxBurnSpeed = 16;

const float kControlPadding = 5;

// Misc variables
sdevice devices[MAX_DEVICES];
int selectedDevice;

BMenu* sessionMenu;
BMenu* deviceMenu;

CompilationDataView* fCompilationDataView;

#pragma mark --Constructor/Destructor--


BurnWindow::BurnWindow(BRect frame, const char* title)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fTabView = _CreateTabView();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 1)
		.Add(_CreateMenuBar())
		.Add(_CreateToolBar())
		.Add(fTabView);
}


#pragma mark --BWindow Overrides--


void BurnWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kOpenWebsiteMessage:
			_OpenWebSite();
			break;
		case kOpenHelpMessage:
			_OpenHelp();
			break;
		case kSpeedSliderMessage:
			_UpdateSpeedSlider(message);
			break;
		case kBurnDiscMessage:
			_BurnDisc();
			break;
		case kBuildImageMessage:
			_BuildImage();
			break;
		case kDeviceChangeMessage[0]:
			selectedDevice=0;
			break;
		case kDeviceChangeMessage[1]:
			selectedDevice=1;
			break;
		case kDeviceChangeMessage[2]:
			selectedDevice=2;
			break;
		case kDeviceChangeMessage[3]:
			selectedDevice=3;
			break;
		case kDeviceChangeMessage[4]:
			selectedDevice=4;
			break;
		default:
			BWindow::MessageReceived(message);
	}
}


#pragma mark --Private Interface Builders--


BMenuBar* BurnWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("GlobalMenuBar");

	BMenu* fileMenu = new BMenu("File");
	menuBar->AddItem(fileMenu);

	BMenuItem* aboutItem = new BMenuItem("About...", new BMessage(B_ABOUT_REQUESTED));
	aboutItem->SetTarget(be_app);
	fileMenu->AddItem(aboutItem);
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));

	BMenu* helpMenu = new BMenu("Help");
	menuBar->AddItem(helpMenu);

	helpMenu->AddItem(new BMenuItem("Usage Instructions", new BMessage(kOpenHelpMessage)));
	helpMenu->AddItem(new BMenuItem("Project Website", new BMessage(kOpenWebsiteMessage)));

	return menuBar;
}


BView* BurnWindow::_CreateToolBar()
{
	BGroupView* groupView = new BGroupView(B_HORIZONTAL, kControlPadding);

	sessionMenu = new BMenu("SessionMenu");
	sessionMenu->SetLabelFromMarked(true);
	BMenuItem* daoItem = new BMenuItem("Disc At Once (DAO)", new BMessage());
	daoItem->SetMarked(true);
	sessionMenu->AddItem(daoItem);
	sessionMenu->AddItem(new BMenuItem("Track At Once (TAO)", new BMessage()));
	BMenuField* sessionMenuField = new BMenuField("SessionMenuField", "", sessionMenu);


	deviceMenu = new BMenu("DeviceMenu");
	deviceMenu->SetLabelFromMarked(true);

	// Checking for devices
	FindDevices(devices);
	for (unsigned int ix=0; ix<MAX_DEVICES; ++ix) {
		if (devices[ix].number.IsEmpty())
			break;
		BString deviceString("");
		deviceString << devices[ix].manufacturer << devices[ix].model << "(" << devices[ix].number << ")";
		BMenuItem* deviceItem = new BMenuItem(deviceString, new BMessage(kDeviceChangeMessage[ix]));
		deviceItem->SetEnabled(true);
		if (ix == 0)
			deviceItem->SetMarked(true);
		deviceMenu->AddItem(deviceItem);
	}
	
	BMenuField* deviceMenuField = new BMenuField("DeviceMenuField", "", deviceMenu);

	// TODO These values should be obtained from the capabilities of the drive and the type of media
	BSlider* burnSlider = new BSlider("SpeedSlider", "Burn Speed: 2X", new BMessage(kSpeedSliderMessage), kMinBurnSpeed, kMaxBurnSpeed, B_HORIZONTAL);
	burnSlider->SetModificationMessage(new BMessage(kSpeedSliderMessage));
	burnSlider->SetLimitLabels("2X", "16X");
	burnSlider->SetHashMarks(B_HASH_MARKS_BOTH);
	burnSlider->SetHashMarkCount(15);


	// TODO Use a grid for the container to get proper alignment
	BLayoutBuilder::Group<>(groupView)
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.Add(new BCheckBox("MultiSessionCheckBox", "MultiSession", new BMessage()))
				.Add(new BCheckBox("OnTheFlyCheckBox", "On The Fly", new BMessage()))
				.End()
			.AddGroup(B_HORIZONTAL)
				.Add(new BCheckBox("DummyModeCheckBox", "Dummy Mode", new BMessage()))
				.Add(new BCheckBox("EjectCheckBox", "Eject After Burning", new BMessage()))
				.End()
			.AddGroup(B_HORIZONTAL)
				.Add(sessionMenuField)
				.End()
			.End()
		.AddGrid(kControlPadding, kControlPadding)
			.Add(burnSlider, 0, 0, 1, 1)
			.Add(deviceMenuField, 0, 1, 1, 1)
			.Add(new BButton("BurnDiscButton", "Burn Disc", new BMessage(kBurnDiscMessage)), 1, 0, 1, 1)
			.Add(new BButton("BuildISOButton", "Build ISO", new BMessage(kBuildImageMessage)), 1, 1, 1, 1)
		.End();

	return groupView;
}


BTabView* BurnWindow::_CreateTabView()
{
	BTabView* tabView = new BTabView("CompilationsTabView", B_WIDTH_FROM_LABEL);

	fCompilationDataView = new CompilationDataView(*this);

	tabView->AddTab(fCompilationDataView);
	tabView->AddTab(new CompilationAudioView());
	tabView->AddTab(new CompilationImageView(*this));
	tabView->AddTab(new CompilationCDRWView(*this));
	tabView->AddTab(new CompilationCloneView(*this));

	return tabView;
}


#pragma mark --Private Message Handlers--


void BurnWindow::_BurnDisc()
{
	(new BAlert("BurnDiscAlert", "Not Implemented Yet!", "Ok"))->Go();
}


void BurnWindow::_BuildImage()
{
	if (fTabView->FocusTab() == 0)
		fCompilationDataView->BuildISO();
	else
		(new BAlert("BuildImageAlert", "On this tab ISO building isn't implemented.", "Ok"))->Go();
}


void BurnWindow::_OpenWebSite()
{
	// TODO Ask BRoster to launch a browser for the project website
	(new BAlert("OpenWebSiteAlert", "Not Implemented Yet", "Ok"))->Go();
}


void BurnWindow::_OpenHelp()
{
	// TODO Ask BRoster to launch a browser for the local documentation
	(new BAlert("OpenHelpAlert", "Not Implemented Yet", "Ok"))->Go();
}


void BurnWindow::_UpdateSpeedSlider(BMessage* message)
{
	BSlider* speedSlider = NULL;
	if (message->FindPointer("source", (void**)&speedSlider) != B_OK)
		return;

	if (speedSlider == NULL)
		return;

	BString speedString("Burn Speed: ");
	speedString << speedSlider->Value() << "X";
	speedSlider->SetLabel(speedString.String());
}

#pragma mark -- Public Methods --

void BurnWindow::FindDevices(sdevice *array)
{
	FILE* f;
	char buff[512];
	BString output[512];
	int lineNumber = 0;
	int xdev = 0;
	
	f = popen("cdrecord -scanbus", "r");
	while (fgets(buff, sizeof(buff), f)!=NULL){
		output[lineNumber] = buff;
		lineNumber++;
	}
	pclose(f);
	
	for (BString &i: output)
	{
		if (i.FindFirst('*') == B_ERROR && i.FindFirst("\' ") != B_ERROR)
		{
			BString device = i.Trim();
			
			// find device number
			int numberRange = device.FindFirst('\t');
			BString number;
			number.SetTo(device, numberRange);
			
			// find manufacturer
			int manuStart = device.FindFirst('\'');
			int manuEnd = device.FindFirst('\'', manuStart);
			BString manu;
			device.CopyInto(manu, manuStart+1, manuEnd-3);
			
			// find model
			int modelStart = device.FindFirst('\'', manuEnd+1);
			int modelEnd = device.FindFirst('\'', modelStart+1);
			BString model;
			device.CopyInto(model, modelStart+3, modelEnd-6);
			
			sdevice dev = { number, manu, model };
			if (xdev <= MAX_DEVICES)
				array[xdev++] = dev;
		}
	}
}

sdevice BurnWindow::GetSelectedDevice() {
	return devices[selectedDevice];
}

/*
* true - DAO/SAO
* false - TAO
*/
bool BurnWindow::GetSessionMode() {
	BString modeLabel = sessionMenu->FindMarked()->Label();
	if(modeLabel.FindFirst("DAO") == B_ERROR)
		return false;
	return true;
}
