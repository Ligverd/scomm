
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
#ifndef _MODULS_H
#define _MODULS_H

bool make_nonblock(int sock);

int Login_ethernet(const char *ATS_ip, in_addr_t ATS_port);

int initTTY(char *comm);

int Create_server_point(in_addr_t port, const char* name);

void init_Client(void);

void destroy_Client(void);

void Reinit_ATS_Connection(void);

bool StrToLog(const char *str);

void Loger(const char *str);

void TraceLoger(const char *str, const char *file, const int line);

int MyWrite(int fd, void* buf, unsigned int len);

#endif
