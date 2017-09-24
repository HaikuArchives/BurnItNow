/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SIZEBAR_H_
#define _SIZEBAR_H_

#include <View.h>

enum {
	AUDIO = 0,
	DATA
};


class SizeBar : public BView {
public:
					SizeBar();
	virtual 		~SizeBar();
	virtual void	Draw(BRect updateRect);

	BString			SetSizeAndMode(off_t fileSize, int32 mode);

private:
	BRect		fOldAllRect;
	BRect		fRectCD650;
	BRect		fRectCD700;
	BRect		fRectCD800;
	BRect		fRectCD900;
	BRect		fRectDVD5;
	BRect		fRectDVD9;
	BRect		fTooBigRect;

	float		fSize;
	int32		fMode;
};

#endif	// _SIZEBAR_H_
