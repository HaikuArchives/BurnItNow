/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "SizeBar.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <StringForSize.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Size bar view"

// colours
const rgb_color colorCD650 = {5, 208, 5, 255};
const rgb_color colorCD700 = {216, 255, 6, 255};
const rgb_color colorCD800 = {255, 170, 6, 255};
const rgb_color colorCD900 = {5, 144, 216, 255};
const rgb_color colorDVD5 = {210, 185, 136, 255};
const rgb_color colorDVD9 = {255, 95, 226, 255};
const rgb_color colorTooBig = {255, 82, 82, 255};

const rgb_color colorCD650_bg = {174, 242, 174, 255};
const rgb_color colorCD700_bg = {233, 255, 174, 255};
const rgb_color colorCD800_bg = {255, 221, 179, 255};
const rgb_color colorCD900_bg = {174, 207, 247, 255};
const rgb_color colorDVD5_bg = {233, 224, 207, 255};
const rgb_color colorDVD9_bg = {255, 192, 241, 255};
const rgb_color colorTooBig_bg = {255, 183, 183, 255};

// capacities, audio [0] and data [1] in KiB
static const float sizeCD650[] = { 764859, 666000 };
static const float sizeCD700[] = { 826875, 720000 };
static const float sizeCD800[] = { 930234, 810000 };
static const float sizeCD900[] = { 1023257, 4592762 };
static const float sizeDVD5[] = { 4592762, 4592762 };
static const float sizeDVD9[] = { 8545894, 4592762 };


