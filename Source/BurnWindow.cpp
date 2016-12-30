/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
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
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PathFinder.h>
#include <RadioButton.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>


// Message constants
const int32 kOpenHelpMessage = 'Help';
const int32 kOpenWebsiteMessage = 'Site';
const int32 kCacheQuitMessage = 'Ccqt';
const int32 kClearCacheMessage = 'Cche';
const int32 kSpeedSliderMessage = 'Sped';

const int32 kBurnDiscMessage = 'BURN';
const int32 kBuildImageMessage = 'IMAG';

const uint32 kDeviceChangeMessage[MAX_DEVICES]
	= { 'DVC0', 'DVC1', 'DVC2', 'DVC3', 'DVC4' };

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Misc variables
sdevice devices[MAX_DEVICES];
int selectedDevice;

CompilationDataView* fCompilationDataView;
CompilationAudioView* fCompilationAudioView;
CompilationImageView* fCompilationImageView;

#pragma mark --Constructor/Destructor--


BurnWindow::BurnWindow(BRect frame, const char* title)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS
		| B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
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


bool
BurnWindow::QuitRequested()
{
	if (fCacheQuitItem->IsMarked())
		_ClearCache();

	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		float infoWeight
			= fCompilationAudioView->fAudioSplitView->ItemWeight((int32)0);
		float tracksWeight
			= fCompilationAudioView->fAudioSplitView->ItemWeight(1);
		bool infoCollapse
			= fCompilationAudioView->fAudioSplitView->IsItemCollapsed((int)0);
		bool tracksCollapse
			= fCompilationAudioView->fAudioSplitView->IsItemCollapsed(1);

		settings->SetSplitWeight(infoWeight, tracksWeight);
		settings->SetSplitCollapse(infoCollapse, tracksCollapse);
		settings->SetEject((bool)fEjectCheck->Value());
		settings->SetCache(fCacheQuitItem->IsMarked());
		settings->SetSpeed(fSpeedSlider->Value());
		settings->SetWindowPosition(ConvertToScreen(Bounds()));
		settings->Unlock();
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
BurnWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref)==B_OK) {
			message->what = B_REFS_RECEIVED;
			be_app->PostMessage(message);
		}
	}

	switch (message->what) {
		case kCacheQuitMessage:
			{
				AppSettings* settings = my_app->Settings();
				bool mark = settings->GetCache();

				if (settings->Lock())
					settings->SetCache(!mark);
				settings->Unlock();

				fCacheQuitItem->SetMarked(!mark);
				break;
			}
		case kClearCacheMessage:
			_ClearCache();
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
			if (fTabView->FocusTab() == 0)
				fCompilationDataView->MessageReceived(message);
			else if (fTabView->FocusTab() == 1)
				fCompilationAudioView->MessageReceived(message);
			else if (fTabView->FocusTab() == 2)
				fCompilationImageView->MessageReceived(message);
			break;
		default:
		if (kDeviceChangeMessage[0] == message->what) {
			selectedDevice = 0;
			break;
		} else if (kDeviceChangeMessage[1] == message->what) {
			selectedDevice = 1;
			break;
		} else if (kDeviceChangeMessage[2] == message->what) {
			selectedDevice = 2;
			break;
		} else if (kDeviceChangeMessage[3] == message->what) {
			selectedDevice = 3;
			break;
		} else if (kDeviceChangeMessage[4] == message->what) {
			selectedDevice = 4;
			break;
		}
		BWindow::MessageReceived(message);
	}
}


#pragma mark --Private Interface Builders--


BMenuBar*
BurnWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("GlobalMenuBar");

	BMenu* fileMenu = new BMenu("App");
	menuBar->AddItem(fileMenu);

	BMenuItem* aboutItem = new BMenuItem("About" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED));
	aboutItem->SetTarget(be_app);
	fileMenu->AddItem(aboutItem);
	fileMenu->AddItem(new BMenuItem("Quit",
		new BMessage(B_QUIT_REQUESTED), 'Q'));

	BMenu* toolsMenu = new BMenu("Tools & settings");
	menuBar->AddItem(toolsMenu);

	fCacheQuitItem = new BMenuItem("Clear cache on quit",
		new BMessage(kCacheQuitMessage));
	toolsMenu->AddItem(fCacheQuitItem);
	
	toolsMenu->AddItem(new BMenuItem("Clear cache now",
		new BMessage(kClearCacheMessage)));

	BMenu* helpMenu = new BMenu("Help");
	menuBar->AddItem(helpMenu);

	helpMenu->AddItem(new BMenuItem("Usage instructions",
		new BMessage(kOpenHelpMessage)));

	//Apply settings (and disable unimplemented options)
	AppSettings* settings = my_app->Settings();
	fCacheQuitItem->SetMarked(settings->GetCache());

	return menuBar;
}


