
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

bool make_nonblock(int sock)
{
    int sock_opt;

    if ((sock_opt = fcntl(sock, F_GETFL, O_NONBLOCK)) < 0)
    {
        close(sock);
        return false;
    }
    if ((sock_opt = fcntl(sock, F_SETFL, sock_opt | O_NONBLOCK)) < 0)
    {
        close(sock);
        return false;
    }
    return true;
}

int Login_ethernet(const char *ATS_ip, in_addr_t ATS_port)
{
    struct sockaddr_in ATS_addr;
    struct hostent *host;

    int fd, sock_opt;

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        Loger("Can't create ATS socket!\n");
        return -1;
    }

    if (!make_nonblock(fd))
    {
        close(fd);
        Loger("Can't make ATS socket nonblock!\n");
        return -1;
    }

    sock_opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt))
        < 0)
    {
        Loger("Can't set ATS socket option SO_REUSEADDR!\n");
    }

    ATS_addr.sin_family = AF_INET;
    ATS_addr.sin_port = htons(ATS_port);
    if (inet_aton(ATS_ip, &ATS_addr.sin_addr) < 0)
    {
        close(fd);
        Loger("ATS IP error!\n");
        return -1;
    }
    host = gethostbyname(ATS_ip);
    if (!host)
    {
        Loger("Error resolve server!\n");
        return -1;
    }
    memcpy(&ATS_addr.sin_addr, host->h_addr_list[0],
           sizeof(ATS_addr.sin_addr));
    int state = 0;
    struct timeval tout;

    tout.tv_sec = 20;
    tout.tv_usec = 0;
    fd_set wset, rset;

    FD_ZERO(&wset);
    FD_SET(fd, &wset);
    rset = wset;
    int n;

    if (connect(fd, (struct sockaddr *) &ATS_addr, sizeof(ATS_addr)) < 0)
    {
        if (errno != EINPROGRESS)
        {
            close(fd);
            printf("Connection to ATS Error!\n");
            return -1;
        }
        state = 1;
        printf("Connection to ATS in process...\n");
    }
    if (state)
    {
        n = select(fd + 1, &rset, &wset, NULL, &tout);
        if (n < 0 || (FD_ISSET(fd, &wset) && FD_ISSET(fd, &rset)))
        {
            close(fd);
            printf("Connection to ATS Error!\n");
            return -1;
        }
        if (n == 0)
        {
            close(fd);
            printf("Time out of connection to ATS!\n");
            return -1;
        }
    }
    char log[50];

    sprintf(log, "Successfuly connected to:%s\n", ATS_ip);
    Loger(log);
    return fd;
}

int initTTY(char *comm)
{
    char str[100];
    int portfd = -1;

    portfd = open(comm, O_RDWR | O_NOCTTY | O_NDELAY);
    if (portfd > 0)
    {
        fcntl(portfd, F_SETFL, FNDELAY);
        struct termios tty;

        tcgetattr(portfd, &tty);
        if (parser.f9600)
        {
            cfsetospeed(&tty, B9600);
            cfsetispeed(&tty, B9600);
        }
        else
        {
            cfsetospeed(&tty, B38400);
            cfsetispeed(&tty, B38400);
        }
        cfmakeraw(&tty);
        tcsetattr(portfd, TCSANOW, &tty);
        sprintf(str, "device:%s opened!\n", parser.sCommDev);
        Loger(str);
    }
    else
    {
        sprintf(str, "Can't open device:%s\n", parser.sCommDev);
        Loger(str);
    }
    return portfd;
}

int Create_server_point(in_addr_t port, const char* name)
{
    struct sockaddr_in server_addr;
    int fd, sock_opt;
    char log[150];

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        sprintf(log, "%s: Can't create server socket!\n", name);
        Loger(log);
        return -1;
    }

    if (!make_nonblock(fd))
    {
        close(fd);
        sprintf(log, "%s: Can't make server socket nonblock!\n", name);
        Loger(log);
        return -1;
    }

    sock_opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt))
        < 0)
    {
        sprintf(log, "%s: Can't set server socket option SO_REUSEADDR!\n", name);
        Loger(log);
    }

    sock_opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &sock_opt, sizeof(sock_opt))
        < 0)
    {
        sprintf(log, "%s: Can't set server socket option SO_KEEPALIVE!\n", name);
        Loger(log);
    }
