/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow Maintainers
 * Distributed under the terms of the MIT License.
 */

#include "RightList.h"

#include "AskName.h"
#include "ImageButton.h"
#include "WeightData.h"
#include "jpWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Path.h>
#include <Button.h>
#include <Resources.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>

#include <stdio.h>
#include <stdlib.h>

extern bool VRCD;
extern char* IMAGE_NAME;
extern char* BURNIT_PATH;
extern char* BURN_DIR;
extern char* WEIGHTS_FILE;

// This function doesn't seem to be used anywhere.

/*BBitmap* GetBitmapResource(type_code type, const char* name)
{
	size_t len = 0;
	BResources* rsrc = BApplication::AppResources();
	const void* data = rsrc->LoadResource(type, name, &len);

	if (data == NULL)
		return NULL;

	BMemoryIO stream(data, len);

	// Try to read as an archived bitmap.
	stream.Seek(0, SEEK_SET);
	BMessage archive;
	status_t err = archive.Unflatten(&stream);
	if (err != B_OK)
		return NULL;

	BBitmap* out = new BBitmap(&archive);
	if (!out)
		return NULL;

	err = (out)->InitCheck();
	if (err != B_OK) {
		delete out;
		out = NULL;
	}

	return out;
}*/


FileListItem::FileListItem(entry_ref* ref, uint32 ix, char* name, 
	BBitmap* icon)
	:
	BListItem()
{
	if (ref != NULL)
		fRef = *ref;
		
	fName = new char[strlen(name)+1];
	strcpy(fName, name);
	fIconBitmap = icon;
	fIndex = ix;
	fWeight = NULL;
}


FileListItem::~FileListItem()
{
	delete fName;
}


void
FileListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	rgb_color rgbTextColor = { 255, 255, 255, 255 };
	rgb_color rgbTextGaugeColor = { 175, 175, 175, 255 };
	rgb_color rgbSelectedColor = { 35, 35, 35, 255 };
	rgb_color rgbSelectedGaugeColor = { 0, 0, 0, 255 };
	rgb_color rgbPatternColor = { 59, 113, 218, 255 };
	
	owner->SetDrawingMode(B_OP_ALPHA);
	
	owner->SetLowColor(rgbTextColor);

	if (IsSelected()) {
		float tempRight = frame.right;
		frame.right = 34 + 1;
		
		owner->SetHighColor(rgbSelectedGaugeColor);
		owner->FillRect(frame);
		
		frame.right = tempRight;
		frame.left += 34 + 1;
		
		owner->SetHighColor(rgbSelectedColor);
		owner->FillRect(frame);
	} else if (fIndex % 2 == 0) {
		frame.left += 34 + 1;
		
		owner->SetHighColor(rgbPatternColor);
		owner->FillRect(frame);	
	}
	
	owner->SetHighColor(rgbTextGaugeColor);
	
	char indexBuffer[8];
	sprintf(indexBuffer, "%u", (unsigned int)fIndex);
	float indexWidth = owner->StringWidth(indexBuffer);
	
	owner->DrawString(indexBuffer, 
		BPoint(34 - indexWidth - 1, frame.bottom - 2));
	
	owner->SetHighColor(rgbTextColor);
	
	if (fIconBitmap != NULL)
		owner->DrawBitmap(fIconBitmap, BPoint(34 + 3, frame.top - 1));
		
	owner->MovePenTo(BPoint(34 + 16 + 3 * 2, frame.bottom - 2));
	owner->DrawString(fName);
}