BView*
BurnWindow::_CreateToolBar()
{
	BGroupView* groupView = new BGroupView(B_HORIZONTAL, kControlPadding);

	fSessionMenu = new BMenu("SessionMenu");
	fSessionMenu->SetLabelFromMarked(true);
	BMenuItem* daoItem = new BMenuItem("Disc At Once (DAO)", new BMessage());
	daoItem->SetMarked(true);
	fSessionMenu->AddItem(daoItem);
	fSessionMenu->AddItem(new BMenuItem("Track At Once (TAO)", new BMessage()));
	BMenuField* fSessionMenuField = new BMenuField("SessionMenuField", "",
		fSessionMenu);
	fSessionMenuField->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fDeviceMenu = new BMenu("DeviceMenu");
	fDeviceMenu->SetLabelFromMarked(true);

	// Checking for devices
	FindDevices(devices);
	for (unsigned int ix = 0; ix < MAX_DEVICES; ++ix) {
		if (devices[ix].number.IsEmpty())
			break;
		BString deviceString("");
		deviceString << devices[ix].manufacturer << devices[ix].model
			<< "(" << devices[ix].number << ")";
		BMenuItem* deviceItem = new BMenuItem(deviceString,
			new BMessage(kDeviceChangeMessage[ix]));
		deviceItem->SetEnabled(true);
		if (ix == 0)
			deviceItem->SetMarked(true);
		fDeviceMenu->AddItem(deviceItem);
	}
	
	BMenuField* fDeviceMenuField = new BMenuField("DeviceMenuField", "",
		fDeviceMenu);
	fDeviceMenuField->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// TODO These values should be obtained from the capabilities
	// of the drive and the type of media

	fMultiCheck = new BCheckBox("MultiSessionCheckBox", "MultiSession",
						new BMessage());
	fOntheflyCheck = new BCheckBox("OnTheFlyCheckBox", "On-the-fly",
						new BMessage());
	fSimulationCheck = new BCheckBox("SimulationCheckBox", "Simulation",
						new BMessage());
	fEjectCheck = new BCheckBox("EjectCheckBox", "Eject after burning",
						new BMessage());

	fSpeedSlider = new BSlider("SpeedSlider", "Burn speed: ",
		new BMessage(kSpeedSliderMessage), 0, 5, B_HORIZONTAL);
	fSpeedSlider->SetModificationMessage(new BMessage(kSpeedSliderMessage));
	fSpeedSlider->SetLimitLabels("Min", "Max");
	fSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTH);
	fSpeedSlider->SetHashMarkCount(6);
	fSpeedSlider->SetValue(5);	// initial speed: max
	fSpeedSlider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	//Apply settings (and disable unimplemented options)
	AppSettings* settings = my_app->Settings();

	fMultiCheck->SetEnabled(false);
	fEjectCheck->SetValue((int32)settings->GetEject());
	fOntheflyCheck->SetEnabled(false);
	fSpeedSlider->SetValue(settings->GetSpeed());
	_UpdateSpeedSlider(NULL);

	// Build layout
	BLayoutBuilder::Group<>(groupView, B_VERTICAL)
		.SetInsets(kControlPadding, 0, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL, kControlPadding * 3)
			.AddGroup(B_VERTICAL)
				.AddGlue()
				.AddGrid(kControlPadding, 0.0)
					.Add(fMultiCheck, 0, 0)
					.Add(fOntheflyCheck, 1, 0)
					.Add(fSimulationCheck, 0, 1)
					.Add(fEjectCheck, 1, 1)
					.End()
				.AddGlue()
				.End()
				.Add(fSpeedSlider)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fSessionMenuField)
			.Add(fDeviceMenuField)
			.End()
		.End();

	return groupView;
}


BTabView*
BurnWindow::_CreateTabView()
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


