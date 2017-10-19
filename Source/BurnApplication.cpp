/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "BurnApplication.h"
#include "BurnWindow.h"
#include "Constants.h"

#include <Alert.h>
#include <Catalog.h>
#include <TextView.h>

#include <stdlib.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"

#pragma mark --Constructor/Destructor--


BurnApplication::BurnApplication()
	:
	BApplication(kAppSignature)
{
}


void
BurnApplication::ReadyToRun()
{
	putenv("LC_ALL=C");	// force English environment for parsing output

	BRect rect = fSettings.GetWindowPosition();
	fWindow = new BurnWindow(rect, B_TRANSLATE_SYSTEM_NAME("BurnItNow"));

	fWindow->Show();
}


#pragma mark --BApplication Overrides--


void
BurnApplication::AboutRequested()
{
	BString text = B_TRANSLATE_COMMENT(
		"BurnItNow %version%\n"
		"\tby the BurnItNow team\n"
		"\tand\n"
		"\tHumdinger,\n"
		"\tPrzemysław Buczkowski,\n"
		"\tRobert Mercer.\n\n"
		"\tCopyright %years%\n\n"
		"BurnItNow is a graphical frontend to cdrecord, readcd "
		"and mkisofs.\n\n"
		"Please report the bugs you find or features you miss. "
		"The contact info is in the usage instructions in the "
		"'Help' menu.",
		"Don't change the variables %years% and %version%.");
	text.ReplaceAll("%years%", kCopyright);
	text.ReplaceAll("%version%", kVersion);

	BAlert *alert = new BAlert("about", text.String(),
		B_TRANSLATE("Thank you"));

	BTextView *view = alert->TextView();
	BFont font;
	view->SetStylable(true);
	view->GetFont(&font);
	font.SetSize(font.Size() + 4);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 9, &font);
	alert->Go();
}


#pragma mark --Application Entry--


int
main(int argc, char** argv)
{
	BurnApplication app;
	app.Run();

	return 0;
}
