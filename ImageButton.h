/*
 * Copyright 2012 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef _IMAGE_BUTTON_HPP_
#define _IMAGE_BUTTON_HPP_

#include <Button.h>

class ImageButton : public BButton
{
public:
						ImageButton(BRect frame, const char* name, 
							BBitmap* image, BMessage* message,
							uint32 resizingMode);
						~ImageButton();
						
	void				Draw(BRect updateRect);
	
	void				SetBitmap(BBitmap* image);
	
private:
	BBitmap*			fImage;
	BPoint				fDrawPoint;
	BRect				fInnerBounds;
};

#endif
