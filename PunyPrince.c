/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Puny Prince v1.0 (May 2023)
 * Copyright (C) 2023 Norbert de Jonge <nlmdejonge@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see [ www.gnu.org/licenses/ ].
 *
 * To properly read this code, set your program's tab stop to: 2.
 */

/*========== Includes ==========*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#include <windows.h>
#undef PlaySound
#endif

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_thread.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
/*========== Includes ==========*/

/*========== Defines ==========*/
#define EXIT_NORMAL 0
#define EXIT_ERROR 1
#define PROG_NAME "Puny Prince"
#define PROG_VERSION "v1.0 (May 2023)"
#define COPYRIGHT "Copyright (C) 2023 Norbert de Jonge"
#define ROOMS 24
#define TILES 30
#define EVENTS 256
#define DIR_GAMES "games"
#define FILE_DAT "LEVELS.DAT"
#define DIR_DAT "LEVELS.DAT"
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 576
#define NUM_SOUNDS 20 /*** Sounds that may play at the same time. ***/
#define REFRESH_PROG 20 /*** That is 50 fps (1000/20). ***/
#define REFRESH_GAME 80 /*** That is 12.5 fps (1000/80). ***/
#define START_LIVES 3

#define MAX_PATHFILE 600
#define MAX_TOWRITE 720
#define MAX_LINE 100
#define MAX_IMG 200
#define MAX_OPTION 100
#define MAX_GAMES 15
#define MAX_MESSAGE 100

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined WIN32 || _WIN32 || WIN64 || _WIN64
#define SLASH "\\"
#else
#define SLASH "/"
#endif
/*========== Defines ==========*/

char cNextDrop, cNextRaise;
int iCurLevel, iCurRoom;
SDL_Window *window;
SDL_Renderer *ascreen;
SDL_Cursor *curArrow;
SDL_Cursor *curWait;
SDL_Cursor *curHand;
int iNoAudio;
TTF_Font *font;
Uint32 oldticks, newticks;
unsigned int progspeed;
Uint32 looptime;
int iOnYes, iOnNo;
int iGoRoom, iGoTile;
int iStartLevel;
int iMode;
int iZoom;
int iCheat;
int iFullscreen;
int iJump;
int iCareful;
int iRunJump;
int iMaxLives;
int iCurLives;
int iLevLives; /*** Obtained during iCurLevel. ***/
int iXPos, iYPos;
char sMessage[MAX_MESSAGE + 2];

/*** DAT ***/
unsigned char sChecksum[1 + 2];
unsigned char sGroup[(ROOMS * TILES) + 2];
unsigned char sVariant[(ROOMS * TILES) + 2];
unsigned char sFirstDoorEvents[EVENTS + 2];
unsigned char sSecondDoorEvents[EVENTS + 2];
unsigned char sRoomLinks[(ROOMS * 4) + 2];
unsigned char sUnknownI[64 + 2];
unsigned char sStartPosition[3 + 2];
unsigned char sUnknownIIandIII[4 + 2];
unsigned char sGuardLocations[ROOMS + 2];
unsigned char sGuardDirections[ROOMS + 2];
unsigned char sUnknownIVaandIVb[48 + 2];
unsigned char sGuardSkills[ROOMS + 2];
unsigned char sUnknownIVc[24 + 2];
unsigned char sGuardColors[ROOMS + 2];
unsigned char sUnknownIVd[16 + 2];
unsigned char sEndCode[2 + 2];
/***/
int arGroup[ROOMS + 2][TILES + 2];
int arVariant[ROOMS + 2][TILES + 2];
int arRoomLinks[ROOMS + 2][4 + 2];
unsigned char arEventsRoom[EVENTS + 2];
unsigned char arEventsTile[EVENTS + 2];
int arEventsNext[EVENTS + 2];

/*** For listing games. ***/
int iNrGames;
char arGames[MAX_PATHFILE + 2][MAX_GAMES + 2];
int iGameSel;

/*** For running games. ***/
char arTiles[ROOMS + 2][TILES + 2];
int arGateTimers[ROOMS + 2][TILES + 2];
int arLettersRoom[(int)'r' + 2][10 + 2];
int arLettersTile[(int)'r' + 2][10 + 2];
int arLinksL[ROOMS + 2];
int arLinksR[ROOMS + 2];
int arLinksU[ROOMS + 2];
int arLinksD[ROOMS + 2];
int iPrinceTile;
int iPrinceDir; /*** Keeping track of this for climbing and such. ***/
int iPrinceHang; /*** 1 = Hanging allowed. 2 = Currently hanging. ***/
int iPrinceSword;
int iPrinceFloat;
int iPrinceSafe;
int iPrinceCoins;
/***/
int arGuardLoc[ROOMS + 2];
/***
1 = [E]asy
2 = [H]ard
3 = [J]affar
4 = [S]hadow
5 = [M]ouse
***/
int arGuardType[ROOMS + 2];
int arGuardHP[ROOMS + 2];
int arGuardAttack[ROOMS + 2];
/***/
int iFlash;
int iFlashR, iFlashG, iFlashB;
/***/
int iSteps; /*** Coins are in iPrinceCoins. ***/
int iShowStepsCoins;
/***/
int iCoinsInLevel;
int iNoMessage;
/***/
int iMouse;
int iMirror;

/*** for text ***/
SDL_Color color_bl = {0x00, 0x00, 0x00, 255};
SDL_Color color_wh = {0xff, 0xff, 0xff, 255};
SDL_Surface *text;
SDL_Texture *textt;
SDL_Rect offset;

static const unsigned long arDATOffsets[16] =
{
	0x0006, /*** demo ***/
	0x0908, /*** 1 ***/
	0x120A, /*** 2 ***/
	0x1B0C, /*** 3 ***/
	0x240E, /*** 4 ***/
	0x2D10, /*** 5 ***/
	0x3612, /*** 6 ***/
	0x3F14, /*** 7 ***/
	0x4816, /*** 8 ***/
	0x5118, /*** 9 ***/
	0x5A1A, /*** 10 ***/
	0x631C, /*** 11 ***/
	0x6C1E, /*** 12a ***/
	0x7520, /*** 12b ***/
	0x7E22, /*** princess ***/
	0x8724 /*** potions ***/
};

static const int arWidth[5] =
{
	8, 8, 8, 9, 9
};
static const int arHeight[5] =
{
	8, 14, 16, 14, 16
};

SDL_Texture *imgscreend;
SDL_Texture *imgscreeng;
SDL_Texture *imgchkb;
SDL_Texture *imgfont[5 + 2];
SDL_Texture *imgfade[5 + 2];
SDL_Texture *imgpopupyn;
SDL_Texture *imgyesoff, *imgyeson;
SDL_Texture *imgnooff, *imgnoon;

struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
} sounds[NUM_SOUNDS];

void ShowUsage (void);
void Generate (void);
int ReadFromFile (int iFd, int iSize, unsigned char *sRetString);
void CreateDir (char *sDir);
void LoadDAT (int iFd, int iUnkSize);
void GetAsEightBits (unsigned char cChar, char *sBinary);
int BitsToInt (char *sString);
int OpenRead (int iLevel, char cType);
int OpenTrunc (int iLevel, char cType);
void SavePuny (int iLevel);
char GetLetter (int iFdE, int iVariant, int iType);
void GetGames (void);
void ListGames (void);
void ShowListGames (void);
void LoadLevel (int iLevel, int iLives);
int ReadLine (int iFd, char *sRetString);
void RunGame (void);
void ShowGame (void);
void LoadFonts (void);
void MixAudio (void *unused, Uint8 *stream, int iLen);
void PreLoad (char *sPNG, SDL_Texture **imgImage);
void Quit (void);
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo);
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY);
void ToggleFullscreen (void);
void ShowTile (char cTile, int iX, int iY, int iFade);
void ShowLiving (char cLiving, int iX, int iY);
void ShowChar (int iW, int iH, int iR, int iG, int iB,
	int iX, int iY, int iM, int iZ, int iFade);
int PopUp (char *sQuestion);
void ShowPopUp (char *sQuestion);
void DisplayText (int iX, int iY, char *sText);
void TryGoLeft (int iTurns);
void TryGoRight (int iTurns);
void TryGoUp (void);
void TryGoDown (void);
void ToggleJump (void);
void ToggleCareful (void);
void ToggleRunJump (void);
char GetCharLeft (void);
char GetCharRight (void);
char GetCharUp (void);
char GetCharDown (void);
char GetCharUpLeft (void);
char GetCharUpRight (void);
int GetRoomLeft (void);
int GetRoomRight (void);
int GetRoomUp (void);
int GetRoomDown (void);
int GetRoomUpLeft (void);
int GetRoomUpRight (void);
int GetTileLeft (void);
int GetTileRight (void);
int GetTileUp (void);
int GetTileDown (void);
int GetTileUpLeft (void);
int GetTileUpRight (void);
int IsEmpty (char cChar);
int IsFloor (char cChar);
void GameActions (void);
void PushButton (char cChar, int iForever);
void Die (void);
void BossKey (void);
void ShowBossKey (void);
void PreventCPUEating (void);
void ExMarks (void);
void OpenURL (char *sURL);
void PlaySound (char *sFile);
void DropLoose (int iRoom, int iTile);
void GetOptionValue (char *sArgv, char *sValue);
void Initialize (void);
void ShowText (int iLine, char *sText, int iR, int iG, int iB, int iInGame);
void MovingStarts (void);
void MovingEnds (void);
int GetX (int iTile);
int GetY (int iTile);
void Teleport (char cGoTo);

