/*
 * Copyright 2016. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */
#ifndef _PATHVIEW_H_
#define _PATHVIEW_H_


#include <StringView.h>


class BurnWindow;


class PathView : public BStringView {
public:
			PathView(const char* name, const char* text);
			~PathView();

	void	MouseDown(BPoint position);
};


#endif	// _PATHVIEW_H_
