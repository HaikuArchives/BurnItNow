/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "BurnWindow.h"
#include "CompilationDataView.h"
#include "CompilationAudioView.h"
#include "CompilationImageView.h"
#include "CompilationBlankView.h"
#include "CompilationCloneView.h"
#include "CompilationDVDView.h"
#include "CompilationShared.h"
#include "Constants.h"
//#include "DirRefFilter.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PathFinder.h>
#include <RadioButton.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main window"


// Misc variables
sdevice devices[MAX_DEVICES];
int selectedDevice;

CompilationDataView* fCompilationDataView;
CompilationAudioView* fCompilationAudioView;
CompilationImageView* fCompilationImageView;
CompilationDVDView* fCompilationDVDView;
CompilationBlankView* fCompilationBlankView;
CompilationCloneView* fCompilationCloneView;

#pragma mark --Constructor/Destructor--


BurnWindow::BurnWindow(BRect frame, const char* title)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS
		| B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS),
		fOpenPanel(NULL)
{
	fTabView = _CreateTabView();
	fTabView->SetBorder(B_NO_BORDER);
	fTabView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 1)
		.Add(_CreateMenuBar())
		.Add(_CreateToolBar())
		.Add(BSpaceLayoutItem::CreateVerticalStrut(kControlPadding))
		.Add(fTabView);
}


BurnWindow::~BurnWindow()
{
	delete fOpenPanel;
}

#pragma mark --BWindow Overrides--


