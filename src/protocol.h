/***************************************************************************
 *   Copyright (C) 2007 by PAX   *
 *   pax@m-200.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// define for SCOMM
#define SCOMVERSION 10

#define XA ('X' * 256 + 'A')
#define YA ('Y' * 256 + 'A')

#define XW ('X' * 256 + 'W')
#define YW ('Y' * 256 + 'W')

#define FA ('F' * 256 + 'A')
#define FW ('F' * 256 + 'W')

#define LENSTRBUF 5000

// define for ATS client
#define MAX_FRAME 1000
#define MAXSIZEONEMESS 220

extern pthread_mutex_t resourse;
extern bool fFisWaitAccept;
extern bool fMustReinitPort;
extern bool link_state;
extern bool fOk;

extern short txHead, txTail, txSend;
extern short LastCode;
extern short cTry;
extern int read_bad_frames;
extern unsigned short curInfo;

extern unsigned short nFisCode;

extern int tmA;
extern int tmB;
extern int tmC;

extern unsigned char buffer[LENSTRBUF * 10 + 100];
extern int frameLen;

unsigned short read_word (short &pos);
void write_word (short& pos, unsigned short w);
void add (short& pos, short del );
void copyto (unsigned char* data, unsigned short len);
void copy_from (unsigned char* data, unsigned short len);
void Write_bytes_to_ATS(unsigned char *data, short size);
void write_frame (unsigned char *data, short len);
void down_read_frame (unsigned char* data, short len);
void Read_ATS_Packet (unsigned char chr);
unsigned int controlsumm (unsigned char* data, int len);
void RecvData (unsigned char* data, short len);
void LinkUp(void);
void Close_all_clients(void);
void LinkDown(void);
void OnOpenChannel(char ch);
void OnCloseChannel(char ch);
void WritePacket (unsigned char ch, unsigned char* data, short len);
void SendData (unsigned char* data, short len);
void ReadPacket(unsigned char ch, unsigned char *data, short len);
void OpenChannel(unsigned char ch);
void CloseChannel(unsigned char ch);
void set_timer_a(int timer);
void set_timer_b(int timer);
void set_timer_c(int timer);
void Timer_A(void);
void Timer_B(void);
void Timer_C(void);
