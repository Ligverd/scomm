
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

int ATS_fd;
CClient *Client[MAX_CLIENT];
int InfoClient_fd = -1;
CParser parser;
CPPP PPP;
//pthread_mutex_t client_mutex[MAX_CLIENT];

volatile bool fRun = true;
volatile bool link_state = false;
struct sigaction sact;

void sig_SIGPIPE_hndlr(int signo)
{
    Loger("Signal SIGPIPE received!\n");
}

void sig_SIGTERM_hndlr(int signo)
{
    Loger("Terminating process...\n");
    link_state = false;
    fRun = false;
}

void sig_SIGALRM_hndlr(int signo)
{
    PPP.ProcessTimers();
}

void *Client_ptread(void *arg)
{
    char log[100];

    pthread_detach(pthread_self());
    unsigned int i = (uintptr_t) arg;

    sprintf(log, "Client%d connected! IP:%s\n", i, Client[i]->IP);
    Loger(log);

    if (link_state)
        PPP.OpenChannel((unsigned char) i);

    struct timeval tv;
    fd_set fds;
    int retval;

    while (link_state && Client[i]->state)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(Client[i]->fd, &fds);

        if ((retval = select(Client[i]->fd + 1, &fds, NULL, NULL, &tv)) > 0)
        {
            unsigned char buff[2048];

            memset(buff, 0, 2048);
            int size = read(Client[i]->fd, buff, 2048);

            if (size > 0)       // if rcv data from client socket
            {
                Client[i]->OnReceive(buff, size);
            }
            else if ((size == 0) || ((size == -1) && (errno != EINTR)))
                break;
        }
        else if (retval == -1 && errno != EINTR)
            break;              //Esli oshibka ne sviazana s prepivanijem signalom            
    }

    if (link_state)
        PPP.CloseChannel((unsigned char) i);

    sprintf(log, "Client%d disconnected! IP:%s\n", i, Client[i]->IP);
    Loger(log);

    sleep(1);
    //pthread_mutex_lock(&client_mutex[i]);
    delete Client[i];           //close in destructor
    Client[i] = NULL;
    //pthread_mutex_unlock(&client_mutex[i]);

    return (NULL);
}

void *InfoClient_ptread(void *arg)
{
    pthread_detach(pthread_self());

    InfoClient_fd = (intptr_t) (arg);

    Loger("Info client connected!\n");

    struct timeval tv;
    fd_set fds;
    int retval;

    while (fRun)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(InfoClient_fd, &fds);
        char str[2000];

        if ((retval = select(InfoClient_fd + 1, &fds, NULL, NULL, &tv)) > 0)
        {
            unsigned char buff[512];

            memset(buff, 0, 512);
            int size = read(InfoClient_fd, buff, 512);

            if (size > 0)       // if rcv data from client socket
            {
                PPP.PrintInfo(str);
                if( MyWrite(InfoClient_fd, str, strlen(str) + 1) < 0 )
                    break;

            }
            else if ((size == 0) || ((size == -1) && (errno != EINTR)))
                break;
        }
        else if (retval == 0)
        {
            PPP.PrintInfo(str);
            if( MyWrite(InfoClient_fd, str, strlen(str) + 1) < 0 )
                break;
        }
        else if (retval == -1 && errno != EINTR)
            break;              //Esli oshibka ne sviazana s prepivanijem signalom        
    }

    Loger("Info client disconnected!\n");
    close(InfoClient_fd);
    InfoClient_fd = -1;
    return (NULL);
}

