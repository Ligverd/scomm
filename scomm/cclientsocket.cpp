/***************************************************************************
                          cclientsocket.cpp  -  description
                             -------------------
    begin                : Tue Feb 3 2004
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


#include "cclientsocket.h"

CClientSocket::CClientSocket(int file_description, int con)
{
   memset(recvBuf, 0, sizeof(recvBuf));
   recvLen = 0;
   ind = 0;
   this->con = con;
   fBinaryRead = false;
   fBinaryWrite = false;
   this->fd = file_description;
}


CClientSocket::~CClientSocket()
{
    close(fd);
}

void CClientSocket::ReceivePacket(unsigned char *buf, short size)
{
    WritePacket(con, buf, size);
}

void CClientSocket::SendPacket(unsigned char *buff, short size)
{
/*
    struct timeval tv;
    fd_set fds;

    tv.tv_sec = time_out / 1000;
    tv.tv_usec = (time_out % 1000) * 1000L;

    FD_ZERO(&fds);
    for (int i = 0; i < current_connect_point; i++) {
        FD_SET(connect_point[i], &fds);
    }

    int ret_val;
    ret_val = -1;
    if (select(maxfd + 1, &fds, NULL, NULL, &tv) > 0) {
        for (int i = 0; i < current_connect_point; i++) {
             if (FD_ISSET(connect_point[i], &fds) > 0) {
                return i;
            }
        }
    }
*/
    unsigned char tmp_buff[1000];
    if (!fBinaryWrite) {
        memmove(tmp_buff, buff, size);
        memmove(tmp_buff + size, "\n\r", 2);
        if (!strncmp((char *)buff, "BINARYMODE", 10) && size == 10)
          fBinaryWrite = true;
    } else {
        memmove(tmp_buff, &size, sizeof(size));
        memmove(tmp_buff + sizeof(size), buff, size);
    }

    struct timeval tv;
    fd_set fds;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    int ret_val;
    ret_val = -1;
    if (select(fd + 1, NULL, &fds, NULL, &tv) > 0) {
       if (FD_ISSET(fd, &fds) > 0) {
             write(fd, tmp_buff, size + 2);
       }
    }

/*
    if (!fBinaryWrite)
     {
          write(fd, buff, size);
          write(fd, "\n\r", 2);
           if (!strncmp((char *)buff, "BINARYMODE", 10) && size == 10)
                 fBinaryWrite = true;
     }
        else
      {
             write(fd, &size, sizeof(size));
             write(fd, buff, size);
        }
*/
}


void CClientSocket::OnReceive( unsigned char *buff, short size )
{
    memmove(recvBuf + recvLen, buff, size);
    recvLen += size;
//    log_tcp(recvBuf, recvLen);
    if (!fBinaryRead)
    {
        int done = 0;
        for (int p = 0; p < recvLen && !fBinaryRead; )
        {
            if (recvBuf[p] == 0xA)
            {
                memmove(recvBuf + p, recvBuf + p + 1, recvLen - p - 1);
                recvLen --;
                continue;
            }
            if (recvBuf[p] == 0xD)
            {
                ReceivePacket(recvBuf + done, p - done);
                if (!strncmp((char*)recvBuf + done, "BINARYMODE", 10) && p - done == 10)
                    fBinaryRead = true;
                p ++;
                if (recvBuf[p] == 0xA)
                    p ++;
                done = p;
            }
            else
                p ++;
        }
        if (done < recvLen)
        {
            memmove(recvBuf, recvBuf  + done, recvLen - done);
            recvLen -= done;
        }
        else
            recvLen = 0;
    }
    if (fBinaryRead)
    {
        while (recvLen >= 2)
        {
            unsigned short size = *(unsigned short *)recvBuf;
            if (size > MAX_FRAME)
            {
                recvLen = 0;
                return;
            }
            if (size + 2 > recvLen)
            {
                return;
            }
            ReceivePacket(recvBuf + 2, size);
            if (recvLen > size + 2)
            {
                memmove(recvBuf, recvBuf + size + 2, recvLen - size - 2);
                recvLen -= size + 2;
            }
            else
                recvLen = 0;
        }
    }
}
