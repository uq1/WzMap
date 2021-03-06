/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include "bytebool.h"

#if defined(WIN32) || defined(WIN64)
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)     // truncate from double to float

#pragma check_stack(off)

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#if defined(WIN32) || defined(WIN64)

#pragma intrinsic( memset, memcpy )

#endif

void Sys_PrintHeading( char *heading );
void Sys_PrintHeadingVerbose( char *heading );

void DoProgress( char label[], int step, int total, qboolean verbose );
void printLabelledProgress (char *label, double current, double max);
void printLabelledProgressVerbose (char *label, double current, double max);
void printProgress (double percentage);
void printProgressVerbose (double percentage);
void printDetailedProgress (double percentage);
void printDetailedProgressVerbose (double percentage);

char *va( const char *format, ... );

#define	MAX_OS_PATH		1024
#define MEM_BLOCKSIZE 4096

// the dec offsetof macro doesnt work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)

#define SAFE_MALLOC
#ifdef SAFE_MALLOC
void *safe_malloc_f( size_t size );
void safe_malloc_logstart();
void *safe_malloc_log(size_t size, const char *file, int line);
void safe_malloc_logend();
void *safe_malloc_info( size_t size, char* info );

extern qboolean generateforest;
extern qboolean generatecity;
extern qboolean FORCED_MODEL_META;

// VorteX: memlog.txt writing
extern qboolean memlog;

#define safe_malloc(s) safe_malloc_log(s, __FILE__, __LINE__);
#else
#define safe_malloc(a) malloc(a)
#endif /* SAFE_MALLOC */

// set these before calling CheckParm
extern int myargc;
extern char **myargv;

char *strlower (char *in);
int Q_strncasecmp( const char *s1, const char *s2, size_t n );
int Q_stricmp( const char *s1, const char *s2 );
void Q_getwd( char *out );

int Q_filelength (FILE *f);
int	FileTime( const char *path );

void	Q_mkdir( const char *path );

extern	char		qdir[MAX_OS_PATH];
extern	char		gamedir[MAX_OS_PATH];
extern  char		writedir[MAX_OS_PATH];
extern  char    *moddirparam;
void SetQdirFromPath( const char *path);
char *ExpandArg( const char *path );	// from cmd line
char *ExpandPath( const char *path );	// from scripts
char *ExpandGamePath (const char *path);
char *ExpandPathAndArchive( const char *path );
void ExpandWildcards( int *argc, char ***argv );


double I_FloatTime( void );

void	Error( const char *error, ... );
int		CheckParm( const char *check );

FILE	*SafeOpenWrite( const char *filename );
FILE	*SafeOpenRead( const char *filename );
void	SafeRead (FILE *f, void *buffer, int count);
void	SafeWrite (FILE *f, const void *buffer, int count);

int		LoadFile( const char *filename, void **bufferptr );
int   LoadFileBlock( const char *filename, void **bufferptr );
int		TryLoadFile( const char *filename, void **bufferptr );
void	SaveFile( const char *filename, const void *buffer, int count );
qboolean	FileExists( const char *filename );

void 	DefaultExtension( char *path, const char *extension );
void 	DefaultPath( char *path, const char *basepath );
void 	StripFilename( char *path );
void 	StripExtension( char *path );

void 	ExtractFilePath( const char *path, char *dest );
void 	ExtractFileBase( const char *path, char *dest );
void	ExtractFileExtension( const char *path, char *dest );

int 	ParseNum (const char *str);

short	BigShort (short l);
short	LittleShort (short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);


char *COM_Parse (char *data);

extern	char		com_token[1024];
extern	qboolean	com_eof;

char *copystring(const char *s);

unsigned short CRC_Block(const unsigned char *data, size_t size);
unsigned short CRC_Block_CaseInsensitive(const unsigned char *data, size_t size);

void	CreatePath( const char *path );
void	QCopyFile( const char *from, const char *to );

extern	qboolean		archive;
extern	char			archivedir[MAX_OS_PATH];

float mix(float x, float y, float a);
float sign(float x);

// sleep for the given amount of milliseconds
void Sys_Sleep(int n);

// for compression routines
typedef struct
{
	void	*data;
	int		count, width, height;
} cblock_t;


enum concol
{
	black=0,
	dark_blue=1,
	dark_green=2,
	dark_aqua,dark_cyan=3,
	dark_red=4,
	dark_purple=5,dark_pink=5,dark_magenta=5,
	dark_yellow=6,
	dark_white=7,
	gray=8,grey=8,
	blue=9,
	green=10,
	aqua=11,cyan=11,
	red=12,
	purple=13,pink=13,magenta=13,
	yellow=14,
	white=15
};

//extern inline void setcolor(concol textcol,concol backcol);
extern inline void setcolor(int textcol,int backcol);

#endif