//#define TCP_OPT 1
#ifdef TCP_OPT
    sock_opt = 60;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &sock_opt, sizeof(sock_opt))
        < 0)
    {
        sprintf(log, "%s: Can't set server socket option TCP_KEEPIDLE!\n", name);
        Loger(log);
    }

    sock_opt = 60;
    if (setsockopt
        (fd, IPPROTO_TCP, TCP_KEEPINTVL, &sock_opt, sizeof(sock_opt)) < 0)
    {
        sprintf(log, "%s: Can't set server socket option TCP_KEEPINTVL!\n", name);
        Loger(log);
    }
#endif

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        close(fd);

        sprintf(log, "%s: Error binding server with port:%d\n", name , port);
        Loger(log);
        return -1;
    }

    if (listen(fd, MAX_CLIENT) < 0)
    {
        close(fd);
        sprintf(log, "%s: Listen server socket error\n", name);
        Loger(log);
        return -1;
    }
    sprintf(log, "%s: server listening port:%d\n", name, ntohs(server_addr.sin_port));
    Loger(log);
    return fd;
}

void init_Client(void)
{
    char i;

    for (i = 0; i < MAX_CLIENT; i++)
    {
        Client[i] = NULL;
        //pthread_mutex_init(&client_mutex[i], NULL);
    }
}

void destroy_Client(void)
{
    char i;

    for (i = 0; i < MAX_CLIENT; i++)
    {
        //pthread_mutex_destroy(&client_mutex[i]);
    }
}

void Reinit_ATS_Connection(void)
{
    PPP.m_dwDevReinits++;
    close(ATS_fd);
    sact.sa_handler = SIG_IGN;
    sigaction(SIGALRM, &sact, NULL);
    if (parser.fComm)
    {
        while ((ATS_fd = initTTY(parser.sCommDev)) < 0)
        {
            sleep(5);
        }
    }
    else
    {
        while ((ATS_fd = Login_ethernet(parser.sAtsIp, parser.AtsPort)) < 0)
        {
            sleep(5);
        }
    }
    sact.sa_handler = sig_SIGALRM_hndlr;
    sigaction(SIGALRM, &sact, NULL);

    struct itimerval real_timer;

    real_timer.it_value.tv_sec = 0;
    real_timer.it_value.tv_usec = 10000;
    real_timer.it_interval.tv_sec = 0;
    real_timer.it_interval.tv_usec = 10000;
    setitimer(ITIMER_REAL, &real_timer, NULL);
}

void get_time_str(char *tm_str, tm T)
{
    sprintf(tm_str, "[%02d-%02d-%04d %02d:%02d:%02d]", T.tm_mday,
            T.tm_mon + 1, (T.tm_year + 1900), T.tm_hour, T.tm_min, T.tm_sec);
}

bool StrToLog(const char *str)
{
    int fd;
    bool fret = true;

    if ((fd = open(parser.sOutDir, O_RDWR | O_CREAT | O_APPEND, 0666)) < 0)
    {
        printf("Can't write to log file:%s\n", parser.sOutDir);
        return false;
    }
    if (write(fd, str, strlen(str)) < 0)
        fret = false;
    close(fd);
    return fret;
}

void Loger(const char *str)
{
    char time_str[30];
    char logstr[500];
    time_t itime;
    tm T;

    time(&itime);
    localtime_r(&itime, &T);
    printf("%s", str);
    get_time_str(time_str, T);
    sprintf(logstr, "%s %s", time_str, str);
    StrToLog(logstr);
}

void TraceLoger(const char *str, const char *file, int line)
{
    char time_str[30];
    char logstr[1000];
    time_t itime;
    tm T;

    time(&itime);
    localtime_r(&itime, &T);
    printf("%s", str);
    if (parser.ftoff == false)
    {
        get_time_str(time_str, T);
        sprintf(logstr, "%s %s line:%d %s", time_str, file, line, str);
        StrToLog(logstr);
    }
}

int MyWrite(int fd, void* buf, unsigned int len)
{
    //otpravka
    struct timeval tv;
    fd_set wset;
    int retval;
    int count = 0;

    do
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&wset);
        FD_SET(fd, &wset);
        if ((retval = select(fd + 1, NULL, &wset, NULL, &tv)) > 0)
        {
            int ERROR = -1;
            socklen_t opt_size = sizeof(ERROR);

            getsockopt(fd, SOL_SOCKET, SO_ERROR, &ERROR, &opt_size);
            if (ERROR == 0)
            {
                int n;
                n = write(fd, buf, len);
                return n;
            }
            else
            {
                return -1;
            }
        }
        else if (retval < 0 && errno != EINTR)
        {
            return -1;
        }
        count++;
    } while (retval < 0 && count < 2);

}
