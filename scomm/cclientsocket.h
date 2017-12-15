/***************************************************************************
                          cclientsocket.h  -  description
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

#ifndef CCLIENTSOCKET_H
#define CCLIENTSOCKET_H


#include <stdlib.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>

//#include <netinet/in.h>
//#include <sys/socket.h>

#include "tty.h"
#include "logfile.h"

/**
  *@author root
  */

class CClientSocket {
public:
       CClientSocket(int file_description, int con);
       ~CClientSocket();

       void OnReceive( unsigned char *buff, short size );
          void SendPacket(unsigned char *buff, short size);
       void ReceivePacket(unsigned char *buff, short size);
protected:
       int fBinaryRead;
       int fBinaryWrite;
       unsigned char recvBuf[10000];
       int ind;
       int con;
       int fd;
          int recvLen;
};

extern CClientSocket *client_socket[MAX_CONNECT_POINT];

#endif
