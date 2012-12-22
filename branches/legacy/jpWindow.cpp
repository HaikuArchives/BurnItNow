/*
 * Copyright 2000-2002, Johan Nilsson.
 * Copyright 2010-2011 BurnItNow maintainers
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "jpWindow.h"
#include "copyright.h"

#include "AboutWindow.h"
#include "AskName.h"
#include "AudioView.h"
#include "BurnView.h"
#include "CDRWView.h"
#include "CopyCDView.h"
#include "DataView.h"
#include "LeftList.h"
#include "LogView.h"
#include "MakeBFS.h"
#include "Prefs.h"
#include "PrefsView.h"
#include "RightList.h"
#include "StatusWindow.h"

#include <stdio.h>
#include <stdlib.h>


#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <File.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>


const char* BlType[] = {"all", "fast", "session", "track", "trtail", "unreserve", "unclose"};
int16 BLANK_TYPE;
int16 BLANK_SPD;
int16 BURN_SPD;
int16 BURN_TYPE;
int16 IMAGE_TYPE;
int16 SCSI_DEV;
int16 CDSIZE;

uint32 nrtracks;
float angles[100];

extern BDirectory* BFSDir;
bool VRCD;
bool ISOFILE;
bool ONTHEFLY;
bool BOOTABLE;
entry_ref BOOTIMAGEREF;

char* IMAGE_NAME;
char* BURNIT_PATH;
char* BURN_DIR;
BPath CDRTOOLS_DIR;

char BURNPROOF[30]; // driveropts = burnproof
char PAD[10]; // -pad (audio)
char DAO[10]; // -dao
char NOFIX[10]; // -nofix (audio)
char PREEMP[10]; // -preemp (audio)
char SWAB[10]; // -swab
char BOOTSTRING[128]; // -c boot.catalog -b boot.catalog/boot.img

char MULTISESSION[10];
char AUDIO_FILES[10240];
char DUMMYMODE[10];
char EJECT[10];
char ISOFILE_DIR[1024];
char DATA_STRING[100];
char VOL_NAME[25];
bool JUST_IMAGE;

bool scsibus = false;

FILE* CLInput;
thread_id OPMkImage = 0, Cntrl = 0, OPBurn = 0, OPBlank = 0;


int32 controller(void* p)
{
	char* command = (char*)p;
	CLInput = popen(command, "r");
	return 0;
}


int32 OutPutMkImage(void* p)
{
	char buffer[1024], buf[10], buf2[10], buf3[100];
	int temp, i;
	StatusWindow* SWin = (StatusWindow*)p;
	while (!feof(CLInput) && !ferror(CLInput)) {
		buffer[0] = 0;

		fgets(buffer, 1024, CLInput);
		if (buffer[6] == '%' || buffer[7] == '%' || buffer[8] == '%') {
			strncpy(&buf[0], &buffer[0], 8);

			for (i = 0; i < (int)strlen(buffer); i++) {
				if (buffer[i] == '.')
					break;
			}
			sprintf(buf3, "Making Image %s", buf);
			strncpy(&buf2[0], &buffer[1], i - 1);
			temp = atoi(buf2);
			SWin->Lock();
			SWin->UpdateStatus(temp, buf3);
			SWin->Unlock();
		}
	}

	if (!JUST_IMAGE) {
		SWin->Lock();
		SWin->SendMessage(new BMessage(BURN_WITH_CDRECORD));
		SWin->Unlock();
	} else {
		SWin->Lock();
		SWin->Ready();
		SWin->Unlock();
		sprintf(IMAGE_NAME, "%s/tmp/BurnItNow.raw", BURNIT_PATH);
	}
	if (BOOTABLE) {
		char temp[1024];
		BPath path;
		BEntry(&BOOTIMAGEREF, true).GetPath(&path);
		sprintf(temp, "%s/boot.catalog/%s", BURN_DIR, path.Leaf());
		BEntry(temp, true).Remove();
		sprintf(temp, "%s/boot.catalog", BURN_DIR);
		BEntry(temp, true).Remove();
	}
	return 0;
}


int32 OutPutBurn(void* p)
{
	bool progress_mode = false;
	char buffer[1024], buf[50];
	int done;
	int left;
	bool noerror = true;
	StatusWindow* SWin = (StatusWindow*)p;
	while (!feof(CLInput) && !ferror(CLInput)) {

		buffer[0] = 0;
		if (!progress_mode) {
			fgets(buffer, 1024, CLInput);
		} else {
			fread(buffer, 1, 33, CLInput);
			buffer[33] = 0;
			if (buffer[0] != 0x0d)
				progress_mode = false;
		}
		if (!strncmp(buffer, "Starting new", 12))
			progress_mode = true;

		if (!strncmp(buffer, "Sense Code:", 11)) {
			SWin->Lock();
			BMessage hejsan(WRITE_TO_LOG);
			SWin->StatusSetText("An error occurred when burning.");
			hejsan.AddString("Text", "An error occurred when burning, the most common problem that it's now empty or it's a damaged CD.\nTry again and if it fails file a new ticket on the OSDrawer.net BurnItNow page.");
			hejsan.AddBool("Error", true);
			SWin->SendMessage(&hejsan);
			noerror = false;
			SWin->Unlock();
			break;
		}
		if (!strncmp(buffer, "Last chance", 11)) {
			SWin->Lock();
			SWin->StatusSetText("Preparing...");
			SWin->Unlock();
		};
		if (!strncmp(&buffer[1], "Track ", 6)) {
			strncpy(&buf[0], &buffer[11], 3);
			done = strtol(buf, 0, 0);
			strncpy(&buf[0], &buffer[18], 3);
			left = strtol(buf, 0, 0);
			if (done) {
				sprintf(buf, "Burning %d of %d done", done, left);
				SWin->Lock();
				SWin->StatusSetMax((float)left);
				SWin->UpdateStatus((float)done, &buffer[1]);
				SWin->Unlock();
			}
		}
		if (!strncmp(buffer, "Fixating", 8)) {
			SWin->Lock();
			SWin->StatusSetColor(black);
			SWin->StatusUpdateReset();
			SWin->StatusSetText("Fixating...");
			SWin->Unlock();
		}

	};
	SWin->Lock();
	if (!noerror) {
		SWin->StatusSetColor(red);
		SWin->StatusUpdateReset();
		SWin->Ready();
		SWin->UpdateStatus(100, "An error occurred when burning.");
	}
	SWin->SendMessage(new BMessage(SET_BUTTONS_TRUE));
	if (noerror)
		SWin->Ready();
	SWin->Unlock();
	sprintf(IMAGE_NAME, "%s/tmp/BurnItNow.raw", BURNIT_PATH);
	BEntry(IMAGE_NAME, true).Remove();
	if (BOOTABLE) {
		char temp[1024];
		BPath path;
		BEntry(&BOOTIMAGEREF, true).GetPath(&path);
		sprintf(temp, "%s/boot.catalog/%s", BURN_DIR, path.Leaf());
		BEntry(temp, true).Remove();
		sprintf(temp, "%s/boot.catalog", BURN_DIR);
		BEntry(temp, true).Remove();
	}
	return 0;
}


int32 OutPutBlank(void* p)
{
	char buffer[1024];
	bool temp = true;
	StatusWindow* SWin = (StatusWindow*)p;

	while (!feof(CLInput) && !ferror(CLInput)) {
		buffer[0] = 0;
		fgets(buffer, 1024, CLInput);

		snooze(500000);
		SWin->Lock();
		if (temp) {
			SWin->StatusSetColor(green);
			temp = false;
		} else {
			SWin->StatusSetColor(blue);
			temp = true;
		}
		SWin->StatusUpdateReset();
		SWin->StatusSetText("Blanking...");

		SWin->Unlock();
	}
	SWin->Lock();
	SWin->Ready();
	SWin->StatusSetText("Blanking Done.");
	SWin->SendMessage(new BMessage(SET_BUTTONS_TRUE));
	SWin->Unlock();
	return 0;
}


jpWindow::jpWindow(BRect frame)
	:
	BWindow(frame, "BurnItNow", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	char temp_char[1024];
	entry_ref temp_ref;
	BRect r;
	BMenu* menu;
	SetTitle("BurnItNow");
	fBurnItPrefs = new Prefs("BurnItNow");
	// Load settings
	InitBurnIt();

	r = Bounds();
	r.top = 20;
	AddChild(fAroundBox = new BBox(r, "BoxAround", B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER));
	// Size Status and button
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = r.bottom - 39;
	r.right -= 85;
	fStatusBar = new BStatusBar(r, "fStatusBar");
	fStatusBar->SetMaxValue(100.0);
	fAroundBox->AddChild(fStatusBar);
	fStatusBar->Reset();

	r = fAroundBox->Bounds();
	r.InsetBy(10.0, 10.0);
	r.top = r.bottom - 23; 
	r.left = r.right - 70;
	fCalcSizeButton = new BButton(r, "calcsize", "Calc. Size", new BMessage(CALCULATE_SIZE));
	fAroundBox->AddChild(fCalcSizeButton);

	// Tabs
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom = 210;

	fTabView = new BTabView(r, "mytabview", B_WIDTH_FROM_LABEL);
	fTabView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	r = fTabView->Bounds();
	r.InsetBy(5.0, 5.0);
	r.bottom -= fTabView->TabHeight();
	fMiscTab = new BTab();
	fBurnView = new BurnView(r);
	fTabView->AddTab(fBurnView, fMiscTab);
	fMiscTab->SetLabel("Burn");

	fMiscTab = new BTab();
	fDataView = new DataView(r);
	fTabView->AddTab(fDataView, fMiscTab);
	fMiscTab->SetLabel("Data");

	fMiscTab = new BTab();
	fAudioView = new AudioView(r);
	fTabView->AddTab(fAudioView, fMiscTab);
	fMiscTab->SetLabel("Audio");

//	fMiscTab = new BTab();
//	fTabView->AddTab(new CopyCDView(r, this), fMiscTab);
//	fMiscTab->SetLabel("CopyCD");

	fMiscTab = new BTab();
	fCDRWView = new CDRWView(r);
	fTabView->AddTab(fCDRWView, fMiscTab);
	fMiscTab->SetLabel("CDRW");

	fMiscTab = new BTab();
	fPrefsView = new PrefsView(r);
	fTabView->AddTab(fPrefsView, fMiscTab);
	fMiscTab->SetLabel("Prefs");

	fMiscTab = new BTab();
	fLogView = new LogView(r);
	fTabView->AddTab(fLogView, fMiscTab);
	fMiscTab->SetLabel("Log");

	fAroundBox->AddChild(fTabView);

	// FileList(RightList)
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 260;
	r.bottom = r.bottom - 50;
	r.left = (r.right / 2) + 3;
	r.right -= (B_V_SCROLL_BAR_WIDTH + 3);
	fRightList = new RightList(r);

	fAroundBox->AddChild(new BScrollView("Scroll files/info", fRightList, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));

	// ParentDir button
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 220;
	r.bottom = 240;
	r.left = (r.right / 2);
	r.right -= (r.right - r.left) / 2;
	fParentDirButton = new BButton(r, "ParentDir", "ParentDir", new BMessage( PARENT_DIR ));
	fAroundBox->AddChild(fParentDirButton);

	// MakeDir button
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 220;
	r.bottom = 240;
	r.left = (r.right / 2) + 2;
	r.left += (r.right - r.left) / 2 + 2;
	r.right -= 2;
	fMakeDirButton = new BButton(r, "MakeDir", "MakeDir", new BMessage( MAKE_DIR ));
	fAroundBox->AddChild(fMakeDirButton);

	if (!VRCD) {
		fMakeDirButton->SetEnabled(false);
		fParentDirButton->SetEnabled(false);
	}

	// LeftList
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 260;
	r.bottom = r.bottom - 50;
	r.right -= ((r.right / 2) + (B_V_SCROLL_BAR_WIDTH) + 3);
	fLeftList = new LeftList(r);

	fAroundBox->AddChild(new BScrollView("Scroll LeftListr", fLeftList, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));

	// Init
	if (VRCD) {
		sprintf(temp_char, "VRCD - [%s]", VOL_NAME);
		if (BOOTABLE)
			sprintf(temp_char, "VRCD(Boot) - [%s]", VOL_NAME);
		else
			sprintf(temp_char, "VRCD - [%s]", VOL_NAME);

		fLeftList->AddItem(new LeftListItem(&temp_ref, temp_char, fLeftList->fVRCDBitmap, NULL), 0);
	}
	if (ISOFILE && strcmp(ISOFILE_DIR, "NONE")) {
		BEntry(ISOFILE_DIR).GetRef(&temp_ref);
		fLeftList->AddItem(new LeftListItem(&temp_ref, temp_ref.name, fLeftList->fISOBitmap, NULL), 0);
	}
	fCDRWView->fBlankMenu->ItemAt(BLANK_TYPE)->SetMarked(true);

	// New VRCD
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 220;
	r.bottom = 240;
	r.right = 120;
	fNewVRCDButton = new BButton(r, "New Virtual CD", "New Virtual CD", new BMessage(NEW_VRCD));
	fAroundBox->AddChild(fNewVRCDButton);

	// Open ISO
	r = fAroundBox->Bounds();
	r.InsetBy(5.0, 5.0);
	r.top = 220;
	r.bottom = 240;
	r.left = 125;
	r.right -= ((r.right / 2) + (B_V_SCROLL_BAR_WIDTH) + 2);
	fAddISOButton = new BButton(r, "Add ISOFile", "Add ISOFile", new BMessage(OPEN_ISO_FILE));
	fAroundBox->AddChild(fAddISOButton);

	// FilePanel open isofile
	fISOOpenPanel = new BFilePanel(B_OPEN_PANEL, &be_app_messenger, NULL, B_FILE_NODE, false, NULL, NULL, true, true);
	fISOSavePanel = new BFilePanel(B_SAVE_PANEL, &be_app_messenger, NULL, B_FILE_NODE, false, NULL, NULL, true, true);

	// FMenuBar
	r = Bounds();
	r.top = 0;
	r.bottom = 19;
	fMenuBar = new BMenuBar(r, "menu_bar");
	AddChild(fMenuBar);

	menu = new BMenu("File");
	menu->AddItem(new BMenuItem("About", new BMessage(MENU_FILE_ABOUT), 'A'));
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	fMenuBar->AddItem(menu);
	fMenuBar->AddItem(new BMenuItem("Help", new BMessage(MENU_HELP)));
	fRecorderCount = 0;
	fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &red);
	fLogView->fLogTextView->Insert("Welcome to BurnItNow ");
	fLogView->fLogTextView->Insert(VERSION);
	fLogView->fLogTextView->Insert("\n");	
	fLogView->fLogTextView->Insert(COPYRIGHT1);
	fLogView->fLogTextView->Insert("\n");	
	fLogView->fLogTextView->Insert(COPYRIGHT2);	
	fLogView->fLogTextView->Insert("\n\n");

	CheckForDevices();
	Show();
}


void jpWindow::AWindow()
{
	fAboutWindow = new AboutWindow();
	fAboutWindow->Show();
}


uint64 jpWindow::GetVRCDSize()
{
	char command[1024];
	char buffer[1024], buf[1024];
	uint32 i;
	uint64 tempas = 0;
	if (IMAGE_TYPE == 0) {
		FILE* f1;
		sprintf(command, "mkisofs -print-size %s %s -gui -f -V \"%s\" -C %s \"%s\" 2>&1", DATA_STRING, BOOTSTRING, VOL_NAME, fBurnDevice->scsiid, BURN_DIR);
		printf("com: %s\n",command);
		f1 = popen(command, "r");
		while (!feof(f1) && !ferror(f1)) {
			buffer[0] = 0;
			fgets(buffer, 1024, f1);	//printf("# %s",buffer);
			if (!strncmp("Total extents scheduled to be written = ", &buffer[0], 40)) {
				for (i = 0; i < strlen(&buffer[40]); i++) {
					if ((buffer[i + 40] != '\n') || (buffer[i + 41] != '\0'))
						buf[i] = buffer[i + 40];
				}
			}
		}
		pclose(f1);
		tempas = atol(buf) * 2048;
	} else if (IMAGE_TYPE == 1)
		tempas = GetBFSSize();

	return tempas;
}


void jpWindow::CalculateSize()
{
	BFile f1;
	char temp_char[1024];
	char what[100];
	uint32 angle_temp[100];
	uint32 tracks, i;
	off_t temp, total_audio, total, total_iso, total_vrcd;
	
	total_audio = 0;
	total = 0;
	total_iso = 0;
	total_vrcd = 0;
	temp = 0;	nrtracks = 0;
	
	if (BURN_TYPE == 0) {
		sprintf(what, "DataCD");
		total_vrcd = total = total_iso = total_audio = 0;
		fStatusBar->SetBarColor(blue);
		if (fLeftList->CountItems() > 0) {
			LeftListItem* item1 = (LeftListItem*)fLeftList->ItemAt(0);
			if (item1->fIconBitmap == fLeftList->fISOBitmap) {
				f1.SetTo(&item1->fRef, B_READ_ONLY);
				f1.GetSize(&temp);
				total_iso += temp;
				angle_temp[0] = temp / 1024 / 1024;
				nrtracks = 1;
			} else if (item1->fIconBitmap == fLeftList->fVRCDBitmap) {
				nrtracks = 1;
				total_vrcd = temp = GetVRCDSize();
				angle_temp[0] = temp / 1024 / 1024;
			}
		}
	} else if (BURN_TYPE == 1) {
		sprintf(what, "AudioCD");
		total_vrcd = total = total_iso = total_audio = temp = 0;
		fStatusBar->SetBarColor(green);
		if (fLeftList->CountItems() > 0) {
			LeftListItem* item1 = (LeftListItem*)fLeftList->ItemAt(0);
			LeftListItem* item2 = (LeftListItem*)fLeftList->ItemAt(1);
			if (item1->fIconBitmap == fLeftList->fAudioBitmap) {
				tracks = fLeftList->CountItems();
				for (i = 0; i < tracks; i++) {
					item1 = (LeftListItem*)fLeftList->ItemAt(i);
					f1.SetTo(&item1->fRef, B_READ_ONLY);
					f1.GetSize(&temp);
					total_audio += temp;
					angle_temp[i] = temp / 1024 / 1024;
					nrtracks++;
				}
			} else if (fLeftList->CountItems() > 1) {
				if (item2->fIconBitmap == fLeftList->fAudioBitmap) {
					tracks = fLeftList->CountItems();
					for (i = 1; i < tracks; i++) {
						item1 = (LeftListItem*)fLeftList->ItemAt(i);
						f1.SetTo(&item1->fRef, B_READ_ONLY);
						f1.GetSize(&temp);
						total_audio += temp;
						angle_temp[i - 1] = temp / 1024 / 1024;
						nrtracks++;
					}
				}
			}
		}
	} else if (BURN_TYPE == 2) {
		sprintf(what, "MixCD");
		total_vrcd = total = total_iso = total_audio = 0;
		fStatusBar->SetBarColor(greenblue);
		if (fLeftList->CountItems() > 0) {
			LeftListItem* item1 = (LeftListItem*)fLeftList->ItemAt(0);
			LeftListItem* item2 = (LeftListItem*)fLeftList->ItemAt(1);
			if (item1->fIconBitmap == fLeftList->fISOBitmap) {
				nrtracks = 1;
				f1.SetTo(&item1->fRef, B_READ_ONLY);
				f1.GetSize(&temp);
				total_iso = temp;
				angle_temp[nrtracks - 1] = temp / 1024 / 1024;
			} else if (item1->fIconBitmap == fLeftList->fVRCDBitmap) {
				nrtracks = 1;
				total_vrcd = temp = GetVRCDSize();
				angle_temp[nrtracks - 1] = temp / 1024 / 1024;
			}

			if (item1->fIconBitmap == fLeftList->fAudioBitmap) {
				tracks = fLeftList->CountItems();
				for (i = 0; i < tracks; i++) {
					item1 = (LeftListItem*)fLeftList->ItemAt(i);
					f1.SetTo(&item1->fRef, B_READ_ONLY);
					f1.GetSize(&temp);
					total_audio += temp;
					nrtracks++;
					angle_temp[nrtracks - 1] = temp / 1024 / 1024;

				}
			}

			else if (fLeftList->CountItems() > 1)
				if (item2->fIconBitmap == fLeftList->fAudioBitmap) {
					tracks = fLeftList->CountItems();
					for (i = 1; i < tracks; i++) {
						item1 = (LeftListItem*)fLeftList->ItemAt(i);
						f1.SetTo(&item1->fRef, B_READ_ONLY);
						f1.GetSize(&temp);
						total_audio += temp;
						nrtracks++;
						angle_temp[nrtracks - 1] = temp / 1024 / 1024;
					}
				}
		}
	}

	total = total_audio + total_iso + total_vrcd;
	sprintf(temp_char, "%s - [%lld of 650 Mb]", what, total / 1024 / 1024);
	fStatusBar->Reset();
	fStatusBar->SetMaxValue(CDSIZE);
	fStatusBar->Update((total / 1024 / 1024), temp_char);

	for (i = 0; i < nrtracks; i++) {
		if (i == 0)
			angles[i] = ((float)360 / (float)(total / 1024 / 1024)) * (float)angle_temp[i];
		else
			angles[i] = ((float)360 / (float)(total / 1024 / 1024)) * (float)angle_temp[i] + angles[i - 1];
	}
}


void jpWindow::InitBurnIt()
{	
	const char* tr;
	char temp_char[1024];
	app_info info;
	BPath path;

	be_app->GetAppInfo(&info);
	entry_ref ref = info.ref;
	BEntry(&ref, true).GetPath(&path);
	path.GetParent(&path);
	BURNIT_PATH = new char[strlen(path.Path())+1];
	strcpy(BURNIT_PATH, path.Path());
	sprintf(temp_char, "%s/tmp", BURNIT_PATH);
	if (!BEntry(temp_char).Exists())
		BDirectory(BURNIT_PATH).CreateDirectory(temp_char, NULL);
	IMAGE_NAME = new char[1024];
	sprintf(IMAGE_NAME, "%s/BurnItNow.raw", temp_char);
	sprintf(temp_char, "%s/VRCD", temp_char);
	if (!BEntry(temp_char).Exists())
		BDirectory(temp_char).CreateDirectory(temp_char, NULL);

	BURN_DIR = new char[strlen(temp_char)+1];
	strcpy(BURN_DIR, temp_char);

	FindCDRTools();

	// Load from pref file
	if (fBurnItPrefs->FindString("ISOFILE_DIR", &tr) == B_OK)
		strcpy(ISOFILE_DIR, tr);
	else
		strcpy(ISOFILE_DIR, "NONE");

	if (fBurnItPrefs->FindString("VOL_NAME", &tr) == B_OK)
		strcpy(VOL_NAME, tr);
	else
		strcpy(VOL_NAME, "BurnItNow");

	if (fBurnItPrefs->FindBool("VRCD", &VRCD) != B_OK)
		VRCD = false;

	if (fBurnItPrefs->FindBool("ISOFILE", &ISOFILE) != B_OK)
		ISOFILE = false;

	if (fBurnItPrefs->FindInt16("BURN_SPD", &BURN_SPD) != B_OK)
		BURN_SPD = 2;

	if (fBurnItPrefs->FindInt16("BLANK_TYPE", &BLANK_TYPE) != B_OK)
		BLANK_TYPE = 1;

	if (fBurnItPrefs->FindInt16("BLANK_SPD", &BLANK_SPD) != B_OK)
		BLANK_SPD = 1;

	if (fBurnItPrefs->FindInt16("BURN_TYPE", &BURN_TYPE) != B_OK)
		BURN_TYPE = 0;

	if (fBurnItPrefs->FindBool("ONTHEFLY", &ONTHEFLY) != B_OK)
		ONTHEFLY = false;

	if (fBurnItPrefs->FindString("MULTISESSION", &tr) == B_OK)
		strcpy(MULTISESSION, tr);
	else
		strcpy(MULTISESSION, " ");

	if (fBurnItPrefs->FindString("DUMMYMODE", &tr) == B_OK)
		strcpy(DUMMYMODE, tr);
	else
		strcpy(DUMMYMODE, " ");

	if (fBurnItPrefs->FindString("EJECT", &tr) == B_OK)
		strcpy(EJECT, tr);
	else
		strcpy(EJECT, " ");

	if (fBurnItPrefs->FindString("DATA_STRING", &tr) == B_OK)
		strcpy(DATA_STRING, tr);
	else
		strcpy(DATA_STRING, " ");

	if (fBurnItPrefs->FindInt16("IMAGE_TYPE", &IMAGE_TYPE) != B_OK)
		IMAGE_TYPE = 0;

	if (fBurnItPrefs->FindInt16("SCSI_DEV", &SCSI_DEV) != B_OK)
		SCSI_DEV = -1;

	if (fBurnItPrefs->FindString("PAD", &tr) == B_OK)
		strcpy(PAD, tr);
	else
		strcpy(PAD, "-pad");

	if (fBurnItPrefs->FindString("DAO", &tr) == B_OK)
		strcpy(DAO, tr);
	else
		strcpy(DAO, " ");

	if (fBurnItPrefs->FindString("BURNPROOF", &tr) == B_OK)
		strcpy(BURNPROOF, tr);
	else
		strcpy(BURNPROOF, " ");

	if (fBurnItPrefs->FindString("NOFIX", &tr) == B_OK)
		strcpy(NOFIX, tr);
	else
		strcpy(NOFIX, " ");

	if (fBurnItPrefs->FindString("PREEMP", &tr) == B_OK)
		strcpy(PREEMP, tr);
	else
		strcpy(PREEMP, " ");

	if (fBurnItPrefs->FindString("SWAB", &tr) == B_OK)
		strcpy(SWAB, tr);
	else
		strcpy(SWAB, " ");

	if (fBurnItPrefs->FindRef("BOOTIMAGEREF", &BOOTIMAGEREF) == B_OK)
		if (BEntry(&BOOTIMAGEREF, true).Exists())
			fBurnItPrefs->FindBool("BOOTABLE", &BOOTABLE);

	// End loading from pref file

	JUST_IMAGE = false;
	CDSIZE = 650;
}


void jpWindow::FindCDRTools()
{	
	BPath path;
	BEntry entry;

	fStatus = find_directory(B_COMMON_BIN_DIRECTORY, &CDRTOOLS_DIR);
	if (fStatus != B_OK)
		return;

	path.SetTo(CDRTOOLS_DIR.Path());
    entry.GetPath(&path);
    path.Append("cdrecord");
    if (entry.Exists() != B_OK)
        printf("Error: cdrecord not found, pathname: %s\n", path.Path());
        
    return;
}


void jpWindow::CheckForDevices()
{	
	bool got_it;
	char command[2048];
	char buffer[1024], buf[512];
	FILE* f;
	int i, k, l, m;
	int msg;
	int pa=0,pb=0,pc=0,px=0;
	char awbuf[50];
	char sidbuf[8];
	char sivbuf[20];
	char sinbuf[50];
	//see struct cdrecorder { char scsiid[7]; char scsi_vendor[20]; char scsi_name[50];};
	
	BString commandstr;
	BEntry cdrecord, mkisofs;
	BPath path1, path2;

	path1.SetTo(CDRTOOLS_DIR.Path());	path1.Append("cdrecord");
	cdrecord.SetTo(path1.Path());
	path2.SetTo(CDRTOOLS_DIR.Path());	path2.Append("mkisofs");
	mkisofs.SetTo(path2.Path());
	
	got_it = false;
	
	commandstr.SetTo(CDRTOOLS_DIR.Path());
	commandstr.Append("/cdrecord -scanbus");	
	strcpy(command, commandstr.String());		
	
	if (cdrecord.Exists() && mkisofs.Exists()) {
		if (fRecorderCount < 100) {
			Lock();
			f = popen(command, "r");
			while (!feof(f) && !ferror(f)) {
				buffer[0] = 0;
				fgets(buffer, 1024, f);
				if (!strncmp(buffer, "scsibus", 7)) scsibus = true;

				for (i = 0; i < (int)strlen(buffer); i++) 
				{	if (buffer[i] >= '0' && buffer[i] <= '9') {
						if (buffer[i + 1] == ',' || buffer[i + 2] == ',') 
						{	got_it = true;
							break;
						}
					}
				}
				
				if (got_it) {
					k= strspn(buffer,"\t 1234567890,)");
					m= strspn(buffer,"\t ,)");

					if (buffer[k] == '\'') {
						l= (int)strlen(buffer);
						buffer[l-1]='\0';	//	printf("[buffer] '%s' \n",buffer);
						pa=0;
						pb=0;
						pc=0;
						px=0;
						memset(sidbuf, 0, 7 );
						memset(sivbuf, 0, 20 );
						memset(sinbuf, 0, 50 );
						
						for (pa = 0; pa < l ; pa++ ) {	
							if (pb == 0 && buffer[pa] == '\'')  
								pb=pa+1;
							if (pb >= 1 && buffer[pa] == '\'') {	
								pc=pa;	
								if(pc > pb) {
									memset(awbuf, 0,  50 );
									memcpy(awbuf, &buffer[pb], (pc-pb) ); 
									pb=0;
									pc=0;
									if( px == 0 )	{ 
										awbuf[7 -1]='\0';  
										strncpy(sidbuf,awbuf, 7);  
									}
									if( px == 1 )	{ 
										awbuf[20-1]='\0';
										strncpy(sivbuf,awbuf ,20);
									}								
									if( px == 2 )	{
										awbuf[50-1]='\0';
										strncpy(sinbuf,awbuf, 50);
									}
									px++;
								}
							}
						}
						sidbuf[7]='\0';
						sivbuf[20]='\0';
						sinbuf[50]='\0';
						//printf("[len=%02ld] '%s' \n",strlen(sidbuf),sidbuf );
						//printf("[len=%02ld] '%s' \n",strlen(sivbuf),sivbuf );
						//printf("[len=%02ld] '%s' \n",strlen(sinbuf),sinbuf );
									
												
						fRecorderCount++;
						memset((char*)&fScsiDevices[fRecorderCount - 1].scsiid[0], 0,  7 );
						/* strncpy(fScsiDevices[fRecorderCount - 1].scsiid, &buffer[m], 7);
						printf("[len=%02ld] scsiid: '%s' \n",strlen(fScsiDevices[fRecorderCount - 1].scsiid)
										,fScsiDevices[fRecorderCount - 1].scsiid);  */
										
						strncpy(sidbuf, &buffer[m], 7);	
						m= strspn(sidbuf,",1234567890"); if(m > 0) { sidbuf[m]='\0';}	
						strncpy(fScsiDevices[fRecorderCount - 1].scsiid, sidbuf, 7);
						printf("[len=%02ld] sidbuf: '%s' \n",strlen(sidbuf),sidbuf);
						
						// scsi vendor
						memset((char*)&fScsiDevices[fRecorderCount - 1].scsi_vendor[0], 0, 20);
						strncpy(fScsiDevices[fRecorderCount - 1].scsi_vendor, sivbuf, 20);

						// scsi name
						memset((char*)&fScsiDevices[fRecorderCount - 1].scsi_name[0], 0, 50);
						strncpy(fScsiDevices[fRecorderCount - 1].scsi_name, sinbuf , 50);
					
						msg = 'dev\0' | fRecorderCount;

						strcpy(buf,sivbuf); strcat(buf," "); 
						strcat(buf,sinbuf); strcat(buf," \0");
						fPrefsView->fRecordersMenu->AddItem(new BMenuItem(buf, new BMessage(msg)));
					}
				}
			}
			pclose(f);

			fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &blue);
			
			if(fRecorderCount == 1)
				sprintf(buf, "Found %d device.\n\n", fRecorderCount); 
			else 
				sprintf(buf, "Found %d devices.\n\n", fRecorderCount);
 			fLogView->fLogTextView->Insert(buf);
 			if (SCSI_DEV != -1) {
 				fBurnDevice = &fScsiDevices[SCSI_DEV - 1];
 				fPrefsView->fRecordersMenu->ItemAt(SCSI_DEV - 1)->SetMarked(true);
 			} else
				   fBurnDevice = NULL;
			Unlock();
		}
	} else {
		Lock();
		fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &blue);
		fLogView->fLogTextView->Insert("Cound not find cdrecord/mkisofs check that it is installed and restart BurnItNow");
		Unlock();
		BAlert* MyAlert = new BAlert("BurnItNow", "Could not find cdrecord/mkisofs. You need to install the cdrtools OptionalPackage", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);

		MyAlert->Go();
		fBurnView->SetButton(false);
		fCDRWView->fBlankButton->SetEnabled(false);
	}
}


