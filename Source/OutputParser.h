/*
 * Copyright 2017. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */
#ifndef OUTPUTPARSER_H
#define OUTPUTPARSER_H

#include <String.h>
#include <SupportDefs.h>


class OutputParser {
public:
				OutputParser(float& noteProgress, BString& noteEta);
	virtual		~OutputParser();

	int32		ParseBlankLine(BString& text, BString newline);
	int32		ParseCdrecordLine(BString& text, BString newline);
	int32		ParseIsoinfo(BString& text, BString newline);
	int32		ParseMkisofsLine(BString& text, BString newline);
	int32		ParseReadcdLine(BString& text, BString newline);
	void		Reset();

private:
	float&		progress;
	BString& 	eta;

	bigtime_t	fLastTime;
	float		fLastSize;
};

#endif // OUTPUTPARSER_H
