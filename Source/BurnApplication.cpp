/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"

#include "BurnWindow.h"

#include <Alert.h>


#pragma mark --Constructor/Destructor--


BurnApplication::BurnApplication()
	:
	BApplication("application/x-vnd.osdrawer.BurnItNow2")
{
	// TODO Get window size/position from saved settings
	fWindow = new BurnWindow(BRect(150, 150, 700, 800), "BurnItNow2");

	fWindow->Lock();
	fWindow->Show();
	fWindow->Unlock();
}


#pragma mark --BApplication Overrides--


void BurnApplication::AboutRequested()
{
	// TODO Replace with real about window
	(new BAlert("AboutAlert", "BurnItNow2\nCopyright 2010-2012, BurnItNow Team. All rights reserved.\nDistributed under the terms of the MIT License.\nhttps://github.com/przemub/burnitnow\nhttp://dev.osdrawer.net/projects/burnitnow", "Ok"))->Go();
}


#pragma mark --Application Entry--


int main(int argc, char** argv)
{
	BurnApplication app;

	app.Run();

	return 0;
}
