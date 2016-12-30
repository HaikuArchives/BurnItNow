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
}


void
BurnApplication::ReadyToRun()
{
	BRect rect = fSettings.GetWindowPosition();
	fWindow = new BurnWindow(rect, "BurnItNow2");

	fWindow->Show();
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
		"BurnItNow is a graphical frontend to cdrecord, readcd "
		"and mkisofs.", "OK"))->Go();
}


#pragma mark --Application Entry--


int
main(int argc, char** argv)
{
	BurnApplication app;
	app.Run();

	return 0;
}
