/***************************************************************************
                          parse.cpp  -  description
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
#include "parse.h"

/* configuration */
static char *logpath;
static char *modname;
static char *configfile;
static char *pidfile;
static int intarg;
static char *chararg;

static struct poptOption optionsTable[] = {
    { "config",  'c', POPT_ARG_STRING, &configfile, 'c',
      "config file to read before parsing other options", "path" },

    { "telnet", 't', POPT_ARG_STRING, &chararg,'t',
      "telnet connectins", "YES/NO" },
    { "telnetipaddr", 'T', POPT_ARG_STRING, &chararg, 'T',
      "IP address to connect telnet", "10.0.0.1" },
    { "telnetipport", 'u', POPT_ARG_INT, &telnetipport, 'u',
      "port on address to connect telnet", "4001" },
    { "telnetlogin", 'L', POPT_ARG_STRING, &chararg, 'L',
      "login to connect telnet mashine", "login" },
    { "telnetpass", 'P', POPT_ARG_STRING, &chararg, 'P',
      "password to connect telnet mashine", "password" },

    { "port", 'p', POPT_ARG_STRING, &chararg, 'p',
      "pathname for port for current machinename", "/dev/<foo>" },
    { "ipport", 'i', POPT_ARG_INT, &ipport, 'i',
      "port number on which to talk to this device", "3000" },
    { "ipaddr", 'I', POPT_ARG_STRING, &chararg, 'I',
      "IP address to which to bind", "10.0.0.1" },
    { "logfile", 'F', POPT_ARG_STRING, &chararg, 'F',
      "Log file name", "none" },
    { "daemon", 'd', POPT_ARG_NONE, &do_daemon, 0,
      "run in background as a daemon", NULL },

      POPT_AUTOHELP
    { NULL, 0, 0, NULL, 0 }
};



void parse_config_file(char *file_name)
{
	int	conf_fd = 0;
	struct stat sb;
    char c;
	conf_fd = open(file_name, O_RDONLY);
	if (conf_fd >= 0)
    {
		poptContext optCon;
		char	*conftext;
		char	*confline;
		char	*confnext;
		int	confc;
		char	**confv;

		fstat(conf_fd, &sb);
		conftext = (char *)malloc(sb.st_size + 1);
		read(conf_fd, conftext, sb.st_size);
		close(conf_fd);
		conftext[sb.st_size] = '\0';
			
		for (confline = conftext;  confline;  confline = confnext)
        {
 		  confnext = strchr(confline, '\n');
		  if (confnext) *confnext++ = '\0';

			if (!confline[0] || confline[0] == '#')
			    continue;

			poptParseArgvString(confline, &confc, (const char ***)&confv);
			optCon = poptGetContext(NULL, confc, (const char **)confv,
						optionsTable, POPT_CONTEXT_KEEP_FIRST);
			while ((c = poptGetNextOpt(optCon)) >= 0)
		        parse(c);
			poptFreeContext(optCon);
		}
		free(conftext);
    }
}

void parse(char command)
{
    switch (command)
    {
      // Look from struct and get value
      case 'c': parse_config_file(configfile); break;
      case 't': if (!strcmp(chararg, "YES"))
                        inttelnet  = true;
                    else inttelnet  = false;
                  break; // telnet YES/NO
      case 'T': telnetipaddr = strdup(chararg);break; // telnet IP
      case 'u': break; // telnet port
      case 'L': telnet_login = strdup(chararg);break; // telnet login
      case 'P': telnet_password = strdup(chararg);break; // telnet password
      case 'p': device_name = strdup(chararg);break; // device name COM port
      case 'i':  break; // SComm IP Port
      case 'I': ipaddr = strdup(chararg); break; // SComm IP Addres
      case 'F': log_file_name = strdup(chararg); break; // Log file name
      case 'd':break; // keep and stay deamon
    }

}


int  do_parse(int argc, char **argv)
 {
	char    c;
	poptContext optCon;

 	optCon = poptGetContext(NULL, argc, (const char **)argv,
				optionsTable, 0);
	while ((c = poptGetNextOpt(optCon)) >= 0)
        {
            parse(c);
        }

	if (c < -1) {

		fprintf(stderr, "%s: %s\n",
			poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
			poptStrerror(c));
		return 1;
	}
	poptFreeContext(optCon);
       return 0;
}

