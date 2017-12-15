/***************************************************************************
                          logfile.h  -  description
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

#ifndef _LOGFILE_H
#define _LOGFILE_H

extern int fdlogfile;

void log_packet_tty(unsigned char *data, short size);

void log_write_tty(unsigned char *data, short size);

void log_tcp(unsigned char *data, short size);

int init_log_file(char *file_name);

#endif