int jpWindow::CheckMulti(char* str)
{	
	FILE* f;
	int temp;
	char buf[1024];
	
	BString commandstr;

	commandstr.SetTo(CDRTOOLS_DIR.Path());
	commandstr.Append("/");
	commandstr.Append("cdrecord -msinfo dev=11,0,0 2>&1");
	printf("com: '%s'",commandstr.String());
	
	f = popen(commandstr.String(), "r");
	while (!feof(f) && !ferror(f)) {
		buf[0] = 0;
		fgets(buf, 1024, f);
		if (buf[0] == '/') {
			temp = strlen(buf) - strlen("Cannot read first writable address\n");
			if (!strncmp(&buf[temp], "Cannot read first writable address", strlen("Cannot read first writable address")))
				return -1;

			temp = strlen(buf) - strlen("Cannot read session offset\n");
			if (!strncmp(&buf[temp], "Cannot read session offset", strlen("Cannot read session offset")))
				return 0;

		}
		if (buf[0] >= '0' && buf[0] <= '9') {
			strncpy(str, buf, strlen(buf) - 1);
			return 1;
		}
	}
	return 1;
}


void jpWindow::BurnNOW()
{	
	char buf[1024];
	int temp;
	temp = 0;
	if (fBurnDevice == NULL) {
		fTabView->Select(5);
		BAlert* MyAlert = new BAlert("BurnItNow", "You have to select device to be able to burn.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		MyAlert->Go();
	} else {
		// DataCD
		if (BURN_TYPE == 0) {
			if (VRCD && !ISOFILE) {
				if (strcmp(BURN_DIR, "NONE")) {
					if (fBurnView->fOnTheFlyCheckBox->Value() == 1) {
						ONTHEFLY = true;
						BurnWithCDRecord();
					} else {
						if (fBurnView->fMultiCheckBox->Value() == 1)
							temp = CheckMulti(buf);

						if (temp != -1)
							MakeImageNOW(temp, buf);
						else {
							BAlert* MyAlert = new BAlert("BurnItNow", "Put a blank CD or a CD that you have burned multisession on before.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
							MyAlert->Go();
						}
					}
				} else {
					BAlert* MyAlert = new BAlert("BurnItNow", "This is a BUG. Please check the OSDrawer.net BurnItNow project issues page to see if this have already been reported, if not then file a new issue, noting that it is \"BUG #13\"", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					MyAlert->Go();
				}

			} else if (!VRCD && ISOFILE) {
				if (strcmp(ISOFILE_DIR, "NONE")) {
					strcpy(IMAGE_NAME, ISOFILE_DIR);
					BurnWithCDRecord();
				} else {
					BAlert* MyAlert = new BAlert("BurnItNow", "You have to choose an ISO file.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					MyAlert->Go();
				}
			} else {
				BAlert* MyAlert = new BAlert("BurnItNow", "You have to Add an \"Virtual CD Directory\" or an ISOfile to be able to burn a DataCD.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				MyAlert->Go();
			}
		}

		// AudioCD
		else if (BURN_TYPE == 1) {
			BPath temp_path;
			int32 count;
			int32 i;
			count = fLeftList->CountItems();
			for (i = 0; i < count; i++) {
				LeftListItem* item = (LeftListItem*)fLeftList->ItemAt(i);
				if (item->fIsAudio) {
					BEntry(&item->fRef).GetPath(&temp_path);
					sprintf(AUDIO_FILES, "%s \"%s\"", AUDIO_FILES , temp_path.Path());
				}
			}
			BurnWithCDRecord();
		}

		// MixCD
		if (BURN_TYPE == 2) {
			BPath temp_path;
			int32 count;
			int32 i;
			count = fLeftList->CountItems();
			for (i = 0; i < count; i++) {
				LeftListItem* item = (LeftListItem*)fLeftList->ItemAt(i);
				if (item->fIsAudio) {
					BEntry(&item->fRef).GetPath(&temp_path);
					sprintf(AUDIO_FILES, "%s \"%s\"", AUDIO_FILES , temp_path.Path());
				}
			}
			if (VRCD && !ISOFILE) {
				if (strcmp(BURN_DIR, "NONE")) {
					if (fBurnView->fOnTheFlyCheckBox->Value() == 1) {
						ONTHEFLY = true;
						BurnWithCDRecord();
					} else {
						if (fBurnView->fMultiCheckBox->Value() == 1)
							temp = CheckMulti(buf);

						if (temp != -1)
							MakeImageNOW(temp, buf);
						else {
							BAlert* MyAlert = new BAlert("BurnItNow", "Put a blank CD or a CD that you have burned multisession on before.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
							MyAlert->Go();
						}
					}
				} else {
					BAlert* MyAlert = new BAlert("BurnItNow", "This is a BUG. Please check the OSDrawer.net BurnItNow project issues page to see if this have already been reported, if not then file a new issue, noting that it is \"BUG #13\"", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					MyAlert->Go();
				}

			} else if (!VRCD && ISOFILE) {
				if (strcmp(ISOFILE_DIR, "NONE")) {
					strcpy(IMAGE_NAME, ISOFILE_DIR);
					BurnWithCDRecord();
				} else {
					BAlert* MyAlert = new BAlert("BurnItNow", "You have to choose an ISO file.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					MyAlert->Go();
				}
			} else {
				BAlert* MyAlert = new BAlert("BurnItNow", "You have to add a Virtual CD Directory/ISOfile to be able to burn a MixCD.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				MyAlert->Go();
			}
		}
	}
}


void jpWindow::BlankNOW()
{	
	int Result;
	
	BString commandstr;

	commandstr.SetTo(CDRTOOLS_DIR.Path());
	commandstr.Append("/");
	
	if (fBurnDevice == NULL) {
		fTabView->Select(5);
		BAlert* MyAlert = new BAlert("BurnItNow", "You have to select device to be able to blank.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		MyAlert->Go();
	} else {
		BAlert* MyAlert = new BAlert("Put in a CDRW", "Put in a CDRW", "Cancel", "Ok", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		Result = MyAlert->Go();
		if (Result) {
			CalculateSize();
			fStatusWindow = new StatusWindow(VOL_NAME);
			fStatusWindow->SetAngles(angles, 0);
			fStatusWindow->Show();

			SetButtons(false);
			char command[2000];
			commandstr << "cdrecord dev=" << fBurnDevice->scsiid << " speed=" << BLANK_SPD ;
			commandstr << " blank=" << BlType[BLANK_TYPE];
			commandstr << " -V";

			printf("BlankNOW: '%s'\n",commandstr.String());

			Lock();
			strcpy (command, commandstr.String());
			resume_thread(Cntrl = spawn_thread(controller, "Blanking", 15, command));
			snooze(500000);
			resume_thread(OPBlank = spawn_thread(OutPutBlank, "OutPutBlank", 15, fStatusWindow));
			snooze(500000);

			MessageLog("Blanking done");
			Unlock();
		}
	}
}


void jpWindow::PutLog(const char* string)
{
	font_height fh;  fLogView->fLogTextView->GetFontHeight( &fh );
	Lock();
	fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &red2);
	fLogView->fLogTextView->Insert("*******\n");
	fLogView->fLogTextView->Insert(string);
	fLogView->fLogTextView->Insert("\n*******\n");
	//fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &black);
	//fLogView->fLogTextView->ScrollToOffset(fLogView->fLogTextView->CountLines() * 100);
	fLogView->fLogTextView->ScrollBy( 0, fh.ascent );
	Unlock();
}


void jpWindow::MessageLog(const char* string)
{	
	font_height fh;  fLogView->fLogTextView->GetFontHeight( &fh );	//  ascent,descent,leading;
	//float fv=10.0;  fv=  fh.ascent ;				// printf("MessageLog(Size %5.2f)\n",fv);
	Lock();
	fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &green);
	fLogView->fLogTextView->Insert(string);
	fLogView->fLogTextView->Insert("\n");
	//fLogView->fLogTextView->SetFontAndColor(0, 0, be_plain_font, B_FONT_ALL, &black);
	//fLogView->fLogTextView->ScrollToOffset(fLogView->fLogTextView->CountLines() * 100);
	//fLogView->fLogTextView->ScrollToOffset(fLogView->fLogTextView->CountLines() );
	fLogView->fLogTextView->ScrollBy( 0, fh.ascent );
	Unlock();
}


void jpWindow::MessageReceived(BMessage* message)
{
	entry_ref temp_ref;
	int temp;
	char buf[30];
	uint8 count;
	switch (message->what) {
		case MENU_HELP: {
				BPath path;
				BEntry helpfile;
				
				path.SetTo(BURNIT_PATH);
				path.Append("Docs/BurnItNowHelp.html");
				helpfile.SetTo(path.Path());
				char *command = new char[strlen(path.Path())+1];
				sprintf(command, path.Path());
				if (helpfile.Exists()) {
					be_roster->Launch("text/html", 1, & command);
				} else {
					printf("\nError: Can't find helpfile: Docs/BurnItNowHelp.html");
				}
				delete command;
			}
			break;

		case CALCULATE_SIZE: {
				if (VRCD && !ISOFILE) {
					BAlert* MyAlert = new BAlert("BurnItNow", "This can take a moment because you have a Virtual CD Directory\nSo dont kill BurnItNow because it doesn't answer, it's just calculating the size.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
					MyAlert->Go();
				}

				CalculateSize();
			}
			break;

		case BURN_WITH_CDRECORD:
			BurnWithCDRecord();
			break;

		case WRITE_TO_LOG: {
				char* temp_str;
				bool temp_bool;
				message->FindString("Text", (const char**)&temp_str);
				message->FindBool("Error", &temp_bool);
				if (temp_bool)
					PutLog(temp_str);
				else
					MessageLog(temp_str);
			}
			break;

		case SET_BUTTONS_TRUE:
			SetButtons(true);
			break;

		case DATA_VIRTUALCD:
			VRCD = true;
			ISOFILE = false;
			fDataView->fBootableCDCheckBox->SetEnabled(true);
			if (fDataView->fBootableCDCheckBox->Value() == 1)
				fDataView->fChooseBootImageButton->SetEnabled(true);
			break;

		case DATA_ISOFILE:
			VRCD = false;
			ISOFILE = true;
			fDataView->fBootableCDCheckBox->SetEnabled(false);
			fDataView->fChooseBootImageButton->SetEnabled(false);
			break;

		case DATA_ISO9660:
			IMAGE_TYPE = 0;
			sprintf(DATA_STRING, " ");
			if (BURN_TYPE == 0 || BURN_TYPE == 2) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(false);
			}
			if (BURN_TYPE == 1)
				fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case DATA_BFS:
			IMAGE_TYPE = 1;
			fDataView->fBootableCDCheckBox->SetEnabled(false);
			fDataView->fChooseBootImageButton->SetEnabled(false);
			fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
			fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case DATA_HFS:
			sprintf(DATA_STRING, "-hfs");
			IMAGE_TYPE = 0;
			if (BURN_TYPE == 0 || BURN_TYPE == 2) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(false);
			}
			if (BURN_TYPE == 1)
				fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case DATA_JOLIET:
			sprintf(DATA_STRING, "-l -D -J");
			IMAGE_TYPE = 0;
			if (BURN_TYPE == 0 || BURN_TYPE == 2) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(false);
			}
			if (BURN_TYPE == 1)
				fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case DATA_ROCK:
			sprintf(DATA_STRING, "-l -L -r");
			IMAGE_TYPE = 0;
			if (BURN_TYPE == 0 || BURN_TYPE == 2) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(false);
			}
			if (BURN_TYPE == 1)
				fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case DATA_WINDOWS:
			sprintf(DATA_STRING, "-D -l");
			IMAGE_TYPE = 0;
			if (BURN_TYPE == 0 || BURN_TYPE == 2) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(false);
			}
			if (BURN_TYPE == 1)
				fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case DATA_REALROCK:
			sprintf(DATA_STRING, "-l -L -R");
			IMAGE_TYPE = 0;
			if (BURN_TYPE == 0 || BURN_TYPE == 2) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(false);
			}
			if (BURN_TYPE == 1)
				fBurnView->fMultiCheckBox->SetEnabled(false);
			break;

		case BURN_DATA_CD:
			BURN_TYPE = 0;
			if (fDataView->fBeOSRadio->Value() != 1) {
				if (fBurnView->fOnTheFlyCheckBox->Value() != 1)
					fBurnView->fMultiCheckBox->SetEnabled(true);

				fBurnView->fOnTheFlyCheckBox->SetEnabled(true);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(true);
			}
			break;

		case BURN_AUDIO_CD:
			BURN_TYPE = 1;
			fBurnView->fOnTheFlyCheckBox->SetEnabled(false);
			fBurnView->fMultiCheckBox->SetEnabled(false);
			fDataView->fBootableCDCheckBox->SetEnabled(false);
			fDataView->fChooseBootImageButton->SetEnabled(false);
			break;

		case BURN_MIX_CD:
			BURN_TYPE = 2;
			if (fDataView->fBeOSRadio->Value() != 1) {
				fBurnView->fOnTheFlyCheckBox->SetEnabled(true);
				fBurnView->fMultiCheckBox->SetEnabled(false);
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					fDataView->fChooseBootImageButton->SetEnabled(true);
			}
			break;

		case AUDIO_PAD:
			if (fAudioView->fPadCheckBox->Value() == 1)
				strcpy(PAD, "-pad");
			else
				strcpy(PAD, " ");
			break;

		case AUDIO_SWAB:
			if (fAudioView->fSwabCheckBox->Value() == 1)
				strcpy(SWAB, "-swab");
			else
				strcpy(SWAB, " ");
			break;

		case AUDIO_NOFIX:
			if (fAudioView->fNoFixCheckBox->Value() == 1)
				strcpy(NOFIX, "-nofix");
			else
				strcpy(NOFIX, " ");
			break;

		case AUDIO_PREEMP:
			if (fAudioView->fPreEmpCheckBox->Value() == 1)
				strcpy(PREEMP, "-preemp");
			else
				strcpy(PREEMP, " ");
			break;

		case BURN_MULTI:
			if (fBurnView->fMultiCheckBox->Value() == 1)
				strcpy(MULTISESSION, "-multi");
			else
				strcpy(MULTISESSION, " ");
			break;

		case BURN_DUMMY_MODE:
			if (fBurnView->fDummyModeCheckBox->Value() == 1)
				strcpy(DUMMYMODE, "-dummy");
			else
				strcpy(DUMMYMODE, " ");
			break;

		case BURN_ONTHEFLY:
			if (fBurnView->fOnTheFlyCheckBox->Value() == 1) {
				fBurnView->fMultiCheckBox->SetEnabled(false);
				ONTHEFLY = true;
			} else {
				if (BURN_TYPE == 0)
					fBurnView->fMultiCheckBox->SetEnabled(true);
				ONTHEFLY = false;
			}
			break;

		case BURN_EJECT:
			if (fBurnView->fEjectCheckBox->Value() == 1)
				strcpy(EJECT, "-eject");
			else
				strcpy(EJECT, " ");
			break;

		case MISC_DAO:
			if (fPrefsView->fDAOCheckBox->Value() == 1)
				strcpy(DAO, "-dao");
			else
				strcpy(DAO, " ");
			break;

		case MISC_BURNPROOF:
			if (fPrefsView->fBurnProofCheckBox->Value() == 1)
				strcpy(BURNPROOF, "driveropts = burnproof");
			else
				strcpy(BURNPROOF, " ");
			break;

		case MENU_FILE_ABOUT:
			AWindow();
			break;

		case OPEN_ISO_FILE:
			fISOOpenPanel->Show();
			break;

		case MAKE_IMAGE:
			BurnNOW();
			break;

		case MAKE_AND_SAVE_IMAGE:
			if (VRCD && !ISOFILE)
				fISOSavePanel->Show();
			else {
				BAlert* MyAlert = new BAlert("BurnItNow", "You have to Add an \"Virtual CD Directory\" able to make an ISOfile.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				MyAlert->Go();
			}
			break;

		case B_SAVE_REQUESTED: {
				entry_ref tempas_ref;
				const char* name;
				message->FindRef("directory", &tempas_ref);
				message->FindString("name", &name);
				sprintf(IMAGE_NAME, "%s/%s", BPath(&tempas_ref).Path(), name);
				JUST_IMAGE = true;
				MakeImageNOW(0, "");
			}
			break;

		case BLANK_IT_NOW:
			BlankNOW();
			break;

		case BLANK_FULL:
			BLANK_TYPE = 0;
			break;

		case BLANK_FAST:
			BLANK_TYPE = 1;
			break;

		case BLANK_SESSION:
			BLANK_TYPE = 2;
			break;

		case BLANK_TRACK:
			BLANK_TYPE = 3;
			break;

		case BLANK_TRACK_TAIL:
			BLANK_TYPE = 4;
			break;

		case BLANK_UNRES:
			BLANK_TYPE = 5;
			break;

		case BLANK_UNCLOSE:
			BLANK_TYPE = 6;
			break;

		case SPEED_CHANGE:
			temp = (int)fBurnView->fSpeedSlider->Value();
			BURN_SPD = (int)(temp + 1) * 2;
			sprintf(buf, "Burning Speed [%dx]", BURN_SPD);
			fBurnView->fSpeedSlider->SetLabel(buf);
			break;

		case BLANK_SPEED_CHANGE:
			temp = (int)fCDRWView->fBlankSpeedSlider->Value();
			BLANK_SPD = (int)(temp + 1) * 2;
			sprintf(buf, "Blank Speed [%dx]", BLANK_SPD);
			fCDRWView->fBlankSpeedSlider->SetLabel(buf);
			break;

		case PARENT_DIR:
			fRightList->ParentDir();
			break;

		case MAKE_DIR:
			fRightList->CreateDir();
			break;

		case MAKE_DIRECTORY:
			const char* temp_char;
			message->FindString("DirName", &temp_char);
			if (strcmp(temp_char, "boot.catalog"))
				fRightList->MakeDir(temp_char);
			else {
				BAlert* MyAlert = new BAlert("BurnItNow", "You cannot create a directory named boot.catalog,\nit is reserved for making bootable CDs.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
				MyAlert->Go();
			}
			break;

		case NEW_VRCD:
			if (!ISOFILE && !VRCD) {
				fLeftList->AddItem(new LeftListItem(&temp_ref, "VRCD - []", fLeftList->fVRCDBitmap, NULL), 0);
				ISOFILE = false;
				VRCD = true;
				fNewVRCDButton->SetEnabled(false);
				fAddISOButton->SetEnabled(false);
				fMakeDirButton->SetEnabled(true);
				fParentDirButton->SetEnabled(true);

				fVolumeNameWindow = new AskName(BRect(200, 200, 440, 240), "Volume name", VOLUME_NAME, VOL_NAME);
				fVolumeNameWindow->Show();
				fDataView->fBootableCDCheckBox->SetEnabled(true);
				if (fDataView->fBootableCDCheckBox->Value() == 1) {
					fDataView->fChooseBootImageButton->SetEnabled(true);
				}
			} else {
				BAlert* MyAlert = new BAlert("BurnItNow", "You can only have one ISOfile/VirtualCD on a CD.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
				MyAlert->Go();
			}
			break;

		case VOLUME_NAME: {
				message->FindString("DirName", &temp_char);
				strcpy(VOL_NAME, temp_char);
				LeftListItem* item;

				if ((item = (LeftListItem*)fLeftList->ItemAt(0)) != NULL) {
					if (item->fIconBitmap == fLeftList->fVRCDBitmap) {
						if (BOOTABLE)
							sprintf(item->fName, "VRCD(Boot) - [%s]", temp_char);
						else
							sprintf(item->fName, "VRCD - [%s]", temp_char);

						item->fName[27] = 0;
						item->fName[26] = ']';
						fLeftList->InvalidateItem(0);
					}
				}
			}
			break;

		case BOOT_CHECKED: {
				LeftListItem* item;
				if (fDataView->fBootableCDCheckBox->Value() == 1) {
					fDataView->fBootableCDCheckBox->SetValue(0);
					fDataView->fChooseBootImageButton->SetEnabled(false);
					BOOTABLE = false;
					fDataView->fFilePanel->Show();
				} else {
					BOOTABLE = false;
					sprintf(BOOTSTRING, " ");
					fDataView->fChooseBootImageButton->SetEnabled(false);
					if ((item = (LeftListItem*)fLeftList->ItemAt(0)) != NULL) {
						if (item->fIconBitmap == fLeftList->fVRCDBitmap) {
							sprintf(item->fName, "VRCD - [%s]", VOL_NAME);
							item->fName[27] = 0;
							item->fName[26] = ']';
							fLeftList->InvalidateItem(0);
						}
					}
				}
			}
			break;

		case BOOT_FILE_PANEL:
			fDataView->fBootableCDCheckBox->SetValue(0);
			fDataView->fChooseBootImageButton->SetEnabled(false);
			BOOTABLE = false;
			fDataView->fFilePanel->Show();
			break;

		case BOOT_CHANGE_IMAGE_NAME: {
				LeftListItem* item;
				off_t size;
				entry_ref ref;
				message->FindRef("refs", &ref);
				if (BEntry(&ref, true).GetSize(&size) == B_OK) {
					if (size < (2880 * 1024)) {
						BOOTIMAGEREF = ref;
						fDataView->fBootableCDCheckBox->SetValue(1);
						fDataView->fChooseBootImageButton->SetEnabled(true);
						BOOTABLE = true;
						if ((item = (LeftListItem*)fLeftList->ItemAt(0)) != NULL) {
							if (item->fIconBitmap == fLeftList->fVRCDBitmap) {
								sprintf(item->fName, "VRCD(Boot) - [%s]", VOL_NAME);
								item->fName[27] = 0;
								item->fName[26] = ']';
								fLeftList->InvalidateItem(0);
							}
						}
					} else {
						sprintf(BOOTSTRING, " ");
						fDataView->fBootableCDCheckBox->SetValue(0);
						fDataView->fChooseBootImageButton->SetEnabled(false);
						BOOTABLE = false;
						BAlert* MyAlert = new BAlert("BurnItNow", "The bootimage cannot be bigger than 2.88 Mb", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
						MyAlert->Go();
					}
				}
			}
			break;

		case CHANGE_VOL_NAME: {
				fVolumeNameWindow = new AskName(BRect(200, 200, 440, 240), "Volume name", VOLUME_NAME, VOL_NAME);
				fVolumeNameWindow->Show();
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
	if ((message->what & 0xffffff00) == 'dev\0') {
		count = (uint8)(message->what & 0xff);
		SCSI_DEV = count;
		fBurnDevice = &fScsiDevices[count - 1];
	}
}


void jpWindow::SaveData()
{
	// Save all prefs
	fBurnItPrefs->SetBool("BOOTABLE", BOOTABLE);
	fBurnItPrefs->SetRef("BOOTIMAGEREF", &BOOTIMAGEREF);
	fBurnItPrefs->SetString("ISOFILE_DIR", ISOFILE_DIR);
	fBurnItPrefs->SetString("VOL_NAME", VOL_NAME);
	fBurnItPrefs->SetBool("VRCD", VRCD);
	fBurnItPrefs->SetBool("ISOFILE", ISOFILE);
	fBurnItPrefs->SetInt16("BURN_SPD", BURN_SPD);
	fBurnItPrefs->SetInt16("BLANK_TYPE", BLANK_TYPE);
	fBurnItPrefs->SetInt16("BLANK_SPD", BLANK_SPD);
	fBurnItPrefs->SetInt16("BURN_TYPE", BURN_TYPE);
	fBurnItPrefs->SetBool("ONTHEFLY", ONTHEFLY);
	fBurnItPrefs->SetString("MULTISESSION", MULTISESSION);
	fBurnItPrefs->SetString("DUMMYMODE", DUMMYMODE);
	fBurnItPrefs->SetString("EJECT", EJECT);
	fBurnItPrefs->SetString("DATA_STRING", DATA_STRING);
	fBurnItPrefs->SetString("PAD", PAD);
	fBurnItPrefs->SetString("DAO", DAO);
	fBurnItPrefs->SetString("BURNPROOF", BURNPROOF);
	fBurnItPrefs->SetString("SWAB", SWAB);
	fBurnItPrefs->SetString("NOFIX", NOFIX);
	fBurnItPrefs->SetString("PREEMP", PREEMP);
	fBurnItPrefs->SetInt16("IMAGE_TYPE", IMAGE_TYPE);
	fBurnItPrefs->SetInt16("SCSI_DEV", SCSI_DEV);

	// All saved
	delete fBurnItPrefs;
}


bool jpWindow::QuitRequested()
{
	SaveData();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void jpWindow::SetISOFile(char* string)
{
	entry_ref temp_ref;
	if (!VRCD && !ISOFILE) {
		strcpy(ISOFILE_DIR, string);
		BEntry(string).GetRef(&temp_ref);
		fLeftList->AddItem(new LeftListItem(&temp_ref, temp_ref.name, fLeftList->fISOBitmap, NULL), 0);
		fNewVRCDButton->SetEnabled(false);
		fAddISOButton->SetEnabled(false);
		ISOFILE = true;
		VRCD = false;
	} else {
		BAlert* MyAlert = new BAlert("BurnItNow", "You can only have one ISOfile/VirtualCD on a CD.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
		MyAlert->Go();
	}
}


void jpWindow::MakeImageNOW(int Multi, const char* str)
{
	int Result;
	BAlert* MyAlert = new BAlert("Burn?", "Do you want to make the image?", "I changed my mind.", "Do it.", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	Result = MyAlert->Go();
	if (Result) {
		fStatusWindow = new StatusWindow(VOL_NAME);
		fStatusWindow->Show();

		fStatusWindow->Lock();
		fStatusWindow->fStatusBar->Reset();
		fStatusWindow->fStatusBar->SetBarColor(blue);
		fStatusWindow->Unlock();
		char command[2000];
		if (BOOTABLE)
			MakeBootImage();

		if (IMAGE_TYPE == 0) {
			if (Multi == 0)
				sprintf(command, "mkisofs -o \"%s\" %s %s -gui -f -V \"%s\" \"%s\" 2>&1", IMAGE_NAME, DATA_STRING, BOOTSTRING, VOL_NAME, BURN_DIR);

			if (Multi == 1)
				sprintf(command, "mkisofs -o \"%s\" %s %s -gui -f -V \"%s\" -C %s -M %s \"%s\" 2>&1", IMAGE_NAME, DATA_STRING, BOOTSTRING, VOL_NAME, str, fBurnDevice->scsiid, BURN_DIR);

			Lock();
			resume_thread(Cntrl = spawn_thread(controller, "MakeingIMAGE", 5, command));
			snooze(500000);
			resume_thread(OPMkImage = spawn_thread(OutPutMkImage, "OutPutMakeingIMAGE", 5, fStatusWindow));
			snooze(500000);
			fBurnView->fBurnButton->SetLabel("Cancel");
			Unlock();

		}
		if (IMAGE_TYPE == 1) { // BFS
			MessageLog("Begin making and copying to bfs file...");
			fStatusWindow->Lock();
			fStatusWindow->StatusSetText("Making BFS image this can take several minutes..");
			fStatusWindow->Unlock();
			MakeBFS();
			MountBFS();
			BFSDir = new BDirectory("/BurnItNowTempMount");
			CopyFiles();
			delete BFSDir;
			UnmountBFS();
			MessageLog("Done with bfs.");
			if (!JUST_IMAGE)
				BurnWithCDRecord();
			else {
				sprintf(IMAGE_NAME, "%s/tmp/BurnItNow.raw", BURNIT_PATH);
				fStatusWindow->Lock();
				fStatusWindow->Ready();
				fStatusWindow->Unlock();
			}
		}
	}
}


void jpWindow::GetTsize(char* tsize)
{	
	char buffer[1024];
	char command[1024];
	FILE* f;

	BString commandstr;
	
	commandstr.SetTo(CDRTOOLS_DIR.Path());
	commandstr << "/";
	commandstr << "mkisofs -print-size " << DATA_STRING << " -f -V " << '"' << VOL_NAME << '"' << " " << '"' << BURN_DIR << '"' << " 2>&1";
	
	printf("GetTsize: '%s'\n",commandstr.String());
	strcpy(command, commandstr.String());
	f = popen(command, "r");
	memset(command, 0, 1024);
	while (!feof(f) && !ferror(f)) {
		buffer[0] = 0;
		fgets(buffer, 1024, f);
		if (!strncmp(buffer, "Total extents scheduled to be written = ", 40)) {
			strncpy(command, &buffer[40], strlen(buffer) - 41);
			sprintf(tsize, "%ss", command);
		}
	}
	pclose(f);
}


void jpWindow::BurnWithCDRecord()
{	
	char tsize[512];
	char command[2048];
	int Result;
		
	BString commandstr;
	
	CalculateSize();

	BAlert* MyAlert = new BAlert("Put in a CD", "Put in a CDR/CDRW", "Cancel", "Ok", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	Result = MyAlert->Go();
	if (Result) {
		if (ONTHEFLY || ISOFILE || (BURN_TYPE == 1)) {
			fStatusWindow = new StatusWindow(VOL_NAME);
			fStatusWindow->Show();
		}
		fStatusWindow->Lock();
		fStatusWindow->fStatusBar->Reset();
		fStatusWindow->fStatusBar->SetBarColor(blue);
		fStatusWindow->Unlock();

		SetButtons(false);
		
		commandstr.SetTo(CDRTOOLS_DIR.Path());
		commandstr << "/";
		commandstr << "cdrecord dev=" << fBurnDevice->scsiid;
		
		if (BURN_TYPE == 0) {
			fStatusWindow->Lock();
			fStatusWindow->SetAngles(angles, 1);
			fStatusWindow->Unlock();

			if (ONTHEFLY && !ISOFILE && VRCD) {
				if (BOOTABLE)
					MakeBootImage();
				GetTsize(tsize);
				commandstr.SetTo(CDRTOOLS_DIR.Path());
				commandstr << "/";
				commandstr << "mkisofs " << DATA_STRING << " -quiet " ;
				commandstr << BOOTSTRING ;
				commandstr << " -f -V " << '"' << VOL_NAME << '"' << " " << '"';
				commandstr << BURN_DIR << '"' << " | " << CDRTOOLS_DIR.Path();
				commandstr << "/cdrecord dev=" << fBurnDevice->scsiid;
				commandstr << " speed=" << BURN_SPD ;
				commandstr << " "  << BURNPROOF;
				commandstr << "tsize" << tsize << " "; 
				commandstr << DAO << " -data ";
				commandstr << DUMMYMODE ;
				commandstr << " " << EJECT << " -v -";
			} 
			else {
				commandstr.SetTo(CDRTOOLS_DIR.Path());
				commandstr << "/";
				commandstr << "cdrecord dev=" << fBurnDevice->scsiid << " speed=" << BURN_SPD ;
				commandstr << " " << BURNPROOF << " " << DAO;
				commandstr << " -data " << DUMMYMODE ;
				commandstr << " " << EJECT ;
				commandstr << " -v" ;	// aw 110910
				commandstr << " " << MULTISESSION	<< "\"" << IMAGE_NAME << "\"";
			}	
			
			commandstr.ReplaceAll("\t"," "); 	commandstr.ReplaceAll("  "," ");
			printf("BURN_TYPE0: '%s'\n",commandstr.String());
			
			Lock();	
			strcpy(command, commandstr.String());
			resume_thread(Cntrl = spawn_thread(controller, "Burning", 15, command));
			snooze(500000);
			resume_thread(OPBurn = spawn_thread(OutPutBurn, "OutPutBurning", 15, fStatusWindow));
			snooze(500000);
			Unlock();
		} else if (BURN_TYPE == 1) {
			fStatusWindow->Lock();
			fStatusWindow->SetAngles(angles, nrtracks);
			fStatusWindow->Unlock();
			commandstr.SetTo(CDRTOOLS_DIR.Path());
			commandstr << "//";
			commandstr << "cdrecord dev=" << fBurnDevice->scsiid;
			commandstr << " speed=" << BURN_SPD; 
			commandstr << " " << BURNPROOF ;
			commandstr << " " << DAO; 
			commandstr << " " << PAD;
			commandstr << " " << PREEMP ;
			commandstr << " " << SWAB ;
			commandstr << " " << NOFIX;
			commandstr << " -audio " << DUMMYMODE;
			commandstr << " " << EJECT ;
			commandstr << " " << AUDIO_FILES;
			commandstr.ReplaceAll("\t"," "); 	commandstr.ReplaceAll("  "," ");
			printf("BURN_TYPE1: '%s'\n",commandstr.String());
			Lock();
			resume_thread(Cntrl = spawn_thread(controller, "Burning", 15, command));
			snooze(500000);
			resume_thread(OPBurn = spawn_thread(OutPutBurn, "OutPutBurning", 15, fStatusWindow));
			snooze(500000);
			Unlock();
		} else if (BURN_TYPE == 2) {
			fStatusWindow->Lock();
			fStatusWindow->SetAngles(angles, nrtracks);
			fStatusWindow->Unlock();

			if (ONTHEFLY && !ISOFILE && VRCD) {
				GetTsize(tsize);
				if (fDataView->fBootableCDCheckBox->Value() == 1)
					MakeBootImage();

				commandstr.SetTo(CDRTOOLS_DIR.Path());
				commandstr << "/";
				commandstr << "mkisofs " << DATA_STRING << BOOTSTRING << " -quiet -f -V " ;
			 	commandstr << '"' << VOL_NAME << '"' << BURN_DIR << '"' << " |";
				commandstr << CDRTOOLS_DIR.Path() << "/cdrecord dev=" ;
			 	commandstr <<  fBurnDevice->scsiid << "speed=" << BURN_SPD ;
			 	commandstr << " " << BURNPROOF;
				commandstr << " tsize=" << tsize ;
			 	commandstr << " " << DAO << " " << DUMMYMODE ;
			 	commandstr << " " << EJECT << " " << PAD ;
			 	commandstr << " " << PREEMP << " " << SWAB ;
			 	commandstr << " " << NOFIX;
				commandstr << " -data - -audio " << AUDIO_FILES;
				MessageLog(commandstr.String());
			} else {
				commandstr.SetTo(CDRTOOLS_DIR.Path());
				commandstr << "//";
				commandstr << "cdrecord dev=" << fBurnDevice->scsiid;
				commandstr << " speed=" << BURN_SPD ;
			 	commandstr << " " << BURNPROOF << " " << DAO << " " << PAD ;
			 	commandstr << " " << PREEMP << " " << SWAB ;
			 	commandstr << " " << NOFIX ;
			 	commandstr << " " << DUMMYMODE ;
			 	commandstr << " " << EJECT; 
				commandstr << " -data " << '"' << IMAGE_NAME << '"' << " -audio " << AUDIO_FILES;
			}
			
			commandstr.ReplaceAll("\t"," "); 	commandstr.ReplaceAll("  "," ");
			printf("BURN_TYPE2: '%s'\n",commandstr.String());
			
			Lock();
			resume_thread(Cntrl = spawn_thread(controller, "Burning", 15, command));
			snooze(500000);
			resume_thread(OPBurn = spawn_thread(OutPutBurn, "OutPutBurning", 15, fStatusWindow));
			snooze(500000);
			Unlock();
		}
	}

	//restore after burning  not checked..  // aw 1109100
	fBurnView->fBurnButton->SetLabel("Burn!");
}


void jpWindow::UpdateStatus(float delta, char* str)
{
	fStatusBar->Update(delta - fStatusBar->CurrentValue(), str);
}


void jpWindow::SetButtons(bool what)
{
	fBurnView->SetButton(what);
	fCDRWView->fBlankButton->SetEnabled(what);
}


void jpWindow::MakeBootImage()
{	
	char temp[1024];
	char temp2[1024];
	BPath path;
	BEntry entry(&BOOTIMAGEREF, true);
	if (entry.Exists()) {
		entry.GetPath(&path);
		sprintf(temp, "%s/boot.catalog", BURN_DIR);
		create_directory(temp, 0777);
		sprintf(BOOTSTRING, "-c boot.catalog/ -b \"boot.catalog/%s\"", path.Leaf());
		sprintf(temp, "%s/%s", temp, path.Leaf());
		sprintf(temp2, "dd if=/dev/zero of=\"%s\" bs=1024 count=2880", temp);
		system(temp2);
		sprintf(temp2, "dd if=\"%s\" of=\"%s\" conv=notrunc", path.Path(), temp);
		system(temp2);
	} else {
		sprintf(BOOTSTRING, " ");
		fDataView->fBootableCDCheckBox->SetValue(0);
		fDataView->fChooseBootImageButton->SetEnabled(false);
		BOOTABLE = false;
		BAlert* MyAlert = new BAlert("BurnItNow", "The boot image you chose doesn't exist.\n BurnItNow will burn this CD without a bootimage.", "Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
		MyAlert->Go();
	}
}
