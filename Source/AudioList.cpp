/*
 * Copyright 2016-2017. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */
#include "AudioList.h"
#include "BurnWindow.h"
#include "Constants.h"

#include <stdio.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Context menu"


// #pragma mark - Listview

AudioListView::AudioListView(const char* name)
	:
	BListView(name, B_MULTIPLE_SELECTION_LIST),
	fDropRect()
{
}


AudioListView::~AudioListView()
{
}


void
AudioListView::AttachedToWindow()
{
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE);

	BListView::AttachedToWindow();
}


void
AudioListView::Draw(BRect rect)
{
	SetHighColor(ui_color(B_CONTROL_BACKGROUND_COLOR));

	BRect bounds(Bounds());
	BRect itemFrame = ItemFrame(CountItems() - 1);
	bounds.top = itemFrame.bottom;
	FillRect(bounds);

	BListView::Draw(rect);

	if (fDropRect.IsValid()) {
		SetHighColor(255, 0, 0, 255);
		StrokeRect(fDropRect);
	}
}


bool
AudioListView::InitiateDrag(BPoint point, int32 dragIndex, bool)
{
	BListItem* item = ItemAt(CurrentSelection(0));
	if (item == NULL) {
		// workaround for a timing problem (see Locale prefs)
		Select(dragIndex);
		item = ItemAt(dragIndex);
	}
	if (item == NULL)
		return false;

	// create drag message
	BMessage message(kDraggedItem);
	for (int32 i = 0;; i++) {
		int32 index = CurrentSelection(i);
		if (index < 0)
			break;
		message.AddPointer("trackitem", ItemAt(CurrentSelection(i)));
	}

	// figure out drag rect
	BRect dragRect(0.0, 0.0, Bounds().Width(), -1.0);

	// figure out, how many items fit into our bitmap
	bool fade = false;

	for (int32 i = 0; message.FindPointer("trackitem", i,
		reinterpret_cast<void**>(&item)) == B_OK; i++) {

		dragRect.bottom += ceilf(item->Height()) + 1.0;

		if (dragRect.Height() > MAX_DRAG_HEIGHT) {
			dragRect.bottom = MAX_DRAG_HEIGHT;
			fade = true;
			break;
		}
	}

	BBitmap* dragBitmap = new BBitmap(dragRect, B_RGB32, true);
	if (dragBitmap->IsValid()) {
		BView* view = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE,
			B_WILL_DRAW);
		dragBitmap->AddChild(view);
		dragBitmap->Lock();
		BRect itemBounds(dragRect) ;
		itemBounds.bottom = 0.0;
		// let all selected items, that fit into our drag_bitmap, draw
		for (int32 i = 0; message.FindPointer("trackitem", i,
				reinterpret_cast<void**>(&item)) == B_OK; i++) {
			AudioListItem* item;
			message.FindPointer("trackitem", i,
				reinterpret_cast<void**>(&item));
			itemBounds.bottom = itemBounds.top + ceilf(item->Height());
			if (itemBounds.bottom > dragRect.bottom)
				itemBounds.bottom = dragRect.bottom;
			item->DrawItem(view, itemBounds);
			itemBounds.top = itemBounds.bottom + 1.0;
		}
		// make a black frame around the edge
		view->SetHighColor(0, 0, 0, 255);
		view->StrokeRect(view->Bounds());
		view->Sync();

		uint8* bits = (uint8*)dragBitmap->Bits();
		int32 height = (int32)dragBitmap->Bounds().Height() + 1;
		int32 width = (int32)dragBitmap->Bounds().Width() + 1;
		int32 bpr = dragBitmap->BytesPerRow();

		if (fade) {
			for (int32 y = 0; y < height - ALPHA / 2; y++, bits += bpr) {
				uint8* line = bits + 3;
				for (uint8* end = line + 4 * width; line < end; line += 4)
					*line = ALPHA;
			}
			for (int32 y = height - ALPHA / 2; y < height;
				y++, bits += bpr) {
				uint8* line = bits + 3;
				for (uint8* end = line + 4 * width; line < end; line += 4)
					*line = (height - y) << 1;
			}
		} else {
			for (int32 y = 0; y < height; y++, bits += bpr) {
				uint8* line = bits + 3;
				for (uint8* end = line + 4 * width; line < end; line += 4)
					*line = ALPHA;
			}
		}
		dragBitmap->Unlock();
	} else {
		delete dragBitmap;
		dragBitmap = NULL;
	}

	if (dragBitmap != NULL)
		DragMessage(&message, dragBitmap, B_OP_ALPHA, BPoint(0.0, 0.0));
	else
		DragMessage(&message, dragRect.OffsetToCopy(point), this);

	return true;
}


