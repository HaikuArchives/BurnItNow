/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BURNVIEW_H_
#define _BURNVIEW_H_


#include <Button.h>
#include <CheckBox.h>
#include <Slider.h>


class BurnView : public BView {
public:
	BurnView(BRect size);
	void SetButton(bool);
	void SetLabel(char*);

	BButton* fBurnButton, *fMakeImageButton;
	BCheckBox* fMultiCheckBox, *fOnTheFlyCheckBox, *fEjectCheckBox, *fDummyModeCheckBox;
	BSlider* fSpeedSlider;
};


#endif	// _BURNVIEW_H_