void *Server_ptread(void *arg)
{
    pthread_detach(pthread_self());
    fd_set fds;
    pthread_t Client_tid;

    struct timeval tv;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    int retval;
    bool fServer_st = false;
    char log[100];
    int Server_fd = -1;

    while (fRun)
    {
        if (link_state)
        {
            if (!fServer_st)
            {
                if ((Server_fd = Create_server_point(parser.ServerPort, "Main server")) < 0)
                {
                    TRACE("Main server point error!\n");
                    return NULL;
                }
                fServer_st = true;
            }

            tv.tv_sec = 1;
            tv.tv_usec = 0;
            FD_ZERO(&fds);
            FD_SET(Server_fd, &fds);
            if ((retval = select(Server_fd + 1, &fds, NULL, NULL, &tv)) > 0)
            {
                int client_fd =
                    accept(Server_fd, (struct sockaddr *) &client_addr,
                           &client_addr_size);
                if (client_fd < 0)
                {
                    TRACE("Client accept error!\n");
                    close(Server_fd);
                    fServer_st = false;
                }
                else
                {
                    if (!link_state)    // Esli kanal s ATS ne rabochij
                    {
                        close(client_fd);
                        sprintf(log, "Client accept denied! IP:%s\n",
                                inet_ntoa(client_addr.sin_addr));
                        Loger(log);
                    }
                    else        //Esli kanal rabochij dobavliajem novogo klienta
                    {
                        if (make_nonblock(client_fd) < 0)
                        {
                            close(client_fd);
                            sprintf(log,
                                    "Can't make client socket nonblock! IP:%s\n",
                                    inet_ntoa(client_addr.sin_addr));
                            TRACE(log);
                        }
                        else
                        {
                            int i = 0;

                            for (i = 0; i < MAX_CLIENT; i++)
                                if (Client[i] == NULL)
                                    break;
                            if (i < MAX_CLIENT)
                            {
                                Client[i] =
                                    new CClient(client_fd, i,
                                                inet_ntoa(client_addr.
                                                          sin_addr), &PPP);
                                if (Client[i])
                                {
                                    if (pthread_create
                                        (&Client_tid, NULL, &Client_ptread,
                                         (void *) i) != 0)
                                    {
                                        sprintf(log,
                                                "Can't create tread for Client%d! IP:%s\n",
                                                i, Client[i]->IP);
                                        TRACE(log);
                                        delete Client[i];       //close in destructor

                                        Client[i] = NULL;
                                    }
                                }
                                else
                                {
                                    close(client_fd);
                                    sprintf(log,
                                            "No mem for Client%d! IP:%s\n", i,
                                            inet_ntoa(client_addr.sin_addr));
                                    TRACE(log);
                                }
                            }
                            else
                                close(client_fd);
                        }
                    }
                }
            }
            else if (retval == -1 && errno != EINTR)    //Esli oshibka ne sviazana s prepivanijem signalom
            {
                TRACE("Main server internal error!\n");
                close(Server_fd);
                fServer_st = false;
            }
        }
        else
        {
            if (fServer_st)
            {
                close(Server_fd);
                fServer_st = false;
            }
#ifdef FREE_BSD
            sleep(1);
#else
            sleep(1);
#endif
        }
    }
    close(Server_fd);
    return (NULL);
}