RightList::RightList(BRect size)
	:
	BListView(size, "RightList", B_MULTIPLE_SELECTION_LIST, B_FOLLOW_NONE,
		B_WILL_DRAW)
{
	SetViewColor(89, 123, 228, 0);
	
	fPath = "VRCD";
	fBDirectory = new BDirectory(BURN_DIR);
	fTDirectory = new BDirectory(BURN_DIR);

	fFileBitmap = BTranslationUtils::GetBitmap('PNG ', "file.png");
	fDirectoryBitmap = BTranslationUtils::GetBitmap('PNG ', "folder.png");
	fInfoBitmap = BTranslationUtils::GetBitmap('PNG ', "info.png");
	
	fItemPopUpMenu = new BPopUpMenu("Items Popup");
	fItemPopUpMenu->SetRadioMode(false);
	//fItemPopUpMenu->AddItem(new BMenuItem("Play", new BMessage('play')));
	
	fItemMakeDir = new BMenuItem("Make Directory", new BMessage(TOOLS_DIR));
	fItemAdd = new BMenuItem("Add", new BMessage(TOOLS_ADD));
	fItemRemove = new BMenuItem("Remove", new BMessage(TOOLS_REM));
	fItemUp = new BMenuItem("Up", new BMessage(TOOLS_UP));
	fItemDown = new BMenuItem("Down", new BMessage(TOOLS_DOWN));
	
	fItemPopUpMenu->AddItem(fItemMakeDir);
	fItemPopUpMenu->AddItem(fItemAdd);
	fItemPopUpMenu->AddItem(fItemRemove);
	fItemPopUpMenu->AddSeparatorItem();
	fItemPopUpMenu->AddItem(fItemUp);
	fItemPopUpMenu->AddItem(fItemDown);
	
	fPanelAdd = new BFilePanel(B_OPEN_PANEL, NULL, NULL, 
		B_FILE_NODE | B_DIRECTORY_NODE, true, new BMessage(TOOLS_ADD_DONE));
	
	fWeights.Load(WEIGHTS_FILE);
	
	printf("[%s]\n", BURN_DIR);
}


RightList::~RightList()
{
	fWeights.Save(WEIGHTS_FILE);

	delete fBDirectory;
	delete fTDirectory;
	delete fFileBitmap;
	delete fDirectoryBitmap;
	delete fInfoBitmap;
}


void
RightList::MessageReceived(BMessage* msg)
{
	entry_ref ref;
	int32 counter = 0;

	switch (msg->what) {
		case B_SIMPLE_DATA:
			if (VRCD) {
				while (msg->FindRef("refs", counter++, &ref) == B_OK)
					MakeLink(&ref);
					
				UpdateDir();
			} else {
				BAlert* MyAlert = new BAlert("BurnItNow", 
					"You have to add a Virtual CD Directory to be able to drop files!", 
					"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
					
				MyAlert->Go();
			}
			
			break;
		
		default:
			BListView::MessageReceived(msg);
			break;
	}
}


void
RightList::AttachedToWindow()
{
	fPanelAdd->SetTarget(Window());
	
	UpdateDir();	
}


void
RightList::Draw(BRect updateRect)
{
	rgb_color rgbGauge = { 35, 75, 125, 255 };
	
	BRect gaugeRect(0, 0, 34, 999999);
	BRect gaugeUpdateRect = gaugeRect & updateRect;
	
	if (gaugeUpdateRect.IsValid()) {
		rgb_color rgbTemp = HighColor();
		SetHighColor(rgbGauge);
		FillRect(gaugeUpdateRect, B_SOLID_HIGH);
		SetHighColor(rgbTemp);	
	}
	
	BListView::Draw(updateRect);
}


void
RightList::DeleteDirFromVRCD(entry_ref* ref)
{
	BDirectory* temp_dir;
	entry_ref temp_ref;
	temp_dir = new BDirectory(ref);
	
	while (temp_dir->GetNextRef(&temp_ref) != B_ENTRY_NOT_FOUND) {
		if (BEntry(&temp_ref, true).IsDirectory()) {
			DeleteDirFromVRCD(&temp_ref);
			BEntry(&temp_ref).Remove();
		} else
			BEntry(&temp_ref).Remove();
	}
	
	delete temp_dir;
}


void
RightList::DeleteFromVRCD(entry_ref* ref)
{
	BEntry temp_entry(ref);
	
	char nameBuffer[B_PATH_NAME_LENGTH];   
	temp_entry.GetName(nameBuffer);
	
	WriteLog("Delete:");  
	WriteLog(nameBuffer);  

	if (temp_entry.IsDirectory()) {	
		DeleteDirFromVRCD(ref);
		temp_entry.Remove();
	} else
		temp_entry.Remove();
}


void
RightList::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_DELETE:
			RemoveSelected();
			break;
		
		default:
			BListView::KeyDown(bytes, numBytes);
	}
}


