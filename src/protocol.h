
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
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define XA ('X' * 256 + 'A')
#define YA ('Y' * 256 + 'A')

#define XW ('X' * 256 + 'W')
#define YW ('Y' * 256 + 'W')

#define FA ('F' * 256 + 'A')
#define FW ('F' * 256 + 'W')

#define LENSTRBUF 5000
#define MAX_FRAME 1000
#define MAXSIZEONEMESS 320

class CPPP
{
  public:

    CPPP();
    ~CPPP();

    void OpenChannel(unsigned char ch);
    void CloseChannel(unsigned char ch);
    void ReadBytesFtomATS(unsigned char chr);
    void PutMessToBuff(unsigned char ch, unsigned char *data, short len);
    void ProcessTimers(void);
    void RunTimers(void);
    bool isOk(void)
    {
        return fOk;
    }
    void PrintInfo(char *str);

    //stat
    unsigned int m_dwReadBytes;
    unsigned int m_dwWriteBytes;
    unsigned int m_dwReadGoodFrames;
    unsigned int m_dwReadBadFrames;
    unsigned int m_dwDevReinits;
    unsigned int m_dwCounterLinkDown;

  private:

    // for work with txBuf
    unsigned short read_word(short &pos);
    void write_word(short &pos, unsigned short w);
    void add(short &pos, short del);
    void copyto(unsigned char *data, unsigned short len);
    void copy_from(unsigned char *data, unsigned short len);

    //protocol methods
    void SendBytesToATS(unsigned char *data, short size);
    void SendFrameToATS(unsigned char *data, short len);
    void ProcessPacketFromATS(unsigned char *data, short len);
    void LinkUp(void);
    void LinkDown(void);
    void OnOpenChannel(unsigned char ch);
    void OnCloseChannel(unsigned char ch);
    void ProcessMessFromATS(unsigned char *data, short len);
    void SendMessToClient(unsigned char ch, unsigned char *data, short len);
    void PutDataToBuff(unsigned char *data, short len);

    void Timer_A(void);
    void Timer_B(void);
    void Timer_C(void);
    void set_timer_a(int timer);
    void set_timer_b(int timer);
    void set_timer_c(int timer);

// Timers:
    int TimerA;
    int TimerB;
    int TimerC;

    bool RunTimerA;
    bool RunTimerB;
    bool RunTimerC;

    int tmA, tmB, tmC;

//Logic
    unsigned short nFisCode;
    bool fFisWaitAccept;

    bool fOk;
    unsigned short curInfo;
    short cTry, nWaitFE;

// data:
    unsigned char frameBuf[MAX_FRAME * 2 + 10];
    int frameLen;

    unsigned char txBuf[LENSTRBUF];
    short txHead, txTail, txSend;

    short LastCode, LastSize;

    unsigned char buffer[LENSTRBUF + 10 + 100];
};

#endif
