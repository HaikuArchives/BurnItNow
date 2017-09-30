/*
 * Copyright 2017, BurnItNow Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#include <ControlLook.h>
#include <String.h>


#define MAX_DEVICES 		5
#define MAX_DRAG_HEIGHT		200.0
#define ALPHA				170

// Message constants
const int32 kOpenHelpMessage = 'Help';
const int32 kOpenWebsiteMessage = 'Site';
const int32 kCacheQuitMessage = 'Ccqt';
const int32 kClearCacheMessage = 'Cche';
const int32 kSpeedSliderMessage = 'Sped';

const int32 kBurnDiscMessage = 'BURN';
const int32 kBurnerMessage = 'Brnr';
const int32 kBuildImageMessage = 'IMAG';

const int32 kDraggedItemMessage = 'drit';
const int32 kDeleteItemMessage = 'deli';
const int32 kPopupClosedMessage = 'popc';

const int32 kBlankMessage = 'Blnk';
const int32 kBlankerMessage = 'Blkr';

const int32 kCreateImageMessage = 'Crat';
const int32 kBurnImageMessage = 'Wrte';
const int32 kClonerMessage = 'Clnr';
const int32 kChooseDirectoryMessage = 'Cusd';

const int32 kChooseImageMessage = 'Chus';
const int32 kParserMessage = 'Scan';

const int32 kCalculateSize = 'clcs';
const int32 kSetFolderSize = 'stsz';
const int32 kNoPathMessage = 'Noph';

const uint32 kDeviceChangeMessage[MAX_DEVICES]
	= { 'DVC0', 'DVC1', 'DVC2', 'DVC3', 'DVC4' };

// colours
const rgb_color colorCD650 = {5, 208, 5, 255};
const rgb_color colorCD700 = {216, 255, 6, 255};
const rgb_color colorCD800 = {255, 170, 6, 255};
const rgb_color colorCD900 = {5, 144, 216, 255};
const rgb_color colorDVD5 = {210, 185, 136, 255};
const rgb_color colorDVD9 = {255, 95, 226, 255};
const rgb_color colorTooBig = {255, 82, 82, 255};

const rgb_color colorCD650_bg = {174, 242, 174, 255};
const rgb_color colorCD700_bg = {233, 255, 174, 255};
const rgb_color colorCD800_bg = {255, 221, 179, 255};
const rgb_color colorCD900_bg = {174, 207, 247, 255};
const rgb_color colorDVD5_bg = {233, 224, 207, 255};
const rgb_color colorDVD9_bg = {255, 192, 241, 255};
const rgb_color colorTooBig_bg = {255, 183, 183, 255};

// capacities, audio [0] and data [1] in KiB
static const float sizeCD650[] = { 764859, 666000 };
static const float sizeCD700[] = { 826875, 720000 };
static const float sizeCD800[] = { 930234, 810000 };
static const float sizeCD900[] = { 1023257, 4592762 };
static const float sizeDVD5[] = { 4592762, 4592762 };
static const float sizeDVD9[] = { 8545894, 4592762 };

// constants
static const BString kWebsiteUrl = "https://github.com/HaikuArchives/BurnItNow";
static const char kAppSignature[] = "application/x-vnd.haikuarchives-BurnItNow";
static const char kSettingsFile[] = "BurnItNow_settings";
static const char kCopyright[] = "2010-2017";
static const char kVersion[] = "v1.0";

static const float kControlPadding = be_control_look->DefaultItemSpacing();

#endif	// _CONSTANTS_H_
