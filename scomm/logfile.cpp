/***************************************************************************
                          logfile.cpp  -  description
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


int fdlogfile;


void log_packet_tty(unsigned char *data, short size)
{

    char *rcv = "rcv:";
    char *rc = "\r\n";
    write(fdlogfile, rcv, 4);
    char buffer[20];

    write(fdlogfile, (unsigned char *)&data[2], 2);
    for (int i = 0; i < size; i++)
    {
        int size_pr = sprintf(buffer, "%x ", (int)data[i]);
        write(fdlogfile, buffer, size_pr);
    }
    write(fdlogfile, data, size);
    write(fdlogfile, rc, 2);

}

void log_tcp(unsigned char *data, short size)
{

    char *snd = "tcp:";
    char *rc = "\r\n";
    char buffer[20];

    write(fdlogfile, snd, 4);
    write(fdlogfile, (unsigned char *)&data[2], 2);

    for (int i = 0; i < size; i++)
    {
        int size_pr = sprintf(buffer, "%x ", (int)data[i]);
        write(fdlogfile, buffer, size_pr);
    }
//    write(fdlogfile, data, size);
    write(fdlogfile, rc, 2);//    fprintf(fdlogfile, "\r\n");

}

void log_write_tty(unsigned char *data, short size)
{

    char *snd = "snd:";
    char *rc = "\r\n";
    char buffer[20];

    write(fdlogfile, snd, 4);
    write(fdlogfile, (unsigned char *)&data[2], 2);

    for (int i = 0; i < size; i++)
    {
        int size_pr = sprintf(buffer, "%x ", (int)data[i]);
        write(fdlogfile, buffer, size_pr);
    }
    write(fdlogfile, data, size);
    write(fdlogfile, rc, 2);//    fprintf(fdlogfile, "\r\n");

//    write(fdlogfile,  data, len);

}


int init_log_file(char *file_name)
{
     fdlogfile = open(file_name, O_RDWR | O_CREAT, 0666);
     return  fdlogfile;
}
