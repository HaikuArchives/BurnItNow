/*
 * Copyright 2016. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */


#include "AudioList.h"
#include "BurnWindow.h"

#include <stdio.h>

#include <Bitmap.h>
#include <ControlLook.h>

// Message constants
const int32 kDraggedItemMessage = 'drit';
const int32 kDeleteItemMessage = 'deli';
const int32 kPopupClosedMessage = 'popc';


// #pragma mark - Listview

AudioListView::AudioListView(const char* name)
	:
	BListView(name),
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


void
AudioListView::FrameResized(float width, float height)
{
	BListView::FrameResized(width, height);

//	static const float spacing = be_control_look->DefaultLabelSpacing();
//
//	for (int32 i = 0; i < CountItems(); i++) {
//		AudioListItem *sItem = dynamic_cast<AudioListItem *> (ItemAt(i));
//		BString string(sItem->GetFilename());
//		TruncateString(&string, B_TRUNCATE_END, width - spacing * 4);
//		sItem->SetDisplayTitle(string);
//	}
}


bool
AudioListView::InitiateDrag(BPoint point, int32 dragIndex, bool wasSelected)
{
	AudioListItem* sItem = dynamic_cast<AudioListItem *> (ItemAt(CurrentSelection()));
	if (sItem == NULL) {
		// workaround for a timing problem (see Locale prefs)
		sItem = dynamic_cast<AudioListItem *> (ItemAt(dragIndex));
		Select(dragIndex);
		if (sItem == NULL)
			return false;
	}
	BString string(sItem->GetFilename());
	BMessage message(kDraggedItemMessage);
	message.AddData("text/plain", B_MIME_TYPE, string, string.Length());
	int32 index = CurrentSelection();
	message.AddInt32("index", index);

	BRect dragRect(0.0f, 0.0f, Bounds().Width(), sItem->Height());
	BBitmap* dragBitmap = new BBitmap(dragRect, B_RGB32, true);
	if (dragBitmap->IsValid()) {
		BView* view = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE,
			B_WILL_DRAW);
		dragBitmap->AddChild(view);
		dragBitmap->Lock();

		sItem->DrawItem(view, dragRect);
		view->SetHighColor(0, 0, 0, 255);
		view->StrokeRect(view->Bounds());
		view->Sync();

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
AudioListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kDraggedItemMessage:
		{
			int32 origIndex;
			int32 dropIndex;
			BPoint dropPoint;

			if (message->FindInt32("index", &origIndex) != B_OK)
				origIndex = CountItems() - 1; // new Fav added at the bottom
			dropPoint = message->DropPoint();
			dropIndex = IndexOf(ConvertFromScreen(dropPoint));
			if (dropIndex > origIndex)
				dropIndex = dropIndex - 1;
			if (dropIndex < 0)
				dropIndex = CountItems() - 1; // move to bottom

			MoveItem(origIndex, dropIndex);
			Select(dropIndex);
			RenumberTracks();
			break;
		}
		case kDeleteItemMessage:
		{
			if (!IsEmpty()) {
				int32 index = CurrentSelection();
				if (index < 0)
					break;
				RemoveItem(index);

				RenumberTracks();
				int32 count = CountItems();
				Select((index > count - 1) ? count - 1 : index);
			}
		}
		case kPopupClosedMessage:
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
				int32 index = CurrentSelection();
				if (index < 0)
					break;
				RemoveItem(index);

				RenumberTracks();
				int32 count = CountItems();
				Select((index > count - 1) ? count - 1 : index);
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
	MakeFocus(true);

	BRect bounds(Bounds());
	BRect itemFrame = ItemFrame(CountItems() - 1);
	bounds.top = itemFrame.bottom;
	if (bounds.Contains(position))
		return;

	uint32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		buttons = Window()->CurrentMessage()->FindInt32("buttons");

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		Select(IndexOf(position));
		_ShowPopUpMenu(ConvertToScreen(position));
		return;
	}
	BListView::MouseDown(position);
}


void
AudioListView::MouseUp(BPoint position)
{
	fDropRect = BRect(-1, -1, -1, -1);
	Invalidate();

	BListView::MouseUp(position);
}


void
AudioListView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
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

	msg = new BMessage(kDeleteItemMessage);
	BMenuItem* item = new BMenuItem("Remove", msg);
	menu->AddItem(item);

	menu->SetTargetForItems(this);
	menu->Go(screen, true, true, true);
	fShowingPopUpMenu = true;
}


// #pragma mark - Items


AudioListItem::AudioListItem(BString filename, BString path, int32 track)
	:
	BListItem(),
	fUpdateNeeded(true)
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
	static const float spacing = be_control_look->DefaultLabelSpacing();

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

	char nummber[4];
	snprintf(nummber, sizeof(nummber), "%" B_PRId32, fTrack + 1);
	track.Append(nummber);
	float trackWidth = font.StringWidth(track.String());

	if (!IsSelected()) {
		BRect trackRect(rect.LeftTop(),
			BPoint(spacing * 2 + trackWidth, rect.bottom));
		view->SetHighColor(tint_color(ui_color(B_LIST_BACKGROUND_COLOR), 1.08));
		view->FillRect(trackRect);
	}
	if (IsSelected())
		view->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		view->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	view->DrawString(track.String(), BPoint(spacing,
	rect.top + fheight.ascent + fheight.descent + fheight.leading));

	BString string(GetFilename());
	if (fUpdateNeeded) {
		view->TruncateString(&string, B_TRUNCATE_END, Width() - spacing * 4);
		fUpdateNeeded = false;
	}

	font.SetFace(B_REGULAR_FACE);
	view->SetFont(&font);

	view->DrawString(string.String(), BPoint(spacing * 3 + trackWidth,
		rect.top + fheight.ascent + fheight.descent + fheight.leading));

	// draw lines
	view->SetHighColor(tint_color(ui_color(B_CONTROL_BACKGROUND_COLOR),
		B_DARKEN_2_TINT));
	view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	view->StrokeLine(BPoint(spacing * 2 + trackWidth, rect.top),
		BPoint(spacing * 2 + trackWidth, rect.bottom));
}


void
AudioListItem::Update(BView* view, const BFont* finfo)
{
	// we need to DefaultLabelSpacing the update method so we can make sure the
	// list item size doesn't change
	BListItem::Update(view, finfo);

	static const float spacing = be_control_look->DefaultLabelSpacing();
	BString string(GetFilename());
	view->TruncateString(&string, B_TRUNCATE_END, Width() - spacing * 4);

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
	fTarget.SendMessage(kPopupClosedMessage);
}
