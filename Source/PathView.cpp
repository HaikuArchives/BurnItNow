/*
 * Copyright 2017. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */

#include <stdio.h>

#include "Constants.h"
#include "PathView.h"

#include <Entry.h>
#include <Messenger.h>
#include <Node.h>
#include <String.h>


PathView::PathView(const char* name, const char* text)
	:
	BStringView(name, text)
{
	SetFontSize(be_plain_font->Size() - 2);
	SetHighColor(tint_color(ui_color(B_CONTROL_TEXT_COLOR), 0.7));
	SetAlignment(B_ALIGN_RIGHT);
}


PathView::~PathView()
{
}


void
PathView::MouseDown(BPoint position)
{
	BString contents(Text());

	if (contents.FindFirst("/") == B_ERROR) {
		BMessage message(kNoPathMessage);
		BMessenger msgr(Parent());
		msgr.SendMessage(&message);
	} else {	
		BEntry entry(contents);
		BNode node(&entry);
		if (node.InitCheck() != B_OK)
			return;

		entry_ref folderRef;
		entry.GetRef(&folderRef);

		if (!node.IsDirectory()) {
			BEntry parent;
			entry.GetParent(&parent);
			parent.GetRef(&folderRef);
		}
	
		BMessenger msgr("application/x-vnd.Be-TRAK");
		BMessage refMsg(B_REFS_RECEIVED);
		refMsg.AddRef("refs",&folderRef);
		msgr.SendMessage(&refMsg);
	}

	BStringView::MouseDown(position);
}
