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
//#define FreeBSD 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifdef FreeBSD
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/queue.h>
#include <netinet/tcp_var.h>
#define TCP_KEEPIDLE TCPCTL_KEEPIDLE
#define TCP_KEEPINTVL TCPCTL_KEEPINTVL
#endif


//#include <netdb.h>
//#include <sys/types.h>
//#include <netinet/in.h>
//#include <sys/socket.h>

#include "protocol.h"
#include "moduls.h"
#include "Client.h"

#define MAX_CLIENT 10
#define LOG_STR_SIZE 128
#define MIN_ATS_PORT 1025
#define MIN_SCOMM_SERVER_PORT 10001
#define MAX_PORT 65535
#define MAX_IN_STR_LEN 128
#define MAX_REINIT_TRY 300

extern int ATS_fd;
extern int Server_fd;
extern int TimerA;
extern int TimerB;
extern int TimerC;
extern CClient *Client[MAX_CLIENT];
extern char ATS_IP[IP_STR_LEN];
extern in_addr_t ATS_PORT;
extern in_addr_t scomm_server_port;
extern int logfilefd;
extern struct itimerval real_timer;
extern struct sigaction sact;
extern bool daem_on;
extern bool fcomm;
extern bool Server_st;
extern char comm_dev[MAX_IN_STR_LEN];
extern char outdir[MAX_IN_STR_LEN];
void sig_SIGALRM_hndlr(int signo);
void sig_SIGTERM_hndlr(int signo);


