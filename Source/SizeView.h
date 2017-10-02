/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SIZEVIEW_H_
#define _SIZEVIEW_H_

#include "SizeBar.h"

#include <StringView.h>
#include <View.h>


class BurnWindow;

class SizeView : public BView {
public:
					SizeView();
					~SizeView();

	void			UpdateSizeDisplay(off_t fileSize, int32 mode,
						int32 medium);
	void			ShowInfoText(const char* info);

private:
	SizeBar*		fSizeBar;
	BStringView*	fSizeInfo;
};

#endif	// _SIZEVIEW_H_