void
AudioListView::GetSelectedItems(BList& indices)
{
	for (int32 i = 0; true; i++) {
		int32 index = CurrentSelection(i);
		if (index < 0)
			break;
		if (!indices.AddItem((void*)(addr_t)index))
			break;
	}
}


void
AudioListView::RemoveSelected()
{
	BList indices;
	GetSelectedItems(indices);
	int32 index = CurrentSelection() - 1;

	DeselectAll();

	if (indices.CountItems() > 0)
		RemoveItemList(indices);

	if (CountItems() > 0) {
		if (index < 0)
			index = 0;

		Select(index);
	}
}


void
AudioListView::RemoveItemList(const BList& indices)
{
	int32 count = indices.CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 index = (int32)(addr_t)indices.ItemAtFast(i) - i;
		delete RemoveItem(index);
	}
}


void
AudioListView::MoveItems(const BList& indices, int32 index)
{
	DeselectAll();
	// we remove the items while we look at them, the insertion index is decreased
	// when the items index is lower, so that we insert at the right spot after
	// removal
	BList removedItems;
	int32 count = indices.CountItems();
	for (int32 i = 0; i < count; i++) {
		int32 removeIndex = (int32)(addr_t)indices.ItemAtFast(i) - i;
		BListItem* item = RemoveItem(removeIndex);
		if (item && removedItems.AddItem((void*)item)) {
			if (removeIndex < index)
				index--;
		}
		// else ??? -> blow up
	}
	count = removedItems.CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = (BListItem*)removedItems.ItemAtFast(i);
		if (AddItem(item, index)) {
			// after we're done, the newly inserted items will be selected
			Select(index, true);
			// next items will be inserted after this one
			index++;
		} else
			delete item;
	}
}


void
AudioListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kDraggedItem:
		{
			BPoint dropPoint = message->DropPoint();
			int32 dropIndex = IndexOf(ConvertFromScreen(dropPoint));
			int32 count = CountItems();
			if (dropIndex < 0 || dropIndex > count)
				dropIndex = count;

			BList indices;
			GetSelectedItems(indices);
			MoveItems(indices, dropIndex);

			RenumberTracks();
			break;
		}
		case kDeleteItem:
		{
			if (!IsEmpty()) {
				RemoveSelected();

				if (!IsEmpty())
					RenumberTracks();
			}
			// fake to update button state and calculate SizeBar
			Looper()->PostMessage(B_REFS_RECEIVED);
			break;
		}
		case kPopupClosed:
		{
			fShowingPopUpMenu = false;
			break;
		}
		default:
		{
			BListView::MessageReceived(message);
			break;
		}
	}
}


void
AudioListView::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_DELETE:
		{
			if (!IsEmpty()) {
				Looper()->PostMessage(kDeleteItem, this);
			}
			break;
		}
		default:
		{
			BListView::KeyDown(bytes, numBytes);
			break;
		}
	}
}


void
AudioListView::MouseDown(BPoint position)
{
	if (!IsEmpty()) {
		bool onSelection = false;
		BListItem* item = ItemAt(IndexOf(position));

		if (item != NULL && item->IsSelected())
			onSelection = true;

		uint32 buttons = 0;
		if (Window() != NULL && Window()->CurrentMessage() != NULL)
			buttons = Window()->CurrentMessage()->FindInt32("buttons");

		if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {

			if (CurrentSelection() < 0 || !onSelection)
				Select(IndexOf(position));

			if (CurrentSelection() >= 0)
				_ShowPopUpMenu(ConvertToScreen(position));
			return;
		}
	}

	BListView::MouseDown(position);
}


void
AudioListView::MouseUp(BPoint position)
{
	BListView::MouseUp(position);

	if (fDropRect.IsValid()) {
		Invalidate(fDropRect);
		fDropRect = BRect();
	}
}


