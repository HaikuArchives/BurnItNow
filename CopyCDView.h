/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COPYCDVIEW_H_
#define _COPYCDVIEW_H_


#include <Window.h>


class CopyCDView : public BView {
public:
	CopyCDView(BRect size, BWindow* targetWindow);
};


#endif	// _COPYCDVIEW_H_
