/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICONLABEL_H_
#define _ICONLABEL_H_


#include <Bitmap.h>
#include <String.h>
#include <View.h>


class IconLabel : public BView {
public:
	IconLabel(BRect size, const char* labelstring, const char* gfx);
	virtual ~IconLabel();
	virtual void Draw(BRect frame);
	BBitmap* GetBitmapResource(type_code type, const char* name);

private:
	BBitmap* fBitmap;
	BString fLabel;
};


#endif	// _ICONLABEL_H_
