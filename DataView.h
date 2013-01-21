/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DATAVIEW_H_
#define _DATAVIEW_H_


#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <RadioButton.h>


class DataView : public BView {
public:
	DataView(BRect size);

	BButton* fChooseBootImageButton;
	char fChooseLabel[1024];
	BCheckBox* fBootableCDCheckBox;
	BFilePanel* fFilePanel;

	BRadioButton* fBeOSRadio;
};


#endif	// _DATAVIEW_H_
