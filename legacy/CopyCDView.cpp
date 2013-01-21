/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CopyCDView.h"

#include "const.h"

#include <Button.h>


CopyCDView::CopyCDView(BRect size, BWindow* targetWindow)
	:
	BView(size, "CopyCDView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect r;
	r = Bounds();
	r.InsetBy(5.0, 5.0);

	BButton* aboutButton = new BButton(r, "About!", "About", new BMessage(MENU_FILE_ABOUT));

	aboutButton->SetTarget(targetWindow);

	AddChild(aboutButton);
}
