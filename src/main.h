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
#ifndef _MAIN_H
#define _MAIN_H
//#define FREE_BSD 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <time.h>
#include <pthread.h>
#include <termios.h>

#ifdef FREE_BSD
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/queue.h>
#include <netinet/tcp_var.h>
#define TCP_KEEPIDLE TCPCTL_KEEPIDLE
#define TCP_KEEPINTVL TCPCTL_KEEPINTVL
#endif

//#include <sys/types.h>
//#include <netinet/in.h>
//#include <sys/socket.h>

#include "protocol.h"
#include "moduls.h"
#include "Client.h"
#include "Parser.h"

#define MAX_CLIENT 10

extern CParser parser;
extern int ATS_fd;
extern int Server_fd;
extern int TimerA;
extern int TimerB;
extern int TimerC;
extern CClient *Client[MAX_CLIENT];

extern int logfilefd;
extern struct itimerval real_timer;
extern struct sigaction sact;

void sig_SIGALRM_hndlr(int signo);

#endif