void
AudioListView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (dragMessage != NULL) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
			{
				int32 index = IndexOf(where);
				if (index < 0)
					index = CountItems();

				fDropRect = ItemFrame(index);
				if (fDropRect.IsValid()) {
					fDropRect.top = fDropRect.top -1;
					fDropRect.bottom = fDropRect.top + 1;
				} else {
					fDropRect = ItemFrame(index - 1);
					if (fDropRect.IsValid())
						fDropRect.top = fDropRect.bottom - 1;
					else {
						// empty view, show indicator at top
						fDropRect = Bounds();
						fDropRect.bottom = fDropRect.top + 1;
					}
				}
				Invalidate();
				break;
			}
			case B_EXITED_VIEW:
			{
				fDropRect = BRect(-1, -1, -1, -1);
				Invalidate();
				break;
			}
		}
	}
	BListView::MouseMoved(where, transit, dragMessage);
}


void
AudioListView::RenumberTracks()
{
	for (int32 i = 0; i < CountItems(); i++) {
		AudioListItem* item = dynamic_cast<AudioListItem *>(ItemAt(i));
		item->SetTrack(i);
	}
}


void
AudioListView::_ShowPopUpMenu(BPoint screen)
{
	if (fShowingPopUpMenu)
		return;

	ContextPopUp* menu = new ContextPopUp("PopUpMenu", this);
	BMessage* msg = NULL;

	msg = new BMessage(kTrackPlayback);
	BMenuItem* item = new BMenuItem(B_TRANSLATE("Play back"), msg);
	item->SetTarget(Parent());
	menu->AddItem(item);

	msg = new BMessage(kDeleteItem);
	item = new BMenuItem(B_TRANSLATE("Remove"), msg);
	item->SetTarget(this);
	menu->AddItem(item);

	menu->Go(screen, true, true, true);
	fShowingPopUpMenu = true;
}


// #pragma mark - Items


AudioListItem::AudioListItem(BString filename, BString path, int32 track)
	:
	BListItem()
{
	fFilename = filename;
	fPath = path;
	fTrack = track;
}


AudioListItem::~AudioListItem()
{
}


void
AudioListItem::DrawItem(BView* view, BRect rect, bool complete)
{
	// set background color
	rgb_color bgColor;

	if (IsSelected())
		bgColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	else
		bgColor = ui_color(B_LIST_BACKGROUND_COLOR);

	view->SetHighColor(bgColor);
	view->SetLowColor(bgColor);
	view->FillRect(rect);

	// text
	BFont font(be_plain_font);
	font.SetFace(B_BOLD_FACE);
	view->SetFont(&font);
	font_height	fheight;
	font.GetHeight(&fheight);

	BString track("");
	if (fTrack < 9)
		track.Append("0");

	char number[4];
	snprintf(number, sizeof(number), "%" B_PRId32, fTrack + 1);
	track.Append(number);
	float trackWidth = font.StringWidth(track.String());

	if (!IsSelected()) {
		BRect trackRect(rect.LeftTop(),
			BPoint(kControlPadding * 2 + trackWidth, rect.bottom));
		view->SetHighColor(tint_color(ui_color(B_LIST_BACKGROUND_COLOR), 1.08));
		view->FillRect(trackRect);
	}
	if (IsSelected())
		view->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		view->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	view->DrawString(track.String(), BPoint(kControlPadding,
	rect.top + fheight.ascent + fheight.descent + fheight.leading));

	BString string(GetFilename());
	view->TruncateString(&string, B_TRUNCATE_END, Width() - kControlPadding * 4);

	font.SetFace(B_REGULAR_FACE);
	view->SetFont(&font);

	view->DrawString(string.String(), BPoint(kControlPadding * 3 + trackWidth,
		rect.top + fheight.ascent + fheight.descent + fheight.leading));

	// draw lines
	view->SetHighColor(tint_color(ui_color(B_CONTROL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	view->StrokeLine(BPoint(kControlPadding * 2 + trackWidth, rect.top),
		BPoint(kControlPadding * 2 + trackWidth, rect.bottom));
}


void
AudioListItem::Update(BView* view, const BFont* finfo)
{
	// we need to DefaultLabelSpacing the update method so we can make sure the
	// list item size doesn't change
	BListItem::Update(view, finfo);

	font_height	fheight;
	finfo->GetHeight(&fheight);

	SetHeight(ceilf(fheight.ascent + 2 + fheight.leading / 2
		+ fheight.descent) + 5);
}


// #pragma mark - Context menu


ContextPopUp::ContextPopUp(const char* name, BMessenger target)
	:
	BPopUpMenu(name, false, false),
	fTarget(target)
{
	SetAsyncAutoDestruct(true);
}


ContextPopUp::~ContextPopUp()
{
	fTarget.SendMessage(kPopupClosed);
}
