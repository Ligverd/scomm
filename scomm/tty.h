/***************************************************************************
                          tty.h  -  description
                             -------------------
    begin                : Fri Jan 30 2004
    copyright            : (C) 2004 by maksim
    email                : maksim@m-200.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>


#include "logfile.h"
#include "cclientsocket.h"
#include "main.h"


 // define for SCOMM
#define SCOMVERSION 5
#define XR ('X' * 256 + 'A')
#define YR ('Y' * 256 + 'A')

#define XW ('X' * 256 + 'W')
#define YW ('Y' * 256 + 'W')

#define FR ('F' * 256 + 'A')
#define FW ('F' * 256 + 'W')

#define LENSTRBUF 5000

#define MAX_CON   12

// define for tty client
#define MAX_FRAME 1000
#define MAXSIZEONEPACKET 220


extern short txHead, txTail, txSend;
extern short LastCode;
extern short cTry;
extern int read_bad_frames;
extern unsigned short curInfo;
extern int fOk;

extern unsigned short nFisCode;
extern int fFisWaitAccept;
extern int fMustReinitPort;

extern int tmA;
extern int tmB;
extern int tmC;

extern unsigned char buffer[LENSTRBUF * 10 + 100];

extern int link_state;
extern int frameLen;

int initTTY(char *tty_name);

unsigned short read_word ( short &pos );

void write_word ( short& pos, unsigned short w );

void add ( short& pos, short del );

void copyto ( unsigned char* data, unsigned short len );

void copy_from ( unsigned char* data, unsigned short len );

void write_bytes_tty(unsigned char *data, short size);

void write_frame (unsigned char *data, short len);

void down_read_frame ( unsigned char* data, short len );

void read_tty_packet (unsigned char chr);

unsigned int controlsumm ( unsigned char* data, int len );

void RecvData ( unsigned char* data, short len );

void LinkUp( void );

void LinkDown(void);

void OnOpenChannel(char ch);

void OnCloseChannel(char ch);

void WritePacket ( char virLink, unsigned char* data, short len );

void SendData ( unsigned char* data, short len );

void ReadPacket(char virLink, unsigned char *data, short len);
