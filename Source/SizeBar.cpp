/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "Constants.h"
#include "SizeBar.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <StringForSize.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Size view"


#pragma mark -- SizeBar --

SizeBar::SizeBar()
	:
	BView("SizeBar", B_WILL_DRAW),
	fOldAllRect(-1, -1, -1, -1),
	fSize(0),
	fMode(0),
	fMedium(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


SizeBar::~SizeBar()
{
}


void
SizeBar::Draw(BRect updateRect)
{
	BRect allRect = Bounds();

	// draw frame
	be_control_look->DrawTextControlBorder(this, allRect, updateRect,
		ui_color(B_PANEL_BACKGROUND_COLOR));

	float barWidth = allRect.Width();
	if (allRect != fOldAllRect) {
		// calculate bars
		if (fMedium == CD_OR_DVD) {
			fRectCD650 = allRect;
			fRectCD650.right = barWidth * 0.27;	// CD-650: 27%
			fRectCD700 = fRectCD650;
			fRectCD700.left = fRectCD650.right;
			fRectCD700.right = barWidth * 0.05 + fRectCD650.right;	// CD-700: 5%
			fRectCD800 = fRectCD700;
			fRectCD800.left = fRectCD700.right;
			fRectCD800.right = barWidth * 0.10 + fRectCD700.right;	// CD-800: 10%
			fRectCD900 = fRectCD800;
			fRectCD900.left = fRectCD800.right;
			fRectCD900.right = barWidth * 0.10 + fRectCD800.right;	// CD-900: 10%
			fRectDVD5 = fRectCD900;
			fRectDVD5.left = fRectCD900.right;
			fRectDVD5.right = barWidth * 0.20 + fRectCD900.right;	// DVD5: 20%
			fRectDVD9 = fRectDVD5;
			fRectDVD9.left = fRectDVD5.right;
			fRectDVD9.right = barWidth * 0.28 + fRectDVD5.right;	// DVD5: 28%
			fTooBigRect = fRectDVD9;
			fTooBigRect.left = fRectDVD9.right;
			fTooBigRect.right = allRect.right;			// Rest: too big red
		} else if (fMedium == CD_ONLY) {
			fRectCD650 = allRect;
			fRectCD650.right = barWidth * 0.72;	// CD-650: 72%
			fRectCD700 = fRectCD650;
			fRectCD700.left = fRectCD650.right;
			fRectCD700.right = barWidth * 0.06 + fRectCD650.right;	// CD-700: 6%
			fRectCD800 = fRectCD700;
			fRectCD800.left = fRectCD700.right;
			fRectCD800.right = barWidth * 0.11 + fRectCD700.right;	// CD-800: 11%
			fRectCD900 = fRectCD800;
			fRectCD900.left = fRectCD800.right;
			fRectCD900.right = barWidth * 0.11 + fRectCD800.right;	// CD-900: 11%
			fTooBigRect = fRectCD900;
			fTooBigRect.left = fRectCD900.right;
			fTooBigRect.right = allRect.right;			// Rest: too big red
		} else if (fMedium == DVD_ONLY) {
			fRectDVD5 = allRect;
			fRectDVD5.right = barWidth * 0.5;	// DVD5: 50%
			fRectDVD9 = fRectDVD5;
			fRectDVD9.left = fRectDVD5.right;
			fRectDVD9.right = barWidth * 0.5 + fRectDVD5.right;	// DVD9: 50%
			fTooBigRect = allRect;
			fTooBigRect.left = fRectDVD9.right;
			fTooBigRect.right = allRect.right;			// Rest: too big red
		}
	}
	// draw background bars
	if (fMedium != DVD_ONLY) {
		SetHighColor(colorCD650_bg);
		FillRect(fRectCD650);

		SetHighColor(colorCD700_bg);
		FillRect(fRectCD700);

		SetHighColor(colorCD800_bg);
		FillRect(fRectCD800);

		SetHighColor(colorCD900_bg);
		FillRect(fRectCD900);
	}

	SetHighColor(colorDVD5_bg);
	FillRect(fRectDVD5);

	SetHighColor(colorDVD9_bg);
	FillRect(fRectDVD9);

	SetHighColor(colorTooBig_bg);
	FillRect(fTooBigRect);

	fOldAllRect = allRect;

	// draw size bar
	if (fSize == 0)
		return;

	BRect sizeBar = allRect;
	float percentage;
	float width = barWidth;
	rgb_color barColor;

	if (fMedium == CD_OR_DVD) {
		if (fSize <= sizeCD650[fMode]) {
			barColor = colorCD650;
			percentage = fSize / sizeCD650[fMode];
			width = fRectCD650.Width() * percentage;
		} else if (fSize <= sizeCD700[fMode]) {
			barColor = colorCD700;
			percentage = (fSize - sizeCD650[fMode])
				/ (sizeCD700[fMode] - sizeCD650[fMode]);
			width = fRectCD650.Width() + (fRectCD700.Width() * percentage);
		} else if (fSize <= sizeCD800[fMode]) {
			barColor = colorCD800;
			percentage = (fSize - sizeCD700[fMode])
				/ (sizeCD800[fMode] - sizeCD700[fMode]);
			width = fRectCD650.Width() + fRectCD700.Width()
				+ (fRectCD800.Width() * percentage);
		} else if (fSize <= sizeCD900[fMode]) {
			barColor = colorCD900;
			percentage = (fSize - sizeCD800[fMode])
				/ (sizeCD900[fMode] - sizeCD800[fMode]);
			width = fRectCD650.Width() + fRectCD700.Width()
				+ fRectCD800.Width() + (fRectCD900.Width() * percentage);
		} else if (fSize <= sizeDVD5[fMode]) {
			barColor = colorDVD5;
			percentage = (fSize - sizeCD900[fMode])
				/ (sizeDVD5[fMode] - sizeCD900[fMode]);
			width = fRectCD650.Width() + fRectCD700.Width()
				+ fRectCD800.Width() + fRectCD900.Width()
				+ (fRectDVD5.Width() * percentage);
		} else if (fSize <= sizeDVD9[fMode]) {
			barColor = colorDVD9;
			percentage = (fSize - sizeDVD5[fMode])
				/ (sizeDVD9[fMode] - sizeDVD5[fMode]);
			width = fRectCD650.Width() + fRectCD700.Width()
				+ fRectCD800.Width() + fRectCD900.Width() + fRectDVD5.Width()
				+ (fRectDVD9.Width() * percentage);
		} else if (fSize > sizeDVD9[fMode])
			barColor = colorTooBig;
	}
	else if (fMedium == CD_ONLY) {
		if (fSize <= sizeCD650[fMode]) {
			barColor = colorCD650;
			percentage = fSize / sizeCD650[fMode];
			width = fRectCD650.Width() * percentage;
			printf("Width: %f\n", width);
		} else if (fSize <= sizeCD700[fMode]) {
			barColor = colorCD700;
			percentage = (fSize - sizeCD650[fMode])
				/ (sizeCD700[fMode] - sizeCD650[fMode]);
			width = fRectCD650.Width() + (fRectCD700.Width() * percentage);
		} else if (fSize <= sizeCD800[fMode]) {
			barColor = colorCD800;
			percentage = (fSize - sizeCD700[fMode])
				/ (sizeCD800[fMode] - sizeCD700[fMode]);
			width = fRectCD650.Width() + fRectCD700.Width()
				+ (fRectCD800.Width() * percentage);
		} else if (fSize <= sizeCD900[fMode]) {
			barColor = colorCD900;
			percentage = (fSize - sizeCD800[fMode])
				/ (sizeCD900[fMode] - sizeCD800[fMode]);
			width = fRectCD650.Width() + fRectCD700.Width()
				+ fRectCD800.Width() + (fRectCD900.Width() * percentage);
		} else if (fSize > sizeCD900[fMode])
			barColor = colorTooBig;
	}
	else if (fMedium == DVD_ONLY) {
		if (fSize <= sizeDVD5[fMode]) {
			barColor = colorDVD5;
			percentage = (fSize / sizeDVD5[fMode]);
			width = fRectDVD5.Width() * percentage;
		} else if (fSize <= sizeDVD9[fMode]) {
			barColor = colorDVD9;
			percentage = (fSize - sizeDVD5[fMode])
				/ (sizeDVD9[fMode] - sizeDVD5[fMode]);
			width = fRectDVD5.Width() + (fRectDVD9.Width() * percentage);
		} else if (fSize > sizeDVD9[fMode])
			barColor = colorTooBig;
	}

	sizeBar.right = width;
	be_control_look->DrawMenuFieldBackground(this, sizeBar, updateRect,
		barColor, false);
}


void
SizeBar::SetSizeModeMedium(off_t fileSize, int32 mode, int32 medium)
{
	fSize = fileSize; // size in KiB
	fMode = mode;
	fMedium = medium;
	Invalidate();
}
