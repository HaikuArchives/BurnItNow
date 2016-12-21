/*
 * Copyright 2010-2016, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "BurnWindow.h"

#include <Alert.h>


#pragma mark --Constructor/Destructor--


BurnApplication::BurnApplication()
	:
	BApplication("application/x-vnd.haikuarchives.BurnItNow2")
{
	// TODO Get window size/position from saved settings
	fWindow = new BurnWindow(BRect(150, 150, 700, 800), "BurnItNow2");

	fWindow->Lock();
	fWindow->Show();
	fWindow->Unlock();
}


#pragma mark --BApplication Overrides--


void
BurnApplication::AboutRequested()
{
	// TODO Replace with real about window
	(new BAlert("AboutAlert",
		"BurnItNow2\nCopyright 2010-2016, BurnItNow Team.\n"
		"All rights reserved.\n"
		"Distributed under the terms of the MIT License.\n\n"
		"https://github.com/przemub/burnitnow\n"
		"https://github.com/HaikuArchives/BurnItNow", "OK"))->Go();
}


#pragma mark --Application Entry--


int
main(int argc, char** argv)
{
	BurnApplication app;
	app.Run();

	return 0;
}
