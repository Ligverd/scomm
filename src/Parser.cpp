
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
#include "main.h"

CParser::CParser()
{
    sAtsIp = NULL;
    sCommDev = NULL;
    sOutDir = NULL;
    AtsPort = 10000;
    ServerPort = 10001;
    InfoServerPort = 0;
    fDaemon = false;
    fComm = false;
    f9600 = false;
    ftoff = false;
    ProtV = 11;
    nLogFileSize = 20;
}

CParser::~CParser()
{
    if (sAtsIp)
        free(sAtsIp);
    if (sCommDev)
        free(sCommDev);
    if (sOutDir)
        free(sOutDir);
}

int CParser::CheckDec(const char *str)
{
    int i;

    for (i = 0; i < strlen(str); i++)
        if (str[i] > '9' || str[i] < '0')
            return 0;
    return 1;
}

int CParser::CheckIp(const char *ipstr)
{
    if (!strcmp(ipstr, "COMM"))
    {
        fComm = true;
        return 1;
    }

    if (!strcmp(ipstr, "localhost"))
    {
        return 1;
    }

    int len = strlen(ipstr);

    if (len > 6 && len < 16)
    {
        char i = 0, p = 0, dot = 0, s[len + 1];

        memset(s, 0, len + 1);
        while (i <= len)
        {
            if (i == len)
            {
                if (dot != 3)
                    return 0;
                if ((atoi(s) < 0) || (atoi(s) > 255))
                    return 0;
                break;
            }
            if (ipstr[i] == '.')
            {
                dot++;
                if ((atoi(s) < 0) || (atoi(s) > 255) || p == 0)
                    return 0;
                memset(s, 0, p);
                i++;
                p = 0;
            }
            if (ipstr[i] > '9' || ipstr[i] < '0')
                return 0;
            s[p++] = ipstr[i++];
        }
    }
    else
        return 0;
    return 1;
}


int CParser::ParseCStringParams(int argc, char *argv[])
{
    switch (argc)
    {
      case 2:
          if (!strcmp(argv[1], "-h"))
          {
              PrintHelp();
              return -1;
          }
          break;
      case 15:
      case 14:
      case 13:
      case 12:
      case 11:
      case 10:
      case 9:
      case 8:
      case 7:
      case 6:
      case 5:
      {
          int i;

          for (i = 4; i < argc; i++)
          {
              if (!strcmp(argv[i], "-outdir"))
              {
                  if (!sOutDir)
                  {
                      i++;
                      if (i < argc)
                      {
                          sOutDir =
                              (char *) malloc(strlen(argv[i]) +
                                              strlen("/scomm.log") + 1);
                          if (!sOutDir)
                          {
                              printf("Parser: Out of memory!\n");
                              return -1;
                          }
                          strcpy(sOutDir, argv[i]);
                          strcat(parser.sOutDir, "/scomm.log");
                      }
                  }
              }
              if (!strcmp(argv[i], "-logfile"))
              {
                  if (!sOutDir)
                  {
                      i++;
                      if (i < argc)
                      {
                          sOutDir = (char *) malloc(strlen(argv[i]) + 1);
                          if (!sOutDir)
                          {
                              printf("Parser: Out of memory!\n");
                              return -1;
                          }
                          strcpy(sOutDir, argv[i]);
                      }
                  }
              }
              else if (!strcmp(argv[i], "-protv"))
              {
                  i++;
                  if (i < argc)
                  {
                      if (CheckDec(argv[i]))
                      {
                          ProtV = atoi(argv[i]);
                      }
                  }
              }
              else if (!strcmp(argv[i], "-logsize"))
              {
                  i++;
                  if (i < argc)
                  {
                      if (CheckDec(argv[i]))
                      {
                          unsigned int sz = atoi(argv[i]);
                          if(sz && sz <= 100)
                            nLogFileSize = sz;
                      }
                  }
              }
              else if (!strcmp(argv[i], "-infoport"))
              {
                  i++;
                  if (i < argc)
                  {
                      if (CheckDec(argv[i]))
                      {
                          InfoServerPort = atoi(argv[i]);
                      }
                  }
              }
              else if (!strcmp(argv[i], "-d"))
              {
                  fDaemon = true;
              }
              else if (!strcmp(argv[i], "-b9600"))
              {
                  f9600 = true;
              }
              else if (!strcmp(argv[i], "-toff"))
              {
                  ftoff = true;
              }
          }
      }
      case 4:
          if (!CheckIp(argv[1]))
          {
              printf("\nATS IP address error!\n");
              PrintHelp();
              return -1;
          }

          sAtsIp = (char *) malloc(strlen(argv[1]) + 1);
          if (!sAtsIp)
          {
              printf("Parser: Out of memory!\n");
              return -1;
          }
          strcpy(sAtsIp, argv[1]);

          if (fComm)
          {
              sCommDev = (char *) malloc(strlen(argv[2]) + 1);
              if (!sCommDev)
              {
                  printf("Parser: Out of memory!\n");
                  return -1;
              }
              strcpy(sCommDev, argv[2]);
          }
          else
          {
              if (!CheckDec(argv[2]))
              {
                  printf("\nATS port error!\n");
                  PrintHelp();
                  return -1;
              }
              AtsPort = atoi(argv[2]);
          }
          if (!CheckDec(argv[3]))
          {
              printf("\nScomm server port error!\n");
              PrintHelp();
              return -1;
          }
          ServerPort = atoi(argv[3]);
          if (!sOutDir)
          {
              sOutDir = (char *) malloc(strlen("scomm.log") + 1);
              if (!sOutDir)
              {
                  printf("Parser: Out of memory!\n");
                  return -1;
              }
              strcpy(sOutDir, "scomm.log");
          }
          if (!InfoServerPort)
              InfoServerPort = ServerPort + 100;
          return 1;
    }
    printf("\nEnter correct command line parameters!\n");
    PrintHelp();
    return -1;
}

void CParser::PrintHelp(void)
{
    printf("\nNAME\n\
\tscomm - Comunication protocol with ATS M-200\n\
\nSYNOPSIS\n\
\tscomm X.X.X.X|\"COMM\" N|dev P [-outdir dir] [-logfile path] [-protv X] [-logsize Y] [-d]\n\n\
If if using eithernet connection\n\
\tX.X.X.X - ATS IP address. X = 0..255.\n\
\tN - ATS TCP port. N = 0..65535.\n\
If if using COMM port\n\
\t\"COMM\"\n\
\tdev - device file name. dev = /dev/ttySx\n\n\
\tP - scomm server TCP port. P = 0..65535\n\
\tdir - log file directory. (default dir = ./)\n\
\tpath - path for scomm logfile. (default path = dir/scomm.log)\n\
\tX - protocol version. X = 0..11 (default X = 10)\n\
\tY - logfile max size in megabytes. (default 20 MB)\n\
\t-d - daemon mode.\n\
\nEXAMPLES\n\
\tUsing COMM port in daemon mode:\n\
[MTA@Vasia /home]./scomm COMM /dev/tty0 10001 -outdir /home/Vasia -d\n\
\tUsing ethernet without logfile:\n\
[MTA@Vasia /home]./scomm 192.168.0.1 10000 10001 -logfile /dev/null\n\n");
}
