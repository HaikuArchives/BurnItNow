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
#define kControlPadding		(be_control_look->DefaultItemSpacing())

// AudioCDs have a larger capacity than DataCDs
enum {
	AUDIO = 0,
	DATA
};

// some tabs deal with CDs/DVDs, some are CD only or DVD only
enum {
	CD_OR_DVD = 0,
	CD_ONLY,
	DVD_ONLY
};

// flags which fAction is in progress
enum {
	IDLE = 0,
	BUILDING,
	BURNING,
	BLANKING
};

// flags from the OutputParser
enum {
	INVALIDWAV = -2,
	SMALLDISC = -1,
	NOCHANGE = 0,
	PERCENT
};

// Message constants
const int32 kOpenHelp = 'Help';
const int32 kOpenWebsite = 'Site';
const int32 kSetCacheFolder = 'Scfo';
const int32 kOpenCacheFolder = 'Ocfo';
const int32 kChooseCacheFolder = 'Cusd';
const int32 kCacheQuit = 'Ccqt';
const int32 kClearCache = 'Cche';
const int32 kSpeedSlider = 'Sped';

const int32 kChooseButton = 'ChoB';

const int32 kTrackSelection = 'Tsel';
const int32 kUpButton = 'UppB';
const int32 kDownButton = 'DwnB';
const int32 kAddButton = 'AddB';
const int32 kRemoveButton = 'RemB';

const int32 kCheckOutput = 'ChkO';

const int32 kBurnButton = 'BurB';
const int32 kBurnOutput = 'BrnO';

const int32 kBuildButton = 'BilB';
const int32 kBuildOutput = 'BilO';
const int32 kGetImageSizeOutput = 'ImgO';

const int32 kBlankButton = 'BlnB';
const int32 kBlankOutput = 'BlnO';

const int32 kDraggedItem = 'drit';
const int32 kDeleteItem = 'deli';
const int32 kPopupClosed = 'popc';

const int32 kCalculateSize = 'clcs';
const int32 kSetFolderSize = 'stsz';

const uint32 kDeviceChange[MAX_DEVICES]
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
static const float sizeCD900[] = { 1023257, 890999 };
static const float sizeDVD5[] = { 4592762, 4592762 };
static const float sizeDVD9[] = { 8545894, 8545894 };

// constants
static const BString kWebsiteUrl = "https://github.com/HaikuArchives/BurnItNow";
static const char kAppSignature[] = "application/x-vnd.haikuarchives-BurnItNow";
static const char kSettingsFile[] = "BurnItNow_settings";
static const char kCacheFileClone[] = "burnitnow_clone.iso";
static const char kCacheFileDVD[] = "burnitnow_dvd.iso";
static const char kCacheFileData[] = "burnitnow_data.iso";

static const char kCopyright[] = "2010-2017";
static const char kVersion[] = "v1.0";

#endif	// _CONSTANTS_H_
