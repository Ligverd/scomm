/***************************************************************************
                          cserversocket.cpp  -  description
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

#include "cserversocket.h"

CServerSocket::CServerSocket() {
    fdserversocket = -1;
}

CServerSocket::~CServerSocket() {
    close(fdserversocket);
}

/** No descriptions */
int CServerSocket::Create(char *ipaddres, int ipport)
{
//    int sock = -1;
    struct sockaddr_in sn;
    struct servent *sp;

    memset(&sn, 0, sizeof(sn));
    sn.sin_family = AF_INET;
//    sp = gethostbyname(ipaddres);   //getservbyname("telnet","tcp");

//    if (sp == NULL)
 //   {
  //     printf("Error getservbyname! \r\n");
 //      return -1;
 //   }

    sn.sin_port = htons(ipport);

    if (ipaddres)
    {
         if (!inet_aton(ipaddres, &sn.sin_addr))
         {
            return -1;
         }
    }
    fdserversocket = socket(AF_INET, SOCK_STREAM, 0);

    fcntl(fdserversocket, F_SETFL, O_NONBLOCK);

    setsockopt(fdserversocket, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

   if (bind(fdserversocket, (struct sockaddr *)&sn, sizeof(sn)) < 0)
    {
        printf("Bind error!\r\n");
        return -1;
    }
    printf("Bins is ok\r\n");

    if (listen(fdserversocket, 10) < 0)
    {
       printf("Listen error!\r\n");
       return -1;
    }
    printf("Listen is ok\r\n");

    return fdserversocket;
}
