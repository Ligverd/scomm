/***************************************************************************
                          cserversocket.h  -  description
                             -------------------
    begin                : Sun Feb 8 2004
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

#ifndef CSERVERSOCKET_H
#define CSERVERSOCKET_H

#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>


/**
  *@author root
  */

class CServerSocket {
public:
    CServerSocket();
    ~CServerSocket();
        /** No descriptions */
        int Create(char *ipaddres, int ipport);
protected:
        int fdserversocket;
};

#endif
