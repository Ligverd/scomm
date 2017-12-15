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
int Server_fd;
CClient *Client[MAX_CLIENT];
char ATS_IP[IP_STR_LEN];
char comm_dev[MAX_IN_STR_LEN];
char outdir[MAX_IN_STR_LEN];
in_addr_t scomm_server_port;
in_addr_t ATS_PORT;

int TimerA;
int TimerB;
int TimerC;

bool RunTimerA;
bool RunTimerB;
bool RunTimerC;
bool daem_on;
bool fcomm;
bool Server_st;

struct itimerval real_timer;
struct sigaction sact;

void sig_SIGPIPE_hndlr(int signo)
{
	Loger("Signal SIGPIPE received!");
}

void sig_SIGTERM_hndlr(int signo)
{
	Loger("Terminating process...");
	if (Server_st) Server_st = false;
	if(link_state) link_state = false;
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
	pthread_detach(pthread_self());
	unsigned int i = (unsigned int)arg;
	if(link_state) OpenChannel((unsigned char)i);
	Loger("connected! IP:",Client[i]->Get_ip_str(),i);
	struct timeval tv;
	fd_set fds;
	int retval;
	while(link_state && Client[i]->Get_state())
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(Client[i]->Socket(), &fds);
		if ((retval =  select(Client[i]->Socket() + 1, &fds, NULL, NULL, &tv)) > 0)
		{
				unsigned char buff[2048];
        		memset(buff, 0, 2048);
        		int size  = read(Client[i]->Socket(), buff, 2048);
        		if (size > 0) // if rcv data from client socket
        		{
					Client[i]->OnReceive(buff, size);
        		}	
			else if ((size == 0) || ((size == -1)&&(errno != EINTR))) break;
		}
		else if (retval==-1 && errno != EINTR) break;//Esli oshibka ne sviazana s prepivanijem signalom		
	}
	Loger("disconnected! IP:",Client[i]->Get_ip_str(),i);
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
	Server_st = true;
	while(Server_st)
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(Server_fd, &fds);
		if ((retval =  select(Server_fd + 1, &fds, NULL, NULL, &tv)) > 0)
		{
			int client_fd = accept(Server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
			if (client_fd < 0)
			{	
				Loger("Client accept error!");
				Reinit_Server();
			}
			else
			{
                		if (!link_state)// Esli kanal s ATS ne rabochij
				// Esli klientskije soedinenija otbiti no v ocheredi ACCEPT est zaprosi
				{
					close(client_fd);
					Loger("Client accept denied!");
				}
				else//Esli kanal rabochij dobavliajem novogo klienta
				{
					if (make_nonblock(client_fd)<0)
					{ 
						close(client_fd);
						Loger("Can't make client socket nonblock!");
					}
					else
					{
						int i = 0;
						for (i = 0; i < MAX_CLIENT; i++) if (Client[i] == NULL) break;
						if (i < MAX_CLIENT) 
						{
							Client[i] = new CClient(client_fd,i);
							Client[i]->Set_ip_str(inet_ntoa(client_addr.sin_addr));
							if(pthread_create(&Client_tid,NULL,&Client_ptread,(void*)i)!=0)
							{
								Loger("Can't create tread for IP:",Client[i]->Get_ip_str(),i);
								delete Client[i];
							}
						}
						else close(client_fd);
					}
				}
			}
		}
		else if (retval==-1 && errno != EINTR)//Esli oshibka ne sviazana s prepivanijem signalom
		{
			Loger("Scomm server internal error!");
			Reinit_Server();
		}
	}
	close(Server_fd);
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
	fMustReinitPort = false;
	int quit = 1;
	TimerA = TimerB = TimerC = -1;
	
	RunTimerA = false;
	RunTimerB = false;
	RunTimerC = false;
	daem_on = false;
	fcomm = false;
	init_Client();
	scomm_server_port = 0;
	memset(outdir,0,MAX_IN_STR_LEN);

	if (argc == 2 )
	{
		if(strcmp(argv[1],"-h") == 0)
		{
			Print_help();
			exit(0);
		}
	}
	if (argc == 4 || argc == 5 || argc == 6)
	{
		bool ext = false;
		struct sockaddr_in test_addr;
		if(IP_check(argv[1])<0) 
		{
			printf("\r\nATS IP address error!\r\n");
			Print_help();
			ext = true;
		}
		strcpy(ATS_IP,argv[1]);

		if(fcomm)
		{
			strcpy(comm_dev,argv[2]);
		}

		else
		{
			ATS_PORT = atoi(argv[2]);
			if (ATS_PORT<MIN_ATS_PORT || ATS_PORT>MAX_PORT || check_d(argv[2])<0)
			{
				printf("\r\nATS port error!\r\n");
				Print_help();
				ext = true;
			}
		}
		scomm_server_port = atoi(argv[3]);
		if (scomm_server_port<MIN_SCOMM_SERVER_PORT || scomm_server_port>MAX_PORT || check_d(argv[3])<0)
		{
			printf("\r\nScomm server port error!\r\n");
			Print_help();
			ext = true;
		}
		else if (scomm_server_port == ATS_PORT)
		{
			printf("\r\nScomm server port can't be equal to ATS port!\r\n");
			ext = true;
		}
		if (argc == 6)
		{
			if(strcmp(argv[4],"-d") == 0)
			{
				daem_on = true;
				strcpy(outdir,argv[5]);
			}
			else
			{
				printf("\r\nDaemon mode error\r\n");
				Print_help();
				ext = true;
			}
			
		}
		if(ext) exit(1);
		if (argc == 5)
		{ 
			if(strcmp(argv[4],"-d") == 0)daem_on = true;
			else strcpy(outdir,argv[4]);
		}
	}
	else
	{
		get_parameters();
	}	
	

	//if ((logfilefd = Open_log_file(strcat(outdir,"Scommlog")))<=0) printf("Can't create logfile!\r\n");
	strcat(outdir,"Scommlog");
	StrToLog("Starting scomm...\r\n");
	printf("\r\n<---------------------------------------scomm_v0.7.1.10------------------------------------------->\r\n");
	if (fcomm)
	{
		if ((ATS_fd = initTTY(comm_dev)) <0)
			exit(1);
	}
	else
	{
		if( (ATS_fd = Login_ethernet(ATS_IP,ATS_PORT)) <0) 
			exit(1);
	}

	if( (Server_fd = Create_server_point(scomm_server_port)) <0) 
	{
		close(ATS_fd);
		exit(1);
	}
	Loger("Init is ok!");
	printf("<----------------------------------------------------------------------------------------------->\r\n");
	
	if(daem_on) {Loger("Daemon mode on!");daemon(0,0);}

	if (pthread_create(&Server_tid,NULL,&Server_ptread,NULL)!=0)
	{
		Loger("Can' create server tread!");
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
	while(quit)
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
					Loger("ATS socket closed!!!");
					Reinit_ATS_Connection();	
				}
			}	
		}
		else if (retval==-1 && errno != EINTR)//Esli oshibka ne sviazana s prepivanijem signalom
		{
			Loger("INTERNALL ERROR!!!");
			Reinit_ATS_Connection();
		}	
	}
	return EXIT_SUCCESS;
}
