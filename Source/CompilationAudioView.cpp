/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <NodeInfo.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include "BurnApplication.h"
#include "CompilationAudioView.h"
#include "CommandThread.h"
#include "Constants.h"
#include "OutputParser.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Audio view"


CompilationAudioView::CompilationAudioView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Audio CD"), B_WILL_DRAW, new BGroupLayout(B_VERTICAL,
		kControlPadding)),
	fBurnerThread(NULL),
	fNotification(B_PROGRESS_NOTIFICATION),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime),
	fAbort(false),
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

	fBurnButton = new BButton("BurnDiscButton", B_TRANSLATE("Burn disc"),
		new BMessage(kBurnButton));
	fBurnButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fAudioBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fAudioBox->SetFont(be_bold_font);
	fAudioBox->SetLabel(B_TRANSLATE("Drop tracks here (only WAV files)"));

	fTrackList = new AudioListView("AudioListView");
	BScrollView* audioScrollView = new BScrollView("AudioScrollView",
		fTrackList, B_WILL_DRAW, false, true);
	audioScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

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
}


#pragma mark -- BView Overrides --


void
CompilationAudioView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationAudioView::MessageReceived(BMessage* message)
{
	switch (message->what) {
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


void
CompilationAudioView::_AddTrack(BMessage* message)
{
	int32 index;
	if (message->WasDropped()) {
		BPoint dropPoint = message->DropPoint();
		index = fTrackList->IndexOf(fTrackList->ConvertFromScreen(dropPoint));
		if (index < 0)
			index = fTrackList->CountItems();
	}

	entry_ref trackRef;
	int32 i = 0;
	while (message->FindRef("refs", i, &trackRef) == B_OK) {
		BEntry entry(&trackRef, true);	// also accept symlinks
		BNode node(&entry);
		if (node.InitCheck() != B_OK)
			return;

		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() != B_OK)
			return;

		char mimeTypeString[B_MIME_TYPE_LENGTH];
		nodeInfo.GetType(mimeTypeString);
		BPath* trackPath = new BPath(&entry);
		BString filename(trackPath->Leaf());
		BString path(trackPath->Path());

		// Check for wav MIME type or file extension
		if ((strcmp("audio/x-wav", mimeTypeString) == 0)
			|| filename.IFindLast(".wav", filename.CountChars())
				== filename.CountChars() - 4) {
			if (!message->WasDropped())
				index = fTrackList->CountItems();
			fTrackList->AddItem(new AudioListItem(filename, path, i), index++);
		}
		if (node.IsDirectory()) {
			BDirectory dir(&entry);
			entry_ref ref;

			while (dir.GetNextRef(&ref) == B_OK) {
				BNode dNode(&ref);
				if (dNode.InitCheck() != B_OK)
					return;

				BNodeInfo dNodeInfo(&dNode);
				if (dNodeInfo.InitCheck() != B_OK)
					return;

				dNodeInfo.GetType(mimeTypeString);

				BPath* dTrackPath = new BPath(&ref);
				BString dFilename(dTrackPath->Leaf());
				BString dPath(dTrackPath->Path());

				// Check for wav MIME type or file extension
				if ((strcmp("audio/x-wav", mimeTypeString) == 0)
					|| dFilename.IFindLast(".wav", dFilename.CountChars())
						== dFilename.CountChars() - 4) {
					if (!message->WasDropped())
						index = fTrackList->CountItems();

					fTrackList->AddItem(new AudioListItem(dFilename, dPath, i),
						index++);
				}
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

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Burning in progress"
		B_UTF8_ELLIPSIS, "Status notification"));
	fBurnButton->SetEnabled(false);

	fNotification.SetGroup("BurnItNow");
	fNotification.SetTitle(B_TRANSLATE("Burning Audio CD"));

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fNotification.SetGroup("BurnItNow");
	fNotification.SetMessageID("BurnItNow_Audio");
	fNotification.SetTitle(B_TRANSLATE("Building data image"));
	fNotification.SetContent(B_TRANSLATE("Burning Audio CD" B_UTF8_ELLIPSIS));
	fNotification.SetProgress(0);
	 // It may take a while for the burning to start...
	fNotification.Send(60 * 1000000LL);

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
	fAction = BURNING;
}


void
CompilationAudioView::_BurnOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseCdrecordLine(text, data);
		if (modified == SMALLDISC)
			fAbort = true;
		if (modified == NOCHANGE || modified == SMALLDISC) {
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
		if (fAbort) {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning aborted: The data didn't fit on the disc.",
				"Status notification"));
			fNotification.SetTitle(B_TRANSLATE("Burning aborted"));
			fNotification.SetContent(B_TRANSLATE(
				"The data didn't fit on the disc."));
		} else {
			fInfoView->SetLabel(B_TRANSLATE_COMMENT(
				"Burning complete. Burn another disc?",
				"Status notification"));
			fNotification.SetProgress(100);
			fNotification.SetContent(B_TRANSLATE("Burning finished!"));
		}
		fNotification.SetMessageID("BurnItNow_Audio");
		fNotification.Send();

		fBurnButton->SetEnabled(true);

		fAction = IDLE;
		fParser.Reset();
	}
}


void
CompilationAudioView::_UpdateProgress()
{
	if (fProgress == 0 || fProgress == 1.0)
		fNotification.SetContent(" ");
	else
		fNotification.SetContent(fETAtime);
	fNotification.SetMessageID("BurnItNow_Audio");
	fNotification.SetProgress(fProgress);
	fNotification.Send();
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