SizeBar::SizeBar()
	:
	BView("SizeBar", B_WILL_DRAW),
	fOldAllRect(-1, -1, -1, -1),
	fSize(0),
	fMode(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


SizeBar::~SizeBar()
{
}


#pragma mark -- BView Overrides --

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
		printf("Calculate bars.\n");
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
	}

	// draw background bars
	SetHighColor(colorCD650_bg);
	FillRect(fRectCD650);
	printf("CD 650: ");
	fRectCD650.PrintToStream();


	SetHighColor(colorCD700_bg);
	FillRect(fRectCD700);
	printf("CD 700: ");
	fRectCD700.PrintToStream();


	SetHighColor(colorCD800_bg);
	FillRect(fRectCD800);
	printf("CD 800: ");
	fRectCD800.PrintToStream();


	SetHighColor(colorCD900_bg);
	FillRect(fRectCD900);
	printf("CD 900: ");
	fRectCD900.PrintToStream();


	SetHighColor(colorDVD5_bg);
	FillRect(fRectDVD5);
	printf("DVD 5: ");
	fRectDVD5.PrintToStream();


	SetHighColor(colorDVD9_bg);
	FillRect(fRectDVD9);
	printf("DVD 9: ");
	fRectDVD9.PrintToStream();


	SetHighColor(colorTooBig_bg);
	FillRect(fTooBigRect);
	printf("too big: ");
	fTooBigRect.PrintToStream();

	fOldAllRect = allRect;

	// draw size bar
	if (fSize == 0)
		return;

	BRect sizeBar = allRect;
	float percentage;
	float width = barWidth;
	rgb_color barColor;

	if (fSize <= sizeCD650[fMode]) {
		barColor = colorCD650;
		percentage = fSize / sizeCD650[fMode];
		width = fRectCD650.Width() * percentage;
		printf("fSize <= sizeCD650[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectCD650.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectCD650.Width(), width);
	} else if (fSize <= sizeCD700[fMode]) {
		barColor = colorCD700;
		percentage = (fSize - sizeCD650[fMode]) / (sizeCD700[fMode] - sizeCD650[fMode]);
		width = fRectCD650.Width() + (fRectCD700.Width() * percentage);
		printf("fSize <= sizeCD700[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectCD700.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectCD700.Width(), width);
	} else if (fSize <= sizeCD800[fMode]) {
		barColor = colorCD800;
		percentage = (fSize - sizeCD700[fMode]) / (sizeCD800[fMode] - sizeCD700[fMode]);
		width = fRectCD650.Width() + fRectCD700.Width()
			+ (fRectCD800.Width() * percentage);
		printf("fSize <= sizeCD800[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectCD800.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectCD800.Width(), width);
	} else if (fSize <= sizeCD900[fMode]) {
		barColor = colorCD900;
		percentage = (fSize - sizeCD800[fMode]) / (sizeCD900[fMode] - sizeCD800[fMode]);
		width = fRectCD650.Width() + fRectCD700.Width()
			+ fRectCD800.Width() + (fRectCD900.Width() * percentage);
		printf("fSize <= sizeCD900[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectCD900.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectCD900.Width(), width);
	} else if (fSize <= sizeCD900[fMode]) {
		barColor = colorTooBig;
		percentage = (fSize - sizeCD900[fMode]) / (sizeDVD5[fMode] - sizeCD900[fMode]);
		width = fRectCD650.Width() + fRectCD700.Width()
			+ fRectCD800.Width() + fRectCD900.Width() + (fRectDVD5.Width() * percentage);
		printf("fSize > sizeCD900[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectCD900.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectCD900.Width(), width);
	} else if (fSize <= sizeDVD5[fMode]) {
		barColor = colorDVD5;
		percentage = (fSize - sizeCD900[fMode]) / (sizeDVD5[fMode] - sizeCD900[fMode]);
		width = fRectCD650.Width() + fRectCD700.Width()
			+ fRectCD800.Width() + fRectCD900.Width()
			+ (fRectDVD5.Width() * percentage);
		printf("fSize <= sizeDVD5[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectDVD5.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectDVD5.Width(), width);
	} else if (fSize <= sizeDVD9[fMode]) {
		barColor = colorDVD9;
		percentage = (fSize - sizeDVD5[fMode]) / (sizeDVD9[fMode] - sizeDVD5[fMode]);
		width = fRectCD650.Width() + fRectCD700.Width()
			+ fRectCD800.Width() + fRectCD900.Width() + fRectDVD5.Width()
			+ (fRectDVD9.Width() * percentage);
		printf("fSize <= sizeDVD9[fMode]\n\tfSize = %f\n\tpercentage = %f\n\tfRectDVD9.Width() = %f\n\twidth = %f\n\n", fSize, percentage, fRectDVD9.Width(), width);
	} else if (fSize > sizeDVD9[fMode]) {
		barColor = colorTooBig;
		printf("fSize > sizeDVD9[fMode]\n\tfSize = %f\n\tsizeBar.Width() = %f\n", fSize, sizeBar.Width());
	}

	sizeBar.right = width;
	be_control_look->DrawMenuFieldBackground(this, sizeBar, updateRect,
		barColor, false);
}


BString
SizeBar::SetSizeAndMode(off_t fileSize, int32 mode)
{
	fSize = (float)fileSize; // size in KiB
	fMode = mode;

	if (fileSize == 0)
		return (B_TRANSLATE("Medium capacity:"));

	BString medium;
	if (fileSize <= sizeCD650[mode]) {
		fileSize = sizeCD650[mode] - fileSize;
		medium = "CD-650";
	} else if (fileSize <= sizeCD700[mode]) {
		fileSize = sizeCD700[mode] - fileSize;
		medium = "CD-700";
	} else if (fileSize <= sizeCD800[mode]) {
		fileSize = sizeCD800[mode] - fileSize;
		medium = "CD-800";
	} else if (fileSize <= sizeCD900[mode]) {
		fileSize = sizeCD900[mode] - fileSize;
		medium = "CD-900";
	} else if (fileSize <= sizeDVD5[mode]) {
		fileSize = sizeDVD5[mode] - fileSize;
		medium = "DVD5";
	} else if (fileSize <= sizeDVD9[mode]) {
		fileSize = sizeDVD9[mode] - fileSize;
		medium = "DVD9";
	}
	printf("Left fileSize: %i\n", fileSize);
	
	char label[B_PATH_NAME_LENGTH];
	BString space;

	if (fileSize <= sizeDVD9[mode]) {
		string_for_size(fileSize * 1024, label, sizeof(label));	// size in KiB
		space = B_TRANSLATE_COMMENT("%size% left (%medium%)",
			"How much space is left on a medium; don't translate variables");
		space.ReplaceFirst("%size%", label);
		space.ReplaceFirst("%medium%", medium);
	} else {
		fileSize = fileSize - sizeDVD9[mode];
		string_for_size(fileSize * 1024, label, sizeof(label));	// size in KiB
		space = B_TRANSLATE_COMMENT("%size% over DVD9",
			"How many MiBs we're over the capacity of a DVD9");
		space.ReplaceFirst("%size%", label);
	}
	Invalidate();
	return space;
}


#pragma mark -- Private Methods --