bool
BurnWindow::QuitRequested()
{
	BString text = _ActionInProgress();

	if (text != "") {
		text << B_TRANSLATE("\nDo you want to quit BurnItNow anyway (this "
			"won't stop the task currently in progress" B_UTF8_ELLIPSIS);
		BAlert* alert = new BAlert("stopquit", text.String(),
			B_TRANSLATE("Quit anyway"), B_TRANSLATE("Cancel"));

		int32 button = alert->Go();
		if (button)
			return false;
	}

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
		case kSetCacheFolder:
			_SetCacheFolder();
			break;
		case kOpenCacheFolder:
			_OpenCacheFolder();
			break;
		case kChooseCacheFolder:
			_ChangeCacheFolder(message);
			break;
		case kCacheQuit:
			{
				AppSettings* settings = my_app->Settings();
				bool mark = settings->GetCache();

				if (settings->Lock())
					settings->SetCache(!mark);
				settings->Unlock();

				fCacheQuitItem->SetMarked(!mark);
				break;
			}
		case kClearCache:
			_ClearCache();
			break;
		case kOpenWebsite:
			_OpenWebSite();
			break;
		case kOpenHelp:
			_OpenHelp();
			break;
		case kSpeedSlider:
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
			else if (fTabView->FocusTab() == 3)
				fCompilationDVDView->MessageReceived(message);
			break;
		default:
		if (kDeviceChange[0] == message->what) {
			selectedDevice = 0;
			break;
		} else if (kDeviceChange[1] == message->what) {
			selectedDevice = 1;
			break;
		} else if (kDeviceChange[2] == message->what) {
			selectedDevice = 2;
			break;
		} else if (kDeviceChange[3] == message->what) {
			selectedDevice = 3;
			break;
		} else if (kDeviceChange[4] == message->what) {
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

	BMenu* fileMenu = new BMenu(B_TRANSLATE("App"));
	menuBar->AddItem(fileMenu);

	BMenuItem* item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	fileMenu->AddItem(item);
	fileMenu->AddItem(new BMenuItem("Quit",
		new BMessage(B_QUIT_REQUESTED), 'Q'));

	BMenu* cacheMenu = new BMenu(B_TRANSLATE("Cache"));
	menuBar->AddItem(cacheMenu);

	cacheMenu->AddItem(new BMenuItem(B_TRANSLATE(
		"Set cache folder" B_UTF8_ELLIPSIS),
		new BMessage(kSetCacheFolder)));

	cacheMenu->AddItem(new BMenuItem(B_TRANSLATE(
		"Open cache folder"), new BMessage(kOpenCacheFolder)));

	cacheMenu->AddItem(new BMenuItem(B_TRANSLATE("Clear cache now"),
		new BMessage(kClearCache)));

	fCacheQuitItem = new BMenuItem(B_TRANSLATE("Clear cache on quit"),
		new BMessage(kCacheQuit));
	cacheMenu->AddItem(fCacheQuitItem);

	BMenu* helpMenu = new BMenu(B_TRANSLATE("Help"));
	menuBar->AddItem(helpMenu);

	helpMenu->AddItem(new BMenuItem(B_TRANSLATE("Usage instructions"),
		new BMessage(kOpenHelp)));

	helpMenu->AddItem(new BMenuItem(B_TRANSLATE("Project website"),
		new BMessage(kOpenWebsite)));

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
	BMenuItem* daoItem = new BMenuItem(B_TRANSLATE_COMMENT("Disc at once (DAO)",
		"Official term of a burn mode, probably don't translate"),
		new BMessage());
	daoItem->SetMarked(true);
	fSessionMenu->AddItem(daoItem);
	fSessionMenu->AddItem(new BMenuItem(B_TRANSLATE_COMMENT("Track at once (TAO)",
		"Official term of a burn mode, probably don't translate"),
		new BMessage()));
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
			new BMessage(kDeviceChange[ix]));
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

//	Not implemented. Should be moved to DataView?
//	fMultiCheck = new BCheckBox("MultiSessionCheckBox",
//		B_TRANSLATE("MultiSession"), new BMessage());
//	fOntheflyCheck = new BCheckBox("OnTheFlyCheckBox",
//		B_TRANSLATE("On-the-fly"), new BMessage());
	fSimulationCheck = new BCheckBox("SimulationCheckBox",
		B_TRANSLATE("Simulation"), new BMessage());
	fEjectCheck = new BCheckBox("EjectCheckBox",
		B_TRANSLATE("Eject after burning"), new BMessage());

	fSpeedSlider = new BSlider("SpeedSlider", B_TRANSLATE("Burn speed:"),
		new BMessage(kSpeedSlider), 0, 5, B_HORIZONTAL);
	fSpeedSlider->SetModificationMessage(new BMessage(kSpeedSlider));
	fSpeedSlider->SetLimitLabels(B_TRANSLATE_COMMENT("Min",
		"Abbreviation for minimal burn speed"), B_TRANSLATE_COMMENT("Max",
		"Abbreviation for maximal burn speed"));
	fSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTH);
	fSpeedSlider->SetHashMarkCount(6);
	fSpeedSlider->SetValue(5);	// initial speed: max
	fSpeedSlider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	//Apply settings (and disable unimplemented options)
	AppSettings* settings = my_app->Settings();

//	fMultiCheck->SetEnabled(false);
//	fOntheflyCheck->SetEnabled(false);
	fEjectCheck->SetValue((int32)settings->GetEject());
	fSpeedSlider->SetValue(settings->GetSpeed());
	_UpdateSpeedSlider(NULL);

	// Build layout
	BLayoutBuilder::Group<>(groupView, B_VERTICAL, 0)
		.SetInsets(kControlPadding, 0, kControlPadding, kControlPadding)
		.AddGroup(B_HORIZONTAL, kControlPadding * 3)
			.AddGroup(B_VERTICAL)
				.AddGlue()
				.AddGrid(kControlPadding, 0.0)
//					.Add(fMultiCheck, 0, 0)
//					.Add(fOntheflyCheck, 1, 0)
					.Add(fSimulationCheck, 0, 0)
					.Add(fEjectCheck, 0, 1)
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
	tabView->SetViewColor(B_TRANSPARENT_COLOR);

	fCompilationDataView = new CompilationDataView(*this);
	fCompilationAudioView = new CompilationAudioView(*this);
	fCompilationImageView = new CompilationImageView(*this);
	fCompilationDVDView = new CompilationDVDView(*this);
	fCompilationBlankView = new CompilationBlankView(*this);
	fCompilationCloneView = new CompilationCloneView(*this);

	tabView->AddTab(fCompilationDataView);
	tabView->AddTab(fCompilationAudioView);
	tabView->AddTab(fCompilationImageView);
	tabView->AddTab(fCompilationDVDView);
	tabView->AddTab(fCompilationBlankView);
	tabView->AddTab(fCompilationCloneView);

	return tabView;
}