void
RightList::MouseDown(BPoint point)
{
	BMessage* msg = Window()->CurrentMessage();
	uint32 clicks = msg->FindInt32("clicks");
	uint32 button = msg->FindInt32("buttons");
	
	if (button == fLastButton && clicks > 1)
		fClickCount++;
	else
		fClickCount = 1;

	fLastButton = button;
	
	if (button == B_SECONDARY_MOUSE_BUTTON) {
		int32 itemn = IndexOf(point);
		
		if (itemn >= 0) {
			BMenuItem* selectedItem;
			BPoint p = point;
			
			ConvertToScreen(&p);
			
			if (!IsItemSelected(itemn))
				Select(itemn);
			
			fItemUp->SetEnabled(!IsItemSelected(0));
			fItemDown->SetEnabled(!IsItemSelected(CountItems() - 1));
			fItemRemove->SetEnabled(CurrentSelection() != -1);
			
			selectedItem = fItemPopUpMenu->Go(p);
			
			if (selectedItem != NULL)
				Window()->PostMessage(selectedItem->Message());
		}
	} else if (button == B_PRIMARY_MOUSE_BUTTON && fClickCount == 2) {
		int32 selection = CurrentSelection();
		
		if (selection >= 0) {
			FileListItem* item = (FileListItem*)ItemAt(selection);
			
			fPath += '/';
			fPath += item->fName;
			
			if (item->fIconBitmap != NULL && item->fIconBitmap == fDirectoryBitmap)
				if (fBDirectory->SetTo(&item->fRef) == B_OK)
					UpdateDir();
		}
		
		fClickCount = 0;
	} else
		BListView::MouseDown(point);
}


void
RightList::SelectionChanged()
{
	jpWindow* parent = (jpWindow*)Window();
	
	int32 selection = CurrentSelection();
	
	if (selection != -1) {
		parent->fRemoveButton->SetEnabled(true);
		parent->fMoveUpButton->SetEnabled(!IsItemSelected(0));
		parent->fMoveDownButton->SetEnabled(!IsItemSelected(CountItems() - 1));
	} else {
		parent->fRemoveButton->SetEnabled(false);
		parent->fMoveUpButton->SetEnabled(false);
		parent->fMoveDownButton->SetEnabled(false);
	}
}


void
RightList::UpdateInfo(char* str1 = NULL, char* str2 = NULL, char* str3 = NULL, char* str4 = NULL, char* str5 = NULL)
{
	MakeEmpty();
	
	if (str1 != NULL)
		AddItem(new FileListItem(NULL, 0, str1, fInfoBitmap));
	if (str2 != NULL)
		AddItem(new FileListItem(NULL, 0, str2, NULL));
	if (str3 != NULL)
		AddItem(new FileListItem(NULL, 0, str3, NULL));
	if (str4 != NULL)
		AddItem(new FileListItem(NULL, 0, str4, NULL));
	if (str5 != NULL)
		AddItem(new FileListItem(NULL, 0, str5, NULL));
}


void
RightList::UpdateDir()
{	
	BEntry temp_entry;
	entry_ref temp_ref;		
	BPath temp_path;
	
	status_t ret = B_OK;  
 	
 	MakeEmpty();

	if(fBDirectory->GetEntry(&temp_entry) == B_OK) {
		ret = fBDirectory->SetTo(&temp_entry);	
			
		if(ret != B_OK) {
			//printf(" cant  fBDirectory->SetTo(&temp_entry)\n");
			// ... ?
		}	
	} else {
		//printf(" cant fBDirectory->GetEntry(&temp_entry)\n");
		// ... ?
	}

	uint32 ix = 1;
	
	while (fBDirectory->GetNextRef(&temp_ref) != B_ENTRY_NOT_FOUND) {
		FileListItem* item;
		
		if (BEntry(&temp_ref, true).IsDirectory())
			item = new FileListItem(&temp_ref, ix, temp_ref.name, fDirectoryBitmap);
		else
			item = new FileListItem(&temp_ref, ix, temp_ref.name, fFileBitmap);
		
		item->fWeight = fWeights.MatchEntry(fPath + '/' + temp_ref.name);
		
		AddItem(item);
		
		ix++;
	}
	
	SortItems(_SortByWeightCallback);
	UpdateIndexes();
	
	temp_entry.GetPath(&temp_path);
	jpWindow* parent = (jpWindow*)Window();
	
	if (strcmp(temp_path.Path(), BURN_DIR) != 0)
		parent->fParentDirButton->SetEnabled(true);
	else
		parent->fParentDirButton->SetEnabled(false);
		
	SelectionChanged();
}


