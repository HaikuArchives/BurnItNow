/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BURNAPPLICATION_H_
#define _BURNAPPLICATION_H_

#include "AppSettings.h"

#include <Application.h>


#define my_app dynamic_cast<BurnApplication*>(be_app)


class BurnWindow;


class BurnApplication : public BApplication {
public:
					BurnApplication();

	virtual void	ReadyToRun();
	virtual void	AboutRequested();

	AppSettings* 	Settings() { return &fSettings; }

private:
	BurnWindow*		fWindow;
	AppSettings		fSettings;
};


#endif	// _BURNAPPLICATION_H_
