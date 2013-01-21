/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ASKNAME_H_
#define _ASKNAME_H_


#include <TextView.h>
#include <Window.h>


class AskName : public BWindow {
public:
	AskName(BRect frame, const char* title, uint32 mess, const char* what);
	virtual void SendText();
	virtual void MessageReceived(BMessage* msg);

private:
	BTextView* fNameTextView;
	uint32 fMessageWhat;
};


#endif	// _ASKNAME_H_