void
RightList::UpdateWeights()
{
	FileListItem** items = (FileListItem**)Items();
	BEntry temp_entry;
	BPath temp_path;

	for (int32 i = 0; i < CountItems(); i++) {
		if (items[i]->fWeight == NULL) {
			items[i]->fWeight = new WeightData::Entry;
	
			items[i]->fWeight->path = fPath;
			items[i]->fWeight->path += '/';
			items[i]->fWeight->path += items[i]->fName;
			
			fWeights.List.push_back(items[i]->fWeight);
		}
		
		items[i]->fWeight->weight = -(i + 1);
	}
}


void
RightList::UpdateIndexes()
{
	FileListItem** items = (FileListItem**)Items();
	
	for (int32 i = 0; i < CountItems(); i++)
		items[i]->fIndex = i + 1;
}


void
RightList::ParentDir()
{	
	BEntry temp_entry, temp_entry2;  
	BPath temp_path;
	
	if (fBDirectory->GetEntry(&temp_entry) == B_OK) {
		if( temp_entry.GetParent(&temp_entry2) == B_OK) {
			if (temp_entry2.GetPath(&temp_path) == B_OK) {
				if (!strncmp(temp_path.Path(), BURN_DIR, strlen(BURN_DIR))) {
					temp_entry.GetParent(fBDirectory);
					fPath = fPath.substr(0, fPath.rfind('/'));
					UpdateDir();
				}
			}
		}
	}
}


void
RightList::CreateDir()
{
	AskName* makeDirWindow = new AskName(BRect(200, 200, 500, 250), "Make directory", MAKE_DIRECTORY, "");
	makeDirWindow->Show();
}


