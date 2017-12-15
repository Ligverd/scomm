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
#include "Parser.h"

int ATS_fd;
int Server_fd;
CClient *Client[MAX_CLIENT];
CParser parser;

int TimerA;
int TimerB;
int TimerC;

bool RunTimerA;
bool RunTimerB;
bool RunTimerC;

struct itimerval real_timer;
struct sigaction sact;

void sig_SIGPIPE_hndlr(int signo)
{
	Loger("Signal SIGPIPE received!\n");
}

void sig_SIGTERM_hndlr(int signo)
{
	Loger("Terminating process...\n");
	link_state = false;
	close(ATS_fd);
	exit(signo);
}

void sig_SIGALRM_hndlr(int signo)
{
	if (TimerA > 0)
	{
		TimerA--;
		if (TimerA == 0) RunTimerA = true;
	}
	if (TimerB > 0)
	{
		TimerB--;
		if (TimerB == 0) RunTimerB = true;
	}
	if (TimerC > 0)
	{
		TimerC--;
		if (TimerC == 0) RunTimerC = true;
	}
	real_timer.it_value.tv_sec = 0;
	real_timer.it_value.tv_usec = 10000;
	setitimer(ITIMER_REAL, &real_timer, NULL);
}

void *Client_ptread(void* arg)
{
	char log[64];
	pthread_detach(pthread_self());
	unsigned int i = (unsigned int)arg;
	if(link_state) OpenChannel((unsigned char)i);
	sprintf(log,"Client%d connected! IP:%s\n",i,Client[i]->IP);
	Loger(log);
	struct timeval tv;
	fd_set fds;
	int retval;
	while(link_state && Client[i]->state)
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(Client[i]->fd, &fds);
		if ((retval =  select(Client[i]->fd + 1, &fds, NULL, NULL, &tv)) > 0)
		{
				unsigned char buff[2048];
        		memset(buff, 0, 2048);
        		int size  = read(Client[i]->fd, buff, 2048);
        		if (size > 0) // if rcv data from client socket
        		{
					Client[i]->OnReceive(buff, size);
        		}	
			else if ((size == 0) || ((size == -1)&&(errno != EINTR))) break;
		}
		else if (retval==-1 && errno != EINTR) break;//Esli oshibka ne sviazana s prepivanijem signalom		
	}
	sprintf(log,"Client%d disconnected! IP:%s\n",i,Client[i]->IP);
	Loger(log);
	delete Client[i];
	Client[i] = NULL;
	if(link_state) CloseChannel((unsigned char)i);
	return(NULL);
}

