/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <compat/sys/stat.h>
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Notification.h>
#include <ScrollView.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>

#include "BurnApplication.h"
#include "CompilationAudioView.h"
#include "CompilationShared.h"
#include "CommandThread.h"
#include "Constants.h"
#include "OutputParser.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Compilation views"


CompilationAudioView::CompilationAudioView(BurnWindow& parent)
	:
	BView(B_TRANSLATE_COMMENT("Audio CD", "Tab label"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBurnerThread(NULL),
	fOpenPanel(NULL),
	fNoteID(""),
	fID(0),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime),
	fAbort(0),
	fAction(IDLE)
{
	fWindowParent = &parent;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fInfoView->SetFont(be_bold_font);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Ready",
		"Status notification"));

	fOutputView = new BTextView("AudioInfoTextView");
	fOutputView->SetWordWrap(true);
	fOutputView->MakeEditable(false);
	BScrollView* fOutputScrollView = new BScrollView("AudioInfoScrollView",
		fOutputView, B_WILL_DRAW, false, true);
	fOutputScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fBurnButton = new BButton("BurnDiscButton", B_TRANSLATE_COMMENT("Burn disc",
		"Button label"), new BMessage(kBurnButton));
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fAudioBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fAudioBox->SetFont(be_bold_font);
	fAudioBox->SetLabel(B_TRANSLATE("Drop tracks here (only WAV files)"));

	fTrackList = new AudioListView("AudioListView");
	fTrackList->SetSelectionMessage(new BMessage(kTrackSelection));
	fTrackList->SetInvocationMessage(new BMessage(kTrackPlayback));

	BScrollView* audioScrollView = new BScrollView("AudioScrollView",
		fTrackList, B_WILL_DRAW, false, true);
	audioScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fUpButton = new BButton("UpButton", "\xe2\xac\x86",
		new BMessage(kUpButton));
	fUpButton->SetToolTip(B_TRANSLATE_COMMENT("Move up", "Tool tip"));
	fUpButton->SetExplicitSize(BSize(StringWidth("\xe2\xac\x86") * 3,
		B_SIZE_UNSET));

	fDownButton = new BButton("DownButton", "\xe2\xac\x87",
		new BMessage(kDownButton));
	fDownButton->SetToolTip(B_TRANSLATE_COMMENT("Move down", "Tool tip"));
	fDownButton->SetExplicitSize(BSize(StringWidth("\xe2\xac\x87") * 3,
		B_SIZE_UNSET));

	fPlayButton = new BButton("PlayButton", "\xE2\x96\xB6",
		new BMessage(kTrackPlayback));
	fPlayButton->SetToolTip(B_TRANSLATE_COMMENT("Play back", "Tool tip"));
	fPlayButton->SetExplicitSize(BSize(StringWidth("\xE2\x96\xB6") * 3,
		B_SIZE_UNSET));

	fAddButton = new BButton("AddButton", B_TRANSLATE_COMMENT(
		"Add" B_UTF8_ELLIPSIS, "Button label"), new BMessage(kAddButton));
	fAddButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fRemoveButton = new BButton("RemoveButton", B_TRANSLATE_COMMENT("Remove",
		"Button label"), new BMessage(kDeleteItem));
	fRemoveButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fBurnButton)
			.AddGlue()
			.End()
		.AddSplit(B_HORIZONTAL, B_USE_SMALL_SPACING)
		.GetSplitView(&fAudioSplitView)
			.AddGroup(B_VERTICAL)
				.Add(fInfoView)
				.Add(fOutputScrollView)
				.End()
			.AddGroup(B_VERTICAL)
				.Add(fAudioBox)
				.Add(audioScrollView)
				.AddGroup(B_HORIZONTAL)
					.Add(fUpButton)
					.Add(fDownButton)
					.AddGlue()
					.Add(fPlayButton)
					.AddGlue()
					.Add(fAddButton)
					.Add(fRemoveButton)
					.End()
				.End()
			.End()
		.Add(fSizeView);

	// Apply settings to splitview
	float infoWeight;
	float tracksWeight;
	bool infoCollapse;
	bool tracksCollapse;
	AppSettings* settings = my_app->Settings();
	if (settings->Lock()) {
		settings->GetSplitWeight(infoWeight, tracksWeight);
		settings->GetSplitCollapse(infoCollapse, tracksCollapse);
		settings->Unlock();
	}
	fAudioSplitView->SetItemWeight(0, infoWeight, false);
	fAudioSplitView->SetItemCollapsed(0, infoCollapse);

	fAudioSplitView->SetItemWeight(1, tracksWeight, true);
	fAudioSplitView->SetItemCollapsed(1, tracksCollapse);

	_UpdateSizeBar();
}


