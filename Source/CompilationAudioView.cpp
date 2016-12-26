/*
 * Copyright 2010-2012, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "CompilationAudioView.h"
#include "CommandThread.h"

#include <Alert.h>
#include <Button.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <NodeInfo.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

static const float kControlPadding = be_control_look->DefaultItemSpacing();

// Message constants
const int32 kBurnerMessage = 'Brnr';
const int32 kBurnDiscMessage = 'BURN';

CompilationAudioView::CompilationAudioView(BurnWindow& parent)
	:
	BView("Audio", B_WILL_DRAW, new BGroupLayout(B_VERTICAL, kControlPadding)),
	fBurnerThread(NULL)
{
	windowParent = &parent;
	fCurrentPath = 0;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fBurnerInfoBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fBurnerInfoBox->SetFont(be_bold_font);
	fBurnerInfoBox->SetLabel("Ready");

	fBurnerInfoTextView = new BTextView("AudioInfoTextView");
	fBurnerInfoTextView->SetWordWrap(false);
	fBurnerInfoTextView->MakeEditable(false);
	BScrollView* infoScrollView = new BScrollView("AudioInfoScrollView",
		fBurnerInfoTextView, 0, true, true);

	BButton* burnDiscButton = new BButton("BurnDiscButton",
		"Burn disc", new BMessage(kBurnDiscMessage));
	burnDiscButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fAudioBox = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fAudioBox->SetFont(be_bold_font);
	fAudioBox->SetLabel("Drop tracks here (only WAV files)");

	fAudioList = new BListView("AudioListView");
	BScrollView* audioScrollView = new BScrollView("AudioScrollView",
		fAudioList, 0, true, true);

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(burnDiscButton)
			.AddGlue()
			.End()
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL)
				.Add(fBurnerInfoBox)
				.Add(infoScrollView)
				.End()
			.AddGroup(B_VERTICAL)
				.Add(fAudioBox)
				.Add(audioScrollView)
				.End()
			.End();
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

	BButton* burnDiscButton
		= dynamic_cast<BButton*>(FindView("BurnDiscButton"));
	if (burnDiscButton != NULL) {
		burnDiscButton->SetTarget(this);
		burnDiscButton->SetEnabled(false);
	}
}


void
CompilationAudioView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBurnerMessage:
			_BurnerParserOutput(message);
			break;
		case kBurnDiscMessage:
			BurnDisc();
			break;
		case B_REFS_RECEIVED:
			_AddTrack(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Private Methods --


void
CompilationAudioView::_BurnerParserOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		data << "\n";
		fBurnerInfoTextView->Insert(data.String());
	}
	int32 code = -1;
	if (message->FindInt32("thread_exit", &code) == B_OK) {
		if (code == 0) {
			fBurnerInfoBox->SetLabel("Burning complete. Burn another disc?");
			BButton* burnDiscButton
				= dynamic_cast<BButton*>(FindView("BurnDiscButton"));
			if (burnDiscButton != NULL)
				burnDiscButton->SetEnabled(true);
		}
	}
}


void
CompilationAudioView::_AddTrack(BMessage* message)
{
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

		// Check for wav MIME type or file extension
		if ((strcmp("audio/x-wav", mimeTypeString) == 0)
			|| filename.IFindLast(".wav", filename.CountChars())
				== filename.CountChars() - 4) {
			fTrackPaths[fCurrentPath++] = trackPath;
			BStringItem* item = new BStringItem(filename);
			fAudioList->AddItem(item);
		}

		if (node.IsDirectory()) {
			BDirectory dir(&entry);
			entry_ref ref;

			while (dir.GetNextRef(&ref) == B_OK) {
				BNode inDirNode(&ref);
				if (inDirNode.InitCheck() != B_OK)
					return;

				BNodeInfo InDirNodeInfo(&inDirNode);
				if (InDirNodeInfo.InitCheck() != B_OK)
					return;

				InDirNodeInfo.GetType(mimeTypeString);

				BPath* trackPath = new BPath(&ref);
				BString filename(trackPath->Leaf());

				// Check for wav MIME type or file extension
				if ((strcmp("audio/x-wav", mimeTypeString) == 0)
					|| filename.IFindLast(".wav", filename.CountChars())
						== filename.CountChars() - 4) {
					fTrackPaths[fCurrentPath++] = trackPath;
					BStringItem* item = new BStringItem(filename);
					fAudioList->AddItem(item);
				}
			}
		}
		if (fCurrentPath > 0) {
			BButton* burnDiscButton
				= dynamic_cast<BButton*>(FindView("BurnDiscButton"));
			if (burnDiscButton != NULL)
				burnDiscButton->SetEnabled(true);
		}
	i++;
	}
}


#pragma mark -- Public Methods --


void
CompilationAudioView::BurnDisc()
{
	if (fAudioList->IsEmpty())
		return;

	fBurnerInfoTextView->SetText(NULL);
	fBurnerInfoBox->SetLabel("Burning in progress" B_UTF8_ELLIPSIS);

	BButton* burnDiscButton
		= dynamic_cast<BButton*>(FindView("BurnDiscButton"));
	if (burnDiscButton != NULL)
		burnDiscButton->SetEnabled(false);

	BString device("dev=");
	device.Append(windowParent->GetSelectedDevice().number.String());
	sessionConfig config = windowParent->GetSessionConfig();
	
	fBurnerThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kBurnerMessage), this));
	
	fBurnerThread->AddArgument("cdrecord");

	if (config.simulation)
		fBurnerThread->AddArgument("-dummy");
	if (config.eject)
		fBurnerThread->AddArgument("-eject");
	if (config.mode == "-sao")
		fBurnerThread->AddArgument("-dao");	// for max compatibility
	else
		fBurnerThread->AddArgument(config.mode);

	fBurnerThread->AddArgument("speed=4")	// for max compatibility
		->AddArgument("fs=4m")				// for max compatibility
		->AddArgument(device)
		->AddArgument("-audio")
		->AddArgument("-copy")
		->AddArgument("-pad")
		->AddArgument("padsize=63s");

	for (unsigned int ix = 0; ix <= MAX_TRACKS; ix++) {
		if (fTrackPaths[ix] == NULL)
			break;
		fBurnerThread->AddArgument(fTrackPaths[ix]->Path());
	}
	fBurnerThread->Run();
}
