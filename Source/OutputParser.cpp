/*
 * Copyright 2017. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */

#include "OutputParser.h"


bool
OutputParser(BString& text, BString newline)
{
	int32 resultNewline;
	int32 resultText;

	// print on top of the last line (e.g. for percentage outputs)
	resultNewline = newline.FindFirst("done, estimate finish");
	if (resultNewline != B_ERROR) {
		resultText = text.FindFirst("done, estimate finish");
		if (resultText != B_ERROR) {
			int32 offset = text.FindLast("\n");
			if (offset != B_ERROR)
				text.Remove(offset, text.CountChars() - offset);
		}
		text << "\n" << newline;
		return true;
	}

	// replace the newline string
	resultNewline = newline.FindFirst(
		"Last chance to quit, starting real write in");
	if (resultNewline != B_ERROR) {
		text << "\n" << "Waiting...";
		return true;
	}

	return false;
}
