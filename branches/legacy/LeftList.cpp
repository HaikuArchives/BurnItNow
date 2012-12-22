/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "LeftList.h"

#include "DataView.h"
#include "jpWindow.h"
#include "RightList.h"

#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>

extern bool VRCD;
extern bool ISOFILE;
extern bool SHOW_INFO;

extern char* IMAGE_NAME;
extern char* BURNIT_PATH;
extern char* BURN_DIR;


BBitmap* GetBitmapResource(type_code type, const char* name);

LeftListItem::LeftListItem(entry_ref* ref, const char* name, BBitmap* icon, struct AudioInfo* Info)
	:
	BListItem()
{
	fRef = *ref;
	strcpy(fName, name);
	fIconBitmap = icon;

	if (Info != NULL) {
		fIsAudio = true;
		fAudioInfo.bps = Info->bps;
		fAudioInfo.frame_rate = Info->frame_rate;
		fAudioInfo.channels = Info->channels;
		fAudioInfo.total_time = Info->total_time;
		strcpy(fAudioInfo.pretty_name, Info->pretty_name);
		strcpy(fAudioInfo.short_name, Info->short_name);
	} else
		fIsAudio = false;
}


void LeftListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	char temp_char[1024];
	rgb_color rgbColor = {255, 255, 255, 255};
	rgb_color rgbSelectedColor = {235, 235, 200, 255};
	rgb_color rgbPatternColor = {244, 244, 255, 255};
	rgb_color black = {0, 0, 0, 255};


	if (IsSelected())
		rgbColor = rgbSelectedColor;
	else if (!fIsAudio)
		rgbColor = rgbPatternColor;

	owner->SetHighColor(rgbColor);
	owner->SetLowColor(rgbColor);
	owner->FillRect(frame);

	owner->SetHighColor(black);

	owner->SetDrawingMode(B_OP_ALPHA);
	owner->DrawBitmap(fIconBitmap, BPoint(1, frame.top + 1));
	
	owner->MovePenTo(BPoint(21, frame.bottom - 1));
	if (fIsAudio) {
		sprintf(temp_char, "%s - %d:%d", fName, (int)(fAudioInfo.total_time / 1000000) / 60, (int)(fAudioInfo.total_time / 1000000) % 60);
		owner->DrawString(temp_char);
	} else
		owner->DrawString(fName);
}


LeftList::LeftList(BRect size)
	:
	BListView(size, "LeftList", B_MULTIPLE_SELECTION_LIST, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(255, 255, 255, 0);

	fVRCDBitmap = BTranslationUtils::GetBitmap('PNG ', "vrcd.png");
	fISOBitmap = BTranslationUtils::GetBitmap('PNG ', "iso_16.png");
	fAudioBitmap = BTranslationUtils::GetBitmap('PNG ', "audio.png");

	fTrackPopUpMenu = new BPopUpMenu("Tracks Popup");
	fTrackPopUpMenu->SetRadioMode(false);
	fTrackPopUpMenu->AddItem(new BMenuItem("Move Up", new BMessage('mvup')));
	fTrackPopUpMenu->AddItem(new BMenuItem("Remove", new BMessage('remt')));
	fTrackPopUpMenu->AddItem(new BMenuItem("Move Down", new BMessage('mvdn')));
	fTrackPopUpMenu->AddItem(new BMenuItem("Play", new BMessage('play')));
}


LeftList::~LeftList()
{
	delete fVRCDBitmap;
	delete fISOBitmap;
	delete fAudioBitmap;
}


