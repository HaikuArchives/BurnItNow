/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationAudioView.h"

#include <LayoutBuilder.h>
#include <ScrollView.h>


const float kControlPadding = 5;


CompilationAudioView::CompilationAudioView()
	:
	BView("Audio CD", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BView* audioManagerView = new BView("AudioManagerView", B_WILL_DRAW);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.Add(new BScrollView("AudioManagerScrollView", audioManagerView, 0, true, true));
}