void
BurnWindow::_ClearCache()
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

		while (true) {
			if (dir->GetNextEntry(entry) != B_OK)
				break;

			entry->Remove();
		}

		entry = new BEntry(path.Path());
		entry->Remove();

//		(new BAlert("ClearCacheAlert", "Cache cleared successfully.",
//			"OK"))->Go();
	}
}


void
BurnWindow::_OpenSettings()
{
	(new BAlert("OpenSettingsAlert", "Not implemented yet", "OK"))->Go();
}


void
BurnWindow::_OpenWebSite()
{
	// TODO Ask BRoster to launch a browser for the project website
	(new BAlert("OpenWebSiteAlert", "Not implemented yet", "OK"))->Go();
}


void
BurnWindow::_OpenHelp()
{
	BPathFinder pathFinder;
	BStringList paths;
	BPath path;
	BEntry entry;

	status_t error = pathFinder.FindPaths(B_FIND_PATH_DOCUMENTATION_DIRECTORY,
		"BurnItNow", paths);

	for (int i = 0; i < paths.CountStrings(); ++i) {
		if (error == B_OK && path.SetTo(paths.StringAt(i)) == B_OK
			&& path.Append("ReadMe.html") == B_OK) {
			entry = path.Path();
			entry_ref ref;
			entry.GetRef(&ref);
			be_roster->Launch(&ref);
		}
	}
}


void
BurnWindow::_UpdateSpeedSlider(BMessage* message)
{
	BString speedString("Burn speed: ");
	if (fSpeedSlider->Value() == 0) {
		speedString << "Min";
		fConfig.speed = "speed=0";
	} else if (fSpeedSlider->Value() == 1) {
		speedString << "4x (best for audio CDs)";
		fConfig.speed = "speed=4";
	} else if (fSpeedSlider->Value() == 2) {
		speedString << "8x";
		fConfig.speed = "speed=8";
	} else if (fSpeedSlider->Value() == 3) {
		speedString << "16x";
		fConfig.speed = "speed=16";
	} else if (fSpeedSlider->Value() == 4) {
		speedString << "32x";
		fConfig.speed = "speed=32";
	} else if (fSpeedSlider->Value() == 5) {
		speedString << "Max";
		fConfig.speed = "";
	}

	fSpeedSlider->SetLabel(speedString.String());
}


#pragma mark -- Public Methods --

void
BurnWindow::FindDevices(sdevice* array)
{
	FILE* f;
	char buff[512];
	BString output[512];
	int lineNumber = 0;
	int xdev = 0;
	
	f = popen("cdrecord -scanbus", "r");
	while (fgets(buff, sizeof(buff), f)!=NULL) {
		output[lineNumber] = buff;
		lineNumber++;
	}
	pclose(f);
	
	for (BString* i = output; i < (output + 512); i++)
	{
		if (i->FindFirst('*') == B_ERROR && i->FindFirst("\' ") != B_ERROR) {
			BString device = i->Trim();
			
			// find device number
			int numberRange = device.FindFirst('\t');
			BString number;
			number.SetTo(device, numberRange);
			
			// find manufacturer
			int manuStart = device.FindFirst('\'');
			int manuEnd = device.FindFirst('\'', manuStart);
			BString manu;
			device.CopyInto(manu, manuStart + 1, manuEnd - 3);
			
			// find model
			int modelStart = device.FindFirst('\'', manuEnd + 1);
			int modelEnd = device.FindFirst('\'', modelStart + 1);
			BString model;
			device.CopyInto(model, modelStart + 3, modelEnd - 6);
			
			sdevice dev = { number, manu, model };
			if (xdev <= MAX_DEVICES)
				array[xdev++] = dev;
		}
	}
}


sdevice
BurnWindow::GetSelectedDevice()
{
	return devices[selectedDevice];
}


sessionConfig
BurnWindow::GetSessionConfig()
{
	BString modeLabel = fSessionMenu->FindMarked()->Label();
	if (modeLabel.FindFirst("DAO") == B_ERROR)
		fConfig.mode = "-tao";
	else
		fConfig.mode = "-sao";

	fConfig.multisession = fMultiCheck->Value();
	fConfig.onthefly = fOntheflyCheck->Value();
	fConfig.simulation = fSimulationCheck->Value();
	fConfig.eject = fEjectCheck->Value();
	// Speed slider value get's updated in _UpdateSpeedSlider()

	return fConfig;
}
