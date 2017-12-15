/***************************************************************************
                          tty.cpp  -  description
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
#include "tty.h"


unsigned char buffer[LENSTRBUF * 10 + 100];
unsigned char txBuf [ LENSTRBUF];
unsigned char frameBuf[MAX_FRAME*2 + 10];
short txHead, txTail, txSend;
short LastCode;
short cTry;
int read_bad_frames;
unsigned short curInfo;
int fOk;

int frameLen;

int tmA;
int tmB;
int tmC;

unsigned short nFisCode;
int fFisWaitAccept;
int fMustReinitPort;

int link_state;

int initTTY(char *tty_name)
{
/*
    txHead = 0;
    txTail = 0;
    txSend = 0;
    LastCode = -1;
    fOk = 0;

    frameLen = 0;

    cTry = 0;

     tmA = 200;
     tmB = 300;
     tmC = 500;

    link_state = false;

    curInfo = (unsigned short) -1;

    fFisWaitAccept = false;
    fMustReinitPort = false;
*/

   int portfd = -1;
   portfd = open(tty_name, O_RDWR | O_NOCTTY | O_NDELAY);
   if (portfd > 0)
   {
       fcntl(portfd, F_SETFL, FNDELAY);
       struct termios tty;
       tcgetattr(portfd, &tty);
       cfsetospeed(&tty, B38400);
       cfsetispeed(&tty, B38400);
       cfmakeraw(&tty);
       tcsetattr(portfd, TCSANOW, &tty);
   }
   return portfd;
}


unsigned short read_word ( short &pos )
{
    unsigned short ret;
    ret = txBuf[pos ++];
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
    ret += (txBuf[pos ++]) << 8;
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
    return ret;
}

void write_word ( short& pos, unsigned short w )
{
    txBuf[pos ++] = (unsigned char)w;
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
    txBuf[pos ++] = (unsigned char)(w >> 8);
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
}

void add ( short& pos, short del )
{
    pos += del;
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
}

void copyto ( unsigned char* data, unsigned short len )

{
    if (txHead + len <= LENSTRBUF)
    {
        memcpy(txBuf + txHead, data, len);
        add(txHead, len);
    }
    else
    {
        short del = LENSTRBUF - txHead;
        memcpy(txBuf + txHead, data, del);
        add(txHead, len);
        memcpy(txBuf, data + del, txHead);
    }
}

void copy_from ( unsigned char* data, unsigned short len )
{
    if (txTail + len <= LENSTRBUF)
    {
        memcpy(data, txBuf + txTail, len);
    }
    else
    {
        short del = LENSTRBUF - txTail;
        memcpy(data, txBuf + txTail, del);
        memcpy(data + del, txBuf, len - del);
    }
}


void write_bytes_tty(unsigned char *data, short size)
{
    // !!!! WARRNING this is temoriary line
    // may be check driver to ready send bytes to tty
//    log_write_tty(data,  size);
//    write(connect_point[TTY_POINT], data, size);


    struct timeval tv;
    fd_set fds;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(connect_point[TTY_POINT], &fds);

    if (select(connect_point[TTY_POINT] + 1, NULL, &fds, NULL, &tv) > 0) {
       if (FD_ISSET(connect_point[TTY_POINT], &fds) > 0) {
          write(connect_point[TTY_POINT], data, size);
       } else
          {
             printf("TTY is locked\r\n");
          }
    }
}


void write_frame (unsigned char *data, short len)
{

    unsigned char buf[MAX_FRAME * 2 + 10];
    unsigned int sum = 0;
    unsigned char t1 = 0, t2 = 0;
    unsigned short t3 = 0;


    if (len > MAX_FRAME)
        return;

    int out = 0;
    buf[out ++] = 0xAA;
    buf[out ++] = 0x11;
    for (int in = 0; in < len; in ++)
    {
        t1 ^= data[in];
        t2 += 1 + data[in];
        t3 += (data[in] + 1) * (data[in] + 2) * (in + 3);
        if (data[in] == 0xAA)
        {
            buf[out ++] = 0xAA;
            buf[out ++] = 0x33;
        }
        else
            buf[out ++] = data[in];
    }
    sum = ((unsigned int)t1) + (((unsigned int)t2) << 8) + (((unsigned int)t3) << 16);
    sum += SCOMVERSION;
    buf[out ++] = 0xAA;
    buf[out ++] = 0x22;
    *(unsigned int*)&(buf[out]) = sum;
    out += sizeof(unsigned int);
    write_bytes_tty(buf, out);

}