#pragma mark --Private Message Handlers--


void
BurnWindow::_SetCacheFolder()
{
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
			B_DIRECTORY_NODE, false, new BMessage(kChooseCacheFolder),
			new DirRefFilter(), true);
		fOpenPanel->Window()->SetTitle(B_TRANSLATE("Choose cache folder"));
	}
	fOpenPanel->Show();
}


void
BurnWindow::_OpenCacheFolder()
{
	BPath folder;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(folder);
		settings->Unlock();
	}

	entry_ref ref;
	status_t status = get_ref_for_path(folder.Path(), &ref);
	if (status != B_OK)
		return;

	BMessenger msgr("application/x-vnd.Be-TRAK");
	BMessage refMsg(B_REFS_RECEIVED);
	refMsg.AddRef("refs", &ref);
	msgr.SendMessage(&refMsg);
}


void
BurnWindow::_ChangeCacheFolder(BMessage* message)
{
	if (_CheckOldCacheFolder() == false)
		return;

	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK)
		return;

	BPath newFolder(&ref);
	if (newFolder.InitCheck() != B_OK)
		return;

	// test if the new location is writable
	BPath testPath;
	BFile testFile;
	entry_ref testRef;

	testPath = newFolder;
	testPath.Append("testfile");
	get_ref_for_path(testPath.Path(), &testRef);

	testFile.SetTo(&testRef, B_READ_WRITE | B_CREATE_FILE);
	status_t result = testFile.InitCheck();
	if (result != B_OK) {
		BString text = B_TRANSLATE(
			"Changing the cache folder to '%folder%' failed.\n\n"
			"Maybe the location is read-only?");
		text.ReplaceFirst("%folder%", newFolder.Path());

		BAlert *alert = new BAlert("newcache", text.String(),
			B_TRANSLATE("OK"));
		alert->Go();

		BEntry testEntry(&testRef);
		testEntry.Remove();
		testFile.Unset();
		return;
	}
	BEntry testEntry(&testRef);
	testEntry.Remove();
	testFile.Unset();

	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->SetCacheFolder(newFolder.Path());
		settings->Unlock();
	}
}


bool
BurnWindow::_CheckOldCacheFolder()
{
	BPath oldCacheFolder;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(oldCacheFolder);
		settings->Unlock();
	}

	int32 button = true;
	BDirectory folder(oldCacheFolder.Path());
	if ((folder.Contains(kCacheFileClone, B_FILE_NODE))
		|| (folder.Contains(kCacheFileDVD, B_FILE_NODE))
		|| (folder.Contains(kCacheFileData, B_FILE_NODE))) {

		BString text = B_TRANSLATE(
			"There are still cached ISO images in the old cache folder at "
			"'%oldfolder%'.\n\n"
			"Do you want to keep or delete those images, or cancel the "
			"setting of a new cache folder?");
		text.ReplaceFirst("%oldfolder%", oldCacheFolder.Path());

		BAlert *alert = new BAlert("oldcache", text.String(),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Keep"), B_TRANSLATE("Delete"));

		button = alert->Go();

		if (button == 1)
			button = true;
		else if (button == 2) {
			_ClearCache();
			button = true;
		}
	}
	return button;
}