/*****************************************************************************/
int main (int argc, char *argv[])
/*****************************************************************************/
{
	time_t tm;
	int iArgLoop;
	char sStartLevel[MAX_OPTION + 2];
	char sMode[MAX_OPTION + 2];
	char sZoom[MAX_OPTION + 2];

	/*** Defaults. ***/
	iCheat = 0;
	iFullscreen = 0;
	iStartLevel = 1;
	iMode = 2;
	iZoom = 2;

	if (argc > 1)
	{
		for (iArgLoop = 1; iArgLoop <= argc - 1; iArgLoop++)
		{
			if ((strcmp (argv[iArgLoop], "-h") == 0) ||
				(strcmp (argv[iArgLoop], "-?") == 0) ||
				(strcmp (argv[iArgLoop], "--help") == 0))
			{
				ShowUsage();
			}
			else if ((strcmp (argv[iArgLoop], "-v") == 0) ||
				(strcmp (argv[iArgLoop], "--version") == 0))
			{
				printf ("%s %s\n", PROG_NAME, PROG_VERSION);
				exit (EXIT_NORMAL);
			}
			else if ((strcmp (argv[iArgLoop], "-n") == 0) ||
				(strcmp (argv[iArgLoop], "--noaudio") == 0))
			{
				iNoAudio = 1;
			}
			else if ((strcmp (argv[iArgLoop], "-f") == 0) ||
				(strcmp (argv[iArgLoop], "--fullscreen") == 0))
			{
				iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP;
			}
			else if ((strcmp (argv[iArgLoop], "-g") == 0) ||
				(strcmp (argv[iArgLoop], "--generate") == 0))
			{
				Generate();
				exit (EXIT_NORMAL);
			}
			else if ((strncmp (argv[iArgLoop], "-l=", 3) == 0) ||
				(strncmp (argv[iArgLoop], "--level=", 8) == 0))
			{
				GetOptionValue (argv[iArgLoop], sStartLevel);
				iStartLevel = atoi (sStartLevel);
				if ((iStartLevel < 1) || (iStartLevel > 14))
				{
					iStartLevel = 1;
				}
			}
			else if ((strncmp (argv[iArgLoop], "-m=", 3) == 0) ||
				(strncmp (argv[iArgLoop], "--mode=", 7) == 0))
			{
				GetOptionValue (argv[iArgLoop], sMode);
				iMode = atoi (sMode);
				if ((iMode < 0) || (iMode > 4))
				{
					iMode = 2;
				}
			}
			else if ((strncmp (argv[iArgLoop], "-z=", 3) == 0) ||
				(strncmp (argv[iArgLoop], "--zoom=", 7) == 0))
			{
				GetOptionValue (argv[iArgLoop], sZoom);
				iZoom = atoi (sZoom);
				if ((iZoom < 1) || (iZoom > 7))
				{
					iZoom = 2;
				}
			}
			else if ((strcmp (argv[iArgLoop], "-c") == 0) ||
				(strcmp (argv[iArgLoop], "--cheat") == 0))
			{
				iCheat = 1;
			}
			else
			{
				ShowUsage();
			}
		}
	}

	srand ((unsigned)time(&tm));
	GetGames();
	switch (iNrGames)
	{
		case 0:
			printf ("[FAILED] No games in directory \"%s\"!\n", DIR_GAMES);
			exit (EXIT_ERROR);
		case 1:
			Initialize();
			iGameSel = 1;
			RunGame();
			break;
		default:
			Initialize();
			ListGames();
			break;
	}

	return 0;
}
/*****************************************************************************/
void ShowUsage (void)
/*****************************************************************************/
{
	printf ("%s %s\n%s\n\n", PROG_NAME, PROG_VERSION, COPYRIGHT);
	printf ("Usage:\n");
	printf ("  %s [OPTIONS]\n\nOptions:\n", PROG_NAME);
	printf ("  -h, -?,    --help           display this help and exit\n");
	printf ("  -v,        --version        output version information and"
		" exit\n");
	printf ("  -n,        --noaudio        do not play sound effects\n");
	printf ("  -f,        --fullscreen     start in fullscreen\n");
	printf ("  -g,        --generate       generate level files from"
		" LEVELS.DAT\n");
	printf ("  -l=NR,     --level=NR       start in level NR\n");
	printf ("  -m=MODE,   --mode=MODE      start in mode MODE\n");
	printf ("  -z=ZOOM,   --zoom=ZOOM      start with zoom ZOOM\n");
	printf ("  -c         --cheat          enable cheats\n");
	printf ("\n");
	printf ("Modes:\n");
	printf ("  0 (8x8), 1 (8x14), 2 (8x16), 3 (9x14), 4 (9x16)\n");
	printf ("Zooms:\n");
	printf ("  1 (100%%), 2, 3, 4, 5, 6, 7 (700%%)\n");
	printf ("\n");
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void Generate (void)
/*****************************************************************************/
{
	int iFd;
	char sDir[MAX_PATHFILE + 2];
	int iUnkSize;

	/*** Used for looping. ***/
	int iLoopLevel;

	iFd = open (FILE_DAT, O_RDONLY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not open %s: %s!\n", FILE_DAT, strerror (errno));
		exit (EXIT_ERROR);
	}

	CreateDir (DIR_GAMES);
	snprintf (sDir, MAX_PATHFILE, "%s%s%s", DIR_GAMES, SLASH, DIR_DAT);
	CreateDir (sDir);

	for (iLoopLevel = 0; iLoopLevel <= 15; iLoopLevel++)
	{
		lseek (iFd, arDATOffsets[iLoopLevel], SEEK_SET);
		if (iLoopLevel == 15) { iUnkSize = 3; } else { iUnkSize = 4; }
		LoadDAT (iFd, iUnkSize);
		SavePuny (iLoopLevel);
	}

	printf ("[ INFO ] Done. Created: %s%s*\n", sDir, SLASH);

	close (iFd);
}
/*****************************************************************************/
int ReadFromFile (int iFd, int iSize, unsigned char *sRetString)
/*****************************************************************************/
{
	int iLength;
	int iRead;
	char sRead[1 + 2];
	int iEOF;

	iLength = 0; iEOF = 0;
	do {
		iRead = read (iFd, sRead, 1);
		switch (iRead)
		{
			case -1:
				printf ("[FAILED] Could not read: %s!\n", strerror (errno));
				exit (EXIT_ERROR);
				break;
			case 0: iEOF = 1; break;
			default:
				sRetString[iLength] = sRead[0];
				iLength++;
				break;
		}
	} while ((iLength < iSize) && (iEOF == 0));
	sRetString[iLength] = '\0';

	return (iLength);
}
/*****************************************************************************/
void CreateDir (char *sDir)
/*****************************************************************************/
{
	struct stat stStatus;

	if (stat (sDir, &stStatus) == -1)
	{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
mkdir (sDir);
#else
mkdir (sDir, 0700);
#endif
	}
}
/*****************************************************************************/
void LoadDAT (int iFd, int iUnkSize)
/*****************************************************************************/
{
	int iGroup, iVariant;
	char sBinaryFDoors[9 + 2]; /*** 8 chars, plus \0 ***/
	char sBinarySDoors[9 + 2]; /*** 8 chars, plus \0 ***/
	char sEventsRoom[10 + 2];
	char sEventsTile[10 + 2];

	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopTile;
	int iLoopEvent;

	ReadFromFile (iFd, 1, sChecksum);
	ReadFromFile (iFd, (ROOMS * TILES), sGroup);
	ReadFromFile (iFd, (ROOMS * TILES), sVariant);
	ReadFromFile (iFd, EVENTS, sFirstDoorEvents);
	ReadFromFile (iFd, EVENTS, sSecondDoorEvents);
	ReadFromFile (iFd, (ROOMS * 4), sRoomLinks);
	ReadFromFile (iFd, 64, sUnknownI);
	ReadFromFile (iFd, 3, sStartPosition);
	ReadFromFile (iFd, iUnkSize, sUnknownIIandIII);
	ReadFromFile (iFd, ROOMS, sGuardLocations);
	ReadFromFile (iFd, ROOMS, sGuardDirections);
	ReadFromFile (iFd, 48, sUnknownIVaandIVb);
	ReadFromFile (iFd, ROOMS, sGuardSkills);
	ReadFromFile (iFd, 24, sUnknownIVc);
	ReadFromFile (iFd, ROOMS, sGuardColors);
	ReadFromFile (iFd, 16, sUnknownIVd);
	ReadFromFile (iFd, 2, sEndCode);

	/*** Tiles. ***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
		{
			iGroup = sGroup[((iLoopRoom - 1) * 30) + (iLoopTile - 1)];
			if (iGroup != 43) /*** stuck loose ***/
				{ while (iGroup > 32) { iGroup-=32; } }
			arGroup[iLoopRoom][iLoopTile] = iGroup;
			/***/
			iVariant = sVariant[((iLoopRoom - 1) * 30) + (iLoopTile - 1)];
			arVariant[iLoopRoom][iLoopTile] = iVariant;
		}
	}

	/*** Room links. ***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		arRoomLinks[iLoopRoom][1] = sRoomLinks[((iLoopRoom - 1) * 4) + 0];
		arRoomLinks[iLoopRoom][2] = sRoomLinks[((iLoopRoom - 1) * 4) + 1];
		arRoomLinks[iLoopRoom][3] = sRoomLinks[((iLoopRoom - 1) * 4) + 2];
		arRoomLinks[iLoopRoom][4] = sRoomLinks[((iLoopRoom - 1) * 4) + 3];
	}

	/*** Events. ***/
	for (iLoopEvent = 1; iLoopEvent <= EVENTS; iLoopEvent++)
	{
		GetAsEightBits (sFirstDoorEvents[iLoopEvent - 1], sBinaryFDoors);
		GetAsEightBits (sSecondDoorEvents[iLoopEvent - 1], sBinarySDoors);
		snprintf (sEventsRoom, 10, "%c%c%c%c%c",
			sBinarySDoors[0], sBinarySDoors[1], sBinarySDoors[2],
			sBinaryFDoors[1], sBinaryFDoors[2]);
		arEventsRoom[iLoopEvent] = BitsToInt (sEventsRoom);
		snprintf (sEventsTile, 10, "%c%c%c%c%c",
			sBinaryFDoors[3], sBinaryFDoors[4], sBinaryFDoors[5],
			sBinaryFDoors[6], sBinaryFDoors[7]);
		arEventsTile[iLoopEvent] = BitsToInt (sEventsTile) + 1;
		switch (sBinaryFDoors[0])
		{
			case '0': arEventsNext[iLoopEvent] = 1; break;
			case '1': arEventsNext[iLoopEvent] = 0; break;
		}
	}
}
/*****************************************************************************/
void GetAsEightBits (unsigned char cChar, char *sBinary)
/*****************************************************************************/
{
	int i = CHAR_BIT;
	int iTemp;

	iTemp = 0;
	while (i > 0)
	{
		i--;
		if (cChar&(1 << i))
		{
			sBinary[iTemp] = '1';
		} else {
			sBinary[iTemp] = '0';
		}
		iTemp++;
	}
	sBinary[iTemp] = '\0';
}
/*****************************************************************************/
int BitsToInt (char *sString)
/*****************************************************************************/
{
	/*** Converts binary to decimal. ***/
	/*** Example: 11111111 to 255 ***/

	int iTemp = 0;

	for (; *sString; iTemp = (iTemp << 1) | (*sString++ - '0'));
	return (iTemp);
}
/*****************************************************************************/
int OpenRead (int iLevel, char cType)
/*****************************************************************************/
{
	char sPathFile[MAX_PATHFILE + 2];
	int iFd;

	snprintf (sPathFile, MAX_PATHFILE, "%s%s%s%slevel%02i%c%s",
		DIR_GAMES, SLASH, arGames[iGameSel], SLASH, iLevel, cType, ".txt");
	iFd = open (sPathFile, O_RDONLY);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not read %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	return (iFd);
}
/*****************************************************************************/
int OpenTrunc (int iLevel, char cType)
/*****************************************************************************/
{
	char sPathFile[MAX_PATHFILE + 2];
	int iFd;

	snprintf (sPathFile, MAX_PATHFILE, "%s%s%s%slevel%02i%c%s",
		DIR_GAMES, SLASH, DIR_DAT, SLASH, iLevel, cType, ".txt");
	iFd = open (sPathFile, O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0600);
	if (iFd == -1)
	{
		printf ("[FAILED] Could not create %s: %s!\n",
			sPathFile, strerror (errno));
		exit (EXIT_ERROR);
	}

	return (iFd);
}
/*****************************************************************************/
void SavePuny (int iLevel)
/*****************************************************************************/
{
	int iFdE, iFdR, iFdS, iFdT;
	char sToWrite[MAX_TOWRITE + 2];
	int iGroup, iVariant;
	int iUnknown;
	char cWrite;
	int iFixed;
	int iLeft, iRight, iUp, iDown;
	int iTile;

	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopTile;

	/*** Open files for writing. ***/
	iFdE = OpenTrunc (iLevel, 'e');
	iFdR = OpenTrunc (iLevel, 'r');
	iFdS = OpenTrunc (iLevel, 's');
	iFdT = OpenTrunc (iLevel, 't');

	/*** Save tiles and events. ***/
	cNextDrop = 'a';
	cNextRaise = 'A';
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
		{
			iGroup = arGroup[iLoopRoom][iLoopTile];
			iVariant = arVariant[iLoopRoom][iLoopTile];
			iUnknown = 0;
			switch (iGroup)
			{
				case 0: case 32: /*** empty ***/
					switch (iVariant)
					{
						case 0: cWrite = '.'; break; /*** no pattern ***/
						case 1: cWrite = '.'; break; /*** pattern ***/
						case 2: cWrite = '.'; break; /*** pattern ***/
						case 3: cWrite = '`'; break; /*** window ***/
						/*** 0x04, 0x05, 0x0C, 0x0D, 0x32, 0x33, 0x34, 0x35 ***/
						case 255: cWrite = '.'; break; /*** no pattern ***/
						default: iUnknown = 1; break;
					}
					break;
				case 1: /*** floor ***/
					switch (iVariant)
					{
						case 0: cWrite = '_'; break; /*** no pattern ***/
						case 1: cWrite = '_'; break; /*** pattern ***/
						case 2: cWrite = '_'; break; /*** pattern ***/
						case 3: cWrite = '_'; break; /*** pattern ***/
						case 4: cWrite = '$'; break; /*** coin ***/
						case 5: cWrite = '8'; break; /*** fake wall ***/
						case 6: cWrite = '9'; break; /*** fake empty ***/
						case 13: cWrite = '8'; break; /*** fake wall ***/
						case 14: cWrite = '9'; break; /*** fake empty ***/
						/*** 0x32, 0x33, 0x34, 0x35 ***/
						case 255: cWrite = '_'; break; /*** no pattern ***/
						default: iUnknown = 1; break;
					}
					break;
				case 2: /*** spikes ***/
					switch (iVariant)
					{
						case 0: cWrite = '*'; break;
						case 1: cWrite = '^'; break;
						case 2: cWrite = '^'; break;
						case 3: cWrite = '^'; break;
						case 4: cWrite = '^'; break;
						case 5: cWrite = '_'; break;
						case 6: cWrite = '_'; break;
						case 7: cWrite = '_'; break;
						case 8: cWrite = '_'; break;
						case 9: cWrite = '_'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 3: /*** pillar ***/
					switch (iVariant)
					{
						case 0: cWrite = '|'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 4: /*** gate ***/
					switch (iVariant)
					{
						case 0: cWrite = ')'; break; /*** closed ***/
						case 1: cWrite = '"'; break; /*** open ***/
						case 2: cWrite = ')'; break; /*** closed ***/
						default: iUnknown = 1; break;
					}
					break;
				case 5: /*** stuck button ***/
					switch (iVariant)
					{
						case 0: cWrite = '_'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 6: /*** drop ***/
					cWrite = GetLetter (iFdE, iVariant, 6);
					break;
				case 7: /*** tapestry + floor ***/
					switch (iVariant)
					{
						case 0: cWrite = '#'; break;
						case 1: cWrite = '#'; break;
						case 2: cWrite = '#'; break;
						case 3: cWrite = '#'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 8: /*** pillar bottom ***/
					switch (iVariant)
					{
						case 0: cWrite = ';'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 9: /*** pillar top ***/
					switch (iVariant)
					{
						case 0: cWrite = ':'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 10: /*** potion ***/
					switch (iVariant)
					{
						case 0: cWrite = '0'; break; /*** empty ***/
						case 1: cWrite = '1'; break; /*** heal ***/
						case 2: cWrite = '2'; break; /*** life ***/
						case 3: cWrite = '3'; break; /*** float ***/
						case 4: cWrite = '4'; break; /*** flip ***/
						case 5: cWrite = '5'; break; /*** hurt ***/
						case 6: cWrite = '6'; break; /*** special blue ***/
						/*** 0x07, 0x08, 0x09, 0x0A, 0x0B ***/
						default: iUnknown = 1; break;
					}
					break;
				case 11: /*** loose ***/
					switch (iVariant)
					{
						case 0:
							if ((iLevel == 12) && (iLoopRoom == 8) && (iLoopTile == 13))
							{
								/*** Because this would be impossible. ***/
								printf ("[ WARN ] Level %i: converted loose into floor"
									" (room %i, tile %i).\n", iLevel, iLoopRoom, iLoopTile);
								cWrite = '_';
							} else {
								cWrite = '~';
							}
							break;
						default: iUnknown = 1; break;
					}
					break;
				case 12: /*** gate top ***/
					switch (iVariant)
					{
						case 0: cWrite = '('; break;
						case 1: cWrite = '('; break;
						case 2: cWrite = '('; break;
						case 3: cWrite = '('; break;
						case 4: cWrite = '('; break;
						case 5: cWrite = '('; break;
						case 6: cWrite = '('; break;
						case 7: cWrite = '('; break;
						default: iUnknown = 1; break;
					}
					break;
				case 13: /*** mirror ***/
					switch (iVariant)
					{
						case 0: cWrite = '%'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 14: /*** debris ***/
					switch (iVariant)
					{
						case 0: cWrite = '-'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 15: /*** raise ***/
					cWrite = GetLetter (iFdE, iVariant, 15);
					break;
				case 16: /*** level door left ***/
					switch (iVariant)
					{
						case 0: cWrite = '['; break;
						/*** 0x20, 0x40, 0xFD, 0xFF ***/
						default: iUnknown = 1; break;
					}
					break;
				case 17: /*** level door right ***/
					switch (iVariant)
					{
						case 0: cWrite = ']'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 18: /*** chomper ***/
					switch (iVariant)
					{
						case 0:
							if ((iLoopTile != 1) && (iLoopTile != 11) && (iLoopTile != 21))
							{
								cWrite = '@';
							} else {
								/*** Because this would be too difficult. ***/
								printf ("[ WARN ] Level %i: converted chomper into pillar"
									" (room %i, tile %i).\n", iLevel, iLoopRoom, iLoopTile);
								cWrite = '|';
							}
							break;
						case 1: cWrite = '_'; break;
						case 2: cWrite = '#'; break;
						case 3: cWrite = '_'; break;
						case 4: cWrite = '_'; break;
						case 5: cWrite = '_'; break;
						/*** 0x80, 0x81, 0x82, 0x83, 0x84, 0x85 ***/
						default: iUnknown = 1; break;
					}
					break;
				case 19: /*** torch ***/
					switch (iVariant)
					{
						case 0: cWrite = '\''; break;
						/*** 0x01-0x3F ***/
						default: iUnknown = 1; break;
					}
					break;
				case 20: /*** wall ***/
					switch (iVariant)
					{
						case 0: cWrite = '#'; break;
						case 1: cWrite = '#'; break;
						/*** 0x04, 0x06, 0x0C, 0x0E ***/
						default: iUnknown = 1; break;
					}
					break;
				case 21: /*** skeleton ***/
					switch (iVariant)
					{
						case 0: cWrite = '+'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 22: /*** sword ***/
					switch (iVariant)
					{
						case 0: cWrite = '!'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 23: /*** balcony/teleports left ***/
					switch (iVariant)
					{
						case 0: cWrite = '_'; break;
						case 1: cWrite = 'S'; break;
						case 2: cWrite = 'T'; break;
						case 3: cWrite = 'U'; break;
						case 4: cWrite = 'V'; break;
						case 5: cWrite = 'W'; break;
						case 6: cWrite = 'X'; break;
						case 7: cWrite = 'Y'; break;
						case 8: cWrite = 'Z'; break;
						case 9: cWrite = 's'; break;
						case 10: cWrite = 't'; break;
						case 11: cWrite = 'u'; break;
						case 12: cWrite = 'v'; break;
						case 13: cWrite = 'w'; break;
						case 14: cWrite = 'x'; break;
						case 15: cWrite = 'y'; break;
						case 16: cWrite = 'z'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 24: /*** balcony/teleport right ***/
					switch (iVariant)
					{
						case 0: cWrite = '_'; break;
						case 1: cWrite = ','; break;
						default: iUnknown = 1; break;
					}
					break;
				case 25: /*** lattice pillar ***/
					switch (iVariant)
					{
						case 0: cWrite = '|'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 26: /*** lattice top ***/
					switch (iVariant)
					{
						case 0: cWrite = '/'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 27: /*** small lattice ***/
					switch (iVariant)
					{
						case 0: cWrite = '\\'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 28: /*** lattice left ***/
					switch (iVariant)
					{
						case 0: cWrite = '\\'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 29: /*** lattice right ***/
					switch (iVariant)
					{
						case 0: cWrite = '\\'; break;
						default: iUnknown = 1; break;
					}
					break;
				case 30: /*** torch + debris ***/
					switch (iVariant)
					{
						case 0: cWrite = '\''; break;
						/*** 0x01-0x3F ***/
						default: iUnknown = 1; break;
					}
					break;
				case 43: /*** stuck loose ***/
					switch (iVariant)
					{
						case 0: cWrite = '~'; break;
						default: iUnknown = 1; break;
					}
					break;
				default:
					iUnknown = 1; break;
			}
			if (iUnknown == 1)
			{
				if (iGroup != 31)
				{
					printf ("[ WARN ] Level %i: unknown in room %i, tile %i (group %i,"
						" variant %i).\n", iLevel, iLoopRoom, iLoopTile, iGroup, iVariant);
				}
				cWrite = '?';
			}
			snprintf (sToWrite, MAX_TOWRITE, "%c", cWrite);
			write (iFdT, sToWrite, 1);
			if ((iLoopTile == 10) || (iLoopTile == 20) || (iLoopTile == 30))
				{ write (iFdT, "\n", 1); }
		}
		write (iFdT, "\n", 1);
	}

	/*** Save room links. ***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		iFixed = 0;
		iLeft = arRoomLinks[iLoopRoom][1];
		if ((iLeft < 0) || (iLeft > 24)) { iLeft = 0; iFixed = 1; }
		iRight = arRoomLinks[iLoopRoom][2];
		if ((iRight < 0) || (iRight > 24)) { iRight = 0; iFixed = 2; }
		iUp = arRoomLinks[iLoopRoom][3];
		if ((iUp < 0) || (iUp > 24)) { iUp = 0; iFixed = 3; }
		iDown = arRoomLinks[iLoopRoom][4];
		if ((iDown < 0) || (iDown > 24)) { iDown = 0; iFixed = 4; }
		if (iFixed != 0)
		{
			printf ("[ WARN ] Level %i: fixed link %i in room %i.\n",
				iLevel, iFixed, iLoopRoom);
		}
		snprintf (sToWrite, MAX_TOWRITE, "%02i %02i %02i %02i\n",
			iLeft, iRight, iUp, iDown);
		write (iFdR, sToWrite, strlen (sToWrite));
	}

	/*** Save starting locations. ***/
	snprintf (sToWrite, MAX_TOWRITE, "P %02i%02i\n",
		sStartPosition[0], sStartPosition[1] + 1);
	write (iFdS, sToWrite, strlen (sToWrite));
	/***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		iTile = sGuardLocations[iLoopRoom - 1] + 1;
		if (iTile != 31)
		{
			if (iLevel == 13)
			{
				snprintf (sToWrite, MAX_TOWRITE, "J %02i%02i\n", iLoopRoom, iTile);
			} else {
				switch (sGuardSkills[iLoopRoom - 1])
				{
					case 0: case 1: case 2: case 3: case 4: case 8: /*** easy ***/
						snprintf (sToWrite, MAX_TOWRITE, "E %02i%02i\n", iLoopRoom, iTile);
						break;
					case 5: case 6: case 7: case 9: case 10: case 11: /*** hard ***/
						snprintf (sToWrite, MAX_TOWRITE, "H %02i%02i\n", iLoopRoom, iTile);
						break;
				}
			}
			write (iFdS, sToWrite, strlen (sToWrite));
		}
	}

	close (iFdE);
	close (iFdR);
	close (iFdS);
	close (iFdT);
}
/*****************************************************************************/
char GetLetter (int iFdE, int iVariant, int iType)
/*****************************************************************************/
{
	char cReturn;
	int iEvent;
	char sToWrite[MAX_TOWRITE + 2];
	char sTemp[MAX_TOWRITE + 2];

	switch (iType)
	{
		case 6: /*** drop ***/
			if (cNextDrop == 's')
			{
				printf ("[ WARN ] Too many drop buttons.\n");
				return ('?');
			}
			cReturn = cNextDrop;
			cNextDrop++;
			break;
		case 15: /*** raise ***/
			if (cNextRaise == 'S')
			{
				printf ("[ WARN ] Too many drop buttons.\n");
				return ('?');
			}
			cReturn = cNextRaise;
			cNextRaise++;
			break;
		default:
			printf ("[FAILED] Impossible button type: %i!\n", iType);
			exit (EXIT_ERROR);
	}
	iEvent = iVariant + 1;

	snprintf (sToWrite, MAX_TOWRITE, "%c %02i%02i",
		cReturn, arEventsRoom[iEvent], arEventsTile[iEvent]);
	while (arEventsNext[iEvent] == 1)
	{
		iEvent++;
		snprintf (sTemp, MAX_TOWRITE, "%s", sToWrite);
		snprintf (sToWrite, MAX_TOWRITE, "%s %02i%02i",
			sTemp, arEventsRoom[iEvent], arEventsTile[iEvent]);
	}
	write (iFdE, sToWrite, strlen (sToWrite));
	write (iFdE, "\n", 1);

	return (cReturn);
}
/*****************************************************************************/
void GetGames (void)
/*****************************************************************************/
{
	DIR *dDir;
	struct dirent *stDirent;

	dDir = opendir (DIR_GAMES);
	if (dDir == NULL)
	{
		printf ("[FAILED] Cannot open directory \"%s\": %s!\n",
			DIR_GAMES, strerror (errno));
		exit (EXIT_ERROR);
	}

	iNrGames = 0;
	while ((stDirent = readdir (dDir)) != NULL)
	{
		if ((strcmp (stDirent->d_name, ".") != 0) &&
			(strcmp (stDirent->d_name, "..") != 0))
		{
			iNrGames++;
			if (iNrGames <= MAX_GAMES)
			{
				snprintf (arGames[iNrGames], MAX_PATHFILE, "%s", stDirent->d_name);
			} else {
				printf ("[ WARN ] Ignoring game \"%s\".", stDirent->d_name);
			}
		}
	}

	closedir (dDir);
}
/*****************************************************************************/
void ListGames (void)
/*****************************************************************************/
{
	SDL_Event event;
	int iGameToSel;

	/*** Used for looping. ***/
	int iLoopLines;

	iGameSel = 1;

	ShowListGames();
	while (1)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN: /*** https://wiki.libsdl.org/SDL2/SDL_Keycode ***/
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_q:
							Quit(); break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							RunGame();
							break;
						case SDLK_DOWN:
							if (iGameSel < iNrGames) { iGameSel++; }
							break;
						case SDLK_UP:
							if (iGameSel > 1) { iGameSel--; }
							break;
					}
					ShowListGames();
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					for (iLoopLines = 3; iLoopLines <= 17; iLoopLines++)
					{
						if (InArea (102, -24 + (iLoopLines * 32),
							102 + 819, -24 + (iLoopLines * 32) + 32) == 1)
						{
							iGameToSel = iLoopLines - 2;
							if ((iGameToSel != iGameSel) && (iGameToSel <= iNrGames))
							{
								iGameSel = iGameToSel;
								ShowListGames();
							}
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						RunGame();
						ShowListGames();
					}
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowListGames(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		PreventCPUEating();
	}
}
/*****************************************************************************/
void ShowListGames (void)
/*****************************************************************************/
{
	int iR, iG, iB;
	char sGameName[MAX_PATHFILE + 2];
	char sGameTemp[MAX_PATHFILE + 2];
	char cChar;

	/*** Used for looping. ***/
	int iLoopGames;
	int iLoopChar;

	ShowImage (imgscreend, 0, 0, "imgscreend");

	ShowText (1, "Choose game/mod:", 0xaa, 0xaa, 0xaa, 0);
	for (iLoopGames = 1; iLoopGames <= iNrGames; iLoopGames++)
	{
		if (iLoopGames == iGameSel)
		{
			iR = 0xff; iG = 0xff; iB = 0xff;
		} else {
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
		}
		snprintf (sGameName, MAX_PATHFILE, "%s", "");
		for (iLoopChar = 0; iLoopChar < (int)strlen (arGames[iLoopGames]);
			iLoopChar++)
		{
			snprintf (sGameTemp, MAX_PATHFILE, "%s", sGameName);
			cChar = arGames[iLoopGames][iLoopChar];
			if (cChar == '_') { cChar = ' '; }
			snprintf (sGameName, MAX_PATHFILE, "%s%c", sGameTemp, cChar);
		}
		ShowText (iLoopGames + 2, sGameName, iR, iG, iB, 0);
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void LoadLevel (int iLevel, int iLives)
/*****************************************************************************/
{
	int iFdE, iFdR, iFdS, iFdT;
	char sRow1[10 + 2];
	char sRow2[10 + 2];
	char sRow3[10 + 2];
	char sRead[2 + 2];
	int iEOF;
	char sLine[MAX_LINE + 2];
	char cChar;
	int iChar;
	char sRoom[10 + 2]; int iRoom;
	char sTile[10 + 2]; int iTile;
	int iEventNr;
	char sLink[10 + 2];

	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopTile;
	int iLoopChar;

	/*** Open files for reading. ***/
	iFdE = OpenRead (iLevel, 'e');
	iFdR = OpenRead (iLevel, 'r');
	iFdS = OpenRead (iLevel, 's');
	iFdT = OpenRead (iLevel, 't');

	/*** Events. ***/
	do {
		iEOF = ReadLine (iFdE, sLine);
		if (iEOF == 0)
		{
			cChar = sLine[0];
			iChar = 2;
			iEventNr = 1;
			do {
				snprintf (sRoom, 10, "%c%c", sLine[iChar], sLine[iChar + 1]);
				iRoom = atoi (sRoom);
				snprintf (sTile, 10, "%c%c", sLine[iChar + 2], sLine[iChar + 3]);
				iTile = atoi (sTile);
				iChar+=5; /*** Including the space. ***/
				arLettersRoom[(int)cChar][iEventNr] = iRoom;
				arLettersTile[(int)cChar][iEventNr] = iTile;
				iEventNr++;
			} while (iChar < (int)strlen (sLine));
		}
	} while (iEOF == 0);

	/*** Room links. ***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		ReadLine (iFdR, sLine);
		snprintf (sLink, 10, "%c%c", sLine[0], sLine[1]);
		arLinksL[iLoopRoom] = atoi (sLink);
		snprintf (sLink, 10, "%c%c", sLine[3], sLine[4]);
		arLinksR[iLoopRoom] = atoi (sLink);
		snprintf (sLink, 10, "%c%c", sLine[6], sLine[7]);
		arLinksU[iLoopRoom] = atoi (sLink);
		snprintf (sLink, 10, "%c%c", sLine[9], sLine[10]);
		arLinksD[iLoopRoom] = atoi (sLink);
	}

	/*** Starting locations. ***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		arGuardLoc[iLoopRoom] = 31; /*** disable ***/
	}
	do {
		iEOF = ReadLine (iFdS, sLine);
		if (iEOF == 0)
		{
			cChar = sLine[0];
			snprintf (sRoom, 10, "%c%c", sLine[2], sLine[3]);
			iRoom = atoi (sRoom);
			snprintf (sTile, 10, "%c%c", sLine[4], sLine[5]);
			iTile = atoi (sTile);
			switch (cChar)
			{
				case 'P':
					iCurRoom = iRoom;
					iPrinceTile = iTile;
					break;
				case 'E':
					arGuardLoc[iRoom] = iTile;
					arGuardType[iRoom] = 1;
					arGuardHP[iRoom] = 3;
					arGuardAttack[iRoom] = 0;
					break;
				case 'H':
					arGuardLoc[iRoom] = iTile;
					arGuardType[iRoom] = 2;
					arGuardHP[iRoom] = 5;
					arGuardAttack[iRoom] = 0;
					break;
				case 'J':
					arGuardLoc[iRoom] = iTile;
					arGuardType[iRoom] = 3;
					arGuardHP[iRoom] = 7;
					arGuardAttack[iRoom] = 0;
					break;
			}
		}
	} while (iEOF == 0);

	/*** Tiles. ***/
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		read (iFdT, sRow1, 10);
		read (iFdT, sRead, 1); /*** \n ***/
		read (iFdT, sRow2, 10);
		read (iFdT, sRead, 1); /*** \n ***/
		read (iFdT, sRow3, 10);
		read (iFdT, sRead, 2); /*** \n\n ***/
		for (iLoopChar = 1; iLoopChar <= 10; iLoopChar++)
		{
			arTiles[iLoopRoom][iLoopChar] = sRow1[iLoopChar - 1];
			arTiles[iLoopRoom][iLoopChar + 10] = sRow2[iLoopChar - 1];
			arTiles[iLoopRoom][iLoopChar + 20] = sRow3[iLoopChar - 1];
		}
	}

	close (iFdE);
	close (iFdR);
	close (iFdS);
	close (iFdT);

	/*** Defaults. ***/
	switch (iLevel)
	{
		case 1: iPrinceSword = 0; break;
		default: iPrinceSword = 1; break;
	}
	iLevLives = 0;
	iMaxLives = iLives;
	iCurLives = iMaxLives;
	iPrinceHang = 0;
	iPrinceFloat = 0;
	iFlash = 0;
	iPrinceCoins = 0;

	/*** Count total coins. ***/
	iCoinsInLevel = 0;
	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
		{
			if (arTiles[iLoopRoom][iLoopTile] == '$') { iCoinsInLevel++; }
			arGateTimers[iLoopRoom][iLoopTile] = 0;
		}
	}

	/*** Special events related. ***/
	switch (iCurLevel)
	{
			case 4:
				iMirror = 0;
				break;
			case 6:
				arGuardLoc[1] = 11;
				arGuardType[1] = 4;
				arGuardHP[1] = iMaxLives;
				arGuardAttack[1] = 0;
				break;
			case 7:
				iPrinceDir = 1;
				iPrinceHang = 1;
				break;
			case 8:
				iMouse = 0;
				break;
	}

	ShowGame();

	if (iCoinsInLevel != 0)
	{
		snprintf (sMessage, MAX_MESSAGE, "Coins required to raise the exit"
			" door: %i\n", iCoinsInLevel);
		SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_INFORMATION,
			"Coins", sMessage, window);
	}

	/*** Special events related. ***/
	if (iCurLevel == 1)
	{
		cChar = arTiles[5][3];
		if (((cChar >= 'A') && (cChar <= 'R')) ||
			((cChar >= 'a') && (cChar <= 'r')))
		{
			SDL_Delay (100);
			PushButton (cChar, 0);
		}
	}
}
/*****************************************************************************/
int ReadLine (int iFd, char *sRetString)
/*****************************************************************************/
{
	int iEOF, iLength, iRead;
	char sRead[1 + 2];

	iEOF = 0;
	iLength = 0;
	snprintf (sRetString, MAX_LINE, "%s", "");
	do {
		iRead = read (iFd, sRead, 1);
		switch (iRead)
		{
			case -1:
				printf ("[FAILED] Could not read line: %s!\n", strerror (errno));
				exit (EXIT_ERROR);
				break;
			case 0: iEOF = 1; break;
			default:
				if (sRead[0] != '\n')
				{
					sRetString[iLength] = sRead[0];
					iLength++;
				}
				break;
		}
	} while ((sRead[0] != '\n') && (iEOF == 0));
	sRetString[iLength] = '\0';

	return (iEOF);
}
/*****************************************************************************/
void RunGame (void)
/*****************************************************************************/
{
	int iGame;
	SDL_Event event;
	int iChoice;
	char cLeft, cRight;
	int iModRoom, iModTile;
	char cChar;

	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopTile;

	iGame = 1;

	/*** Defaults. ***/
	iJump = 0;
	iCareful = 0;
	iRunJump = 0;
	iCurLevel = iStartLevel;
	iSteps = 0;
	iShowStepsCoins = 0;
	LoadLevel (iCurLevel, START_LIVES);

	while (iGame == 1)
	{
		GameActions();

		/*** This is for the game animation. ***/
		newticks = SDL_GetTicks();
		if (newticks > oldticks + REFRESH_GAME)
		{
			if (iFlash > 0) { iFlash--; }
			if (iPrinceFloat > 0) { iPrinceFloat--; }
			for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
			{
				for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
				{
					if (arGateTimers[iLoopRoom][iLoopTile] > 0)
					{
						arGateTimers[iLoopRoom][iLoopTile]--;
						if (arGateTimers[iLoopRoom][iLoopTile] == 0)
						{
							if (arTiles[iLoopRoom][iLoopTile] == '"')
							{
								arTiles[iLoopRoom][iLoopTile] = ')';
								if (iLoopRoom == iCurRoom)
								{
									PlaySound ("wav/gate_close_fast.wav");
								}
							}
						}
					}
				}
			}
			/*** Special events related. ***/
			switch (iCurLevel)
			{
				case 4:
					if ((iCurRoom == 11) && (iMirror == 1))
					{
						arTiles[4][5] = '%';
						iMirror = 2;
					}
					break;
				case 8:
					if ((iCurRoom == 16) && (iMouse > 0))
					{
						iMouse++;
						if (iMouse > 150)
						{
							arGuardLoc[16] = 8;
							arGuardType[16] = 5;
							arGuardHP[16] = iMaxLives;
							arGuardAttack[16] = 0;
							/***/
							cChar = arTiles[16][8];
							if (((cChar >= 'A') && (cChar <= 'R')) ||
								((cChar >= 'a') && (cChar <= 'r')))
							{
								PushButton (cChar, 0);
							}
							iMouse = -1;
						}
					}
					break;
			}
			ShowGame();
			oldticks = newticks;
		}

		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					if (event.key.repeat == 1)
					{
						if (iPrinceSword == 2) { iPrinceSword = 1; }
						break;
					}
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_q:
							iChoice = PopUp ("Stop playing?");
							if (iChoice == 1) { iGame = 0; }
							break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
							if ((event.key.keysym.mod & KMOD_LALT) ||
								(event.key.keysym.mod & KMOD_RALT))
							{
								ToggleFullscreen();
							} else {
								iShowStepsCoins = 1;
							}
							break;
						case SDLK_F9:
							BossKey();
							break;
						case SDLK_LEFT:
							if (iPrinceHang == 2) { break; }
							MovingStarts();
							iPrinceDir = 1;
							if (iRunJump == 1)
							{
								TryGoLeft (6);
							} else if (iJump == 1) {
								TryGoLeft (3);
							} else if (iCareful == 1) {
								cLeft = GetCharLeft();
								switch (cLeft)
								{
									case '~':
										iModRoom = GetRoomLeft();
										iModTile = GetTileLeft();
										DropLoose (iModRoom, iModTile);
										break;
									case '.':
										iPrinceHang = 1;
										iPrinceSafe = 1; /*** In case of spikes below. ***/
										TryGoLeft (1);
										break;
									case '^':
										iPrinceSafe = 1;
										TryGoLeft (1);
										break;
								}
							} else {
								TryGoLeft (1);
							}
							MovingEnds();
							break;
						case SDLK_RIGHT:
							if (iPrinceHang == 2) { break; }
							MovingStarts();
							iPrinceDir = 2;
							if (iRunJump == 1)
							{
								TryGoRight (6);
							} else if (iJump == 1) {
								TryGoRight (3);
							} else if (iCareful == 1) {
								cRight = GetCharRight();
								switch (cRight)
								{
									case '~':
										iModRoom = GetRoomRight();
										iModTile = GetTileRight();
										DropLoose (iModRoom, iModTile);
										break;
									case '.':
										iPrinceHang = 1;
										iPrinceSafe = 1; /*** In case of spikes below. ***/
										TryGoRight (1);
										break;
									case '^':
										iPrinceSafe = 1;
										TryGoRight (1);
										break;
								}
							} else {
								TryGoRight (1);
							}
							MovingEnds();
							break;
						case SDLK_UP:
							MovingStarts();
							TryGoUp();
							MovingEnds();
							break;
						case SDLK_DOWN:
							MovingStarts();
							iCareful = 0; /*** Spikes always hurt. ***/
							TryGoDown();
							MovingEnds();
							break;
						case SDLK_a:
							if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								/*** Restart iCurLevel. ***/
								LoadLevel (iCurLevel, iMaxLives - iLevLives);
							}
							break;
						case SDLK_c:
							ToggleCareful();
							break;
						case SDLK_j:
							ToggleJump();
							break;
						case SDLK_k:
							if (iCheat == 1)
							{
								arGuardHP[iCurRoom] = 0;
								PlaySound ("wav/hit_guard.wav");
								/*** Special events related. ***/
								if ((iCurLevel == 13) && (arGuardType[iCurRoom] == 3))
								{
									cChar = arTiles[24][1];
									if (((cChar >= 'A') && (cChar <= 'R')) ||
										((cChar >= 'a') && (cChar <= 'r')))
									{
										PushButton (cChar, 0);
									}
									iFlash+=25;
									iFlashR = 0xff; iFlashG = 0xff; iFlashB = 0xff;
								}
							}
							break;
						case SDLK_l:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iCheat == 1)
								{
									if (iCurLevel < 14)
									{
										iCurLevel++;
										LoadLevel (iCurLevel, iMaxLives);
									}
								}
							}
							break;
						case SDLK_m:
							iMode++;
							if (iMode > 4) { iMode = 0; }
							break;
						case SDLK_r:
							if ((event.key.keysym.mod & KMOD_LCTRL) ||
								(event.key.keysym.mod & KMOD_RCTRL))
							{
								iSteps = 0;
								iCurLevel = 1;
								LoadLevel (iCurLevel, START_LIVES);
							} else {
								ToggleRunJump();
							}
							break;
						case SDLK_s:
							if (iPrinceSword == 1) { iPrinceSword = 2; }
							break;
						case SDLK_t:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iCheat == 1)
								{
									if (iMaxLives < 10)
									{
										iMaxLives++;
										iCurLives = iMaxLives;
									}
								}
							}
							break;
						case SDLK_w:
							if ((event.key.keysym.mod & KMOD_LSHIFT) ||
								(event.key.keysym.mod & KMOD_RSHIFT))
							{
								if (iCheat == 1)
								{
									iPrinceFloat+=50;
									iFlash+=25;
									iFlashR = 0x00; iFlashG = 0xaa; iFlashB = 0x00;
								}
							}
							break;
						case SDLK_z:
							iZoom++;
							if (iZoom > 7) { iZoom = 1; }
							break;
					}
					ShowGame();
					break;
				case SDL_KEYUP:
					switch (event.key.keysym.sym)
					{
						case SDLK_s:
							if (iPrinceSword == 2) { iPrinceSword = 1; }
							break;
					}
					ShowGame();
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					if (InArea (936, 53, 936 + 74, 53 + 15) == 1)
					{
						SDL_SetCursor (curHand);
					} else {
						SDL_SetCursor (curArrow);
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						/*** Mode. ***/
						if (InArea (9, 9, 9 + 14, 9 + 14) == 1) { iMode = 0; }
						if (InArea (9, 28, 9 + 14, 28 + 14) == 1) { iMode = 1; }
						if (InArea (9, 47, 9 + 14, 47 + 14) == 1) { iMode = 2; }
						if (InArea (9, 66, 9 + 14, 66 + 14) == 1) { iMode = 3; }
						if (InArea (9, 85, 9 + 14, 85 + 14) == 1) { iMode = 4; }

						if (InArea (9, 126, 9 + 14, 126 + 14) == 1) { iZoom = 1; }
						if (InArea (9, 145, 9 + 14, 145 + 14) == 1) { iZoom = 2; }
						if (InArea (9, 164, 9 + 14, 164 + 14) == 1) { iZoom = 3; }
						if (InArea (9, 183, 9 + 14, 183 + 14) == 1) { iZoom = 4; }
						if (InArea (9, 202, 9 + 14, 202 + 14) == 1) { iZoom = 5; }
						if (InArea (9, 221, 9 + 14, 221 + 14) == 1) { iZoom = 6; }
						if (InArea (9, 240, 9 + 14, 240 + 14) == 1) { iZoom = 7; }

						/*** Fullscreen. ***/
						if (InArea (9, 281, 9 + 14, 281 + 14) == 1)
							{ if (iFullscreen != 0) { ToggleFullscreen(); } }
						if (InArea (9, 300, 9 + 14, 300 + 14) == 1)
							{ if (iFullscreen == 0) { ToggleFullscreen(); } }
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == 1)
					{
						if (InArea (936, 53, 936 + 74, 53 + 15) == 1)
							{ OpenURL ("https://www.annebras.nl/miniprince/"); }
					}
					ShowGame();
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.y > 0) /*** scroll wheel up ***/
					{
						if (iZoom < 7) { iZoom++; }
					}
					if (event.wheel.y < 0) /*** scroll wheel down ***/
					{
						if (iZoom > 1) { iZoom--; }
					}
					ShowGame();
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowGame(); } break;
				case SDL_QUIT:
					iChoice = PopUp ("Stop playing?");
					if (iChoice == 1) { iGame = 0; }
					break;
			}
		}

		PreventCPUEating();
	}
}
/*****************************************************************************/
void ShowGame (void)
/*****************************************************************************/
{
	int iX, iY;
	int iStartX, iStartY;
	char sSteps[10 + 2], sCoins[10 + 2];
	int iSwordRoom, iSwordTile;
	char cChar;

	/*** Used for looping. ***/
	int iLoopTile;
	int iLoopLives;

	ShowImage (imgscreeng, 0, 0, "imgscreeng");

	ExMarks();

	iStartX = 102 + ((819 - (arWidth[iMode] * 11 * iZoom)) / 2);
	iStartY = 8 + ((560 - (arHeight[iMode] * 3 * iZoom)) / 2);

	/*** iSwordRoom & iSwordTile ***/
	if (iPrinceDir == 1)
	{
		iSwordRoom = GetRoomLeft();
		iSwordTile = GetTileLeft();
	} else {
		iSwordRoom = GetRoomRight();
		iSwordTile = GetTileRight();
	}

	/*** Flash. ***/
	if (iFlash > 0)
	{
		iX = iStartX + (-1 * (arWidth[iMode] * iZoom));
		iY = iStartY + (-1 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iY = iStartY + (0 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iY = iStartY + (1 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iY = iStartY + (2 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iX = iStartX + (11 * (arWidth[iMode] * iZoom));
		iY = iStartY + (-1 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iY = iStartY + (0 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iY = iStartY + (1 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
		iY = iStartY + (2 * (arHeight[iMode] * iZoom));
		ShowChar (12, 14, iFlashR, iFlashG, iFlashB, iX, iY, iMode, iZoom, 0);
	}

	/*** Left of room. ***/
	iX = iStartX + (0 * (arWidth[iMode] * iZoom));
	iY = iStartY + (0 * (arHeight[iMode] * iZoom));
	if (arLinksL[iCurRoom] != 0)
	{
		ShowTile (arTiles[arLinksL[iCurRoom]][10], iX, iY, 1);
	} else {
		ShowTile ('#', iX, iY, 1);
	}
	iY = iStartY + (1 * (arHeight[iMode] * iZoom));
	if (arLinksL[iCurRoom] != 0)
	{
		ShowTile (arTiles[arLinksL[iCurRoom]][20], iX, iY, 1);
	} else {
		ShowTile ('#', iX, iY, 1);
	}
	iY = iStartY + (2 * (arHeight[iMode] * iZoom));
	if (arLinksL[iCurRoom] != 0)
	{
		ShowTile (arTiles[arLinksL[iCurRoom]][30], iX, iY, 1);
	} else {
		ShowTile ('#', iX, iY, 1);
	}

	/*** Above room. ***/
	iY = iStartY + (-1 * (arHeight[iMode] * iZoom));
	for (iLoopTile = 1; iLoopTile <= 10; iLoopTile++)
	{
		if (iShowStepsCoins == 0)
		{
			iX = iStartX + (iLoopTile * (arWidth[iMode] * iZoom));
			if (arLinksU[iCurRoom] != 0)
			{
				ShowTile (arTiles[arLinksU[iCurRoom]][iLoopTile + 20], iX, iY, 1);
			} else {
				ShowTile ('#', iX, iY, 1);
			}
		} else {
			snprintf (sSteps, 10, " S:%i", iSteps);
			ShowText (1, sSteps, 0xaa, 0xaa, 0xaa, 1);
		}
	}

	/*** Room. ***/
	for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
	{
		if ((iLoopTile >= 1) && (iLoopTile <= 10))
		{
			iX = iStartX + ((iLoopTile - 0) * (arWidth[iMode] * iZoom));
			iY = iStartY + (0 * (arHeight[iMode] * iZoom));
		}
		if ((iLoopTile >= 11) && (iLoopTile <= 20))
		{
			iX = iStartX + ((iLoopTile - 10) * (arWidth[iMode] * iZoom));
			iY = iStartY + (1 * (arHeight[iMode] * iZoom));
		}
		if ((iLoopTile >= 21) && (iLoopTile <= 30))
		{
			iX = iStartX + ((iLoopTile - 20) * (arWidth[iMode] * iZoom));
			iY = iStartY + (2 * (arHeight[iMode] * iZoom));
		}
		if (iPrinceTile == iLoopTile)
		{
			ShowLiving ('P', iX, iY);
		} else if ((iPrinceSword == 2) && (iSwordRoom == iCurRoom) &&
			(iSwordTile == iLoopTile)) {
			ShowTile ('!', iX, iY, 0);
			if ((arGuardLoc[iSwordRoom] == iSwordTile) &&
				(arGuardHP[iSwordRoom] != 0))
			{
				arGuardHP[iSwordRoom]--;
				PlaySound ("wav/hit_guard.wav");
				iPrinceSword = 1;
				/*** Special events related. ***/
				if ((iCurLevel == 13) && (arGuardHP[iSwordRoom] == 0)
					&& (arGuardType[iSwordRoom] == 3))
				{
					cChar = arTiles[24][1];
					if (((cChar >= 'A') && (cChar <= 'R')) ||
						((cChar >= 'a') && (cChar <= 'r')))
					{
						PushButton (cChar, 0);
					}
					iFlash+=25;
					iFlashR = 0xff; iFlashG = 0xff; iFlashB = 0xff;
				}
			}
		} else if (arGuardLoc[iCurRoom] == iLoopTile) {
			if (arGuardHP[iCurRoom] != 0)
			{
				switch (arGuardType[iCurRoom])
				{
					case 1: ShowLiving ('E', iX, iY); break;
					case 2: ShowLiving ('H', iX, iY); break;
					case 3: ShowLiving ('J', iX, iY); break;
					case 4: ShowLiving ('S', iX, iY); break;
					case 5: ShowLiving ('M', iX, iY); break;
				}
			} else {
				ShowLiving ('+', iX, iY);
			}
		} else {
			ShowTile (arTiles[iCurRoom][iLoopTile], iX, iY, 0);
		}
	}

	/*** Lives. ***/
	if (iShowStepsCoins == 0)
	{
		iY = iStartY + (3 * (arHeight[iMode] * iZoom));
		for (iLoopLives = 1; iLoopLives <= iMaxLives; iLoopLives++)
		{
			iX = iStartX + ((iLoopLives - 1) * (arWidth[iMode] * iZoom));
			if (iCurLives >= iLoopLives)
			{
				ShowChar (1, 2, 0xaa, 0x00, 0x00, iX, iY, iMode, iZoom, 0);
			} else {
				ShowChar (14, 3, 0xaa, 0x00, 0x00, iX, iY, iMode, iZoom, 0);
			}
		}
	} else {
		snprintf (sCoins, 10, " C:%i/%i", iPrinceCoins, iCoinsInLevel);
		ShowText (5, sCoins, 0xaa, 0xaa, 0xaa, 1);
	}

	/*** Jump, careful, or runjump. ***/
	if ((iJump == 1) || (iCareful == 1) || (iRunJump == 1))
	{
		iY = iStartY + (3 * (arHeight[iMode] * iZoom));
		iX = iStartX + (10 * (arWidth[iMode] * iZoom));
		if (iJump == 1)
			{ ShowChar (11, 5, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0); }
		if (iCareful == 1)
			{ ShowChar (4, 5, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0); }
		if (iRunJump == 1)
			{ ShowChar (3, 6, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0); }
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void LoadFonts (void)
/*****************************************************************************/
{
	font = TTF_OpenFont ("ttf/Bitstream-Vera-Sans-Bold.ttf", 15);
	if (font == NULL) { printf ("[FAILED] Font gone!\n"); exit (EXIT_ERROR); }
}
/*****************************************************************************/
void MixAudio (void *unused, Uint8 *stream, int iLen)
/*****************************************************************************/
{
	int iTemp;
	int iAmount;

	if (unused != NULL) { } /*** To prevent warnings. ***/

	SDL_memset (stream, 0, iLen); /*** SDL2 ***/
	for (iTemp = 0; iTemp < NUM_SOUNDS; iTemp++)
	{
		iAmount = (sounds[iTemp].dlen-sounds[iTemp].dpos);
		if (iAmount > iLen)
		{
			iAmount = iLen;
		}
		SDL_MixAudio (stream, &sounds[iTemp].data[sounds[iTemp].dpos], iAmount,
			SDL_MIX_MAXVOLUME);
		sounds[iTemp].dpos += iAmount;
	}
}
/*****************************************************************************/
void PreLoad (char *sPNG, SDL_Texture **imgImage)
/*****************************************************************************/
{
	char sImage[MAX_IMG + 2];

	snprintf (sImage, MAX_IMG, "png%s%s", SLASH, sPNG);
	*imgImage = IMG_LoadTexture (ascreen, sImage);
	if (!*imgImage)
	{
		printf ("[FAILED] IMG_LoadTexture: %s!\n", IMG_GetError());
		exit (EXIT_ERROR);
	}
}
/*****************************************************************************/
void Quit (void)
/*****************************************************************************/
{
	TTF_CloseFont (font);
	TTF_Quit();
	SDL_Quit();
	exit (EXIT_NORMAL);
}
/*****************************************************************************/
void ShowImage (SDL_Texture *img, int iX, int iY, char *sImageInfo)
/*****************************************************************************/
{
	SDL_Rect src, dest;
	int iWidth, iHeight;

	SDL_QueryTexture (img, NULL, NULL, &iWidth, &iHeight);
	src.x = 0; src.y = 0; src.w = iWidth; src.h = iHeight;
	dest.x = iX; dest.y = iY; dest.w = iWidth; dest.h = iHeight;
	if (SDL_RenderCopy (ascreen, img, &src, &dest) != 0)
	{
		printf ("[ WARN ] SDL_RenderCopy (%s): %s.\n",
			sImageInfo, SDL_GetError());
	}
}
/*****************************************************************************/
int InArea (int iUpperLeftX, int iUpperLeftY,
	int iLowerRightX, int iLowerRightY)
/*****************************************************************************/
{
	if ((iUpperLeftX <= iXPos) &&
		(iLowerRightX >= iXPos) &&
		(iUpperLeftY <= iYPos) &&
		(iLowerRightY >= iYPos))
	{
		return (1);
	} else {
		return (0);
	}
}
/*****************************************************************************/
void ToggleFullscreen (void)
/*****************************************************************************/
{
	if (iFullscreen == 0)
		{ iFullscreen = SDL_WINDOW_FULLSCREEN_DESKTOP; }
			else { iFullscreen = 0; }

	SDL_SetWindowFullscreen (window, iFullscreen);
	SDL_SetWindowSize (window, WINDOW_WIDTH, WINDOW_HEIGHT);
	SDL_RenderSetLogicalSize (ascreen, WINDOW_WIDTH, WINDOW_HEIGHT);
	SDL_SetWindowPosition (window, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED);
}
/*****************************************************************************/
void ShowTile (char cTile, int iX, int iY, int iFade)
/*****************************************************************************/
{
	int iW, iH;
	int iR, iG, iB;
	int iShowBack, iYellowBack;

	iYellowBack = 0;
	switch (cTile)
	{
		case ' ': /*** should never be used ***/
			iW = 16; iH = 4;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '!': /*** Sword. ***/
			iW = 14; iH = 3;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '"': /*** Gate (open). ***/
			iW = 1; iH = 14;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '#': /*** Wall (inc. floor with tapestry). ***/
			iW = 12; iH = 14;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '$': /*** Coin on floor. ***/
			iW = 10; iH = 1;
			iR = 0xff; iG = 0xff; iB = 0x55;
			break;
		case '%': /*** Mirror. ***/
			iW = 14; iH = 14;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '&': /*** will never be used, available to modders ***/
			iW = 16; iH = 4;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '\'': /*** Torch on floor (inc. with debris). ***/
			iYellowBack = 1;
			iW = 16; iH = 6;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '(': /*** Gate top (inc. tapestry). ***/
			iW = 10; iH = 12;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case ')': /*** Gate (closed). ***/
			iW = 11; iH = 12;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '*': /*** Spikes (in). Harmless. ***/
			iW = 16; iH = 6;
			iR = 0xff; iG = 0xff; iB = 0xff;
			break;
		case '+': /*** Skeleton. ***/
			iW = 6; iH = 13;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case ',': /*** Teleport right. ***/
			iW = 16; iH = 12;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '-': /*** Debris. ***/
			iW = 16; iH = 6;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '.': /*** Empty. ***/
			iW = 1; iH = 1;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '/': /*** Lattice top. ***/
			iW = 9; iH = 14;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '0': /*** Potion (empty). ***/
			iW = 15; iH = 3;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '1': /*** Potion (heal). ***/
			iW = 15; iH = 3;
			iR = 0xaa; iG = 0x00; iB = 0x00;
			break;
		case '2': /*** Potion (life). ***/
			iW = 11; iH = 4;
			iR = 0xaa; iG = 0x00; iB = 0x00;
			break;
		case '3': /*** Potion (float). ***/
			iW = 11; iH = 4;
			iR = 0x00; iG = 0xaa; iB = 0x00;
			break;
		case '4': /*** Potion (flip). ***/
			iW = 11; iH = 4;
			iR = 0x00; iG = 0xaa; iB = 0x00;
			break;
		case '5': /*** Potion (hurt). ***/
			iW = 15; iH = 3;
			iR = 0x00; iG = 0x00; iB = 0xaa;
			break;
		case '6': /*** Potion (special blue). ***/
			iW = 15; iH = 3;
			iR = 0x00; iG = 0x00; iB = 0xaa;
			break;
		case '7': /*** reserved for future use ***/
			iW = 16; iH = 4;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '8': /*** Fake wall (floor that looks like a wall). ***/
			iW = 12; iH = 14;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '9': /*** Fake empty (floor that looks empty). ***/
			iW = 1; iH = 1;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case ':': /*** Pillar top. ***/
			iW = 5; iH = 16;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case ';': /*** Pillar bottom. ***/
			iW = 6; iH = 16;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '<': /*** reserved for future use ***/
			iW = 16; iH = 4;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '=': /*** Chomper (closed). ***/
			iW = 9; iH = 14;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '>': /*** reserved for future use ***/
			iW = 16; iH = 4;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '?': /*** Converted to - and appears in-game as - empty. ***/
			iW = 16; iH = 4;
			iR = 0x00; iG = 0x00; iB = 0x00;
			break;
		case '@': /*** Chomper (open). ***/
			iW = 4; iH = 12;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
			/*** Raise buttons A - R (18). ***/
			iW = 16; iH = 6;
			iR = 0x55; iG = 0xff; iB = 0x55;
			break;
		case 'S': case 'T': case 'U': case 'V':
		case 'W': case 'X': case 'Y': case 'Z':
			/*** Teleports left S - Z (8). ***/
			iW = 11; iH = 14;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '[': /*** Level door left (closed). ***/
			iW = 10; iH = 13;
			iR = 0x00; iG = 0xaa; iB = 0xaa;
			break;
		case '\\': /*** Lattice (inc. small, left, right). ***/
			iW = 16; iH = 13;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case ']': /*** Level door right (closed). ***/
			iW = 12; iH = 12;
			iR = 0x00; iG = 0xaa; iB = 0xaa;
			break;
		case '^': /*** Spikes (out). Harmful. ***/
			iW = 15; iH = 2;
			iR = 0xff; iG = 0xff; iB = 0xff;
			break;
		case '_': /*** Floor (inc. balcony left/right, stuck button). ***/
			iW = 16; iH = 6;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '`': /*** Empty with window. ***/
			iW = 16; iH = 8;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			/*** Drop buttons a - r (18). ***/
			iW = 16; iH = 6;
			iR = 0xff; iG = 0x55; iB = 0x55;
			break;
		case 's': case 't': case 'u': case 'v':
		case 'w': case 'x': case 'y': case 'z':
			/*** Teleports left s - z (8). ***/
			iW = 11; iH = 14;
			iR = 0xaa; iG = 0xaa; iB = 0xaa;
			break;
		case '{': /*** Level door left (open). ***/
			iW = 6; iH = 14;
			iR = 0x00; iG = 0xaa; iB = 0xaa;
			break;
		case '|': /*** Pillar (inc. for lattice). ***/
			iW = 4; iH = 12;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		case '}': /*** Level door right (open). ***/
			iW = 9; iH = 12;
			iR = 0x00; iG = 0xaa; iB = 0xaa;
			break;
		case '~': /*** Loose floor (inc. stuck). ***/
			iW = 16; iH = 6;
			iR = 0x55; iG = 0x55; iB = 0x55;
			break;
		default:
			printf ("[ WARN ] Unknown tile: %i.\n", cTile);
			iW = 16; iH = 4; /*** Fallback. ***/
			iR = 0x00; iG = 0x00; iB = 0x00; /*** Fallback. ***/
			break;
	}
	if (iYellowBack == 1)
	{
		iShowBack = 1 + (int)(12.5 * rand() / (RAND_MAX + 1.0));
		if (iShowBack == 1)
		{
			/*** white ***/
			ShowChar (12, 14, 0xff, 0xff, 0xff, iX, iY, iMode, iZoom, 0);
		} else {
			/*** yellow ***/
			ShowChar (12, 14, 0xff, 0xff, 0x55, iX, iY, iMode, iZoom, 0);
		}
	}
	ShowChar (iW, iH, iR, iG, iB, iX, iY, iMode, iZoom, iFade);
}
/*****************************************************************************/
void ShowLiving (char cLiving, int iX, int iY)
/*****************************************************************************/
{
	switch (cLiving)
	{
		case 'E':
			ShowChar (2, 1, 0xaa, 0x00, 0xaa, iX, iY, iMode, iZoom, 0);
			break;
		case 'H':
			ShowChar (2, 1, 0xff, 0x55, 0xff, iX, iY, iMode, iZoom, 0);
			break;
		case 'J':
			ShowChar (2, 1, 0x55, 0xff, 0xff, iX, iY, iMode, iZoom, 0);
			break;
		case 'S':
			ShowChar (3, 1, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0);
			break;
		case 'M':
			ShowChar (16, 1, 0xff, 0xff, 0xff, iX, iY, iMode, iZoom, 0);
			break;
		case 'P':
			if (iCurLives != 0)
			{
				ShowChar (2, 1, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0);
			} else {
				ShowChar (6, 13, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0);
			}
			break;
		case '+':
			ShowChar (6, 13, 0xaa, 0xaa, 0xaa, iX, iY, iMode, iZoom, 0);
			break;
		default:
			printf ("[FAILED] Invalid cLiving: %c!\n", cLiving);
			exit (EXIT_ERROR);
	}
}
/*****************************************************************************/
void ShowChar (int iW, int iH, int iR, int iG, int iB,
	int iX, int iY, int iM, int iZ, int iFade)
/*****************************************************************************/
{
	SDL_Rect src, dest;

	src.x = (iW - 1) * arWidth[iM];
	src.y = (iH - 1) * arHeight[iM];
	src.w = arWidth[iM];
	src.h = arHeight[iM];
	dest.x = iX;
	dest.y = iY;
	dest.w = arWidth[iM] * iZ;
	dest.h = arHeight[iM] * iZ;
	SDL_SetTextureColorMod (imgfont[iM], iR, iG, iB);
	if (SDL_RenderCopy (ascreen, imgfont[iM], &src, &dest) != 0)
		{ printf ("[ WARN ] SDL_RenderCopy (tile): %s.\n", SDL_GetError()); }
	if (iFade == 1)
	{
		src.x = 0; src.y = 0;
		if (SDL_RenderCopy (ascreen, imgfade[iM], &src, &dest) != 0)
			{ printf ("[ WARN ] SDL_RenderCopy (fade): %s.\n", SDL_GetError()); }
	}
}
/*****************************************************************************/
int PopUp (char *sQuestion)
/*****************************************************************************/
{
	int iPopUp;
	SDL_Event event;

	iPopUp = 2;

	ShowPopUp (sQuestion);
	while (iPopUp == 2)
	{
		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						case SDLK_n:
							iPopUp = 0;
							break;
						case SDLK_KP_ENTER:
						case SDLK_RETURN:
						case SDLK_SPACE:
						case SDLK_y:
							iPopUp = 1;
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					iXPos = event.motion.x;
					iYPos = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == 1)
					{
						if (InArea (333, 377, 333 + 85, 377 + 32) == 1) /*** Yes ***/
						{
							iOnYes = 1;
							ShowPopUp (sQuestion);
						}
						if (InArea (606, 377, 606 + 85, 377 + 32) == 1) /*** No ***/
						{
							iOnNo = 1;
							ShowPopUp (sQuestion);
						}
					}
					break;
				case SDL_MOUSEBUTTONUP:
					iOnYes = 0;
					iOnNo = 0;
					if (event.button.button == 1)
					{
						if (InArea (333, 377, 333 + 85, 377 + 32) == 1) /*** Yes ***/
						{
							iPopUp = 1;
						}
						if (InArea (606, 377, 606 + 85, 377 + 32) == 1) /*** No ***/
						{
							iPopUp = 0;
						}
					}
					ShowPopUp (sQuestion);
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowGame(); ShowPopUp (sQuestion); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		PreventCPUEating();
	}
	ShowGame();

	return (iPopUp);
}
/*****************************************************************************/
void ShowPopUp (char *sQuestion)
/*****************************************************************************/
{
	ShowImage (imgpopupyn, 0, 0, "imgpopupyn");

	/*** Yes ***/
	switch (iOnYes)
	{
		case 0: ShowImage (imgyesoff, 333, 377, "imgyesoff"); break; /*** off ***/
		case 1: ShowImage (imgyeson, 333, 377, "imgyeson"); break; /*** on ***/
	}

	/*** No ***/
	switch (iOnNo)
	{
		case 0: ShowImage (imgnooff, 606, 377, "imgnooff"); break; /*** off ***/
		case 1: ShowImage (imgnoon, 606, 377, "imgnoon"); break; /*** on ***/
	}

	DisplayText (350, 180, sQuestion);

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void DisplayText (int iX, int iY, char *sText)
/*****************************************************************************/
{
	text = TTF_RenderText_Shaded (font, sText, color_bl, color_wh);
	textt = SDL_CreateTextureFromSurface (ascreen, text);
	offset.x = iX; offset.y = iY;
	offset.w = text->w; offset.h = text->h;
	if (SDL_RenderCopy (ascreen, textt, NULL, &offset) != 0)
	{
		printf ("[ WARN ] SDL_RenderCopy (text): %s.\n", SDL_GetError());
	}
	SDL_DestroyTexture (textt); SDL_FreeSurface (text);
}
/*****************************************************************************/
void TryGoLeft (int iTurns)
/*****************************************************************************/
{
	char cLeft;
	char cChar;

	/*** Used for looping. ***/
	int iLoopTurn;

	for (iLoopTurn = 1; iLoopTurn <= iTurns; iLoopTurn++)
	{
		/*** Default, nothing changes. ***/
		iGoRoom = iCurRoom;
		iGoTile = iPrinceTile;

		cLeft = GetCharLeft();

		switch (iTurns)
		{
			case 1: /*** walk ***/
			case 3: /*** jump ***/
				if ((IsEmpty (cLeft) == 1) || (IsFloor (cLeft) == 1))
				{
					iGoRoom = GetRoomLeft();
					iGoTile = GetTileLeft();
				} else {
					PlaySound ("wav/bump.wav");
					return;
				}
				break;
			case 6: /*** runjump ***/
				if ((iLoopTurn == 1) || (iLoopTurn == 2))
				{
					if (IsFloor (cLeft) == 1)
					{
						iGoRoom = GetRoomLeft();
						iGoTile = GetTileLeft();
					} else { return; }
				} else {
					if ((IsEmpty (cLeft) == 1) || (IsFloor (cLeft) == 1) ||
						(cLeft == '%'))
					{
						iGoRoom = GetRoomLeft();
						iGoTile = GetTileLeft();
						if (cLeft == '%')
						{
							iCurLives = 1;
							PlaySound ("wav/mirror.wav");
							/*** Special events related. ***/
							if ((iCurLevel == 4) && (iCurRoom == 4) && (iPrinceTile == 6))
							{
								arGuardLoc[iCurRoom] = 6;
								arGuardType[iCurRoom] = 4;
								arGuardHP[iCurRoom] = iMaxLives;
								arGuardAttack[iCurRoom] = 0;
							}
						}
					} else {
						PlaySound ("wav/bump.wav");
						return;
					}
				}
				/*** Special events related. ***/
				if ((iCurLevel == 6) && (iGoRoom == 1) &&
					(((iGoTile >= 1) && (iGoTile <= 5)) ||
					((iGoTile >= 11) && (iGoTile <= 15)) ||
					((iGoTile >= 21) && (iGoTile <= 25))))
				{
					arGuardLoc[1] = 12;
					cChar = arTiles[1][12];
					if (((cChar >= 'A') && (cChar <= 'R')) ||
						((cChar >= 'a') && (cChar <= 'r')))
					{
						PushButton (cChar, 1);
						arTiles[1][12] = '-';
					}
				}
				break;
		}

		if ((iGoRoom != iCurRoom) || (iGoTile != iPrinceTile))
		{
			/*** Special events related. ***/
			if ((iCurLevel == 12) && (iCurRoom == 13) && (iGoRoom != iCurRoom))
			{
				iCurLevel++;
				LoadLevel (iCurLevel, iMaxLives);
				return;
			} else if ((iCurLevel == 14) && (iGoRoom == 5)) {
				snprintf (sMessage, MAX_MESSAGE, "You used %i steps.\n", iSteps);
				SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_INFORMATION,
					"Victory!", sMessage, window);
				iSteps = 0;
				iCurLevel = 1;
				LoadLevel (iCurLevel, START_LIVES);
			} else {
				iCurRoom = iGoRoom;
				iPrinceTile = iGoTile;
				if ((iTurns == 6) && ((iLoopTurn == 1) || (iLoopTurn == 2)))
					{ GameActions(); }
			}
		}
	}

	if ((iTurns == 3) || (iTurns == 6)) { iPrinceHang = 1; }
}
/*****************************************************************************/
void TryGoRight (int iTurns)
/*****************************************************************************/
{
	char cRight;

	/*** Used for looping. ***/
	int iLoopTurn;

	for (iLoopTurn = 1; iLoopTurn <= iTurns; iLoopTurn++)
	{
		/*** Default, nothing changes. ***/
		iGoRoom = iCurRoom;
		iGoTile = iPrinceTile;

		cRight = GetCharRight();

		switch (iTurns)
		{
			case 1: /*** walk ***/
			case 3: /*** jump ***/
				if ((IsEmpty (cRight) == 1) || (IsFloor (cRight) == 1))
				{
					iGoRoom = GetRoomRight();
					iGoTile = GetTileRight();
				} else {
					PlaySound ("wav/bump.wav");
					return;
				}
				break;
			case 6: /*** runjump ***/
				if ((iLoopTurn == 1) || (iLoopTurn == 2))
				{
					if (IsFloor (cRight) == 1)
					{
						iGoRoom = GetRoomRight();
						iGoTile = GetTileRight();
					} else { return; }
				} else {
					if ((IsEmpty (cRight) == 1) || (IsFloor (cRight) == 1))
					{
						iGoRoom = GetRoomRight();
						iGoTile = GetTileRight();
					} else {
						PlaySound ("wav/bump.wav");
						return;
					}
				}
				break;
		}

		if ((iGoRoom != iCurRoom) || (iGoTile != iPrinceTile))
		{
			iCurRoom = iGoRoom;
			iPrinceTile = iGoTile;
			if ((iTurns == 6) && ((iLoopTurn == 1) || (iLoopTurn == 2)))
				{ GameActions(); }
		}
	}

	if ((iTurns == 3) || (iTurns == 6)) { iPrinceHang = 1; }
}
/*****************************************************************************/
void TryGoUp (void)
/*****************************************************************************/
{
	char cLeft, cRight, cUp, cUpRight, cUpLeft;
	int iRoom, iTile;
	char cGoTo;

	/*** Default, nothing changes. ***/
	iGoRoom = iCurRoom;
	iGoTile = iPrinceTile;

	cLeft = GetCharLeft();
	cRight = GetCharRight();
	cUp = GetCharUp();
	cUpRight = GetCharUpRight();
	cUpLeft = GetCharUpLeft();

	/*** Level door left/right (open). ***/
	if ((arTiles[iCurRoom][iPrinceTile] == '{') ||
		(arTiles[iCurRoom][iPrinceTile] == '}'))
	{
		iCurLevel++;
		LoadLevel (iCurLevel, iMaxLives);
		return;
	}

	/*** Teleports left/right. ***/
	if (((arTiles[iCurRoom][iPrinceTile] >= 'S') &&
		(arTiles[iCurRoom][iPrinceTile] <= 'Z')) ||
		((arTiles[iCurRoom][iPrinceTile] >= 's') &&
		(arTiles[iCurRoom][iPrinceTile] <= 's')) ||
		(arTiles[iCurRoom][iPrinceTile] == ','))
	{
		if (arTiles[iCurRoom][iPrinceTile] == ',')
			{ cGoTo = GetCharLeft(); }
				else { cGoTo = arTiles[iCurRoom][iPrinceTile]; }
		Teleport (cGoTo);
		return;
	}

	if (IsEmpty (cUp) == 1)
	{
		if ((iPrinceDir == 1) && (IsFloor (cUpLeft) == 1))
		{
			iGoRoom = GetRoomUpLeft();
			iGoTile = GetTileUpLeft();
		}
		if ((iPrinceDir == 2) && (IsFloor (cUpRight) == 1))
		{
			iGoRoom = GetRoomUpRight();
			iGoTile = GetTileUpRight();
		}
	}
	if (IsFloor (cUp) == 1)
	{
		if (((IsEmpty (cUpLeft) == 1) &&
			((IsEmpty (cLeft) == 1) || (IsFloor (cLeft) == 1))) ||
			((IsEmpty (cUpRight) == 1) &&
			((IsEmpty (cRight) == 1) || (IsFloor (cRight) == 1))))
		{
			iGoRoom = GetRoomUp();
			iGoTile = GetTileUp();
		}
	}

	/*** Loose floors. ***/
	if (cUp == '~')
	{
		if (iPrinceTile > 10)
		{
			DropLoose (iCurRoom, iPrinceTile - 10);
		} else if (arLinksU[iCurRoom] != 0) {
			DropLoose (arLinksU[iCurRoom], iPrinceTile + 20);
		}
	} else if ((iPrinceDir == 1) && (cUpLeft == '~')) {
		iRoom = GetRoomUpLeft();
		iTile = GetTileUpLeft();
		DropLoose (iRoom, iTile);
	} else if ((iPrinceDir == 2) && (cUpRight == '~')) {
		iRoom = GetRoomUpRight();
		iTile = GetTileUpRight();
		DropLoose (iRoom, iTile);
	} else if (cUpLeft == '~') {
		iRoom = GetRoomUpLeft();
		iTile = GetTileUpLeft();
		DropLoose (iRoom, iTile);
	} else if (cUpRight == '~') {
		iRoom = GetRoomUpRight();
		iTile = GetTileUpRight();
		DropLoose (iRoom, iTile);
	}

	if ((iGoRoom != iCurRoom) || (iGoTile != iPrinceTile))
	{
		iCurRoom = iGoRoom;
		iPrinceTile = iGoTile;
	}
}
/*****************************************************************************/
void TryGoDown (void)
/*****************************************************************************/
{
	char cLeft, cRight, cDown;

	/*** Default, nothing changes. ***/
	iGoRoom = iCurRoom;
	iGoTile = iPrinceTile;

	cLeft = GetCharLeft();
	cRight = GetCharRight();
	cDown = GetCharDown();

	switch (arTiles[iCurRoom][iPrinceTile])
	{
		case '!': /*** Sword. ***/
			iPrinceSword = 1;
			iFlash+=25;
			iFlashR = 0xaa; iFlashG = 0xaa; iFlashB = 0xaa;
			arTiles[iCurRoom][iPrinceTile] = '_';
			/*** Special events related. ***/
			if ((iCurLevel == 12) && (iCurRoom == 15) && (iPrinceTile == 2))
			{
				arTiles[2][1] = '9';
				arTiles[2][2] = '9';
				arTiles[2][3] = '9';
				arTiles[2][4] = '9';
				arTiles[2][5] = '9';
				arTiles[2][6] = '9';
				arTiles[2][7] = '9';
				arTiles[2][8] = '9';
				arTiles[13][7] = '9';
				arTiles[13][8] = '9';
				arTiles[13][9] = '9';
				arTiles[13][10] = '9';
			}
			break;
		case '0': /*** Potion (empty). ***/
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
		case '1': /*** Potion (heal). ***/
			PlaySound ("wav/drinking.wav");
			if (iCurLives < iMaxLives) { iCurLives++; }
			iFlash+=25;
			iFlashR = 0xaa; iFlashG = 0x00; iFlashB = 0x00;
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
		case '2': /*** Potion (life). ***/
			PlaySound ("wav/drinking.wav");
			iMaxLives++;
			iCurLives = iMaxLives;
			iLevLives++;
			iFlash+=25;
			iFlashR = 0xaa; iFlashG = 0x00; iFlashB = 0x00;
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
		case '3': /*** Potion (float). ***/
			PlaySound ("wav/drinking.wav");
			iPrinceFloat+=50;
			iFlash+=25;
			iFlashR = 0x00; iFlashG = 0xaa; iFlashB = 0x00;
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
		case '4': /*** Potion (flip). ***/
			/*** Not yet implemented. ***/
			break;
		case '5': /*** Potion (hurt). ***/
			PlaySound ("wav/drinking.wav");
			SDL_Delay (500);
			PlaySound ("wav/hit_prince.wav");
			iCurLives--;
			if (iCurLives == 0) { Die(); return; }
			iFlash+=25;
			iFlashR = 0x00; iFlashG = 0x00; iFlashB = 0xaa;
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
		case '6': /*** Potion (special blue). ***/
			/*** Not yet implemented. ***/
			break;
	}

	if ((IsEmpty (cDown) == 1) || (IsFloor (cDown) == 1))
	{
		if ((IsEmpty (cLeft) == 1) || (IsEmpty (cRight) == 1))
		{
			iGoRoom = GetRoomDown();
			iGoTile = GetTileDown();
		}
	}

	if ((iGoRoom != iCurRoom) || (iGoTile != iPrinceTile))
	{
		iCurRoom = iGoRoom;
		iPrinceTile = iGoTile;
	}
}
/*****************************************************************************/
void ToggleJump (void)
/*****************************************************************************/
{
	if (iJump == 0)
	{
		iJump = 1;
		iCareful = 0;
		iRunJump = 0;
	} else {
		iJump = 0;
	}
}
/*****************************************************************************/
void ToggleCareful (void)
/*****************************************************************************/
{
	if (iCareful == 0)
	{
		iCareful = 1;
		iJump = 0;
		iRunJump = 0;
	} else {
		iCareful = 0;
	}
}
/*****************************************************************************/
void ToggleRunJump (void)
/*****************************************************************************/
{
	if (iRunJump == 0)
	{
		iRunJump = 1;
		iJump = 0;
		iCareful = 0;
	} else {
		iRunJump = 0;
	}
}
/*****************************************************************************/
char GetCharLeft (void)
/*****************************************************************************/
{
	if ((iPrinceTile != 1) && (iPrinceTile != 11) && (iPrinceTile != 21))
	{
		return (arTiles[iCurRoom][iPrinceTile - 1]);
	} else if (arLinksL[iCurRoom] != 0) {
		return (arTiles[arLinksL[iCurRoom]][iPrinceTile + 9]);
	} else {
		return ('#');
	}
}
/*****************************************************************************/
char GetCharRight (void)
/*****************************************************************************/
{
	if ((iPrinceTile != 10) && (iPrinceTile != 20) && (iPrinceTile != 30))
	{
		return (arTiles[iCurRoom][iPrinceTile + 1]);
	} else if (arLinksR[iCurRoom] != 0) {
		return (arTiles[arLinksR[iCurRoom]][iPrinceTile - 9]);
	} else {
		return ('#');
	}
}
/*****************************************************************************/
char GetCharUp (void)
/*****************************************************************************/
{
	if (iPrinceTile > 10)
	{
		return (arTiles[iCurRoom][iPrinceTile - 10]);
	} else if (arLinksU[iCurRoom] != 0) {
		return (arTiles[arLinksU[iCurRoom]][iPrinceTile + 20]);
	} else {
		return ('#');
	}
}
/*****************************************************************************/
char GetCharDown (void)
/*****************************************************************************/
{
	if (iPrinceTile <= 20)
	{
		return (arTiles[iCurRoom][iPrinceTile + 10]);
	} else if (arLinksD[iCurRoom] != 0) {
		return (arTiles[arLinksD[iCurRoom]][iPrinceTile - 20]);
	} else {
		return ('#');
	}
}
/*****************************************************************************/
char GetCharUpLeft (void)
/*****************************************************************************/
{
	if (((iPrinceTile >= 12) && (iPrinceTile <= 20)) ||
		((iPrinceTile >= 22) && (iPrinceTile <= 30)))
	{
		return (arTiles[iCurRoom][iPrinceTile - 11]);
	} else if ((iPrinceTile == 11) || (iPrinceTile == 21)) {
		if (arLinksL[iCurRoom] != 0)
		{
			return (arTiles[arLinksL[iCurRoom]][iPrinceTile - 1]);
		} else {
			return ('#');
		}
	} else if ((iPrinceTile >= 2) && (iPrinceTile <= 10)) {
		if (arLinksU[iCurRoom] != 0)
		{
			return (arTiles[arLinksU[iCurRoom]][iPrinceTile + 19]);
		} else {
			return ('#');
		}
	} else { /*** iPrinceTile == 1 ***/
		if (arLinksL[iCurRoom] != 0)
		{
			if (arLinksU[arLinksL[iCurRoom]] != 0)
			{
				return (arTiles[arLinksU[arLinksL[iCurRoom]]][30]);
			} else {
				return ('#');
			}
		} else {
			return ('#');
		}
	}
}
/*****************************************************************************/
char GetCharUpRight (void)
/*****************************************************************************/
{
	if (((iPrinceTile >= 11) && (iPrinceTile <= 19)) ||
		((iPrinceTile >= 21) && (iPrinceTile <= 29)))
	{
		return (arTiles[iCurRoom][iPrinceTile - 9]);
	} else if ((iPrinceTile == 20) || (iPrinceTile == 30)) {
		if (arLinksR[iCurRoom] != 0)
		{
			return (arTiles[arLinksR[iCurRoom]][iPrinceTile - 19]);
		} else {
			return ('#');
		}
	} else if ((iPrinceTile >= 1) && (iPrinceTile <= 9)) {
		if (arLinksU[iCurRoom] != 0)
		{
			return (arTiles[arLinksU[iCurRoom]][iPrinceTile + 21]);
		} else {
			return ('#');
		}
	} else { /*** iPrinceTile == 10 ***/
		if (arLinksR[iCurRoom] != 0)
		{
			if (arLinksU[arLinksR[iCurRoom]] != 0)
			{
				return (arTiles[arLinksU[arLinksR[iCurRoom]]][21]);
			} else {
				return ('#');
			}
		} else {
			return ('#');
		}
	}
}
/*****************************************************************************/
int GetRoomLeft (void)
/*****************************************************************************/
{
	if ((iPrinceTile != 1) && (iPrinceTile != 11) && (iPrinceTile != 21))
	{
		return (iCurRoom);
	} else if (arLinksL[iCurRoom] != 0) {
		return (arLinksL[iCurRoom]);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetRoomRight (void)
/*****************************************************************************/
{
	if ((iPrinceTile != 10) && (iPrinceTile != 20) && (iPrinceTile != 30))
	{
		return (iCurRoom);
	} else if (arLinksR[iCurRoom] != 0) {
		return (arLinksR[iCurRoom]);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetRoomUp (void)
/*****************************************************************************/
{
	if (iPrinceTile > 10)
	{
		return (iCurRoom);
	} else if (arLinksU[iCurRoom] != 0) {
		return (arLinksU[iCurRoom]);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetRoomDown (void)
/*****************************************************************************/
{
	if (iPrinceTile <= 20)
	{
		return (iCurRoom);
	} else if (arLinksD[iCurRoom] != 0) {
		return (arLinksD[iCurRoom]);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetRoomUpLeft (void)
/*****************************************************************************/
{
	if (((iPrinceTile >= 12) && (iPrinceTile <= 20)) ||
		((iPrinceTile >= 22) && (iPrinceTile <= 30)))
	{
		return (iCurRoom);
	} else if ((iPrinceTile == 11) || (iPrinceTile == 21)) {
		if (arLinksL[iCurRoom] != 0)
		{
			return (arLinksL[iCurRoom]);
		} else {
			return (0);
		}
	} else if ((iPrinceTile >= 2) && (iPrinceTile <= 10)) {
		if (arLinksU[iCurRoom] != 0)
		{
			return (arLinksU[iCurRoom]);
		} else {
			return (0);
		}
	} else { /*** iPrinceTile == 1 ***/
		if (arLinksL[iCurRoom] != 0)
		{
			if (arLinksU[arLinksL[iCurRoom]] != 0)
			{
				return (arLinksU[arLinksL[iCurRoom]]);
			} else {
				return (0);
			}
		} else {
			return (0);
		}
	}
}
/*****************************************************************************/
int GetRoomUpRight (void)
/*****************************************************************************/
{
	if (((iPrinceTile >= 11) && (iPrinceTile <= 19)) ||
		((iPrinceTile >= 21) && (iPrinceTile <= 29)))
	{
		return (iCurRoom);
	} else if ((iPrinceTile == 20) || (iPrinceTile == 30)) {
		if (arLinksR[iCurRoom] != 0)
		{
			return (arLinksR[iCurRoom]);
		} else {
			return (0);
		}
	} else if ((iPrinceTile >= 1) && (iPrinceTile <= 9)) {
		if (arLinksU[iCurRoom] != 0)
		{
			return (arLinksU[iCurRoom]);
		} else {
			return (0);
		}
	} else { /*** iPrinceTile == 10 ***/
		if (arLinksR[iCurRoom] != 0)
		{
			if (arLinksU[arLinksR[iCurRoom]] != 0)
			{
				return (arLinksU[arLinksR[iCurRoom]]);
			} else {
				return (0);
			}
		} else {
			return (0);
		}
	}
}
/*****************************************************************************/
int GetTileLeft (void)
/*****************************************************************************/
{
	if ((iPrinceTile != 1) && (iPrinceTile != 11) && (iPrinceTile != 21))
	{
		return (iPrinceTile - 1);
	} else if (arLinksL[iCurRoom] != 0) {
		return (iPrinceTile + 9);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetTileRight (void)
/*****************************************************************************/
{
	if ((iPrinceTile != 10) && (iPrinceTile != 20) && (iPrinceTile != 30))
	{
		return (iPrinceTile + 1);
	} else if (arLinksR[iCurRoom] != 0) {
		return (iPrinceTile - 9);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetTileUp (void)
/*****************************************************************************/
{
	if (iPrinceTile > 10)
	{
		return (iPrinceTile - 10);
	} else if (arLinksU[iCurRoom] != 0) {
		return (iPrinceTile + 20);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetTileDown (void)
/*****************************************************************************/
{
	if (iPrinceTile <= 20)
	{
		return (iPrinceTile + 10);
	} else if (arLinksD[iCurRoom] != 0) {
		return (iPrinceTile - 20);
	} else {
		return (0);
	}
}
/*****************************************************************************/
int GetTileUpLeft (void)
/*****************************************************************************/
{
	if (((iPrinceTile >= 12) && (iPrinceTile <= 20)) ||
		((iPrinceTile >= 22) && (iPrinceTile <= 30)))
	{
		return (iPrinceTile - 11);
	} else if ((iPrinceTile == 11) || (iPrinceTile == 21)) {
		if (arLinksL[iCurRoom] != 0)
		{
			return (iPrinceTile - 1);
		} else {
			return (0);
		}
	} else if ((iPrinceTile >= 2) && (iPrinceTile <= 10)) {
		if (arLinksU[iCurRoom] != 0)
		{
			return (iPrinceTile + 19);
		} else {
			return (0);
		}
	} else { /*** iPrinceTile == 1 ***/
		if (arLinksL[iCurRoom] != 0)
		{
			if (arLinksU[arLinksL[iCurRoom]] != 0)
			{
				return (30);
			} else {
				return (0);
			}
		} else {
			return (0);
		}
	}
}
/*****************************************************************************/
int GetTileUpRight (void)
/*****************************************************************************/
{
	if (((iPrinceTile >= 11) && (iPrinceTile <= 19)) ||
		((iPrinceTile >= 21) && (iPrinceTile <= 29)))
	{
		return (iPrinceTile - 9);
	} else if ((iPrinceTile == 20) || (iPrinceTile == 30)) {
		if (arLinksR[iCurRoom] != 0)
		{
			return (iPrinceTile - 19);
		} else {
			return (0);
		}
	} else if ((iPrinceTile >= 1) && (iPrinceTile <= 9)) {
		if (arLinksU[iCurRoom] != 0)
		{
			return (iPrinceTile + 21);
		} else {
			return (0);
		}
	} else { /*** iPrinceTile == 10 ***/
		if (arLinksR[iCurRoom] != 0)
		{
			if (arLinksU[arLinksR[iCurRoom]] != 0)
			{
				return (21);
			} else {
				return (0);
			}
		} else {
			return (0);
		}
	}
}
/*****************************************************************************/
int IsEmpty (char cChar)
/*****************************************************************************/
{
	switch (cChar)
	{
		case '.': return (1); break; /*** Empty. ***/
		case '/': return (1); break; /*** Lattice top. ***/
		case ':': return (1); break; /*** Pillar top. ***/
		case '?': return (1); break; /*** Unknown tile. Converted to empty. ***/
		case '\\': return (1); break; /*** Lattice (inc. small, left, right). ***/
		case '`': return (1); break; /*** Empty with window. ***/
	}

	return (0);
}
/*****************************************************************************/
int IsFloor (char cChar)
/*****************************************************************************/
{
	/*** What are NOT floors? ***/
	switch (cChar)
	{
		case '#': return (0); break; /*** Wall (inc. floor with tapestry). ***/
		case '%': return (0); break; /*** Mirror. ***/
		case '(': return (0); break; /*** Gate top (inc. tapestry). ***/
		case ')': return (0); break; /*** Gate (closed). ***/
		case '.': return (0); break; /*** Empty. ***/
		case '/': return (0); break; /*** Lattice top. ***/
		case ':': return (0); break; /*** Pillar top. ***/
		case '?': return (0); break; /*** Unknown tile. Converted to empty. ***/
		case '\\': return (0); break; /*** Lattice (inc. small, left, right). ***/
		case '`': return (0); break; /*** Empty with window. ***/
	}

	return (1);
}
/*****************************************************************************/
void GameActions (void)
/*****************************************************************************/
{
	char cSpike;
	int iDown;
	char cTile;
	char cUpLeft, cUpRight;
	int iChompBool;
	int iChompNoise;
	int iGrabbedLedge;
	int iRoomLeft, iTileLeft;
	int iRoomRight, iTileRight;
	int iGuardBool;

	/*** Used for looping. ***/
	int iLoopTile;

	iRoomLeft = GetRoomLeft();
	iTileLeft = GetTileLeft();
	iRoomRight = GetRoomRight();
	iTileRight = GetTileRight();

	/******************/
	/* STEP 1: SPIKES */
	/******************/
	for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
	{
		if ((arTiles[iCurRoom][iLoopTile] == '*') ||
			(arTiles[iCurRoom][iLoopTile] == '^'))
		{
			/*** cSpike ***/
			cSpike = '*';
			if ((iLoopTile >= 11) && (iLoopTile <= 30))
			{
				if (((iLoopTile - 10) == iPrinceTile) ||
					((iLoopTile - 20) == iPrinceTile))
					{ cSpike = '^'; }
				if (((iLoopTile >= 12) && (iLoopTile <= 20)) ||
					((iLoopTile >= 22) && (iLoopTile <= 30)))
				{
					if (((iLoopTile - 11) == iPrinceTile) ||
						((iLoopTile - 21) == iPrinceTile)) { cSpike = '^'; }
				}
				if (((iLoopTile >= 11) && (iLoopTile <= 19)) ||
					((iLoopTile >= 21) && (iLoopTile <= 29)))
				{
					if (((iLoopTile - 9) == iPrinceTile) ||
						((iLoopTile - 19) == iPrinceTile)) { cSpike = '^'; }
				}
			}
			/*** Prince next to, or on, spike. ***/
			if (((iCurRoom == iRoomLeft) && (iLoopTile == iTileLeft)) ||
				((iCurRoom == iRoomRight) && (iLoopTile == iTileRight)) ||
				(iLoopTile == iPrinceTile))
			{
				cSpike = '^';
			}

			switch (cSpike)
			{
				case '*':
					if (arTiles[iCurRoom][iLoopTile] == '^')
					{
						arTiles[iCurRoom][iLoopTile] = '*';
					}
					break;
				case '^':
					if (arTiles[iCurRoom][iLoopTile] == '*')
					{
						arTiles[iCurRoom][iLoopTile] = '^';
						PlaySound ("wav/spikes_out.wav");
					}
					break;
			}
		}
	}

	/*******************/
	/* STEP 2: CHOMPER */
	/*******************/
	iChompBool = (SDL_GetTicks() / 1000) % 2;
	iChompNoise = 0;
	for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
	{
		if (((iLoopTile >= 1) && (iLoopTile <= 10) &&
			(iPrinceTile >= 1) && (iPrinceTile <= 10)) ||
			((iLoopTile >= 11) && (iLoopTile <= 20) &&
			(iPrinceTile >= 11) && (iPrinceTile <= 20)) ||
			((iLoopTile >= 21) && (iLoopTile <= 30) &&
			(iPrinceTile >= 21) && (iPrinceTile <= 30)))
		{
			if (iChompBool == 0)
			{
				if (arTiles[iCurRoom][iLoopTile] == '=')
				{
					arTiles[iCurRoom][iLoopTile] = '@';
				}
			} else {
				if (arTiles[iCurRoom][iLoopTile] == '@')
				{
					if (iChompNoise == 0)
					{
						PlaySound ("wav/chomper.wav");
						iChompNoise = 1;
					}
					arTiles[iCurRoom][iLoopTile] = '=';
				}
			}
		} else {
			if (arTiles[iCurRoom][iLoopTile] == '=')
			{
				arTiles[iCurRoom][iLoopTile] = '@';
			}
		}
	}

	/****************/
	/* STEP 3: FALL */
	/****************/
	iDown = 0;
	iGrabbedLedge = 0;
	while ((IsEmpty (arTiles[iCurRoom][iPrinceTile]) == 1) &&
		(iPrinceHang != 2) && (iCurLives != 0))
	{
		if (iPrinceTile <= 20)
		{
			iPrinceTile+=10;
		} else if (arLinksD[iCurRoom] != 0) {
			iCurRoom = arLinksD[iCurRoom];
			iPrinceTile-=20;
		} else {
			/*** Special events related. ***/
			if ((iCurLevel == 6) && (iCurRoom == 3))
			{
				iCurLevel++;
				LoadLevel (iCurLevel, iMaxLives);
				return;
			} else {
				iCurLives = 0;
			}
		}
		SDL_Delay (100);
		ShowGame();
		iDown++;
		if ((iDown == 3) && (iPrinceFloat == 0)) { PlaySound ("wav/scream.wav"); }

		cUpLeft = GetCharUpLeft();
		cUpRight = GetCharUpRight();
/***
This cannot be used, because of e.g. level 7, room 14:
if ((iPrinceHang == 1) &&
	(((iPrinceDir == 1) && (IsFloor (cUpLeft) == 1)) ||
	((iPrinceDir == 2) && (IsFloor (cUpRight) == 1))))
***/
		if ((iPrinceHang == 1) &&
			(IsFloor (arTiles[iCurRoom][iPrinceTile]) != 1) &&
			((IsFloor (cUpLeft) == 1) || (IsFloor (cUpRight) == 1)))
		{
			/*** Turn around if necessary. ***/
			switch (iPrinceDir)
			{
				case 1: /*** left ***/
					if (IsFloor (cUpLeft) == 0) { iPrinceDir = 2; }
					break;
				case 2: /*** right ***/
					if (IsFloor (cUpRight) == 0) { iPrinceDir = 1; }
			}

			PlaySound ("wav/grab.wav");
			iGrabbedLedge = 1;
			iPrinceHang = 2;
		}
	}
	switch (iDown)
	{
		case 0:
			break;
		case 1:
			if (iGrabbedLedge == 0)
			{
				PlaySound ("wav/landing_soft.wav");
			}
			break;
		case 2:
			if (iGrabbedLedge == 0)
			{
				if (iPrinceFloat == 0)
				{
					iCurLives--;
					if (iCurLives != 0)
					{
						PlaySound ("wav/landing_hurt.wav");
					} else {
						PlaySound ("wav/landing_dead.wav");
					}
				} else {
					PlaySound ("wav/landing_soft.wav");
				}
			}
			break;
		default:
			if (iPrinceFloat == 0)
			{
				iCurLives = 0;
				PlaySound ("wav/landing_dead.wav");
			} else {
				PlaySound ("wav/landing_soft.wav");
			}
			break;
	}

	/*****************/
	/* STEP 4: TILES */
	/*****************/
	cTile = arTiles[iCurRoom][iPrinceTile];
	switch (cTile)
	{
		case '^': /*** Spikes (out). Harmful. ***/
			if (iPrinceSafe == 0)
			{
				PlaySound ("wav/spikes_death_b.wav");
				iCurLives = 0;
			}
			break;
		case '~': /*** Loose floor (inc. stuck). ***/
			DropLoose (iCurRoom, iPrinceTile);
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
			/*** Raise buttons A - R (18). ***/
			PushButton (cTile, 0);
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			/*** Drop buttons a - r (18). ***/
			PushButton (cTile, 0);
			break;
		case '=': /*** Chomper (closed). ***/
			PlaySound ("wav/chomper_death.wav");
			iCurLives = 0;
			break;
		case '$': /*** Coin on floor. ***/
			arTiles[iCurRoom][iPrinceTile] = '_';
			iPrinceCoins++;
			PlaySound ("wav/coin.wav");
			iShowStepsCoins = 1;
			break;
		case '8': /*** Fake wall (floor that looks like a wall). ***/
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
		case '9': /*** Fake empty (floor that looks empty). ***/
			arTiles[iCurRoom][iPrinceTile] = '_';
			break;
	}

	/*************************/
	/* STEP 5: GUARD ATTACKS */
	/*************************/
	if ((arGuardLoc[iCurRoom] == iPrinceTile) &&
		(arGuardHP[iCurRoom] != 0) && (arGuardType[iCurRoom] <= 3))
	{
		iCurLives = 0;
		PlaySound ("wav/hit_prince.wav");
	}
	if ((arGuardLoc[iRoomRight] == iTileRight) &&
		(arGuardHP[iRoomRight] != 0))
	{
		switch (arGuardType[iRoomRight])
		{
			case 1: iGuardBool = (SDL_GetTicks() / 500) % 2; break; /*** E ***/
			case 2: iGuardBool = (SDL_GetTicks() / 250) % 2; break; /*** H ***/
			case 3: iGuardBool = (SDL_GetTicks() / 200) % 2; break; /*** J ***/
			default: iGuardBool = 0;
		}
		if (iGuardBool == 0)
		{
			arGuardAttack[iRoomRight] = 0;
		} else {
			if (arGuardAttack[iRoomRight] == 0)
			{
				iCurLives--;
				PlaySound ("wav/hit_prince.wav");
				arGuardAttack[iRoomRight] = 1;
			}
		}
	}
	if ((arGuardLoc[iRoomLeft] == iTileLeft) &&
		(arGuardHP[iRoomLeft] != 0))
	{
		switch (arGuardType[iRoomLeft])
		{
			case 1: iGuardBool = (SDL_GetTicks() / 500) % 2; break; /*** easy ***/
			case 2: iGuardBool = (SDL_GetTicks() / 250) % 2; break; /*** hard ***/
			default: iGuardBool = 0;
		}
		if (iGuardBool == 0)
		{
			arGuardAttack[iRoomLeft] = 0;
		} else {
			if (arGuardAttack[iRoomLeft] == 0)
			{
				iCurLives--;
				PlaySound ("wav/hit_prince.wav");
				arGuardAttack[iRoomLeft] = 1;
			}
		}
	}

	/***************/
	/* STEP 6: DIE */
	/***************/
	if (iCurLives == 0)
	{
		Die();
	}
}
/*****************************************************************************/
void PushButton (char cChar, int iForever)
/*****************************************************************************/
{
	int iRoom, iTile;
	int iFrames;

	/*** Used for looping. ***/
	int iLoopEvent;

	if (iForever == 0)
	{
		iFrames = 125; /*** = 10 seconds ***/
	} else {
		iFrames = 45000; /*** = 1 hour, should be enough ***/
	}

	/*** Raise. ***/
	if ((cChar >= 'A') && (cChar <= 'R'))
	{
		for (iLoopEvent = 1; iLoopEvent <= 10; iLoopEvent++)
		{
			iRoom = arLettersRoom[(int)cChar][iLoopEvent];
			iTile = arLettersTile[(int)cChar][iLoopEvent];
			if (arTiles[iRoom][iTile] == ')')
			{
				PlaySound ("wav/gate_open.wav");
				arTiles[iRoom][iTile] = '"';
				arGateTimers[iRoom][iTile] = iFrames;
				/*** Special events related. ***/
				if ((iCurLevel == 5) && (iRoom == 24) && (iTile == 2))
				{
					arGuardLoc[iRoom] = 4;
					arGuardType[iRoom] = 4;
					arGuardHP[iRoom] = iMaxLives;
					arGuardAttack[iRoom] = 0;
					/***/
					if ((arTiles[24][4] >= '0') && (arTiles[24][4] <= '6'))
					{
						arTiles[24][4] = '_';
						PlaySound ("wav/drinking.wav");
					}
				}
			} else if (arTiles[iRoom][iTile] == '"') {
				arGateTimers[iRoom][iTile] = iFrames;
			}

			/*** Level door left/right. ***/
			if (arTiles[iRoom][iTile] == '[')
			{
				if (iCoinsInLevel == iPrinceCoins)
				{
					PlaySound ("wav/level_door_open.wav");
					arTiles[iRoom][iTile] = '{';
					if ((iTile != 10) && (iTile != 20) && (iTile != 30))
					{
						if (arTiles[iRoom][iTile + 1] == ']')
							{ arTiles[iRoom][iTile + 1] = '}'; }
					} else if (arLinksR[iRoom] != 0) {
						if (arTiles[arLinksR[iRoom]][iTile - 9] == ']')
							{ arTiles[arLinksR[iRoom]][iTile - 9] = '}'; }
					}
					/*** Special events related. ***/
					switch (iCurLevel)
					{
						case 4: iMirror = 1; break;
						case 8: iMouse = 1; break;
					}
				} else if (iNoMessage == 0) {
					snprintf (sMessage, MAX_MESSAGE, "Coins collected: %i / %i",
						iPrinceCoins, iCoinsInLevel);
					SDL_ShowSimpleMessageBox (SDL_MESSAGEBOX_INFORMATION,
						"Note", sMessage, window);
					iNoMessage = 1;
				}
			}
		}
	}

	/*** Drop. ***/
	if ((cChar >= 'a') && (cChar <= 'r'))
	{
		for (iLoopEvent = 1; iLoopEvent <= 10; iLoopEvent++)
		{
			iRoom = arLettersRoom[(int)cChar][iLoopEvent];
			iTile = arLettersTile[(int)cChar][iLoopEvent];
			if (arTiles[iRoom][iTile] == '"')
			{
				arTiles[iRoom][iTile] = ')';
				PlaySound ("wav/gate_close_fast.wav");
			}
		}
	}
}
/*****************************************************************************/
void Die (void)
/*****************************************************************************/
{
	ShowGame();
	SDL_Delay (1000);
	LoadLevel (iCurLevel, iMaxLives - iLevLives);
}
/*****************************************************************************/
void BossKey (void)
/*****************************************************************************/
{
	int iBossKey;
	SDL_Event event;

	iBossKey = 1;

	ShowBossKey();
	while (iBossKey == 1)
	{
		/*** This is for the animation. ***/
		newticks = SDL_GetTicks();
		if (newticks > oldticks + REFRESH_GAME)
		{
			ShowBossKey();
			oldticks = newticks;
		}

		while (SDL_PollEvent (&event))
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym)
					{
						case SDLK_F9:
							iBossKey = 0;
							break;
					}
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
						{ ShowGame(); ShowBossKey(); } break;
				case SDL_QUIT:
					Quit(); break;
			}
		}

		PreventCPUEating();
	}

	ShowGame();
}
/*****************************************************************************/
void ShowBossKey (void)
/*****************************************************************************/
{
	ShowImage (imgscreend, 0, 0, "imgscreend");

	ShowText (1, "C:\\>", 0xaa, 0xaa, 0xaa, 0);
	if ((SDL_GetTicks() / 500) % 2 == 0)
	{
		/*** _ ***/
		ShowChar (16, 6, 0xaa, 0xaa, 0xaa,
			102 + (4 * (arWidth[4] * 2)), 8, 4, 2, 0);
	}

	/*** refresh screen ***/
	SDL_RenderPresent (ascreen);
}
/*****************************************************************************/
void PreventCPUEating (void)
/*****************************************************************************/
{
	progspeed = REFRESH_PROG;
	while ((SDL_GetTicks() - looptime) < progspeed)
	{
		SDL_Delay (10);
	}
	looptime = SDL_GetTicks();
}
/*****************************************************************************/
void ExMarks (void)
/*****************************************************************************/
{
	int iX, iY;

	/*** Mode check. ***/
	iX = 9;
	switch (iMode)
	{
		case 0: iY = 9; break;
		case 1: iY = 28; break;
		case 2: iY = 47; break;
		case 3: iY = 66; break;
		case 4: iY = 85; break;
		default:
			printf ("[ WARN ] Invalid iMode value: %i.\n", iMode);
			iMode = 2; iY = 47; break; /*** Fallback. ***/
	}
	ShowImage (imgchkb, iX, iY, "imgchkb");

	/*** Zoom check. ***/
	iX = 9;
	switch (iZoom)
	{
		case 1: iY = 126; break;
		case 2: iY = 145; break;
		case 3: iY = 164; break;
		case 4: iY = 183; break;
		case 5: iY = 202; break;
		case 6: iY = 221; break;
		case 7: iY = 240; break;
		default:
			printf ("[ WARN ] Invalid iZoom value: %i.\n", iZoom);
			iZoom = 2; iY = 145; break; /*** Fallback. ***/
	}
	ShowImage (imgchkb, iX, iY, "imgchkb");

	/*** Fullscreen check. ***/
	iX = 9;
	if (iFullscreen == 0)
	{
		iY = 281;
	} else {
		iY = 300;
	}
	ShowImage (imgchkb, iX, iY, "imgchkb");
}
/*****************************************************************************/
void OpenURL (char *sURL)
/*****************************************************************************/
{
#if defined WIN32 || _WIN32 || WIN64 || _WIN64
ShellExecute (NULL, "open", sURL, NULL, NULL, SW_SHOWNORMAL);
#else
pid_t pid;
pid = fork();
if (pid == 0)
{
	execl ("/usr/bin/xdg-open", "xdg-open", sURL, (char *)NULL);
	exit (EXIT_NORMAL);
}
#endif
}
/*****************************************************************************/
void PlaySound (char *sFile)
/*****************************************************************************/
{
	int iIndex;
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	SDL_AudioCVT cvt;

	if (iNoAudio == 1) { return; }
	for (iIndex = 0; iIndex < NUM_SOUNDS; iIndex++)
	{
		if (sounds[iIndex].dpos == sounds[iIndex].dlen)
		{
			break;
		}
	}
	if (iIndex == NUM_SOUNDS) { return; }

	if (SDL_LoadWAV (sFile, &wave, &data, &dlen) == NULL)
	{
		printf ("[FAILED] Could not load %s: %s!\n", sFile, SDL_GetError());
		exit (EXIT_ERROR);
	}
	SDL_BuildAudioCVT (&cvt, wave.format, wave.channels, wave.freq, AUDIO_S16, 2,
		44100);
	/*** The "+ 1" is a workaround for SDL bug #2274. ***/
	cvt.buf = (Uint8 *)malloc (dlen * (cvt.len_mult + 1));
	memcpy (cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio (&cvt);
	SDL_FreeWAV (data);

	if (sounds[iIndex].data)
	{
		free (sounds[iIndex].data);
	}
	SDL_LockAudio();
	sounds[iIndex].data = cvt.buf;
	sounds[iIndex].dlen = cvt.len_cvt;
	sounds[iIndex].dpos = 0;
	SDL_UnlockAudio();
}
/*****************************************************************************/
void DropLoose (int iRoom, int iTile)
/*****************************************************************************/
{
	int iRoomC, iTileC;
	int iCrashed;
	char cChar;

	PlaySound ("wav/loose_wobble_1.wav");
	SDL_Delay (100);
	PlaySound ("wav/loose_wobble_2.wav");
	SDL_Delay (100);
	PlaySound ("wav/loose_wobble_3.wav");
	arTiles[iRoom][iTile] = '.';
	iCrashed = 0;
	iRoomC = iRoom; iTileC = iTile;
	do {
		if (iTileC <= 20)
		{
			iTileC+=10;
		} else if (arLinksD[iRoomC] != 0) {
			iRoomC = arLinksD[iRoomC];
			iTileC-=20;
		}
		if (IsEmpty (arTiles[iRoomC][iTileC]) == 0)
		{
			SDL_Delay (100);
			PlaySound ("wav/loose_crash.wav");
			iCrashed = 1;
			cChar = arTiles[iRoomC][iTileC];
			if (((cChar >= 'A') && (cChar <= 'R')) ||
				((cChar >= 'a') && (cChar <= 'r')))
			{
				PushButton (cChar, 1);
				arTiles[iRoomC][iTileC] = '-';
			}
			if (cChar == '_')
				{ arTiles[iRoomC][iTileC] = '-'; }
		}
	} while (iCrashed == 0);
	SDL_Delay (100);
}
/*****************************************************************************/
void GetOptionValue (char *sArgv, char *sValue)
/*****************************************************************************/
{
	int iTemp;
	char sTemp[MAX_OPTION + 2];

	iTemp = strlen (sArgv) - 1;
	snprintf (sValue, MAX_OPTION, "%s", "");
	while (sArgv[iTemp] != '=')
	{
		snprintf (sTemp, MAX_OPTION, "%c%s", sArgv[iTemp], sValue);
		snprintf (sValue, MAX_OPTION, "%s", sTemp);
		iTemp--;
	}
}
/*****************************************************************************/
void Initialize (void)
/*****************************************************************************/
{
	SDL_AudioSpec fmt;
	char sImage[MAX_IMG + 2];
	SDL_Surface *imgicon;

	if (SDL_Init (SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
	{
		printf ("[FAILED] Unable to init SDL: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	atexit (SDL_Quit);

	window = SDL_CreateWindow (PROG_NAME " " PROG_VERSION,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIDTH, WINDOW_HEIGHT, iFullscreen);
	if (window == NULL)
	{
		printf ("[FAILED] Unable to create a window: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	ascreen = SDL_CreateRenderer (window, -1, 0);
	if (ascreen == NULL)
	{
		printf ("[FAILED] Unable to set video mode: %s!\n", SDL_GetError());
		exit (EXIT_ERROR);
	}
	SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	if (iFullscreen != 0)
	{
		SDL_RenderSetLogicalSize (ascreen, WINDOW_WIDTH, WINDOW_HEIGHT);
	}

	if (TTF_Init() == -1)
	{
		printf ("[FAILED] Could not initialize TTF!\n");
		exit (EXIT_ERROR);
	}

	LoadFonts();

	curArrow = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_ARROW);
	curWait = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT);
	curHand = SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_HAND);

	if (iNoAudio != 1)
	{
		fmt.freq = 44100;
		fmt.format = AUDIO_S16;
		fmt.channels = 2;
		fmt.samples = 512;
		fmt.callback = MixAudio;
		fmt.userdata = NULL;
		if (SDL_OpenAudio (&fmt, NULL) < 0)
		{
			printf ("[FAILED] Unable to open audio: %s!\n", SDL_GetError());
			exit (EXIT_ERROR);
		}
		SDL_PauseAudio (0);
	}

	/*** icon ***/
	snprintf (sImage, MAX_IMG, "png%sPunyPrince_icon.png", SLASH);
	imgicon = IMG_Load (sImage);
	if (imgicon == NULL)
	{
		printf ("[ WARN ] Could not load \"%s\": %s.\n", sImage, strerror (errno));
	} else {
		SDL_SetWindowIcon (window, imgicon);
	}

	/*******************/
	/* Preload images. */
	/*******************/
	SDL_SetCursor (curWait);
	PreLoad ("screen_d.png", &imgscreend);
	PreLoad ("screen_g.png", &imgscreeng);
	PreLoad ("chk_black.png", &imgchkb);
	PreLoad ("8x8.png", &imgfont[0]);
	PreLoad ("8x8f.png", &imgfade[0]);
	PreLoad ("8x14.png", &imgfont[1]);
	PreLoad ("8x14f.png", &imgfade[1]);
	PreLoad ("8x16.png", &imgfont[2]);
	PreLoad ("8x16f.png", &imgfade[2]);
	PreLoad ("9x14.png", &imgfont[3]);
	PreLoad ("9x14f.png", &imgfade[3]);
	PreLoad ("9x16.png", &imgfont[4]);
	PreLoad ("9x16f.png", &imgfade[4]);
	PreLoad ("popup_yn.png", &imgpopupyn);
	PreLoad ("yesoff.png", &imgyesoff);
	PreLoad ("yeson.png", &imgyeson);
	PreLoad ("nooff.png", &imgnooff);
	PreLoad ("noon.png", &imgnoon);
	SDL_SetCursor (curArrow);
}
/*****************************************************************************/
void ShowText (int iLine, char *sText, int iR, int iG, int iB, int iInGame)
/*****************************************************************************/
{
	char cChar;
	int iStartX, iStartY;
	int iX, iY, iW, iH;

	/*** Used for looping. ***/
	int iLoopChar;

	if (((iInGame == 0) && (iLine > 17)) ||
		((iInGame == 1) && (iLine > 5)))
	{
		printf ("[ WARN ] Trying to show text on line %i!\n", iLine);
		return;
	}

	if (iInGame == 0)
	{
		iY = 8 + ((iLine - 1) * (arHeight[4] * 2));
	} else {
		iStartX = 102 + ((819 - (arWidth[iMode] * 11 * iZoom)) / 2);
		iStartY = 8 + ((560 - (arHeight[iMode] * 3 * iZoom)) / 2);
		/***/
		iY = iStartY + ((iLine - 2) * (arHeight[iMode] * iZoom));
	}
	for (iLoopChar = 0; iLoopChar < (int)strlen (sText); iLoopChar++)
	{
		cChar = sText[iLoopChar];
		switch (cChar)
		{
			/*** row 3 ***/
			case ' ': iW = 1; iH = 3; break;
			case '!': iW = 2; iH = 3; break;
			case '"': iW = 3; iH = 3; break;
			case '#': iW = 4; iH = 3; break;
			case '$': iW = 5; iH = 3; break;
			case '%': iW = 6; iH = 3; break;
			case '&': iW = 7; iH = 3; break;
			case '\'': iW = 8; iH = 3; break;
			case '(': iW = 9; iH = 3; break;
			case ')': iW = 10; iH = 3; break;
			case '*': iW = 11; iH = 3; break;
			case '+': iW = 12; iH = 3; break;
			case ',': iW = 13; iH = 3; break;
			case '-': iW = 14; iH = 3; break;
			case '.': iW = 15; iH = 3; break;
			case '/': iW = 16; iH = 3; break;

			/*** row 4 ***/
			case '0': iW = 1; iH = 4; break;
			case '1': iW = 2; iH = 4; break;
			case '2': iW = 3; iH = 4; break;
			case '3': iW = 4; iH = 4; break;
			case '4': iW = 5; iH = 4; break;
			case '5': iW = 6; iH = 4; break;
			case '6': iW = 7; iH = 4; break;
			case '7': iW = 8; iH = 4; break;
			case '8': iW = 9; iH = 4; break;
			case '9': iW = 10; iH = 4; break;
			case ':': iW = 11; iH = 4; break;
			case ';': iW = 12; iH = 4; break;
			case '<': iW = 13; iH = 4; break;
			case '=': iW = 14; iH = 4; break;
			case '>': iW = 15; iH = 4; break;
			case '?': iW = 16; iH = 4; break;

			/*** row 5 ***/
			case '@': iW = 1; iH = 5; break;
			case 'A': iW = 2; iH = 5; break;
			case 'B': iW = 3; iH = 5; break;
			case 'C': iW = 4; iH = 5; break;
			case 'D': iW = 5; iH = 5; break;
			case 'E': iW = 6; iH = 5; break;
			case 'F': iW = 7; iH = 5; break;
			case 'G': iW = 8; iH = 5; break;
			case 'H': iW = 9; iH = 5; break;
			case 'I': iW = 10; iH = 5; break;
			case 'J': iW = 11; iH = 5; break;
			case 'K': iW = 12; iH = 5; break;
			case 'L': iW = 13; iH = 5; break;
			case 'M': iW = 14; iH = 5; break;
			case 'N': iW = 15; iH = 5; break;
			case 'O': iW = 16; iH = 5; break;

			/*** row 6 ***/
			case 'P': iW = 1; iH = 6; break;
			case 'Q': iW = 2; iH = 6; break;
			case 'R': iW = 3; iH = 6; break;
			case 'S': iW = 4; iH = 6; break;
			case 'T': iW = 5; iH = 6; break;
			case 'U': iW = 6; iH = 6; break;
			case 'V': iW = 7; iH = 6; break;
			case 'W': iW = 8; iH = 6; break;
			case 'X': iW = 9; iH = 6; break;
			case 'Y': iW = 10; iH = 6; break;
			case 'Z': iW = 11; iH = 6; break;
			case '[': iW = 12; iH = 6; break;
			case '\\': iW = 13; iH = 6; break;
			case ']': iW = 14; iH = 6; break;
			case '^': iW = 15; iH = 6; break;
			case '_': iW = 16; iH = 6; break;

			/*** row 7 ***/
			case '`': iW = 1; iH = 7; break;
			case 'a': iW = 2; iH = 7; break;
			case 'b': iW = 3; iH = 7; break;
			case 'c': iW = 4; iH = 7; break;
			case 'd': iW = 5; iH = 7; break;
			case 'e': iW = 6; iH = 7; break;
			case 'f': iW = 7; iH = 7; break;
			case 'g': iW = 8; iH = 7; break;
			case 'h': iW = 9; iH = 7; break;
			case 'i': iW = 10; iH = 7; break;
			case 'j': iW = 11; iH = 7; break;
			case 'k': iW = 12; iH = 7; break;
			case 'l': iW = 13; iH = 7; break;
			case 'm': iW = 14; iH = 7; break;
			case 'n': iW = 15; iH = 7; break;
			case 'o': iW = 16; iH = 7; break;

			/*** row 8 ***/
			case 'p': iW = 1; iH = 8; break;
			case 'q': iW = 2; iH = 8; break;
			case 'r': iW = 3; iH = 8; break;
			case 's': iW = 4; iH = 8; break;
			case 't': iW = 5; iH = 8; break;
			case 'u': iW = 6; iH = 8; break;
			case 'v': iW = 7; iH = 8; break;
			case 'w': iW = 8; iH = 8; break;
			case 'x': iW = 9; iH = 8; break;
			case 'y': iW = 10; iH = 8; break;
			case 'z': iW = 11; iH = 8; break;
			case '{': iW = 12; iH = 8; break;
			case '|': iW = 13; iH = 8; break;
			case '}': iW = 14; iH = 8; break;
			case '~': iW = 15; iH = 8; break;
			/*** NO 16 ***/

			default:
				printf ("[FAILED] Unknown character '%c' in ShowText()!\n", cChar);
				exit (EXIT_ERROR);
		}
		if (iInGame == 0)
		{
			iX = 102 + (iLoopChar * (arWidth[4] * 2));
			ShowChar (iW, iH, iR, iG, iB, iX, iY, 4, 2, 0);
		} else {
			iX = iStartX + ((iLoopChar - 1) * (arWidth[iMode] * iZoom));
			ShowChar (iW, iH, iR, iG, iB, iX, iY, iMode, iZoom, 0);
		}
	}
}
/*****************************************************************************/
void MovingStarts (void)
/*****************************************************************************/
{
	iSteps++;
	if (iPrinceSword == 2) { iPrinceSword = 1; }
	switch (iPrinceHang)
	{
		case 2: iPrinceHang = 1; break;
		default: iPrinceHang = 0; break;
	}
	iShowStepsCoins = 0;
	iNoMessage = 0;
}
/*****************************************************************************/
void MovingEnds (void)
/*****************************************************************************/
{
	if (iCareful == 0) { iPrinceSafe = 0; }
	iJump = 0;
	iCareful = 0;
	iRunJump = 0;
}
/*****************************************************************************/
int GetX (int iTile)
/*****************************************************************************/
{
	/*** Currently unused. ***/

	int iUseTile;
	int iStartX, iX;

	iUseTile = iTile;
	while (iUseTile > 10) { iUseTile-=10; }

	iStartX = 102 + ((819 - (arWidth[iMode] * 11 * iZoom)) / 2);
	iX = iStartX + (iUseTile * (arWidth[iMode] * iZoom));

	return (iX);
}
/*****************************************************************************/
int GetY (int iTile)
/*****************************************************************************/
{
	/*** Currently unused. ***/

	int iStartY, iY;

	iStartY = 8 + ((560 - (arHeight[iMode] * 3 * iZoom)) / 2);
	if ((iTile >= 1) && (iTile <= 10))
	{
		iY = iStartY + (0 * (arHeight[iMode] * iZoom));
	} else if ((iTile >= 11) && (iTile <= 20)) {
		iY = iStartY + (1 * (arHeight[iMode] * iZoom));
	} else if ((iTile >= 21) && (iTile <= 30)) {
		iY = iStartY + (2 * (arHeight[iMode] * iZoom));
	} else {
		printf ("[FAILED] Invalid iTile in GetY(): %i!\n", iTile);
		exit (EXIT_ERROR);
	}

	return (iY);
}
/*****************************************************************************/
void Teleport (char cGoTo)
/*****************************************************************************/
{
	/*** Used for looping. ***/
	int iLoopRoom;
	int iLoopTile;

	for (iLoopRoom = 1; iLoopRoom <= ROOMS; iLoopRoom++)
	{
		for (iLoopTile = 1; iLoopTile <= TILES; iLoopTile++)
		{
			if ((arTiles[iLoopRoom][iLoopTile] == cGoTo) &&
				((iLoopRoom != iCurRoom) || (iLoopTile != iPrinceTile)) &&
				((iLoopRoom != GetRoomLeft()) || (iLoopTile != GetTileLeft())))
			{
				iCurRoom = iLoopRoom;
				iPrinceTile = iLoopTile;
				PlaySound ("wav/mirror.wav");
				return;
			}
		}
	}

	printf ("[ WARN ] No similar teleport ('%c') found.\n", cGoTo);
}
/*****************************************************************************/