CompilationAudioView::~CompilationAudioView()
{
	delete fBurnerThread;
	delete fOpenPanel;
}


#pragma mark -- BView Overrides --


void
CompilationAudioView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fTrackList->SetTarget(this);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);

	fUpButton->SetTarget(this);
	fUpButton->SetEnabled(false);
	fDownButton->SetTarget(this);
	fDownButton->SetEnabled(false);
	fPlayButton->SetTarget(this);
	fPlayButton->SetEnabled(false);
	fAddButton->SetTarget(this);
	fAddButton->SetEnabled(true);
	fRemoveButton->SetTarget(fTrackList);
	fRemoveButton->SetEnabled(false);
}


void
CompilationAudioView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kTrackSelection:
			_UpdateButtons();
			break;
		case kTrackPlayback:
		{
			BMessenger msgr("application/x-vnd.Be-TRAK");
			BMessage refMsg(B_REFS_RECEIVED);

			BList indices;
			fTrackList->GetSelectedItems(indices);

			int32 count = indices.CountItems();
			for (int32 i = 0; i < count; i++) {
				int32 index = (int32)(addr_t)indices.ItemAtFast(i);
				AudioListItem* sItem = dynamic_cast<AudioListItem *>
					(fTrackList->ItemAt(index));

				if (sItem == NULL)
					break;

				entry_ref ref;
				BEntry track(sItem->GetPath());
				track.GetRef(&ref);

				if (track.InitCheck() == B_OK)
					refMsg.AddRef("refs", &ref);
			}
			msgr.SendMessage(&refMsg);
			break;
		}
		case kUpButton:
		{
			int32 index = fTrackList->CurrentSelection();
			if (index < 1)
				break;

			BList indices;
			fTrackList->GetSelectedItems(indices);

			int32 count = indices.CountItems();
			for (int32 i = 0; i < count; i++) {
				int32 swapIndex = (int32)(addr_t)indices.ItemAtFast(i);
				if (swapIndex > 0)
					fTrackList->SwapItems(swapIndex, swapIndex - 1);
			}
			fTrackList->RenumberTracks();
			_UpdateButtons();
			break;
		}
		case kDownButton:
		{
			int32 index = fTrackList->CurrentSelection();
			if ((index < 0) || index == fTrackList->CountItems() - 1)
				break;

			BList indices;
			fTrackList->GetSelectedItems(indices);
			fTrackList->DeselectAll();

			int32 count = indices.CountItems();
			for (int32 i = count - 1; i >= 0; i--) {
				int32 swapIndex = (int32)(addr_t)indices.ItemAtFast(i);
				if (swapIndex < fTrackList->CountItems() - 1) {
					fTrackList->SwapItems(swapIndex + 1, swapIndex);
					fTrackList->Select(swapIndex + 1, true);
				} else
					continue;
			}
			fTrackList->RenumberTracks();
			_UpdateButtons();
			break;
		}
		case kAddButton:
		{
			if (fOpenPanel == NULL) {
				fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
				NULL, B_FILE_NODE | B_DIRECTORY_NODE, true, NULL,
				new AudioRefFilter(), true);
				fOpenPanel->Window()->SetTitle(B_TRANSLATE_COMMENT(
					"Add WAV files", "File panel title"));
			}
			fOpenPanel->Show();
			break;
		}
		case kBurnOutput:
			_BurnOutput(message);
			break;
		case kBurnButton:
			_Burn();
			break;
		case B_REFS_RECEIVED:
			_AddTrack(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Public Methods --


int32
CompilationAudioView::InProgress()
{
	return fAction;
}


#pragma mark -- Private Methods --


bool
AudioRefFilter::Filter(const entry_ref* ref, BNode* node,
	struct stat_beos* stat, const char* filetype)
{
	if (S_ISDIR(stat->st_mode) || (S_ISLNK(stat->st_mode)))
		return true;

	BStringList audioMimes;
	audioMimes.Add("audio/wav");
	audioMimes.Add("audio/x-wav");

	BMimeType refType;
	BMimeType::GuessMimeType(ref, &refType);

	if (audioMimes.HasString(refType.Type()))
		return true;

	return false;
}


void
CompilationAudioView::_AddTrack(BMessage* message)
{
	int32 index = -1;
	if (message->WasDropped()) {
		BPoint dropPoint = message->DropPoint();
		index = fTrackList->IndexOf(fTrackList->ConvertFromScreen(dropPoint));
	} else
		index = fTrackList->CurrentSelection();

	if (index < 0)
		index = fTrackList->CountItems();

	entry_ref trackRef;
	int32 i = 0;
	while (message->FindRef("refs", i, &trackRef) == B_OK) {
		BEntry entry(&trackRef, true);	// also accept symlinks
		BNode node(&entry);
		if (node.InitCheck() != B_OK)
			return;

		BPath* trackPath = new BPath(&entry);
		BString filename(trackPath->Leaf());
		BString path(trackPath->Path());


		BString extension = GetExtension(&trackRef);
		BStringList extStrings;
		extStrings.Add("image");
		extStrings.Add("img");
		extStrings.Add("iso");

		// Check for wav MIME type or file extension
		BStringList audioMimes;
		audioMimes.Add("audio/wav");
		audioMimes.Add("audio/x-wav");
		BMimeType refType;
		BMimeType::GuessMimeType(&trackRef, &refType);

		if (audioMimes.HasString(refType.Type())
				|| extStrings.HasString(extension))
			fTrackList->AddItem(new AudioListItem(filename, path, i), index++);

		if (node.IsDirectory()) {
			BDirectory dir(&entry);
			entry_ref ref;

			while (dir.GetNextRef(&ref) == B_OK) {
				BNode dNode(&ref);
				extension = GetExtension(&ref);

				BPath* dTrackPath = new BPath(&ref);
				BString dFilename(dTrackPath->Leaf());
				BString dPath(dTrackPath->Path());

				// Check for wav MIME type or file extension
				BMimeType::GuessMimeType(&ref, &refType);

				if (audioMimes.HasString(refType.Type())
						|| extStrings.HasString(extension))
					fTrackList->AddItem(new AudioListItem(dFilename, dPath, i),
						index++);
			}
		}
		i++;
	}
	if (!fTrackList->IsEmpty()) {
		fBurnButton->SetEnabled(true);
		fInfoView->SetLabel(B_TRANSLATE_COMMENT("Burn the disc",
			"Status notification"));
		fTrackList->RenumberTracks();
	} else
		fBurnButton->SetEnabled(false);

	_UpdateSizeBar();
}



void
CompilationAudioView::_Burn()
{
	if (fTrackList->IsEmpty())
		return;

	fAction = BURNING;	// flag we're burning
	fBurnButton->SetEnabled(false);

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Burning in progress"
		B_UTF8_ELLIPSIS, "Status notification"));

	BNotification burnProgress(B_PROGRESS_NOTIFICATION);
	burnProgress.SetGroup("BurnItNow");
	burnProgress.SetTitle(B_TRANSLATE_COMMENT("Burning Audio CD",
		"Notification title"));
	burnProgress.SetProgress(0);

	char id[5];
	snprintf(id, sizeof(id), "%" B_PRId32, fID++); // new ID
	fNoteID = "BurnItNow_Audio-";
	fNoteID.Append(id);

	burnProgress.SetMessageID(fNoteID);
	 // It may take a while for the burning to start...
	burnProgress.Send(60 * 1000000LL);

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnOutput), this));

	fBurnerThread->AddArgument("cdrecord");

	if (config.simulation)
		fBurnerThread->AddArgument("-dummy");
	if (config.eject)
		fBurnerThread->AddArgument("-eject");
	if (config.mode == "-sao")
		fBurnerThread->AddArgument("-dao"); // for max compatibility
	else
		fBurnerThread->AddArgument(config.mode);

	if (config.speed != "")
		fBurnerThread->AddArgument(config.speed);

	if ((config.speed == "speed=0") || (config.speed == "speed=4"))
		fBurnerThread->AddArgument("fs=4m"); // for max compatibility
	else
		fBurnerThread->AddArgument("fs=16m");

	fBurnerThread->AddArgument(device)
		->AddArgument("-v")	// to get progress output
		->AddArgument("-gracetime=2")
		->AddArgument("-audio")
		->AddArgument("-copy")
		->AddArgument("-pad")
		->AddArgument("padsize=63s");

	for (unsigned int i = 0; i <= MAX_TRACKS; i++) {
		AudioListItem* sItem = dynamic_cast<AudioListItem *>
			(fTrackList->ItemAt(i));

		if (sItem == NULL)
			break;

		BString track(sItem->GetPath());
		BPath test(track);
		if (test.InitCheck() == B_OK)
			fBurnerThread->AddArgument(track);
	}
	fBurnerThread->Run();
	fParser.Reset();
}


