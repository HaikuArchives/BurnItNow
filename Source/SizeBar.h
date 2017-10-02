/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SIZEBAR_H_
#define _SIZEBAR_H_

#include <View.h>


class BurnWindow;

class SizeBar : public BView {
public:
					SizeBar();
	virtual 		~SizeBar();
	virtual void	Draw(BRect updateRect);

	void			SetSizeModeMedium(off_t fileSize, int32 mode,
						int32 medium);

private:
	BRect		fOldAllRect;
	BRect		fRectCD650;
	BRect		fRectCD700;
	BRect		fRectCD800;
	BRect		fRectCD900;
	BRect		fRectDVD5;
	BRect		fRectDVD9;
	BRect		fTooBigRect;

	off_t		fSize;
	int32		fMode;
	int32		fMedium;
};

#endif	// _SIZEBAR_H_
