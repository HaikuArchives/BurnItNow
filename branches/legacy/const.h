/*
 * Copyright 2000-2002, Johan Nilsson. All rights reserved.
 * Copyright 2011 BurnItNow maintainers 
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONST_H_
#define _CONST_H_


const uint32 MENU_FILE_QUIT = 'MFqu';
const uint32 MENU_FILE_ABOUT = 'MFab';
const uint32 MENU_HELP = 'MHlp';
const uint32 OPEN_ISO_FILE = 'OPif';
const uint32 MAKE_IMAGE = 'MaIm';
const uint32 MAKE_AND_SAVE_IMAGE = 'MASI';
const uint32 BLANK_IT_NOW = 'BlIN';
const uint32 BLANK_FAST = 'BlFa';
const uint32 BLANK_FULL = 'BlFu';
const uint32 BLANK_SESSION = 'BlSe';
const uint32 BLANK_TRACK = 'BlTa';
const uint32 BLANK_TRACK_TAIL = 'BlTT';
const uint32 BLANK_UNRES = 'BlUr';
const uint32 BLANK_UNCLOSE = 'BlUc';
const uint32 BURN_DATA_CD = 'BuDC';
const uint32 BURN_AUDIO_CD = 'BuAC';
const uint32 BURN_MIX_CD = 'BuMC';
const uint32 BURN_MULTI = 'BuMu';
const uint32 BURN_ONTHEFLY = 'BuOF';
const uint32 BURN_DUMMY_MODE = 'BuDM';
const uint32 BURN_EJECT = 'BuEj';


const uint32 DATA_ISO9660 = 'DaIS';
const uint32 DATA_HFS = 'DaHF';
const uint32 DATA_BFS = 'DaBF';
const uint32 DATA_WINDOWS = 'DaWI';
const uint32 DATA_REALROCK = 'DaRR';
const uint32 DATA_JOLIET = 'DaJo';
const uint32 DATA_ROCK = 'DaRo';

const uint32 DATA_VIRTUALCD = 'DaVC';
const uint32 DATA_ISOFILE = 'DaIF';


const uint32 BOOT_CHECKED = 'BoCh';
const uint32 BOOT_FILE_PANEL = 'BoFP';
const uint32 BOOT_CHANGE_IMAGE_NAME = 'BCIN';

const uint32 VOLUME_NAME = 'VolN';
const uint32 CHANGE_VOL_NAME = 'ChVN';

const uint32 SPEED_CHANGE = 'SpCh';
const uint32 BLANK_SPEED_CHANGE = 'BSCh';


const uint32 PARENT_DIR = 'PaDi';
const uint32 MAKE_DIR = 'MaDi';
const uint32 NEW_VRCD = 'NeVR';
const uint32 MAKE_DIRECTORY = 'MDir';

const uint32 AUDIO_PAD = 'AuPa';
const uint32 AUDIO_PREEMP = 'AuPr';
const uint32 AUDIO_SWAB = 'AuSw';
const uint32 AUDIO_NOFIX = 'AuNF';

const uint32 MISC_DAO = 'MiDA';
const uint32 MISC_BURNPROOF = 'MiBP';


const uint32 BURN_WITH_CDRECORD = 'BWCR';
const uint32 SET_BUTTONS_TRUE = 'SBTr';
const uint32 SET_BUTTONS_FALSE = 'SBFa';
const uint32 WRITE_TO_LOG = 'WTLo';
const uint32 CALCULATE_SIZE = 'CaSi';


const rgb_color red = {200, 0, 0, 255};
const rgb_color red2 = {255, 0, 0, 255};
const rgb_color black = {0, 0, 0, 255};
const rgb_color green = {0, 200, 50, 255};
const rgb_color blue = {0, 0, 200, 255};
const rgb_color white = {255, 255, 255, 255};
const rgb_color darkblue = {70, 70, 200, 255};
const rgb_color greenblue = {0, 255, 255, 255};


struct cdrecorder {
	char scsiid[8];
	char scsi_vendor[20];
	char scsi_name[50];
};


#endif	// _CONST_H_