void
RightList::MakeDir(const char* name)
{
	char temp_char[2048];
	BEntry temp_entry;
	BPath temp_path;
	
	fBDirectory->GetEntry(&temp_entry);
	temp_entry.GetPath(&temp_path);
	
	sprintf(temp_char, "%s/%s", temp_path.Path(), name);
	
	if (!BEntry(temp_char).Exists()) {
		fBDirectory->CreateDirectory(temp_char, NULL);
		UpdateDir();
	} else {
		BAlert* MyAlert = new BAlert("BurnItNow", "The directory is already exists!", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
		MyAlert->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
		MyAlert->Go();
	}
}


void
RightList::MakeDir(entry_ref* ref)
{
	entry_ref temp_ref;
	char temp_char[4096];
	
	BPath temp_path;
	BDirectory* temp_dir;
	BEntry temp_entry;
	
	temp_dir = new BDirectory(ref);
	
	while (temp_dir->GetNextRef(&temp_ref) == B_OK) {
		if (BEntry(&temp_ref, false).IsDirectory()) {
			fTDirectory->GetEntry(&temp_entry);
			temp_entry.GetPath(&temp_path);
			memset(temp_char, 0, sizeof(temp_char));
			sprintf(temp_char, "%s/%s", temp_path.Path(), temp_ref.name);
			
			if (fTDirectory->CreateDirectory(temp_char, NULL) == B_OK) {
				if (fTDirectory->SetTo(temp_char) == B_OK) {
					MakeDir(&temp_ref);
					fTDirectory->SetTo(temp_path.Path());
				}
			}
		} else {
			if (!BEntry(&temp_ref, true).IsDirectory()) {
				if (fTDirectory->GetEntry(&temp_entry) == B_OK) {
					if (temp_entry.GetPath(&temp_path) == B_OK) {
						sprintf(temp_char, "%s/%s", temp_path.Path(), temp_ref.name);
						
						if (BEntry(&temp_ref, false).GetPath(&temp_path) == B_OK)
							fTDirectory->CreateSymLink(temp_char, temp_path.Path(), NULL);
					}
				}
			} else {
				WriteLog("RightList::MakeDir -> #1");
					// Don't follow linked directorys
			}
		}
	}
	
	delete temp_dir;
}


void
RightList::MakeLink(entry_ref* ref)
{
	entry_ref temp_ref;
	char temp_char[4096];
	BPath temp_path;
	BEntry temp_entry;

	if (BEntry(ref, false).IsDirectory()) {
		fBDirectory->GetEntry(&temp_entry);
		temp_entry.GetPath(&temp_path);
		
		sprintf(temp_char, "%s/%s", temp_path.Path(), ref->name);
		
		if (fTDirectory->CreateDirectory(temp_char, NULL) == B_OK) {
			if (fTDirectory->SetTo(temp_char) == B_OK) {
				MakeDir(ref);
				fTDirectory->Rewind();
			}
		}
	} else {
		if (!BEntry(ref, true).IsDirectory()) {
			if (fBDirectory->GetEntry(&temp_entry) == B_OK) {
				if (temp_entry.GetPath(&temp_path) == B_OK) {
					sprintf(temp_char, "%s/%s", temp_path.Path(), ref->name);
					
					if (BEntry(ref, false).GetPath(&temp_path) == B_OK)
						fBDirectory->CreateSymLink(temp_char, temp_path.Path(), NULL);
				}
			}
		} else {
			WriteLog("RightList::MakeLink -> #1");
				// Don't follow linked directorys
		}
	}

}


void
RightList::WriteLog(const char* str)
{	
	jpWindow* parent = dynamic_cast<jpWindow*>(Window());	
	
	if (parent != NULL)
		parent->MessageLog(str);
}


void
RightList::Add()
{
	fPanelAdd->Show();
}


void
RightList::AddDone()
{
	entry_ref ref;
	
	while (fPanelAdd->GetNextSelectedRef(&ref) == B_OK)
		MakeLink(&ref);
		
	UpdateDir();
}


void
RightList::RemoveSelected()
{
	int32 selection;
				
	for (int32 i = 0; true; i++) {
		selection = CurrentSelection(i);
		
		if (selection == -1) 
			break;
		
		FileListItem* selectedItem = (FileListItem*)ItemAt(selection);
		DeleteFromVRCD(&selectedItem->fRef);	
		fWeights.RemoveEntries(fPath + '/' + selectedItem->fName);	
	}
	
	UpdateDir();
}


void
RightList::MoveSelectedUp()
{
	for (int32 i = 1; i < CountItems(); i++)
		if (IsItemSelected(i))
			SwapItems(i, i - 1);	
	
	UpdateWeights();
	UpdateIndexes();
}


void
RightList::MoveSelectedDown()
{
	for (int32 i = CountItems() - 1 - 1; i >= 0; i--)
		if (IsItemSelected(i))
			SwapItems(i, i + 1);	
	
	UpdateWeights();
	UpdateIndexes();
}

int
RightList::_SortByWeightCallback(const void* a, const void* b)
{
	FileListItem* itemA = *(FileListItem**)a;
	FileListItem* itemB = *(FileListItem**)b;
	
	if (itemA->fWeight == NULL || itemB->fWeight == NULL) {
		// This is required, because for some reason SortItems()
		// changes the places of items even upon 0 result.
		
		if (itemA->fIndex < itemB->fIndex)
			return -1;
		else
			return 1;
		
		return 0;
	}
	
	if (itemA->fWeight->weight < itemB->fWeight->weight)
		return 1;
		
	if (itemA->fWeight->weight > itemB->fWeight->weight)
		return -1;
	
	return 0;
}
