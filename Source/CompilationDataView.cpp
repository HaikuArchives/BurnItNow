/*
 * Copyright 2010, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationDataView.h"

#include <LayoutBuilder.h>
#include <ScrollView.h>


const float kControlPadding = 5;


CompilationDataView::CompilationDataView()
	:
	BView("Data CD", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding))
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BView* dataManagerView = new BView("DataManagerView", B_WILL_DRAW);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding, kControlPadding, kControlPadding, kControlPadding)
		.Add(new BScrollView("DataManagerScrollView", dataManagerView, 0, true, true));
}
