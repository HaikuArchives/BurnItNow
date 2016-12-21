/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
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
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <Slider.h>
#include <StatusBar.h>


// Message constants
const int32 kOpenHelpMessage = 'Help';
const int32 kOpenWebsiteMessage = 'Site';
const int32 kOpenSettingsMessage = 'Stng';
const int32 kClearCacheMessage = 'Cche';

const int32 kSpeedSliderMessage = 'Sped';
const int32 kBurnDiscMessage = 'BURN';
const int32 kBuildImageMessage = 'IMAG';

const uint32 kDeviceChangeMessage[MAX_DEVICES] = { 'DVC0', 'DVC1', 'DVC2', 'DVC3', 'DVC4' };

// Misc constants
const int32 kMinBurnSpeed = 2;
const int32 kMaxBurnSpeed = 52;

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Misc variables
sdevice devices[MAX_DEVICES];
int selectedDevice;

BMenu* sessionMenu;
BMenu* deviceMenu;

CompilationDataView* fCompilationDataView;
CompilationAudioView* fCompilationAudioView;
CompilationImageView* fCompilationImageView;

#pragma mark --Constructor/Destructor--


BurnWindow::BurnWindow(BRect frame, const char* title)
	:
	BWindow(frame, title, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fTabView = _CreateTabView();
	fTabView->SetBorder(B_NO_BORDER);
	fTabView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 1)
		.Add(_CreateMenuBar())
		.Add(_CreateToolBar())
		.Add(fTabView);
}


#pragma mark --BWindow Overrides--


void BurnWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref)==B_OK) {
			message->what = B_REFS_RECEIVED;
			be_app->PostMessage(message);
		}
	}

	switch (message->what) {
		case kClearCacheMessage:
			_ClearCache();
			break;
		case kOpenSettingsMessage:
			_OpenSettings();
			break;
		case kOpenWebsiteMessage:
			_OpenWebSite();
			break;
		case kOpenHelpMessage:
			_OpenHelp();
			break;
		case kSpeedSliderMessage:
			_UpdateSpeedSlider(message);
			break;
		case B_REFS_RECEIVED:
			// Redirect message to current tab
			if(fTabView->FocusTab() == 1)
				fCompilationAudioView->MessageReceived(message);
			else if(fTabView->FocusTab() == 2)
				fCompilationImageView->MessageReceived(message);
			break;
		default:
		if( kDeviceChangeMessage[0] == message->what ){selectedDevice=0; break;}
		else if( kDeviceChangeMessage[1] == message->what ){selectedDevice=1; break;}
		else if( kDeviceChangeMessage[2] == message->what ){selectedDevice=2; break;}
		else if( kDeviceChangeMessage[3] == message->what ){selectedDevice=3; break;}
		else if( kDeviceChangeMessage[4] == message->what ){selectedDevice=4; break;}
			BWindow::MessageReceived(message);
	}
}


#pragma mark --Private Interface Builders--


