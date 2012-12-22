/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IconLabel.h"

#include <string.h>

#include <Application.h>
#include <Resources.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>

IconLabel::IconLabel(BRect size, const char* labelstring, const char* gfx)
	:
	BView(size, "IconLabel", B_FOLLOW_NONE, B_WILL_DRAW),
	fLabel(labelstring)
{
	fBitmap = BTranslationUtils::GetBitmap(B_PNG_FORMAT, gfx);
	SetFont(be_bold_font);
}


IconLabel::~IconLabel()
{
	delete fBitmap;
}


void IconLabel::Draw(BRect frame)
{
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	DrawBitmap(fBitmap, BPoint(1, 0));
	DrawString(fLabel.String(), BPoint(19, 12));
}


BBitmap* IconLabel::GetBitmapResource(type_code type, const char* name)
{
	size_t len = 0;
	BResources* rsrc = BApplication::AppResources();
	const void* data = rsrc->LoadResource(type, name, &len);

	if (data == NULL) {
		fLabel = "FAN1";
		return NULL;
	}

	BMemoryIO stream(data, len);

	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	if (archive.Unflatten(&stream) != B_OK) {
		fLabel = "FAN2";
		return NULL;
	}

	BBitmap* out = new BBitmap(&archive);
	if (out == NULL) {
		fLabel = "FAN3";
		return NULL;
	}

	if (out->InitCheck() != B_OK) {
		delete out;
		out = NULL;
		fLabel = "FAN4";
	}

	return out;
}