void down_read_frame ( unsigned char* data, short len )
{
    unsigned short Type = ((unsigned short*)data)[0];
    unsigned short Code = ((unsigned short*)data)[1];
    switch (Type)
    {
    case FR:
        {
            if (Code == nFisCode)
                fFisWaitAccept = false;
        }
        break;
    case XR:
        {
            ((unsigned short*)buffer)[0] = YW;
            ((unsigned short*)buffer)[1] = Code;
            write_frame(buffer, 4);

            if (LastCode != Code)
            {
                LastCode = Code;
                int pos = 4;

                while (pos < len)
                {
                    unsigned short s = (*(unsigned short*)&data[pos]) - 2;
                    pos += 2;
                    if (s <= MAXSIZEONEPACKET)
                        RecvData(data + pos, s);
                    else
                    {
                        printf("WARNING\r\n");
                        return;
                    }
                    pos += s;
                }
                if (pos != len)
                    printf("WARNING\r\n");
            }
            else
            {
            }
        }
        break;
    case YR:
        {
            if (Code == txTail)
            {
                txTail = txSend;
                if (txTail != txHead)
                {
                    ((unsigned short*)buffer)[0] = XW;
                    ((unsigned short*)buffer)[1] = txTail;

                    len = txHead - txTail;
                    add(len, LENSTRBUF);
                    if (len > MAX_FRAME  - 6)
                        len = MAX_FRAME - 6;

                    short pos = txSend, s = 0, ss;
                    while (s <= len)
                    {
                        txSend = pos;
                        if (txSend == txHead)
                            break;
                        s += (ss = read_word(pos));
                        add(pos, ss - 2);
                    }
                    len = txSend - txTail;
                    add(len, LENSTRBUF);
                    if (!len)

                        printf("FATAL ERROR\r\n");

                    copy_from(buffer + 4, len);

                    write_frame(buffer, len + 4);
                    set_timer_a(tmA + len/10);
                    cTry = 0;
                }
                else
                     set_timer_a(0);
            }
        }
        break;
    }

}


void read_tty_packet (unsigned char chr)
{
//    log_packet_tty(frameBuf, frameLen);
    if (frameLen >= MAX_FRAME * 2 + 10)
       frameLen = 0;

    frameBuf[frameLen++] = chr;

    if (frameLen == 1 && !(frameBuf[0] == 0xAA))
    {
        frameLen = 0;
    }
    if (frameLen == 2 && !(frameBuf[0] == 0xAA && frameBuf[1] == 0x11))
        frameLen = 0;
/*
    if (frameLen >= 8 && (frameBuf[frameLen - 6] == 0xAA && frameBuf[frameLen - 5] == 0x22))
    {
        log_packet_tty(frameBuf, frameLen);
    }
*/
    if (frameLen >= 8 && (frameBuf[frameLen - 6] == 0xAA && frameBuf[frameLen - 5] == 0x22))
    {
        int in, out;
        for (in = 0, out = 0; in < frameLen; )
        {
            if (frameBuf[in] == 0xAA)
            {
                in ++;
                switch (frameBuf[in ++])
                {
                case 0x11:
                    out = 0;
                    break;
                case 0x22:
                    {
                        unsigned int sum1 = *(unsigned int*)&(frameBuf[in]);
                        in += 4;
                        unsigned int sum2 = 0;
                        unsigned char t1 = 0, t2 = 0;
                        unsigned short t3 = 0;
                        for (int p = 0; p < out; p ++)
                        {
                            t1 ^= frameBuf[p];
                            t2 += 1 + frameBuf[p];
                            t3 += (frameBuf[p] + 1) * (frameBuf[p] + 2) * (p + 3);
                        }
                        sum2 = ((unsigned int)t1) + (((unsigned int)t2) << 8) + (((unsigned int)t3) << 16);
                        sum2 += SCOMVERSION;
                        if (in != frameLen)
                        {
                            frameLen = 0;
                            return;
                        }
                        if (sum1 != sum2)
                        {
                            frameLen = 0;
                            read_bad_frames ++;
                            return;
                        }
                    }
                    break;
                case 0x33:  // AA
                    frameBuf[out ++] = 0xAA;
                    break;
                default:
                    frameLen = 0;
                    return;
                }
            }
            else
            {
                frameBuf[out ++] = frameBuf[in ++];
            }
        }
        frameLen = 0;

        down_read_frame(frameBuf, out);
        //upReadFrame(frameBuf, out);
    }

}


unsigned int controlsumm ( unsigned char* data, int len )
{
     unsigned char t1 = 0, t2 = 0;
     unsigned short t3 = 0;
     for (int p = 0; p < len; p ++)
     {
         t1 ^= data[p];
         t2 += 1 + data [p];
         t3 += (data [p] + 1) * (data [p] + 2) * (p + 3);
     }
     return ((unsigned int)t1) + (((unsigned int)t2) << 8) + (((unsigned int)t3) << 16);
}

