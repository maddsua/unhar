//  2022 maddsua | https://gitlab.com/maddsua
//	No warranties are given, etc...
// This file is a component of the HAR Extractor

#ifndef STATCFG
#define STATCFG

#define VER_AUTHSTAMP	"2022 maddsua"

#define windowSizeX			285
#define windowSizeY			320

#define stcTxtStrBuff		32

#define jsonMinLen		16
#define fsMaxName		128
#define netEmptyResp	8
#define coreUtilPers	0.75
#define weakCPUcores	4


#define parFlagArray	5
#define pf_runn			0
#define pf_addr			1
#define pf_type			2
#define pf_encd			3
#define pf_cont			4

#define IMG_BANNER		1000
#define IMG_FILE		1001
#define IMG_FOLDER		1002
#define IMG_HELP		1003

#define GUIID_BANNER 	10
#define GUIID_SRCFILE	11
#define GUIID_SELFILE	12
#define GUIID_DESTDIR	13
#define GUIID_SELDIR	14
#define GUIID_SELMIME	15
#define GUIID_BTNRUN	16
#define GUIID_LABEL_0	17
#define GUIID_LABEL_1	18
#define GUIID_LABEL_2	19
#define GUIID_PROGINFO	20

#define GUIID_PROGRESS	25
#define GUIID_BTNHELP	26

#define datypeID_All	0
#define datypeID_Text	1
#define datypeID_Media	2
#define datypeID_TS		3
#define datypeID_Video	4
#define datypeID_Audio	5
#define datypeID_Img	6
#define datypeID_Font	7
#define datypeID_App	8

#define ffmpegListFile	"mpegtsparts.txt"

#endif
