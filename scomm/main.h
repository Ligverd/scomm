/***************************************************************************
                          main.h  -  description
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
#ifndef _MAIN_H
#define _MAIN_H


// define for connections
#define MAX_CONNECT_POINT 10


#define TTY_POINT 0
#define TCP_SERVER_POINT 1
#define TCP_CLIENT_POINT 2


#include "tty.h"

extern int connect_point[MAX_CONNECT_POINT];
extern int current_connect_point;
extern char IP_name[MAX_CON][20];
// configuration variable
extern  int do_daemon;   // stay deamon
extern char *device_name;  // device name for TTY
extern char *telnet_password; // password for TELNET connection
extern char *telnet_login;  // login for TELNET connection
extern int inttelnet; // TELNET YES/NO
extern int telnetipport; // TELNET IP Port
extern int ipport;  // TELNET SComm port
extern  char *telnetipaddr; // TELNET IP
extern char *ipaddr; // SComm IP
extern char *log_file_name; // Name log file

// set value of timer
void set_timer_a(int timer);
void set_timer_b(int timer);
void set_timer_c(int timer);

// handlers messgae of timer timer A is very prioryty
void timer_a(void);
void timer_b(void);
void timer_c(void);

void SaveToConnectLog(char *ip_message);
void CloseChannel(char virLink);

#endif