void LinkUp( void )
{
  SaveToConnectLog("Link up");
  link_state = true;
}

void LinkDown( void )
{
  SaveToConnectLog("Link down");
  link_state = false;

  // close all cliens
  for (int i = 0; i < MAX_CONNECT_POINT; i++)
  {
      if (client_socket[i]) {
         delete client_socket[i];
         client_socket[i] = NULL;
         connect_point[i + TCP_CLIENT_POINT] = 0;
         current_connect_point--;
      }
  }
}

void OnOpenChannel(char ch)
{
}

void OnCloseChannel(char ch)
{
}

void RecvData ( unsigned char* data, short len )
{
    if (len < 1)
    {
        //WARNING
        printf("WARNING\r\n");
        return;
    }
    char to = *(char*)data;
    data ++;
    len --;
    if (to == -5)
    {
        // сбросим нашу сторону
        if (fOk)
        {
            fOk = 0;
            curInfo = (unsigned short) -1;
            LinkDown();
        }
    }
    else if (to == -1 && len == 2)
    {
        if (!fOk)
        {
            // сбросим все каналы у удаленной стороны
          WritePacket(-5, NULL, 0);
        }

        unsigned short info = *(unsigned short*)data;
        WritePacket(-2, (unsigned char*)&info, sizeof(info));
        curInfo = info;
    }
    else if (to == -2 && len == 2)
    {
        unsigned short info = *(unsigned short*)data;
        unsigned char id;
        id = 0;
        if (info == ((id) + (id << 8)))
        {
            if (!fOk && curInfo != (unsigned short)-1)
            {
                fOk = 1;
                LinkUp();
            }
        }
        else
        {
            if (fOk)
            {
                fOk = 0;
                curInfo = (unsigned short)-1;
                LinkDown();
            }
        }
    }
    else if (!fOk){
        return;
    }
    else if (to == -3 && len == 1) // открытие виртуального канала
    {
        char ch = (char)*data;
        if (ch >= 0 && ch < MAX_CON)
             OnOpenChannel(ch);
    }
    else if (to == -4 && len == 1) // закрытие виртуального канала
    {
        char ch = (char)*data;
        if (ch >= 0 && ch < MAX_CON)
             OnCloseChannel(ch);
    }
    else if (to >= 0)
    {
        if (to >= 0 && to < MAX_CON)
             ReadPacket(to, data, len);
    }
    else
       printf("WARNING\r\n");
       // WARNING
}

void WritePacket ( char virLink, unsigned char* data, short len )
{
    unsigned char buf[MAXSIZEONEPACKET];

    if (len + 1 > MAXSIZEONEPACKET)
    {
//        WARNING
        printf("Warning\r\n");
        return;
    }
    *(char*)buf = virLink;
    memcpy(buf + 1, data, len);
    SendData(buf, len + 1);
}

void ReadPacket ( char virLink, unsigned char* data, short len )
{

    if (virLink >= 0 && virLink < MAX_CON && client_socket[virLink])
        client_socket[virLink]->SendPacket(data, len);
}


void SendData ( unsigned char* data, short len )
{
    short use;

    len += 2;

    if ((( unsigned  short)len) > MAXSIZEONEPACKET)
    {
        printf("Warning\r\n");
//        WARNING
        return;
    }
    use = txHead - txTail;
    add(use, LENSTRBUF);
    if (use + len >= LENSTRBUF)
    {
//        WARNING
        printf("Warning\r\n");
        txHead = txTail = txSend = 0;
        return;
    }

    write_word(txHead, len);
    len -= 2;

    copyto(data, len);

    if (txTail == txSend && txTail != txHead) // free & not empty
    {
        ((unsigned short*)buffer)[0] = XW;
        ((unsigned short*)buffer)[1] = txTail;

        len = txHead - txTail;
        add(len, LENSTRBUF);
        if (len > MAX_FRAME  - 6)
            len = MAX_FRAME  - 6;

        short pos = txSend, s = 0, ss;
        while (s <= len)
        {
            txSend = pos;
            if (txSend == txHead)
                break;
            s += (ss = read_word(pos));
            add(pos, ss - 2);
        }
        len = txSend - txTail;
        add(len, LENSTRBUF);
        if (!len)   {
            printf("Warning\r\n");
            exit(0);
        }
        copy_from(buffer + 4, len);

        write_frame(buffer, len + 4);
        set_timer_a(tmA + len/10);
        cTry = 0;
    }
}