void LeftList::MessageReceived(BMessage* msg)
{
	struct AudioInfo fAudioInfo;
	BMediaFile* testfile;
	bool fIsAudio = false;
	BMediaTrack* track;
	media_codec_info codecInfo;
	media_format format;
	memset(&format, 0, sizeof(format));

	entry_ref ref;
	int32 counter = 0;

	switch (msg->what) {

		case B_SIMPLE_DATA:
			while (msg->FindRef("refs", counter++, &ref) == B_OK) {
				if ((testfile = new BMediaFile(&ref)) != NULL) {
					testfile->InitCheck();
					track = testfile->TrackAt(0);
					if (track != NULL) {
						track->EncodedFormat(&format);
						if (format.IsAudio()) {
							memset(&format, 0, sizeof(format));
							format.type = B_MEDIA_RAW_AUDIO;
							track->DecodedFormat(&format);
							fAudioInfo.total_time = track->Duration();
							media_raw_audio_format* raf = &(format.u.raw_audio);
							fAudioInfo.bps = (int32)(raf->format & 0xf);
							fAudioInfo.frame_rate = (int32)raf->frame_rate;
							fAudioInfo.channels = (int32)raf->channel_count;
							track->GetCodecInfo(&codecInfo);
							strcpy(fAudioInfo.pretty_name, codecInfo.pretty_name);
							strcpy(fAudioInfo.short_name, codecInfo.short_name);
							fIsAudio = true;
						}
					}
				} else
					WriteLog("MediaFile NULL (file doesnt exists!?)");

				delete testfile;
				if (fIsAudio) {
					if (!strcmp(fAudioInfo.pretty_name, "Raw Audio") && (fAudioInfo.channels == 2) && (fAudioInfo.frame_rate == 44100) && (fAudioInfo.bps == 2))
						AddItem(new LeftListItem(&ref, ref.name, fAudioBitmap, &fAudioInfo));
					else {
						BAlert* MyAlert = new BAlert("BurnItNow", "You can only burn 16 bits stereo 44.1 kHz Raw Audio files.\n (More audio files will be supported in the future)", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
						MyAlert->Go();
					}
				} else {
					BPath temp_path;
					BEntry(&ref).GetPath(&temp_path);
					jpWindow* win = dynamic_cast<jpWindow*>(Window());
					if (win != NULL)
						win->SetISOFile((char*)temp_path.Path());
				}
			}
			break;
		default:
			BListView::MessageReceived(msg);
			break;
	}
}


void LeftList::KeyDown(const char* bytes, int32 numBytes)
{
	BPath temp_path;
	int32 result;
	switch (bytes[0]) {
		case B_DELETE: {
				int32 selection = CurrentSelection();
				if (selection >= 0) {
					BAlert* MyAlert = new BAlert("BurnItNow", "Are you sure you want to delete this selection", "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
					MyAlert->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
					result = MyAlert->Go();
					if (result == 0) {
						LeftListItem* item = (LeftListItem*)RemoveItem(selection);
						if (!item->fIsAudio) {
							VRCD = false;
							ISOFILE = false;
							jpWindow* win = dynamic_cast<jpWindow*>(Window());
							if (win != NULL) {
								win->fMakeDirButton->SetEnabled(false);
								win->fParentDirButton->SetEnabled(false);
								win->fNewVRCDButton->SetEnabled(true);
								win->fAddISOButton->SetEnabled(true);
								win->fDataView->fBootableCDCheckBox->SetEnabled(false);
								win->fDataView->fChooseBootImageButton->SetEnabled(false);
							}

						}
						if (item != NULL)
							delete item;
					}
				}
				break;
			}
		default:
			BListView::KeyDown(bytes, numBytes);
	}
}


void LeftList::MouseDown(BPoint point)
{
	char temp1[150];
	char temp2[150];
	char temp3[150];
	char temp4[150];
	char temp5[150];
	BMessage* msg = Window()->CurrentMessage();
	uint32 clicks = msg->FindInt32("clicks");
	uint32 button = msg->FindInt32("buttons");


	if ((button == fLastButton) && (clicks > 1))
		fClickCount++;
	else
		fClickCount = 1;

	fLastButton = button;

	if ((button == B_SECONDARY_MOUSE_BUTTON)) {
		int32 itemn = IndexOf(point);
		if (itemn >= 0) {
			BMenuItem* selected;
			BPoint p = point;
			ConvertToScreen(&p);
			Select(itemn);
			selected = fTrackPopUpMenu->Go(p);
			if (selected) {
				int32 selection = CurrentSelection();
				if (!strcmp(selected->Label(), "Move Up")) {
					LeftListItem* item = (LeftListItem*)ItemAt(selection - 1);
					LeftListItem* item2 = (LeftListItem*)ItemAt(selection);
					if ((selection - 1 > 0) && (item->fIsAudio) && (item2->fIsAudio))
						SwapItems(selection, selection - 1);

				} else if (!strcmp(selected->Label(), "Move Down")) {
					LeftListItem* item = (LeftListItem*)ItemAt(selection);
					if ((selection + 1 <= CountItems()) && (item->fIconBitmap == fAudioBitmap))
						SwapItems(selection, selection + 1);

				} else if (!strcmp(selected->Label(), "Play")) {
					LeftListItem* item = (LeftListItem*)ItemAt(selection);
					if (item->fIconBitmap == fAudioBitmap)
						be_roster->Launch(&item->fRef);
				} else if (!strcmp(selected->Label(), "Remove")) {
					int32 result;
					BAlert* MyAlert = new BAlert("BurnItNow", "Are you sure you want to delete this selection", "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
					MyAlert->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
					result = MyAlert->Go();
					if (result == 0) {
						LeftListItem* item = (LeftListItem*)RemoveItem(itemn);
						if (!item->fIsAudio) {
							VRCD = false;
							ISOFILE = false;
							jpWindow* win = dynamic_cast<jpWindow*>(Window());
							if (win != NULL) {
								win->fMakeDirButton->SetEnabled(false);
								win->fParentDirButton->SetEnabled(false);
								win->fNewVRCDButton->SetEnabled(true);
								win->fAddISOButton->SetEnabled(true);
							}
						}
						if (item != NULL)
							delete item;
					}
				}
			}
			return;
		}
	}
	if ((button == B_PRIMARY_MOUSE_BUTTON) && (fClickCount == 2)) {
		int32 selection = CurrentSelection();
		if (selection >= 0) {
			LeftListItem* item = (LeftListItem*)ItemAt(selection);
			if (item->fIsAudio) {
				sprintf(temp1, "%s", item->fAudioInfo.pretty_name);
				sprintf(temp2, "%d Channels", (int)item->fAudioInfo.channels);
				sprintf(temp3, "%.1f kHz", ((float)item->fAudioInfo.frame_rate / (float)1000));
				sprintf(temp4, "%d bits", (int)item->fAudioInfo.bps * 8);
				sprintf(temp5, "Info on AudioFile");
				jpWindow* win = dynamic_cast<jpWindow*>(Window());
				if (win != NULL)
					win->fRightList->UpdateInfo(temp5, temp1, temp2, temp3, temp4);
			}
			if (item->fIconBitmap == fVRCDBitmap) {
				jpWindow* win = dynamic_cast<jpWindow*>(Window());
				if (win != NULL)
					win->fRightList->UpdateDir();
			}
			if (item->fIconBitmap == fISOBitmap) {
				sprintf(temp1, "Info on ISOFile");
				sprintf(temp2, "%s", item->fName);
				sprintf(temp3, "X Mb");
				jpWindow* win = dynamic_cast<jpWindow*>(Window());
				if (win != NULL)
					win->fRightList->UpdateInfo(temp1, temp2, temp3, NULL, NULL);
			}
		}
		fClickCount = 0;

	} else {
		BListView::MouseDown(point);
	}
}


void LeftList::WriteLog(const char* string)
{
	jpWindow* win = dynamic_cast<jpWindow*>(Window());
	if (win != NULL)
		win->PutLog(string);
}
