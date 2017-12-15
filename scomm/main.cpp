/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : ðÔÎ ñÎ× 30 10:30:42 MSK 2004
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


#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include "tty.h"
#include "logfile.h"
#include "cclientsocket.h"
#include "cserversocket.h"

#include "main.h"
#include "parse.h"


int connect_point[MAX_CONNECT_POINT];
int current_connect_point;
//int maxfd;
int timerA;
int timerB;
int timerC;

int runTimerA;
int runTimerB;
int runTimerC;

CClientSocket *client_socket[MAX_CONNECT_POINT];
CServerSocket *server_socket;

char IP_name[MAX_CON][20];

// configurstion variable
int do_daemon;
char *device_name;
char *telnet_password;
char *telnet_login;
char *log_file_name;
int inttelnet;
int telnetipport;
int ipport;
char *telnetipaddr;
char *ipaddr;
int fdiplog;

int max(int x, int y)
{
    return x > y ? x : y;
}



int check_data(int time_out)
{
    struct timeval tv;
    fd_set fds;
    int maxfd = 0;
    tv.tv_sec = time_out / 1000;
    tv.tv_usec = (time_out % 1000) * 1000L;

    FD_ZERO(&fds);
    for (int i = 0; i < MAX_CONNECT_POINT; i++) {
        if (connect_point[i]) {
          FD_SET(connect_point[i], &fds);
          maxfd = max(maxfd, connect_point[i]);
        }
    }

    int ret_val;
    ret_val = -1;
    if (select(maxfd + 1, &fds, NULL, NULL, &tv) > 0) {
        for (int i = 0; i < MAX_CONNECT_POINT; i++) {
             if (connect_point[i]) {
                if (FD_ISSET(connect_point[i], &fds) > 0) {
                  return i;
                }
              }
        }
    }

    return ret_val;
}

void sig_pipe(int signal)
{
    SaveToConnectLog("SIG PIPE recv");
}   


void cleanup_devices_and_exit(int signal)
{
    for (int i = 0; i < MAX_CONNECT_POINT; i++)
    {
        if (client_socket[i])
           delete client_socket[i];
    }
    delete server_socket;

    close(connect_point[TTY_POINT]);

    SaveToConnectLog("Terminated");
    close(fdlogfile);
    close(fdiplog);

    free(device_name);
    free(telnet_password);
    free(telnet_login);
    free(telnetipaddr);
    free(ipaddr);

    exit (0);
}


void timer_a(void)
{

    short len;
    if (txSend != txTail)
    {
        if (cTry >= 10)
        {
            cTry = 0;
            txHead = 0;
            txTail = 0;
            txSend = 0;
            if (fOk)
            {
                fOk = false;
                curInfo = (unsigned short)-1;
                LinkDown();
            }
            return;
        }

        ((unsigned short*)buffer)[0] = XW;
        ((unsigned short*)buffer)[1] = txTail;

        len = txSend - txTail;
        add(len, LENSTRBUF);

        copy_from(buffer + 4, len);

        write_frame(buffer, len + 4);
        set_timer_a(tmA + len/10 );
        cTry ++;
    }
}


void timer_b(void)
{
   unsigned char buf[4];
   ((unsigned short*)buf)[0] = FW;
   ((unsigned short*)buf)[1] = ++nFisCode;

   if (fFisWaitAccept)
   {
      fMustReinitPort = true;
      write_frame(buf, 4);
      fFisWaitAccept = false;
      set_timer_b(tmB);
      return;
   }

   write_frame(buf,4);
   write_frame(buf,4);
   write_frame(buf,4);
   fFisWaitAccept = true;
   set_timer_b(tmB);
}

void timer_c(void)
{
    unsigned short info = 0;
    WritePacket(-1, (unsigned char *)&info, sizeof(info));
    set_timer_c(tmC);
}
struct itimerval it;

void scomm_timer (int sig,  siginfo_t *sa, void *a){
    if (timerA > 0) {
            timerA--;
       if (timerA == 0)
        {
           runTimerA = true;
//            timer_a();
        }
    }

    if (timerB > 0) {
            timerB--;
    if (timerB == 0)
      {
            runTimerB = true;
//            timer_b();
      }
    }

    if (timerC > 0) {
            timerC--;
       if (timerC == 0)
       {
            runTimerC = true;
//            timer_c();
        }
    }
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 10000;
    setitimer(ITIMER_REAL, &it, NULL);
}

void set_timer_a(int timer)
{
    timerA = timer;
}


void set_timer_b(int timer)
{
   timerB = timer;
}

void set_timer_c(int timer)
{
   timerC = timer;
}


void OpenChannel(char virLink)
{
    unsigned char buf[10];
    *(char *)(buf + 0) = -3;
    *(char *)(buf + 1) = virLink;
     SendData (buf, 2);

}


