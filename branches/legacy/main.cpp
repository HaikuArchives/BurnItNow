/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "jpWindow.h"

#include "const.h"

#include <Application.h>
#include <Entry.h>
#include <Path.h>


extern char* IMAGE_NAME;
extern char* BURNIT_PATH;
extern char* BURN_DIR;


class jpApp : public BApplication
{
public:
	jpApp();
	virtual void RefsReceived(BMessage* msg);
	virtual void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();
private:
	jpWindow* baseWindow;
};


jpApp::jpApp()
	:
	BApplication("application/x-vnd.osdrawer-BurnItNow")
{
	BRect windowRect;
	windowRect.Set(200, 30, 700, 530);
	baseWindow = new jpWindow(windowRect);
}


bool jpApp::QuitRequested()
{
	BApplication::QuitRequested();
	return true;
}


void jpApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MAKE_DIRECTORY:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		case BURN_WITH_CDRECORD:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		case WRITE_TO_LOG:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		case SET_BUTTONS_TRUE:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		case VOLUME_NAME:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		case BOOT_CHANGE_IMAGE_NAME:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		case B_SAVE_REQUESTED:
			baseWindow->Lock();
			baseWindow->MessageReceived(msg);
			baseWindow->Unlock();
			break;
		default:
			BApplication::MessageReceived(msg);
	}
}


void jpApp::RefsReceived(BMessage* msg)
{
	entry_ref ref;
	msg->FindRef("refs", &ref);
	BEntry entry(&ref, true);
	BPath path;
	entry.GetPath(&path);
	char* temppath = (char*)path.Path();

	char BurnDir[1024];
	strcpy(BurnDir, temppath);

	baseWindow->Lock();
	baseWindow->SetISOFile(BurnDir);
	baseWindow->Unlock();
}


int main()
{
	jpApp* testapp;
	testapp = new jpApp;
	testapp->Run();
	delete testapp;
	delete BURNIT_PATH;
	delete BURN_DIR;
	delete IMAGE_NAME;
	return 0;
}
