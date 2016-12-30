/*
 * Copyright 2016. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *	Humdinger, humdingerb@gmail.com
 */

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Message.h>

#include <stdio.h>
#include <stdlib.h>

#include "AppSettings.h"

static const char kSettingsFile[] = "BurnItNow_settings";

AppSettings::AppSettings()
	:
	fEject(true),
	fCache(false),
	fSpeed(5),
	fPosition(150, 150, 700, 600),
	fInfoWeight(0.5),
	fTracksWeight(0.5),
	fInfoCollapse(false),
	fTracksCollapse(false),
	dirtySettings(false)

{
	BPath path;
	BMessage msg;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		status_t ret = path.Append(kSettingsFile);
		if (ret == B_OK) {
			BFile file(path.Path(), B_READ_ONLY);

			if ((file.InitCheck() == B_OK) && (msg.Unflatten(&file) == B_OK)) {
				if (msg.FindBool("eject", &fEject) != B_OK) {
					fEject = true;
					dirtySettings = true;
				}
				if (msg.FindBool("cache", &fCache) != B_OK) {
					fCache = false;
					dirtySettings = true;
				}
				if (msg.FindInt32("speed", &fSpeed) != B_OK) {
					fSpeed = 5;
					dirtySettings = true;
				}
				if (msg.FindRect("windowlocation", &fPosition) != B_OK)
					fPosition.Set(150, 150, 700, 600);

				if (msg.FindFloat("audio_split_info", &fInfoWeight) != B_OK)
					fInfoWeight = 0.5;

				if (msg.FindFloat("audio_split_tracks", &fTracksWeight) != B_OK)
					fTracksWeight = 0.5;

				if (msg.FindBool("audio_collapse_info", &fInfoCollapse) != B_OK)
					fInfoCollapse = false;

				if (msg.FindBool("audio_collapse_tracks", &fTracksCollapse) != B_OK)
					fTracksCollapse = false;
			}
		}
	}
}


AppSettings::~AppSettings()
{
	if (!dirtySettings)
		return;

	BPath path;
	BMessage msg;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return;

	status_t ret = path.Append(kSettingsFile);
	if (ret == B_OK) {
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
		ret = file.InitCheck();

		if (ret == B_OK) {
			msg.AddBool("eject", fEject);
			msg.AddBool("cache", fCache);
			msg.AddInt32("speed", fSpeed);
			msg.AddRect("windowlocation", fPosition);
			msg.AddFloat("audio_split_info", fInfoWeight);
			msg.AddFloat("audio_split_tracks", fTracksWeight);
			msg.AddBool("audio_collapse_info", fInfoCollapse);
			msg.AddBool("audio_collapse_tracks", fTracksCollapse);
			msg.Flatten(&file);
		}
	}
}


bool
AppSettings::Lock()
{
	return fLock.Lock();
}


void
AppSettings::Unlock()
{
	fLock.Unlock();
}


bool
AppSettings::GetCache()
{
	return fCache;
}


bool
AppSettings::GetEject()
{
	return fEject;
}


int32
AppSettings::GetSpeed()
{
	return fSpeed;
}


BRect
AppSettings::GetWindowPosition()
{
	return fPosition;
}


void
AppSettings::GetSplitWeight(float& left, float& right)
{
	left = fInfoWeight;
	right = fTracksWeight;
}


void
AppSettings::GetSplitCollapse(bool& left, bool& right)
{
	left = fInfoCollapse;
	right = fTracksCollapse;
}


void
AppSettings::SetEject(bool eject)
{
	if (fEject == eject)
		return;
	fEject = eject;
	dirtySettings = true;
}


void
AppSettings::SetCache(bool cache)
{
	if (fCache == cache)
		return;
	fCache = cache;
	dirtySettings = true;
}


void
AppSettings::SetSpeed(int32 speed)
{
	if (fSpeed == speed)
		return;
	fSpeed = speed;
	dirtySettings = true;
}


void
AppSettings::SetWindowPosition(BRect where)
{
	if (fPosition == where)
		return;
	fPosition = where;
	dirtySettings = true;
}


void
AppSettings::SetSplitWeight(float left, float right)
{
	if ((fInfoWeight == left) && (fTracksWeight == right))
		return;
	fInfoWeight = left;
	fTracksWeight = right;
	dirtySettings = true;
}


void
AppSettings::SetSplitCollapse(bool left, bool right)
{
	if ((fInfoCollapse == left) && (fTracksCollapse == right))
		return;
	fInfoCollapse = left;
	fTracksCollapse = right;
	dirtySettings = true;
}