void CloseChannel(char virLink)
{
    unsigned char buf[10];
    *(char *)(buf + 0) = -4;
    *(char *)(buf + 1) = virLink;
     SendData (buf, 2);
}

void show_scomm(int ns)
{
    write(ns, "Link is down",12);
}

int login_telnet()
{
    struct sockaddr_in addr;
//    struct in_addr inaddr;
    struct hostent *host;
 //   char *ipaddr, *ipport;
 //   unsigned short ipportno;
    unsigned char buff[500];
    int fd;

    /* If the argument can be converted to an IP, do so. If not, try
     * to look it up in DNS. */
    host = gethostbyname(telnetipaddr);

    if (!host) {
      printf("Error find server\r\n");
      exit(0);
    }

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      printf("Don't create socket!\r\n");
      exit(0);
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK)) {
      printf("Don't set NO BLOCKET\r\n");
      close(fd);
      exit(0);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(telnetipport);

    memcpy(&addr.sin_addr, host->h_addr_list[0], sizeof(addr.sin_addr));
    int state;
    fcntl(fd, F_SETFL, O_NONBLOCK);

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)))
    {
        if (errno == EINPROGRESS)
        {
          state = 1;
          printf("In process\r\n");
        } else {
          printf("Don't connect to server\r\n");
          close(fd);
          exit(0);
        }
      } else {
          state = 0;
          printf("Ok\r\n");

    }
    if (state == 1) {
      struct timeval tv;
      fd_set fds;
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      if (select(fd + 1, &fds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(fd, &fds) > 0) {
                read(fd, buff, 200);
                printf("Connect is ok\r\n");
            }
      }
    }
//    int connect_state = 0;

    write(fd,"\r\n",2);

    while (1) {
      struct timeval tv;
      fd_set fds;
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);
      memset(buff, 0, 500);
      if (select(fd + 1, &fds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(fd, &fds) > 0) {

                int size  = read(fd, buff, 500);
                if (size > 0) {
                  printf("%s\n", (char *)buff);
                  if (strstr((char *)buff, "login:"))
                  {
                      write(fd,telnet_login,strlen(telnet_login));
                      write(fd,"\r",1);
                  }
                  if (strstr((char *)buff,"Password:"))
                  {
                      write(fd,telnet_password,strlen(telnet_password));
                      write(fd,"\r",1);
                      break;
                  }
              }

          }

      }
    }

   return fd;

}

void SaveToConnectLog(char *ip_message)
{
    tm *current_time;
    time_t itime;
    char *tstr;

    if ((itime = time(NULL)) < 0)
    {
        printf("Warrning\r\n");
    }
    current_time = gmtime(&itime);
    tstr = ctime(&itime);
     char str_buffer[200];
     sprintf(str_buffer,"%02d:%02d:%02d %02d:%02d:%d %s\r\n", current_time->tm_hour, current_time->tm_min, current_time->tm_sec,
     current_time->tm_mday, current_time->tm_mon + 1, 
     current_time->tm_year + 1900, ip_message);


    if (fdiplog > 0)
      write(fdiplog, str_buffer, strlen(str_buffer));

    printf(str_buffer);
//    sprintf(now_file_name,"_%02d_%02d_%d.%s", current_time->tm_mday, current_time->tm_mon + 1, current_time->tm_year + 1900, ext_name);
}

