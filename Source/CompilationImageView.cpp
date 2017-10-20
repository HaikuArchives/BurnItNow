/*
 * Copyright 2010-2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <compat/sys/stat.h>

#include <Alert.h>
#include <ControlLook.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>

#include "CommandThread.h"
#include "CompilationImageView.h"
#include "Constants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Image view"


CompilationImageView::CompilationImageView(BurnWindow& parent)
	:
	BView(B_TRANSLATE("Image file"), B_WILL_DRAW,
		new BGroupLayout(B_VERTICAL, kControlPadding)),
	fImageParserThread(NULL),
	fOpenPanel(NULL),
	fImagePath(new BPath()),
	fNotification(B_PROGRESS_NOTIFICATION),
	fProgress(0),
	fETAtime("--"),
	fParser(fProgress, fETAtime)
{
	fWindowParent = &parent;
	fAction = IDLE;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fInfoView = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	fInfoView->SetFont(be_bold_font);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT("Choose the image to burn",
		"Status notification"));

	fPathView = new PathView("ImageFileStringView",
		B_TRANSLATE("Image: <none>"));

	fOutputView = new BTextView("OutputView");
	fOutputView->SetWordWrap(false);
	fOutputView->MakeEditable(false);
	BScrollView* fOutputScrollView = new BScrollView("OutputScroller",
		fOutputView, B_WILL_DRAW, true, true);
	fOutputScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 64));

	fChooseButton = new BButton("ChooseImageButton",
		B_TRANSLATE("Choose image"), new BMessage(kChoose));
	fChooseButton->SetTarget(this);

	fBurnButton = new BButton("BurnImageButton", B_TRANSLATE("Burn disc"),
		new BMessage(kBurnDisc));
	fBurnButton->SetTarget(this);

	fSizeView = new SizeView();

	BLayoutBuilder::Group<>(dynamic_cast<BGroupLayout*>(GetLayout()))
		.SetInsets(kControlPadding)
		.AddGroup(B_HORIZONTAL)
			.Add(fPathView)
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fChooseButton)
				.Add(fBurnButton)
				.End()
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fInfoView)
			.Add(fOutputScrollView)
			.End()
		.Add(fSizeView);

	_UpdateSizeBar();
}


CompilationImageView::~CompilationImageView()
{
	delete fImagePath;
	delete fImageParserThread;
	delete fOpenPanel;
}


#pragma mark -- BView Overrides --


void
CompilationImageView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fChooseButton->SetTarget(this);
	fChooseButton->SetEnabled(true);

	fBurnButton->SetTarget(this);
	fBurnButton->SetEnabled(false);
}


void
CompilationImageView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kChoose:
			_ChooseImage();
			break;
		case kBurnDisc:
			_BurnImage();
			break;
		case kParser:
			_BurnOutput(message);
			break;
		case B_REFS_RECEIVED:
			_OpenImage(message);
			break;
		default:
			BView::MessageReceived(message);
	}
}


#pragma mark -- Public Methods --


int32
CompilationImageView::InProgress()
{
	return fAction;
}


#pragma mark -- Private Methods --


bool
ImageRefFilter::Filter(const entry_ref* ref, BNode* node,
	struct stat_beos* stat, const char* filetype)
{
	if (S_ISDIR(stat->st_mode) || (S_ISLNK(stat->st_mode)))
		return true;

	BPath* path = new BPath(ref);
	BString filename(path->Leaf());

	if ((strcmp("application/x-cd-image", filetype) == 0)
		|| filename.IFindLast(".iso", filename.CountChars())
			== (filename.CountChars() - 4)
		|| filename.IFindLast(".img", filename.CountChars())
			== (filename.CountChars() - 4)) {
		return true;
	}
	return false;
}


void
CompilationImageView::_BurnImage()
{
	if (fImagePath->Path() == NULL) {
		(new BAlert("ChooseImageFirstAlert", B_TRANSLATE(
			"First choose an image to burn."), B_TRANSLATE("OK")))->Go();
		return;
	}
	if (fImagePath->InitCheck() != B_OK)
		return;

	if (fImageParserThread != NULL)
		delete fImageParserThread;

	fAction = BURNING;	// flag we're burning

	fOutputView->SetText(NULL);
	fInfoView->SetLabel(B_TRANSLATE_COMMENT(
		"Burning in progress" B_UTF8_ELLIPSIS, "Status notification"));
	fChooseButton->SetEnabled(false);
	fBurnButton->SetEnabled(false);

	fNotification.SetGroup("BurnItNow");
	fNotification.SetMessageID("BurnItNow_Image");
	fNotification.SetTitle(B_TRANSLATE("Burning image"));
	fNotification.SetProgress(0);
	fNotification.Send(60 * 1000000LL);

	BString device("dev=");
	device.Append(fWindowParent->GetSelectedDevice().number.String());
	sessionConfig config = fWindowParent->GetSessionConfig();

	fImageParserThread = new CommandThread(NULL,
		new BInvoker(new BMessage(kParser), this));

	fImageParserThread->AddArgument("cdrecord");

	if (config.simulation)
		fImageParserThread->AddArgument("-dummy");
	if (config.eject)
		fImageParserThread->AddArgument("-eject");
	if (config.speed != "")
		fImageParserThread->AddArgument(config.speed);

	fImageParserThread->AddArgument(config.mode)
		->AddArgument("fs=16m")
		->AddArgument(device)
		->AddArgument("-v")	// to get progress output
		->AddArgument("-gracetime=2")
		->AddArgument("-pad")
		->AddArgument("padsize=63s")
		->AddArgument(fImagePath->Path())
		->Run();

	fParser.Reset();
}


void
CompilationImageView::_BurnOutput(BMessage* message)
{
	BString data;

	if (message->FindString("line", &data) == B_OK) {
		BString text = fOutputView->Text();
		int32 modified = fParser.ParseLine(text, data);
		if (modified == NOCHANGE) {
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
	if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (fAction == BUILDING)) {
		fInfoView->SetLabel(B_TRANSLATE_COMMENT("Burn the disc",
			"Status notification"));
		fChooseButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fAction = IDLE;

	} else if ((message->FindInt32("thread_exit", &code) == B_OK)
			&& (fAction == BURNING)) {
		fInfoView->SetLabel(B_TRANSLATE_COMMENT(
			"Burning complete. Burn another disc?", "Status notification"));
		fChooseButton->SetEnabled(true);
		fBurnButton->SetEnabled(true);

		fNotification.SetMessageID("BurnItNow_Image");
		fNotification.SetProgress(100);
		fNotification.SetContent(B_TRANSLATE("Burning finished!"));
		fNotification.Send();

		fAction = IDLE;
		fParser.Reset();
	}
}


void
CompilationImageView::_ChooseImage()
{
	// TODO Create a RefFilter for the panel?
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
			NULL, B_FILE_NODE, false, NULL, new ImageRefFilter(), true);
		fOpenPanel->Window()->SetTitle(B_TRANSLATE("Choose image"));
	}
	fOpenPanel->Show();
}


void
CompilationImageView::_OpenImage(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK)
		return;

	BEntry entry(&ref, true);	// also accept symlinks
	BNode node(&entry);
	if (node.InitCheck() != B_OK)
		return;

	BNodeInfo nodeInfo(&node);
	if (nodeInfo.InitCheck() != B_OK)
		return;

	char mimeTypeString[B_MIME_TYPE_LENGTH];
	nodeInfo.GetType(mimeTypeString);
	BPath* path = new BPath(&entry);
	BString filename(path->Leaf());

	// Check for wav MIME type or file extension
	if ((strcmp("application/x-cd-image", mimeTypeString) == 0)
		|| filename.IFindLast(".iso", filename.CountChars())
			== (filename.CountChars() - 4)
		|| filename.IFindLast(".img", filename.CountChars())
			== (filename.CountChars() - 4)) {

		fImagePath->SetTo(&entry);
		fPathView->SetText(fImagePath->Path());
		fOutputView->SetText(NULL);
		fBurnButton->SetEnabled(true);

		if (fImageParserThread != NULL)
			delete fImageParserThread;

		fAction = BUILDING;	// flag we're opening ISO

		fImageParserThread = new CommandThread(NULL,
			new BInvoker(new BMessage(kParser), this));
		fImageParserThread->AddArgument("isoinfo")
			->AddArgument("-d")
			->AddArgument("-i")
			->AddArgument(fImagePath->Path())
			->Run();
	}
	_UpdateSizeBar();
}


void
CompilationImageView::_UpdateProgress()
{
	if (fProgress == 0 || fProgress == 1.0)
		fNotification.SetContent(" ");
	else
		fNotification.SetContent(fETAtime);
	fNotification.SetMessageID("BurnItNow_Image");
	fNotification.SetProgress(fProgress);
	fNotification.Send();
}


void
CompilationImageView::_UpdateSizeBar()
{
	off_t fileSize = 0;

	BEntry entry(fImagePath->Path());
	entry.GetSize(&fileSize);

	fSizeView->UpdateSizeDisplay(fileSize / 1024, DATA, CD_OR_DVD);
	// size in KiB
}