BMenuBar* BurnWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("GlobalMenuBar");

	BMenu* fileMenu = new BMenu("File");
	menuBar->AddItem(fileMenu);

	BMenuItem* aboutItem = new BMenuItem("About" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED));
	aboutItem->SetTarget(be_app);
	fileMenu->AddItem(aboutItem);
	fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));

	BMenu* toolsMenu = new BMenu("Tools & settings");
	menuBar->AddItem(toolsMenu);
	
	toolsMenu->AddItem(new BMenuItem("Clear cache", new BMessage(kClearCacheMessage)));
	toolsMenu->AddItem(new BMenuItem("Settings" B_UTF8_ELLIPSIS,
		new BMessage(kOpenSettingsMessage), 'S'));

	BMenu* helpMenu = new BMenu("Help");
	menuBar->AddItem(helpMenu);

	helpMenu->AddItem(new BMenuItem("Usage instructions", new BMessage(kOpenHelpMessage)));
	helpMenu->AddItem(new BMenuItem("Project website", new BMessage(kOpenWebsiteMessage)));

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
	sessionMenuField->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

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
	deviceMenuField->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// TODO These values should be obtained from the capabilities of the drive and the type of media
	BSlider* burnSlider = new BSlider("SpeedSlider", "Burn speed: 2X",
		new BMessage(kSpeedSliderMessage), kMinBurnSpeed, kMaxBurnSpeed, B_HORIZONTAL);
	burnSlider->SetModificationMessage(new BMessage(kSpeedSliderMessage));
	burnSlider->SetLimitLabels("2X", "52X");
	burnSlider->SetHashMarks(B_HASH_MARKS_BOTH);
	burnSlider->SetHashMarkCount(17);
	burnSlider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(groupView, B_VERTICAL)
		.SetInsets(kControlPadding, 0, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL, kControlPadding * 3)
			.AddGroup(B_VERTICAL)
				.AddGlue()
				.AddGrid(kControlPadding, 0.0)
					.Add(new BCheckBox("MultiSessionCheckBox", "MultiSession", new BMessage()), 0, 0)
					.Add(new BCheckBox("OnTheFlyCheckBox", "On-the-fly", new BMessage()), 1, 0)
					.Add(new BCheckBox("DummyModeCheckBox", "Dummy mode", new BMessage()), 0, 1)
					.Add(new BCheckBox("EjectCheckBox", "Eject after burning", new BMessage()), 1, 1)
					.End()
				.AddGlue()
				.End()
				.Add(burnSlider)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(sessionMenuField)
			.Add(deviceMenuField)
			.End()
		.End();

	return groupView;
}


BTabView* BurnWindow::_CreateTabView()
{
	BTabView* tabView = new BTabView("CompilationsTabView", B_WIDTH_FROM_LABEL);

	fCompilationDataView = new CompilationDataView(*this);
	fCompilationAudioView = new CompilationAudioView(*this);
	fCompilationImageView = new CompilationImageView(*this);

	tabView->AddTab(fCompilationDataView);
	tabView->AddTab(fCompilationAudioView);
	tabView->AddTab(fCompilationImageView);
	tabView->AddTab(new CompilationCDRWView(*this));
	tabView->AddTab(new CompilationCloneView(*this));

	return tabView;
}


#pragma mark --Private Message Handlers--


void BurnWindow::_ClearCache()
{
	BPath cachePath;
	if (find_directory(B_SYSTEM_CACHE_DIRECTORY, &cachePath) != B_OK)
		return;

	BPath path = cachePath;
	status_t ret = path.Append("burnitnow_cache.iso");
	if (ret == B_OK) {
		BEntry* entry = new BEntry(path.Path());
		entry->Remove();

		path = cachePath;
		path.Append("burnitnow_iso.iso");
		entry = new BEntry(path.Path());
		entry->Remove();

		path = cachePath;
		path.Append("burnitnow_cache");

		BDirectory* dir = new BDirectory(path.Path());

		while (true)
		{
			if (dir->GetNextEntry(entry) != B_OK)
				break;

			entry->Remove();
		}

		entry = new BEntry(path.Path());
		entry->Remove();

		(new BAlert("ClearCacheAlert", "Cache cleared successfully.", "OK"))->Go();
	}
}

void BurnWindow::_OpenSettings()
{
	(new BAlert("OpenSettingsAlert", "Not implemented yet", "OK"))->Go();
}

void BurnWindow::_OpenWebSite()
{
	// TODO Ask BRoster to launch a browser for the project website
	(new BAlert("OpenWebSiteAlert", "Not implemented yet", "OK"))->Go();
}


void BurnWindow::_OpenHelp()
{
	// TODO Ask BRoster to launch a browser for the local documentation
	(new BAlert("OpenHelpAlert", "Not implemented yet", "OK"))->Go();
}


void BurnWindow::_UpdateSpeedSlider(BMessage* message)
{
	BSlider* speedSlider = NULL;
	if (message->FindPointer("source", (void**)&speedSlider) != B_OK)
		return;

	if (speedSlider == NULL)
		return;

	BString speedString("Burn speed: ");
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
	
	for (BString *i=output; i<(output+512) ; i++)
	{
		if (i->FindFirst('*') == B_ERROR && i->FindFirst("\' ") != B_ERROR)
		{
			BString device = i->Trim();
			
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