void *InfoServer_ptread(void *arg)
{
    pthread_detach(pthread_self());
    fd_set fds;
    pthread_t InfoClient_tid;

    struct timeval tv;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    bool fInfoServer_st = false;
    int retval;
    int InfoServer_fd = -1;

    while (fRun)
    {

        if (!fInfoServer_st)
        {
            if ((InfoServer_fd =
                 Create_server_point(parser.InfoServerPort, "Info server")) < 0)
            {
                TRACE("Info server point error!\n");
                return NULL;
            }
            fInfoServer_st = true;
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(InfoServer_fd, &fds);
        if ((retval = select(InfoServer_fd + 1, &fds, NULL, NULL, &tv)) > 0)
        {
            int client_fd =
                accept(InfoServer_fd, (struct sockaddr *) &client_addr,
                       &client_addr_size);
            if (client_fd < 0)
            {
                TRACE("Info client accept error!\n");
                close(InfoServer_fd);
                fInfoServer_st = false;
            }
            else
            {
                if (InfoClient_fd == -1)
                {
                    if (make_nonblock(client_fd) < 0)
                    {
                        close(client_fd);
                        TRACE("Can't make info client socket nonblock\n");
                    }
                    else
                    {
                        if (pthread_create
                            (&InfoClient_tid, NULL, &InfoClient_ptread,
                             (void *) client_fd) != 0)
                        {
                            close(client_fd);
                            TRACE("Can't create tread for info client!\n");
                        }
                    }
                }
                else
                    close(client_fd);
            }
        }
        else if (retval == -1 && errno != EINTR)        //Esli oshibka ne sviazana s prepivanijem signalom
        {
            TRACE("Info server internal error!\n");
            close(InfoServer_fd);
            fInfoServer_st = false;
        }
    }
    close(InfoServer_fd);
    return (NULL);
}

int main(int argc, char **argv)
{
    pthread_t Server_tid, InfoServer_tid;

    init_Client();

    if (parser.ParseCStringParams(argc, argv) < 0)
    {
        exit(EXIT_FAILURE);
    }

    StrToLog("Starting scomm...\n");
    printf
        ("\n<---------------------------------------scomm_v0.8.5.11----------------------------------------->\n");

    if (parser.fComm)
    {
        printf("COMM dev:%s\n", parser.sCommDev);
    }
    else
    {
        printf("ATS IP:%s\n", parser.sAtsIp);
        printf("ATS port:%d\n", parser.AtsPort);
    }
    printf("Server port:%d\n", parser.ServerPort);
    printf("Log file:%s\n", parser.sOutDir);
    printf("Log file size:%d KBytes\n", parser.nLogFileSize);
    if (parser.fDaemon)
        printf("Daemon mode\n");
    printf("Protocol version:%d\n", parser.ProtV);

    Loger("Init is ok!\n");
    printf
        ("<----------------------------------------------------------------------------------------------->\n");

    if (parser.fDaemon)
    {
        Loger("Daemon mode on!\n");
        daemon(0, 0);
    }

    if (pthread_create(&InfoServer_tid, NULL, &InfoServer_ptread, NULL) != 0)
    {
        TRACE("Can't create info server tread!\n");
        close(ATS_fd);
        exit(EXIT_FAILURE);
    }

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

    if (pthread_create(&Server_tid, NULL, &Server_ptread, NULL) != 0)
    {
        TRACE("Can't create server tread!\n");
        close(ATS_fd);
        exit(EXIT_FAILURE);
    }

    sigfillset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = sig_SIGTERM_hndlr;
    sigaction(SIGINT, &sact, NULL);
    sigaction(SIGTERM, &sact, NULL);

    sigemptyset(&sact.sa_mask);
    sact.sa_handler = sig_SIGPIPE_hndlr;
    sigaction(SIGPIPE, &sact, NULL);

    sact.sa_handler = sig_SIGALRM_hndlr;
    sigaction(SIGALRM, &sact, NULL);

    struct itimerval real_timer;

    real_timer.it_value.tv_sec = 0;
    real_timer.it_value.tv_usec = 10000;
    real_timer.it_interval.tv_sec = 0;
    real_timer.it_interval.tv_usec = 10000;
    setitimer(ITIMER_REAL, &real_timer, NULL);

    int retval;
    struct timeval tv;
    fd_set fds;

    while (fRun)
    {
        PPP.RunTimers();

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(ATS_fd, &fds);
        if ((retval = select(ATS_fd + 1, &fds, NULL, NULL, &tv)) > 0)
        {
            unsigned char buff[2048];
            int size = read(ATS_fd, buff, 2048);

            if (size > 0)       // if rcv data from tty
            {
                PPP.m_dwReadBytes += size;
                for (int count_rcv_byte = 0; count_rcv_byte < size;
                     count_rcv_byte++)
                {
                    PPP.ReadBytesFtomATS(buff[count_rcv_byte]);
                }
            }
            else
            {
                if ((size == 0) || ((size == -1) && (errno != EINTR)))
                {
                    Loger("ATS socket closed!\n");
                    Loger("Reconnecting...\n");
                    Reinit_ATS_Connection();
                }
            }
        }
        else if (retval < 0 && errno != EINTR)  //Esli oshibka ne sviazana s prepivanijem signalom
        {
            TRACE("Internal error!\n");
            Loger("Reconnecting...\n");
            Reinit_ATS_Connection();
        }
    }
    sleep(1);
    close(ATS_fd);
    destroy_Client();
    return EXIT_SUCCESS;
}