void *Server_ptread(void* arg)
{
    pthread_detach(pthread_self());
    fd_set fds;
    pthread_t Client_tid;
    struct timeval tv;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    int retval;
    bool fServer_st = false;
    char log[75];
    while(1)
    {
        if(link_state)
        {
            if(!fServer_st)
            {
                Server_fd = Create_server_point(parser.ServerPort);
                fServer_st = true;
            }
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            FD_ZERO(&fds);
            FD_SET(Server_fd, &fds);
            if ((retval =  select(Server_fd + 1, &fds, NULL, NULL, &tv)) > 0)
            {
                int client_fd = accept(Server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
                if (client_fd < 0)
                {
                    Loger("Client accept error!\n");
                    close(Server_fd);
                    fServer_st = false;
                }
                else
                {
                    if (!link_state)// Esli kanal s ATS ne rabochij
                    {
                        close(client_fd);
                        sprintf(log,"Client accept denied! IP:%s\n",inet_ntoa(client_addr.sin_addr));
                        Loger(log);
                    }
                    else//Esli kanal rabochij dobavliajem novogo klienta
                    {
                        if (make_nonblock(client_fd)<0)
                        {
                            close(client_fd);
                            sprintf(log,"Can't make client socket nonblock! IP:%s\n", inet_ntoa(client_addr.sin_addr));
                            Loger(log);
                        }
                        else
                        {
                            int i = 0;
                            for (i = 0; i < MAX_CLIENT; i++) 
                                if (Client[i] == NULL) break;
                            if (i < MAX_CLIENT) 
                            {
                                Client[i] = new CClient(client_fd, i, inet_ntoa(client_addr.sin_addr));
                                if (Client[i])
                                {
								    if(pthread_create(&Client_tid,NULL,&Client_ptread,(void*)i)!=0)
								    {
									   sprintf(log,"Can't create tread for Client%d! IP:%s\n", i, Client[i]->IP);
									   Loger(log);
									   delete Client[i];
									   Client[i] = NULL;
								    }
							     }
							     else
							     {
								        sprintf(log,"No mem for Client%d! IP:%s\n", i, inet_ntoa(client_addr.sin_addr));
								        Loger(log);
							     }
                            }
                            else close(client_fd);
                        }
				    }
                }
            }
            else if (retval==-1 && errno != EINTR)//Esli oshibka ne sviazana s prepivanijem signalom
            {
                Loger("Scomm server internal error!\n");
                close(Server_fd);
                fServer_st = false;
            }
        }
        else 
        {
            if(fServer_st)
            {
                close( Server_fd);
                fServer_st = false;
            }
        }
    }
    return(NULL);
}

int main(int argc, char **argv)
{
	pthread_t Server_tid;
	txHead = 0;
	txTail = 0;
	txSend = 0;
	LastCode = -1;
	fOk = false;
	frameLen = 0;
	cTry = 0;
	tmA = 200;
	tmB = 300;
	tmC = 500;
	link_state = false;
	curInfo = (unsigned short) -1;
	fFisWaitAccept = false;
	TimerA = TimerB = TimerC = -1;
	
	RunTimerA = false;
	RunTimerB = false;
	RunTimerC = false;
	init_Client();

	if (parser.ParseCStringParams (argc,argv)<0)
	{
		exit(1);
	}

	StrToLog("Starting scomm...\n");
	printf("\n<---------------------------------------scomm_v0.8.1.10----------------------------------------->\n");

	if(parser.fComm)
	{
		printf("COMM dev:%s\n",parser.sCommDev);	
	}
	else
	{
		printf("ATS IP:%s\n",parser.sAtsIp);
		printf("ATS port:%d\n",parser.AtsPort);
	}
	printf("Server port:%d\n",parser.ServerPort);	
	printf("Log file:%s\n",parser.sOutDir);
	if(parser.fDaemon) printf("Daemon mode\n");
	printf("Protocol version:%d\n",parser.ProtV);
	
	if (parser.fComm)
	{
		while ((ATS_fd = initTTY(parser.sCommDev)) < 0)
		{
			sleep(5);
		}
	}
	else
	{
		while ( (ATS_fd = Login_ethernet(parser.sAtsIp,parser.AtsPort)) <0)
		{
			sleep(5);
		}
	}

	Loger("Init is ok!\n");
	printf("<----------------------------------------------------------------------------------------------->\n");
	
	if(parser.fDaemon) {Loger("Daemon mode on!\n");daemon(0,0);}

	if (pthread_create(&Server_tid,NULL,&Server_ptread,NULL)!=0)
	{
		Loger("Can't create server tread!\n");
		close(Server_fd);
		close(ATS_fd);
		exit(1);
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
	//sact.sa_flags |= SA_RESTART;
	sigaction(SIGALRM, &sact, NULL);

	set_timer_b(tmB);
	set_timer_c(tmC);
	
	real_timer.it_value.tv_sec = 0;
	real_timer.it_value.tv_usec = 10000;
	setitimer(ITIMER_REAL, &real_timer, NULL);
	int retval;
	struct timeval tv;
	fd_set fds;

	while(1)
	{
		if (RunTimerA) 
		{
			RunTimerA = false;
			Timer_A();
		}
		if (RunTimerB) 
		{
			RunTimerB = false;
			Timer_B();
		}
		if (RunTimerC) 
		{
			RunTimerC = false;
			Timer_C();
		}
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(ATS_fd, &fds);
		if ((retval =  select(ATS_fd + 1, &fds, NULL, NULL, &tv)) > 0)
		{
			unsigned char buff[2048];
			int size = read(ATS_fd, buff, 2048);
			if (size > 0) // if rcv data from tty
			{
				for (int count_rcv_byte = 0; count_rcv_byte < size; count_rcv_byte++)
				{
					Read_ATS_Packet(buff[count_rcv_byte]);
				}
			}
			else
			{
				if ((size == 0) || ((size == -1)&&(errno != EINTR)))
				{
					Loger("ATS socket closed!\n");
                    Loger("Reconnecting...\n");
					Reinit_ATS_Connection();	
				}
			}
		}
		else if (retval < 0 && errno != EINTR)//Esli oshibka ne sviazana s prepivanijem signalom
		{
			Loger("Internal error!\n");
            Loger("Reconnecting...\n");
			Reinit_ATS_Connection();
		}
	}
	return EXIT_SUCCESS;
}