void
CompilationAudioView::_BurnOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseCdrecordLine(text, data);
		if (modified < 0)
			fAbort = modified;
		if (modified <= 0) {
			data << "\n";
			fOutputView->Insert(data.String());
			fOutputView->ScrollBy(0.0, 50.0);
		} else {
			if (modified == PERCENT)
				_UpdateProgress();
			fOutputView->SetText(text);
			fOutputView->ScrollTo(0.0, 1000000.0);
		}
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		if (fAbort == SMALLDISC) {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning aborted: The data doesn't fit on the disc",
				"Status notification"));

			BNotification burnAbort(B_IMPORTANT_NOTIFICATION);
			burnAbort.SetGroup("BurnItNow");
			burnAbort.SetTitle(B_TRANSLATE_COMMENT("Burning aborted",
				"Notification title"));
			burnAbort.SetContent(B_TRANSLATE_COMMENT(
				"The data doesn't fit on the disc.", "Notification content"));
			burnAbort.SetMessageID(fNoteID);
			burnAbort.Send();
		} else if (fAbort == INVALIDWAV) {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning aborted: Some WAV file has the wrong encoding",
				"Status notification"));

			BNotification burnAbort(B_IMPORTANT_NOTIFICATION);
			burnAbort.SetGroup("BurnItNow");
			burnAbort.SetTitle(B_TRANSLATE_COMMENT("Burning aborted",
				"Notification title"));
			burnAbort.SetContent(B_TRANSLATE_COMMENT(
				"Some WAV file has the wrong encoding", "Notification content"));
			burnAbort.SetMessageID(fNoteID);
			burnAbort.Send();
		} else {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning finished. Burn another disc?",
				"Status notification"));

			BNotification burnSuccess(B_INFORMATION_NOTIFICATION);
			burnSuccess.SetGroup("BurnItNow");
			burnSuccess.SetTitle(B_TRANSLATE_COMMENT("Burning Audio CD",
				"Notification title"));
			burnSuccess.SetContent(B_TRANSLATE_COMMENT("Burning finished!",
				"Notification content"));
			burnSuccess.SetMessageID(fNoteID);
			burnSuccess.Send();
		}

		fBurnButton->SetEnabled(true);

		fAction = IDLE;
		fAbort = 0;
		fParser.Reset();
	}
}