int main(int argc, char **argv)
{
    char ip_message[50];
    ip_message[0] = 0;
    do_daemon = false;
    device_name = NULL;
    telnet_password = NULL;
    telnet_login = NULL;
    inttelnet = false;
    telnetipport = 4001;
    ipport = 10001 ;
    telnetipaddr = NULL;
    ipaddr = NULL;
    int ttyfd = -1;
    fdiplog = -1;
    log_file_name = NULL;
    
    txHead = 0;
    txTail = 0;
    txSend = 0;
    LastCode = -1;
    fOk = 0;

    frameLen = 0;

    cTry = 0;

     tmA = 200;
     tmB = 300;
     tmC = 500;

    link_state = false;

    curInfo = (unsigned short) -1;

    fFisWaitAccept = false;
    fMustReinitPort = false;

    do_parse(argc, argv);

     if (ipaddr == NULL)
     {
        printf("SComm IP absent, use localhost\r\n");
        ipaddr = strdup("127.0.0.1");
     }
     if (log_file_name != NULL)
     {
        fdiplog = open(log_file_name, O_RDWR | O_CREAT | O_APPEND, S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR);
     }

     if (inttelnet)
     {
        if (telnet_password == NULL)
        {
            printf("Password absent\r\n");
            exit(0);
        }
        if (telnet_login == NULL)
        {
            printf("Login absent\r\n");
            exit(0);
        }

        if (telnetipaddr  == NULL)
        {
            printf("IP addres telnet absent\r\n");
            exit(0);
        }
        ttyfd = login_telnet();
     } else
     {
         if (device_name == NULL)
         {
             printf("TTY port absent, use /dev/ttyS0\r\n");
             device_name = strdup("/dev/ttyS0");
         }
          ttyfd = initTTY(device_name); // init COM port server
     }

    if (ttyfd <= 0)
    {
       printf("Error open tty device! \r\n");
       exit(0);
    }

    int quit = 1;
    current_connect_point = 0;
    struct sigaction sact;
    timerA = timerB = timerC = -1;
    runTimerA = false;
    runTimerB = false;
    runTimerC = false;

    server_socket = new CServerSocket();
    if (server_socket == NULL)
     {
           printf("No mem for create server socket!\r\n");
     }

    if (ttyfd <= 0)
    {
       printf("Error open tty device! \r\n");
       exit(0);
    }
    int socketfd = server_socket->Create(ipaddr, ipport); // init TCP server

    if (socketfd <= 0)
    {
        printf("Error open tcp socket! \r\n");
        exit(0);
    }

    if (do_daemon)
        daemon(0,0);

    sact.sa_handler = cleanup_devices_and_exit;
    sigaction(SIGINT, &sact, NULL);
    sigaction(SIGTERM, &sact, NULL);

    sact.sa_handler = sig_pipe;
    sigaction(SIGPIPE, &sact, NULL);

    sigemptyset(&sact.sa_mask);
    sact.sa_sigaction = scomm_timer;
    sact.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &sact, NULL);

    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 10000;
    setitimer(ITIMER_REAL, &it, NULL);

    connect_point[TTY_POINT] = ttyfd;
    connect_point[TCP_SERVER_POINT] = socketfd;
    current_connect_point = TCP_CLIENT_POINT;

    printf("Init is ok!\r\n");


    set_timer_c(tmC);

    while(quit)
    {

        if (runTimerA) {
           runTimerA = false;
           timer_a();
        }
        if (runTimerB) {
           runTimerB = false;
           timer_b();
        }
        if (runTimerC) {
           runTimerC = false;
           timer_c();
        }

        int check = check_data(2000);

        if (check >= 0)
        {
            if (check == TTY_POINT) // mesage from com port
            {
             unsigned char buff[2000];
//          memset(buff, 0, sizeof(buff));

            int size = read(connect_point[TTY_POINT], buff, 2000);

                if (size > 0) // if rcv data from tty
                {
                    for (int count_rcv_byte = 0; count_rcv_byte < size; count_rcv_byte++)
                    {
                        read_tty_packet(buff[count_rcv_byte]);
                    }
                }
            }

            if (check == TCP_SERVER_POINT) // message from server
            {
                struct sockaddr_in sn;
                socklen_t foo = sizeof(sn);
                int ns = accept(socketfd, (struct sockaddr *)&sn, &foo);
                if (ns < 0)
                {
                  printf("Error accept!\r\n");
                  return -1;
                }

                // Add new socket to array sockets
                if (!link_state) // link dont'stay
                {
                     show_scomm(ns);
                     close(ns);
                }
                 else
                    {
                      if (fcntl(ns, F_SETFL, O_NONBLOCK)) {
                        printf("Error to set nonblocket\r\n");
                    }

                    int j = 0;
                    for (j = 0; j < MAX_CON; j++)
                       if (!client_socket[j])
                            break;
                    if (j < MAX_CON) {
                      client_socket[j] = new CClientSocket(ns,j);
                      connect_point[TCP_CLIENT_POINT + j] = ns;
                      current_connect_point++;
                      OpenChannel((char)j);
                      strcpy(IP_name[j], inet_ntoa(sn.sin_addr));           
                      sprintf(ip_message, "open IP = %s", IP_name[j]);
                      SaveToConnectLog(ip_message); 
                    }
                }
            }
            if (check >= TCP_CLIENT_POINT) // message from client
            {
                    unsigned char buff[2000];
                    memset(buff, 0, sizeof(buff));
                    int size  = read(connect_point[check], buff, 2000);
                    if (size > 0) // if rcv data from client socket
                    {
                        //log_tcp(buff, size);
                        client_socket[check - TCP_CLIENT_POINT]->OnReceive(buff, size);
                    } else {
                        if (size == 0) {
                          //printf("Socket close\r\n");
                          delete client_socket[check - TCP_CLIENT_POINT];
                          client_socket[check - TCP_CLIENT_POINT] = NULL;
                          connect_point[check] = 0;
                          current_connect_point--;
                          CloseChannel(check - TCP_CLIENT_POINT);
                          sprintf(ip_message, "close IP = %s", IP_name[check - TCP_CLIENT_POINT]);
                          SaveToConnectLog(ip_message);
                        }
                    }
           }
        }
    }

    return EXIT_SUCCESS;

}
