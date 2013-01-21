/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CDRWVIEW_H_
#define _CDRWVIEW_H_


#include <Button.h>
#include <Menu.h>
#include <Slider.h>


class CDRWView : public BView {
public:
	CDRWView(BRect size);

	BSlider* fBlankSpeedSlider;
	BMenu* fBlankMenu;
	BButton* fBlankButton;
};


#endif	// _CDRWVIEW_H_