void
BurnWindow::_ClearCache()
{
	BPath cachePath;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetCacheFolder(cachePath);
		settings->Unlock();
	}
	if (cachePath.InitCheck() != B_OK)
		return;

	BPath path = cachePath;
	status_t ret = path.Append(kCacheFileClone);
	if (ret == B_OK) {
		BEntry* entry = new BEntry(path.Path());
		entry->Remove();
		
		path = cachePath;
		path.Append(kCacheFileDVD);
		entry = new BEntry(path.Path());
		entry->Remove();

		path = cachePath;
		path.Append(kCacheFileData);
		entry = new BEntry(path.Path());
		entry->Remove();
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
	BMessage message(B_REFS_RECEIVED);
	message.AddString("url", kWebsiteUrl);
	be_roster->Launch("text/html", &message);
}


void
BurnWindow::_OpenHelp()
{
	BPathFinder pathFinder;
	BStringList paths;
	BPath path;
	BEntry entry;

	status_t error = pathFinder.FindPaths(B_FIND_PATH_DOCUMENTATION_DIRECTORY,
		"packages/burnitnow", paths);

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
	BString speedString(B_TRANSLATE("Burn speed:"));
	speedString << " ";
	if (fSpeedSlider->Value() == 0) {
		speedString << B_TRANSLATE_COMMENT("Min",
		"Abbreviation for minimal burn speed");
		fConfig.speed = "speed=0";
	} else if (fSpeedSlider->Value() == 1) {
		speedString << B_TRANSLATE_COMMENT("4x (best for audio CDs)",
			"Multiplier for burn speed");
		fConfig.speed = "speed=4";
	} else if (fSpeedSlider->Value() == 2) {
		speedString << B_TRANSLATE_COMMENT("8x",
			"Multiplier for burn speed");
		fConfig.speed = "speed=8";
	} else if (fSpeedSlider->Value() == 3) {
		speedString << B_TRANSLATE_COMMENT("16x",
			"Multiplier for burn speed");
		fConfig.speed = "speed=16";
	} else if (fSpeedSlider->Value() == 4) {
		speedString << B_TRANSLATE_COMMENT("32x",
			"Multiplier for burn speed");
		fConfig.speed = "speed=32";
	} else if (fSpeedSlider->Value() == 5) {
		speedString << B_TRANSLATE_COMMENT("Max",
		"Abbreviation for maximal burn speed");
		fConfig.speed = "";
	}

	fSpeedSlider->SetLabel(speedString.String());
}


#pragma mark -- Private Methods --

BString
BurnWindow::_ActionInProgress()
{
	BString text = "";

	int32 dataProgress = fCompilationDataView->InProgress();
	int32 audioProgress = fCompilationAudioView->InProgress();
	int32 imageProgress = fCompilationImageView->InProgress();
	int32 dvdProgress = fCompilationDVDView->InProgress();
	int32 cdrwProgress	= fCompilationBlankView->InProgress();
	int32 cloneProgress = fCompilationCloneView->InProgress();

	if ((dataProgress == IDLE)
		&& (audioProgress == IDLE)
		&& (imageProgress == IDLE)
		&& (dvdProgress == IDLE)
		&& (cloneProgress == IDLE))
			return text;

	text = B_TRANSLATE("BurnItNow is currently busy.\n\n");

	if ((dataProgress == BUILDING)
		|| (dvdProgress == BUILDING)
		|| (cloneProgress == BUILDING))
		text << B_TRANSLATE("The building of an image is currently in "
			"progress.\n");

	if ((dataProgress == BURNING)
		|| (audioProgress == BURNING)
		|| (imageProgress == BURNING)
		|| (dvdProgress == BURNING)
		|| (cloneProgress == BURNING))
		text << B_TRANSLATE("The burning of a disc is currently in "
			"progress.\n");

	if (cdrwProgress == BLANKING)
		text << B_TRANSLATE("The blanking of a disc is currently in "
			"progress.\n");

	return text;
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

//	fConfig.multisession = fMultiCheck->Value();
//	fConfig.onthefly = fOntheflyCheck->Value();
	fConfig.simulation = fSimulationCheck->Value();
	fConfig.eject = fEjectCheck->Value();
	// Speed slider value get's updated in _UpdateSpeedSlider()

	return fConfig;
}
