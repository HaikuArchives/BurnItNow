/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnWindow.h"

#include "CompilationAudioView.h"
#include "CompilationDataView.h"
#include "CompilationImageView.h"

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
#include <TabView.h>


// Message constants
const int32 kOpenHelpMessage = 'Help';
const int32 kOpenWebsiteMessage = 'Site';

const int32 kSpeedSliderMessage = 'Sped';
const int32 kBurnDiscMessage = 'BURN';
const int32 kBuildImageMessage = 'IMAG';

// Misc constants
const int32 kMinBurnSpeed = 2;
const int32 kMaxBurnSpeed = 16;

const float kControlPadding = 5;


#pragma mark --Constructor/Destructor--


BurnWindow::BurnWindow(BRect frame, const char* title)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS)
{

	BLayoutBuilder::Group<>(this, B_VERTICAL, 1)
		.Add(_CreateMenuBar())
		.Add(_CreateToolBar())
		.Add(_CreateTabView())
		.Add(_CreateDiskUsageView());
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

	BMenuItem* aboutItem = new BMenuItem("About ...", new BMessage(B_ABOUT_REQUESTED));
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

	BMenu* sessionMenu = new BMenu("SessionMenu");
	sessionMenu->SetLabelFromMarked(true);
	BMenuItem* daoItem = new BMenuItem("Disc At Once(DAO)", new BMessage());
	daoItem->SetMarked(true);
	sessionMenu->AddItem(daoItem);
	sessionMenu->AddItem(new BMenuItem("Track At Once(TAO)", new BMessage()));
	BMenuField* sessionMenuField = new BMenuField("SessionMenuField", "", sessionMenu);


	// TODO Scan for devices and add them to the menu
	BMenu* deviceMenu = new BMenu("DeviceMenu");
	deviceMenu->SetLabelFromMarked(true);

	BMenuItem* deviceItem = new BMenuItem("No Recording Devices Found", new BMessage());
	deviceItem->SetMarked(true);
	deviceItem->SetEnabled(false);
	deviceMenu->AddItem(deviceItem);

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


BView* BurnWindow::_CreateTabView()
{
	BTabView* tabView = new BTabView("CompilationsTabView", B_WIDTH_FROM_LABEL);

	tabView->AddTab(new CompilationDataView());
	tabView->AddTab(new CompilationAudioView());
	tabView->AddTab(new CompilationImageView());

	return tabView;
}


BView* BurnWindow::_CreateDiskUsageView()
{
	BGroupView* diskUsageView = new BGroupView();
	diskUsageView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 50));

	// TODO This StatusBar will probably be moved to a private class variable to
	// allow for easy adjustments from MessageReceived()
	BStatusBar* statusBar = new BStatusBar("MediaTypeStatusBar");

	BMenu* mediaMenu = new BMenu("MediaTypeMenu");
	mediaMenu->SetLabelFromMarked(true);

	// TODO Send actual BMessages and readjust the max position of the StatusBar

	// Create our media pop up menu items and use the 650MiB item as the default
	BMenuItem* mediaMenuItem = new BMenuItem("CD 650MiB", new BMessage());
	mediaMenuItem->SetMarked(true);
	mediaMenu->AddItem(mediaMenuItem);
	mediaMenu->AddItem(new BMenuItem("CD 700MiB", new BMessage()));
//	mediaMenu->AddItem(new BMenuItem("DVD5 4.37GiB", new BMessage()));
//	mediaMenu->AddItem(new BMenuItem("DVD9 7.95GiB", new BMessage()));

	BMenuField* mediaMenuField = new BMenuField("MediaTypeMenuField", "Media:", mediaMenu);
	mediaMenuField->SetExplicitMaxSize(BSize(200, B_SIZE_UNLIMITED));

	BLayoutBuilder::Group<>(diskUsageView)
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.Add(statusBar)
		.AddStrut(10)
		.Add(mediaMenuField);

	return diskUsageView;
}


#pragma mark --Private Message Handlers--


void BurnWindow::_BurnDisc()
{
	(new BAlert("BurnDiscAlert", "Not Implemented Yet!", "Ok"))->Go();
}


void BurnWindow::_BuildImage()
{
	(new BAlert("BuildImageAlert", "Not Implemented Yet!", "Ok"))->Go();
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
