/*
 * Copyright 2017. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */

#include "Constants.h"
#include "OutputParser.h"

#include <Catalog.h>
#include <DateTimeFormat.h>
#include <DurationFormat.h>
#include <TimeFormat.h>

#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Parser"

int32
OutputParser(float& progress, BString& eta, BString& text, BString newline)
{
	int32 resultNewline;
	int32 resultText;

	// detect percentage outputs
	resultNewline = newline.FindFirst("done, estimate finish");
	if (resultNewline != B_ERROR) {
		// get the percentage
		BString percent;
		int32 charCount = newline.FindFirst("%");
		newline.CopyInto(percent, 0, charCount - 1);
		progress = atof(percent.String());

		// get the ETA
		BString when;
		charCount = newline.FindFirst("finish");
		newline.CopyInto(when, charCount + 6, newline.CountChars());

		const char* dateformat("A B d H:M:S Y");
		set_dateformats(&dateformat);
		time_t finishTime = parsedate(when, -1);
		time_t now = (time_t)real_time_clock();

		BString duration;
		BDurationFormat formatter;
		// add 1 sec, otherwise the last second of the progress isn't shown...
		formatter.Format(duration, (now - 1) * 1000000LL, finishTime * 1000000LL);
		eta = B_TRANSLATE("Finished in %duration%");
		eta.ReplaceFirst("%duration%", duration);

		// print on top of the last line
		resultText = text.FindFirst("done, estimate finish");
		if (resultText != B_ERROR) {
			int32 offset = text.FindLast("\n");
			if (offset != B_ERROR)
				text.Remove(offset, text.CountChars() - offset);
		}
		text << "\n" << newline;
		return PERCENT;
	}

	// replace the newline string
	resultNewline = newline.FindFirst(
		"Last chance to quit, starting real write in");
	if (resultNewline != B_ERROR) {
		text << "\n" << "Waiting...";
		return CHANGE;
	}

	return NOCHANGE;
}
