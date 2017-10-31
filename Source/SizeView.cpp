/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "Constants.h"
#include "SizeView.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <StringForSize.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Size view"


#pragma mark -- SizeView --

SizeView::SizeView()
	:
	BView("SizeView", B_WILL_DRAW)
{
	font_height	fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float height = 1.2 * (fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);
	fSizeBar = new SizeBar();
	fSizeBar->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));
	fSizeBar->SetExplicitMinSize(BSize(16.0, height));

	fSpaceLeft = new BStringView("spaceleft", "");
	fSpaceLeft->SetAlignment(B_ALIGN_CENTER);
	float maxWidth(StringWidth(B_TRANSLATE_COMMENT("5,000.12 MiB left (CD-900)",
		"Just for layouting, the widest string for the size info")));
	fSpaceLeft->SetExplicitMinSize(BSize(maxWidth, B_SIZE_UNSET));

	fProjectSize = new BStringView("projectsize", "");
	fProjectSize->SetAlignment(B_ALIGN_CENTER);
	fProjectSize->SetFontSize(be_plain_font->Size() - 2);
	fProjectSize->SetHighColor(tint_color(ui_color(B_CONTROL_TEXT_COLOR), 0.7));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, kControlPadding)
		.AddGroup(B_VERTICAL, 0)
			.Add(fSpaceLeft)
			.Add(fProjectSize)
			.End()
		.Add(fSizeBar);
}


SizeView::~SizeView()
{
}


void
SizeView::UpdateSizeDisplay(off_t fileSize, int32 mode,
	int32 medium)
{
	fSizeBar->SetSizeModeMedium(fileSize, mode, medium);	

	if (fileSize == 0) {
		fSpaceLeft->SetText(B_TRANSLATE("~ Empty project ~"));
		if (!fProjectSize->IsHidden())
			fProjectSize->Hide();
		return;
	}

	if (fProjectSize->IsHidden())
		fProjectSize->Show();

	char label[B_PATH_NAME_LENGTH];
	string_for_size(fileSize * 1024, label, sizeof(label));	// size in bytes
	BString space(B_TRANSLATE_COMMENT("Project size: %size%",
		"Tooltip, don't translate the variable %size%"));
	space.ReplaceFirst("%size%", label);
	fProjectSize->SetText(space);

	off_t spaceLeft;
	BString capacity;	
	if (medium == DVD_ONLY) {
		if (fileSize <= sizeDVD5[mode]) {
			spaceLeft = (off_t)sizeDVD5[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("DVD5", "DVD medium size");
		} else if (fileSize <= sizeDVD9[mode]) {
			spaceLeft = (off_t)sizeDVD9[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("DVD9", "DVD medium size");
		} else {
			spaceLeft = fileSize - (off_t)sizeDVD9[mode];
			capacity = B_TRANSLATE_COMMENT("DVD9", "DVD medium size");
		}
	} else if (medium == CD_ONLY) {
		if (fileSize <= sizeCD650[mode]) {
			spaceLeft = (off_t)sizeCD650[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-650", "CD medium size");
		} else if (fileSize <= sizeCD700[mode]) {
			spaceLeft = (off_t)sizeCD700[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-700", "CD medium size");
		} else if (fileSize <= sizeCD800[mode]) {
			spaceLeft = (off_t)sizeCD800[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-800", "CD medium size");
		} else if (fileSize <= sizeCD900[mode]) {
			spaceLeft = (off_t)sizeCD900[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-900", "CD medium size");
		} else {
			spaceLeft = fileSize - (off_t)sizeCD900[mode];
			capacity = B_TRANSLATE_COMMENT("CD-900", "CD medium size");
		}
	} else {
		if (fileSize <= sizeCD650[mode]) {
			spaceLeft = (off_t)sizeCD650[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-650", "CD medium size");
		} else if (fileSize <= sizeCD700[mode]) {
			spaceLeft = (off_t)sizeCD700[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-700", "CD medium size");
		} else if (fileSize <= sizeCD800[mode]) {
			spaceLeft = (off_t)sizeCD800[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-800", "CD medium size");
		} else if (fileSize <= sizeCD900[mode]) {
			spaceLeft = (off_t)sizeCD900[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("CD-900", "CD medium size");
		} else if (fileSize <= sizeDVD5[mode]) {
			spaceLeft = (off_t)sizeDVD5[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("DVD5", "DVD medium size");
		} else if (fileSize <= sizeDVD9[mode]) {
			spaceLeft = (off_t)sizeDVD9[mode] - fileSize;
			capacity = B_TRANSLATE_COMMENT("DVD9", "DVD medium size");
		} else {
			spaceLeft = fileSize - (off_t)sizeDVD9[mode];
			capacity = B_TRANSLATE_COMMENT("DVD9", "DVD medium size");
		}
	}

	if (medium == CD_ONLY) {
		if (fileSize <= sizeCD900[mode]) {
			string_for_size(spaceLeft * 1024, label, sizeof(label));	// size in bytes
			space = B_TRANSLATE_COMMENT("%size% left (%medium%)",
				"How much space is left on a medium; don't translate variables");
			space.ReplaceFirst("%size%", label);
			space.ReplaceFirst("%medium%", capacity);
		} else {
			string_for_size(spaceLeft * 1024, label, sizeof(label));	// size in bytes
			space = B_TRANSLATE_COMMENT("%size% over CD-900",
				"How many MiBs we're over the capacity of a CD-900");
			space.ReplaceFirst("%size%", label);
		}
	} else {
		if (fileSize <= sizeDVD9[mode]) {
			string_for_size(spaceLeft * 1024, label, sizeof(label));	// size in bytes
			space = B_TRANSLATE_COMMENT("%size% left (%medium%)",
				"How much space is left on a medium; don't translate variables");
			space.ReplaceFirst("%size%", label);
			space.ReplaceFirst("%medium%", capacity);
		} else {
			string_for_size(spaceLeft * 1024, label, sizeof(label));	// size in bytes
			space = B_TRANSLATE_COMMENT("%size% over DVD9",
				"How many MiBs we're over the capacity of a DVD9");
			space.ReplaceFirst("%size%", label);
		}
	}
	fSpaceLeft->SetText(space);
}


void
SizeView::ShowInfoText(const char* info)
{
	fSpaceLeft->SetText(info);
}