void
CompilationAudioView::_UpdateButtons()
{
	int32 selection = fTrackList->CurrentSelection();
	int32 count = fTrackList->CountItems();

	if (selection < 0)
		count = -1;

	fPlayButton->SetEnabled((selection < 0) ? false : true);
	fRemoveButton->SetEnabled((selection < 0) ? false : true);
	fUpButton->SetEnabled((count > 1 && selection > 0) ? true : false);

	// get last selected item in case of multiple selections
	int32 i = 0;
	while ((fTrackList->CurrentSelection(i)) >= 0 ) {
		selection = fTrackList->CurrentSelection(i);
		i++;
	}

	fDownButton->SetEnabled((count > 1 && selection < count - 1)
		? true : false);
}


void
CompilationAudioView::_UpdateProgress()
{
	BNotification burnProgress(B_PROGRESS_NOTIFICATION);
	burnProgress.SetGroup("BurnItNow");
	burnProgress.SetTitle(B_TRANSLATE_COMMENT("Burning Audio CD",
		"Notification title"));
	burnProgress.SetContent(fETAtime);
	burnProgress.SetProgress(fProgress);
	burnProgress.SetMessageID(fNoteID);
	burnProgress.Send();
}


void
CompilationAudioView::_UpdateSizeBar()
{
	off_t fileSize = 0;
	off_t fileSizeSum = 0;
	for (unsigned int i = 0; i <= MAX_TRACKS; i++) {
		AudioListItem* sItem = dynamic_cast<AudioListItem *>
			(fTrackList->ItemAt(i));

		if (sItem == NULL)
			break;

		BEntry entry(sItem->GetPath());
		entry.GetSize(&fileSize);
		fileSizeSum += fileSize;
	}
	fSizeView->UpdateSizeDisplay(fileSizeSum / 1024, AUDIO, CD_ONLY);
	// size in KiB
}
