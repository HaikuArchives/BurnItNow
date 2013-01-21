/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BURNAPPLICATION_H_
#define _BURNAPPLICATION_H_


#include <Application.h>


class BurnWindow;


class BurnApplication : public BApplication {
public:
	BurnApplication();

	virtual void AboutRequested();

private:
	BurnWindow* fWindow;
};


#endif	// _BURNAPPLICATION_H_
