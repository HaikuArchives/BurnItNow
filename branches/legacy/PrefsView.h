/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PREFSVIEW_H_
#define _PREFSVIEW_H_


#include <CheckBox.h>
#include <Menu.h>


class PrefsView : public BView {
public:
	PrefsView(BRect size);

	BMenu* fRecordersMenu;
	BCheckBox* fDAOCheckBox, *fBurnProofCheckBox;
};


#endif	// _PREFSVIEW_H_
