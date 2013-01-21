/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUDIOVIEW_H_
#define _AUDIOVIEW_H_


#include <CheckBox.h>


class AudioView : public BView {
public:
	AudioView(BRect size);

	BCheckBox* fSwabCheckBox, *fNoFixCheckBox, *fPreEmpCheckBox, *fPadCheckBox;
};


#endif	// _AUDIOVIEW_H_
