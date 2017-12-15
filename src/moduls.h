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
extern pthread_mutex_t file;

size_t ReadNonBlock(int fd, char* buff, size_t len,unsigned int del);
int make_nonblock(int sock);
int Login_ethernet(const char* ATS_ip, in_addr_t ATS_port);
int initTTY(unsigned char com);
int initTTY(char* comm);
int Create_server_point(in_addr_t port);
void init_Client(void);
int IP_check(const char * ipstr);
void Reinit_ATS_Connection(void);
void Reinit_Server(void);
int Open_log_file(char *file_name);
void get_time_str(char* tm_str);
void Loger(const char* str);
void Loger(const char* str1,const char* str2,int d);
void get_parameters(void);
int check_d(const char* str);
int max(int x, int y);
void Print_help(void);
void Print_error(void);
