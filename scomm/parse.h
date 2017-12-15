/***************************************************************************
                          parse.h  -  description
                             -------------------
    begin                : Sun Feb 15 2004
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <fcntl.h>
#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>             /* for strerror() */
#include <unistd.h>

#include "main.h"

void parse(char command);

int  do_parse(int argc, char **argv);
