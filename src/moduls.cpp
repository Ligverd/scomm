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
pthread_mutex_t file = PTHREAD_MUTEX_INITIALIZER;

size_t ReadNonBlock(int fd, char* buff, size_t len,unsigned int del)
{
	int n;
	size_t l,ans_len=0;
	fd_set rset;
	struct timeval tout;
	while(len>0)
	{
		l = read(fd,buff+ans_len,len);
		if(l == -1)
		{
			if(errno != EWOULDBLOCK) return ans_len;
			tout.tv_sec = 0;
			tout.tv_usec = del*1000;
			FD_ZERO(&rset);
			FD_SET(fd,&rset);
			if((n = select(fd+1, &rset,  NULL, NULL, &tout))<1) return ans_len;
		}
		else {ans_len+=l;len-=l;}
	}
	return ans_len;
}

int make_nonblock(int sock)
{
	int sock_opt;
	if((sock_opt = fcntl(sock, F_GETFL, O_NONBLOCK)) <0) 
	{
		close(sock); 
		return -1;
	}
	if((sock_opt = fcntl(sock, F_SETFL, sock_opt | O_NONBLOCK)) <0)
	{	
		close(sock); 
		return -1;
	}
	return 0;
}

int Login_ethernet(const char* ATS_ip, in_addr_t ATS_port)
{	
	struct sockaddr_in ATS_addr;
	char buff[LOG_STR_SIZE];
	int fd,sock_opt;
	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
	{
		Loger("Can't create ATS socket!");
		return -1;
	}
	
	if(make_nonblock(fd) <0) 
	{
		close(fd); 
		Loger("Can't make ATS socket nonblock!");
		return -1;
	}
	
	sock_opt = 1;	
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&sock_opt,sizeof(sock_opt)) <0)
	{
		Loger("Can't set ATS socket option SO_REUSEADDR!");
	}
	
	ATS_addr.sin_family = AF_INET;
	ATS_addr.sin_port = htons(ATS_port);
	if( inet_aton(ATS_ip,&ATS_addr.sin_addr) <0) 
	{
		close(fd);
		Loger("ATS IP error!"); 
		return -1;
	}
	int state;
	struct timeval tout;
	tout.tv_sec = 20;
	tout.tv_usec = 0;
	fd_set wset,rset;
	FD_ZERO(&wset);
	FD_SET(fd,&wset);
	rset = wset;
	int n;
    	if (connect(fd, (struct sockaddr *) &ATS_addr, sizeof(ATS_addr))<0)
    	{
		if (errno != EINPROGRESS)
		{ 
			close(fd);
 			Loger("Connection to ATS Error!");
			return -1;
		} 
		state = 1;
        	printf("Connection to ATS in process...\r\n");
	} 
	else state = 0;
	if (state == 1) 
	{	
		n = select(fd+1, &rset, &wset, NULL, &tout);
		if((n<0) || ((n==1) && (FD_ISSET(fd, &wset)&&FD_ISSET(fd, &rset))) )
		{
			close(fd);
			Loger("Connection to ATS Error!");
			return -1;
		}
		if(n==0)
		{
			close(fd);
			Loger("Time out of connection to ATS!");
			return -1;
		}
	}	
	tout.tv_sec = 10;
	tout.tv_usec = 0;
	FD_ZERO(&rset);
	FD_SET(fd,&rset);
	if ((n = select(fd+1, &rset, NULL, NULL, &tout))<1)
	{
		close(fd);
		Loger("Can't get connection info!");
		return -1;
	}
	memset ((char*)&buff,0,LOG_STR_SIZE);//clear
	if ((n = ReadNonBlock(fd,buff,LOG_STR_SIZE,10))<1)
	{
		close(fd);
		Loger("Can't get connection info!");
		return -1;
	}
	for (n = 0;n<strlen(buff);n++) if (buff[n]=='\r')buff[n]=' ';
	char log[LOG_STR_SIZE];
	memset (log,0,LOG_STR_SIZE);
	sprintf(log,"Successfuly connected to ATS! ATS IP:%s\r\nConnection info: %s\r\n",ATS_ip,buff);
	Loger(log);
	return fd;
}

int initTTY(unsigned char comm)
{
	char tty_name [20];
	memset(tty_name,0,20);
	sprintf(tty_name,"/dev/ttyS%d",comm);
	int portfd = -1;
	portfd = open(tty_name, O_RDWR | O_NOCTTY | O_NDELAY);
	memset(tty_name,0,20);
	if (portfd > 0)
	{
		fcntl(portfd, F_SETFL, FNDELAY);
		struct termios tty;
		tcgetattr(portfd, &tty);
		cfsetospeed(&tty, B38400);
		cfsetispeed(&tty, B38400);
		cfmakeraw(&tty);
		tcsetattr(portfd, TCSANOW, &tty);
		sprintf(tty_name,"COMM%d opened!\r\n",comm);
		Loger(tty_name);
	}
	else
	{ 
		sprintf(tty_name,"Can't open COMM%d!\r\n",comm);
		Loger(tty_name);
	}
	return portfd;
}

int initTTY(char* comm)
{
	char str [strlen(comm_dev)+sizeof("Can't open !\r\n")+1];
	memset(str,0,strlen(comm_dev)+sizeof("Can't open !\r\n")+1);
	int portfd = -1;
	portfd = open(comm, O_RDWR | O_NOCTTY | O_NDELAY);
	if (portfd > 0)
	{
		fcntl(portfd, F_SETFL, FNDELAY);
		struct termios tty;
		tcgetattr(portfd, &tty);
		cfsetospeed(&tty, B38400);
		cfsetispeed(&tty, B38400);
		cfmakeraw(&tty);
		tcsetattr(portfd, TCSANOW, &tty);
		sprintf(str,"%s opened!\r\n",comm);
		Loger(str);
	}
	else
	{ 
		sprintf(str,"Can't open %s!\r\n",comm);
		Loger(str);
	}
	return portfd;
}

int Create_server_point(in_addr_t port)
{
	struct sockaddr_in server_addr;
	int fd,sock_opt;

	if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
	{
		Loger("Can't create Scomm server socket!");
		return -1;
	}
	
	if(make_nonblock(fd) <0) 
	{
		close(fd); 
		Loger("Can't make scomm server socket nonblock!");
		return -1;
	}

	sock_opt = 1;
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&sock_opt,sizeof(sock_opt)) <0)
	{
		Loger("Can't set Scomm server socket option SO_REUSEADDR!");
	}
	
	sock_opt = 1;
	if(setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,&sock_opt,sizeof(sock_opt)) <0)
	{
		Loger("Can't set Scomm server socket option SO_KEEPALIVE!");
	}
	
	sock_opt = 60;
	if(setsockopt(fd,IPPROTO_TCP,TCP_KEEPIDLE,&sock_opt,sizeof(sock_opt)) <0)
	{
		Loger("Can't set Scomm server socket option TCP_KEEPIDLE!");
	}
	
	sock_opt = 60;
	if(setsockopt(fd,IPPROTO_TCP,TCP_KEEPINTVL,&sock_opt,sizeof(sock_opt)) <0)
	{
		Loger("Can't set Scomm server socket option TCP_KEEPINTVL!");
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <0)
    	{
        	close(fd);
		char log[LOG_STR_SIZE];
		memset (log,0,LOG_STR_SIZE);
		sprintf(log,"Error binding Scomm server with port:%d\r\n",port);
		Loger(log);
		return -1;
    	}

    	if (listen(fd,MAX_CLIENT) < 0)
    	{
		close(fd);
		Loger("Listen Scomm server socket error!");
		return -1;
    	}
	char log[LOG_STR_SIZE];
	memset (log,0,LOG_STR_SIZE);
	sprintf(log,"Scomm server listening port: %d\r\n",ntohs(server_addr.sin_port));
	Loger(log);
	return fd;
}

void init_Client(void)
{
	char i;
	for(i=0; i < MAX_CLIENT;i++) Client[i] = NULL;	
}

void Reinit_ATS_Connection(void)
{
	Loger("Reiniting connection with ATS...");
	close(ATS_fd);
	unsigned short i = 0;
	sact.sa_handler = SIG_IGN;
	sigaction(SIGALRM, &sact, NULL);
	if (fcomm)
	{
		if (COMMn>=0)
		{
			while ((ATS_fd = initTTY(COMMn)) <0) 
			{
				sleep(1);
				i++;
				if (i == MAX_REINIT_TRY) sig_SIGTERM_hndlr(1);
			}
		}
		else 
		{
			while ((ATS_fd = initTTY(comm_dev)) <0) 
			{
				sleep(1);
				i++;
				if (i == MAX_REINIT_TRY) sig_SIGTERM_hndlr(1);
			}
		}
	}
	else
	{
		while ( (ATS_fd = Login_ethernet(ATS_IP,ATS_PORT)) <0) 
		{
			sleep(1);
			i++;
			if (i == MAX_REINIT_TRY) sig_SIGTERM_hndlr(1);
		}
	}
	sact.sa_handler = sig_SIGALRM_hndlr;
	sigaction(SIGALRM, &sact, NULL);
	real_timer.it_value.tv_sec = 0;
	real_timer.it_value.tv_usec = 10000;
	setitimer(ITIMER_REAL, &real_timer, NULL);

}
void Reinit_Server(void)
{
	close(Server_fd);
	Loger("Reiniting scomm server...");
	unsigned short i = 0;
	while ((Server_fd = Create_server_point(scomm_server_port)) <0) 
	{
		i++;
		scomm_server_port++;
		if (scomm_server_port == ATS_PORT) scomm_server_port++;
		if (scomm_server_port >= MAX_PORT) scomm_server_port = MIN_SCOMM_SERVER_PORT;
		if (i == MAX_REINIT_TRY) sig_SIGTERM_hndlr(1);	
	}
}

int Open_log_file(char *file_name)
{
	int fdlogfile = open(file_name, O_RDWR | O_CREAT| O_TRUNC, 0666);
	return  fdlogfile;
}

void get_time_str(char* tm_str)
{
	time_t itime;
	tm T;
	time (&itime);
	localtime_r(&itime,&T);
	sprintf(tm_str,"%02d.%02d.%04d %02d:%02d:%02d",T.tm_mday,T.tm_mon,(T.tm_year+1900),T.tm_hour,T.tm_min,T.tm_sec);
}

void Loger(const char* str)
{
	char time_str[20];
	char logstr[LOG_STR_SIZE];
	memset (time_str,0,20);
	memset (logstr,0,LOG_STR_SIZE);
	get_time_str(time_str);
	sprintf(logstr,"<%s> %s\r\n",time_str,str);
	printf("%s",logstr);
	pthread_mutex_lock(&file);
	write(logfilefd,logstr,strlen(logstr));
	pthread_mutex_unlock(&file);
}

void Loger(const char* str1,const char* str2,int d)
{
	char time_str[20];
	char logstr[LOG_STR_SIZE];
	memset (time_str,0,20);
	memset (logstr,0,LOG_STR_SIZE);
	get_time_str(time_str);
	sprintf(logstr,"<%s> Client%d %s%s\r\n",time_str,d,str1,str2);
	printf("%s",logstr);
	pthread_mutex_lock(&file);
	write(logfilefd,logstr,strlen(logstr));
	pthread_mutex_unlock(&file);
}

int check_d(const char* str)
{
	unsigned char i;
	for (i = 0;i<strlen(str);i++) if (str[i]>'9'||str[i]<'0')return -1;
	return 0;
}
void get_parameters(void)
{
		char in_str[MAX_IN_STR_LEN];
		memset(in_str,0,MAX_IN_STR_LEN);
		do
		{
			printf("Enter ATS IP address or \"COMM\":");
			scanf("%s",in_str);
			if (IP_check(in_str)<0)
			{
				printf("\r\nATS IP address error! ...(1.1|1.2)\r\n");
				Print_error();
			}
		} while (IP_check(in_str)<0);
		strcpy(ATS_IP,in_str);
		do
		{
			if (fcomm)
			{
				printf("Enter comm port number or device file name:");
				scanf("%s",in_str);
				if (check_d(in_str) < 0)
				{
					COMMn = -1;
					strcpy(comm_dev,in_str);
				}
				else if ((COMMn = atoi(in_str)) > MAX_COMM_PORT_No)
				{
					printf("\r\nComm port number error! ...(2.2)\r\n");
					Print_error();
				}
			}
			else
			{
				printf("Enter ATS port:");
				scanf("%s",in_str);
				ATS_PORT = atoi(in_str);
				if ( ATS_PORT < MIN_ATS_PORT || ATS_PORT>MAX_PORT || check_d(in_str)<0)
				{
					printf("\r\nATS port error! ...(2.1.)\r\n");
					Print_error();
				}
			}
		} while ( (ATS_PORT < MIN_ATS_PORT || ATS_PORT>MAX_PORT)&&!fcomm || COMMn>MAX_COMM_PORT_No&&fcomm );
		
		do
		{
			
			printf("Enter scomm server port:");
			scanf("%s",in_str);
			scomm_server_port = atoi(in_str);
			if (scomm_server_port<MIN_SCOMM_SERVER_PORT||scomm_server_port>MAX_PORT||check_d(in_str)<0)
			{
				printf("\r\nScomm server port error! ...(3.1.)\r\n");
				Print_error();
			}
			else if (scomm_server_port == ATS_PORT)
			{
				printf("Scomm server port can't be equal to ATS port!\r\n");
			}
		} while(scomm_server_port<MIN_SCOMM_SERVER_PORT||scomm_server_port>MAX_PORT||scomm_server_port==ATS_PORT||check_d(in_str)<0);
		
		do
		{
			
			printf("Daemon mode (y/n)?:");
			scanf("%s",in_str);
			if (strcmp(in_str,"y") != 0 && strcmp(in_str,"n") != 0)
			{
				printf("Type \"y\" or \"n\"!\r\n");
			}
			if (strcmp(in_str,"y") == 0) daem_on = true;
			
		} while (strcmp(in_str,"y") != 0 && strcmp(in_str,"n") != 0);

}

int IP_check(const char * ipstr)
{
	if (strcmp(ipstr,"COMM") == 0)
	{
		fcomm = true;
		return 1;
	}
	int len = strlen(ipstr);
	if (len>6 && len<IP_STR_LEN)
	{
		char i=0,p=0,dot=0,s[len+1];
		memset(s,0,len+1);	
		while(i<=len)
		{
			if(i==len)
			{
				if(dot!=3) return -1;
				if((atoi(s)<0) || (atoi(s)>255)) return -1;
				break;
			}
			if (ipstr[i]=='.')
			{
				dot++;
				if((atoi(s)<0) || (atoi(s)>255) || p == 0) return -1;
				memset(s,0,p);
				i++;
				p=0;
			}
			if (ipstr[i]>'9'||ipstr[i]<'0')return -1;
			s[p++]=ipstr[i++];
		}	
	}
	else return -1;
	return 1;
}
void Print_help(void)
{
	printf("\r\nNAME\r\n");
	printf("\tscomm - software realization of comunication protocol with ATS M-200\r\n");
	printf("\r\nSYNOPSIS\r\n");
	printf("\tscomm X.X.X.X|\"COMM\" N|n|dev P [-d]\r\n\r\n");
	printf("\tX.X.X.X - ATS IP address if using eithernet connection with ATS. X = 0..255.\r\n");
	printf("\t\"COMM\" - if using commport connection with ATS\r\n");
	printf("\tN - ATS TCP port, if using eithernet connection with ATS. N = 1025..65535.\r\n");
	printf("\tn - commport number, if using commport connection with ATS. n = 0..10\r\n");
	printf("\tdev - device file name, if using commport connection with ATS. dev = /dev/ttyn\r\n");
	printf("\tP - scomm server TCP port. P = 10000..65535; P != N.\r\n");
	printf("\t-d - daemon mode.\r\n");
	printf("\r\nEXAMPLES\r\n");
	printf("\tUsing commport in daemon mode:[MTA@Pavel /home]./scomm COMM /dev/tty0 20000 -d\r\n");
	printf("\tUsing ethernet:[MTA@Pavel /home]./scomm 192.168.5.46 10000 20000\r\n\r\n");
}
void Print_error(void)
{
	printf("\r\n\t1.1. ATS IP address:X.X.X.X, X = 0..255.\r\n");
	printf("\t1.2. \"COMM\" - if using commport connection with ATS.\r\n");
	printf("\t2.1. ATS TCP port:1025..65535, if using eithernet connection with ATS.\r\n");
	printf("\t2.2. Commport number:0..20, if using commport connection with ATS.\r\n");
	printf("\t2.2. Device file name:/dev/tty0..20, if using commport connection with ATS.\r\n");
	printf("\t3.1. Scomm server TCP port:10001..65535, scomm server TCP port != ATS TCP port.\r\n");
	printf("\t4.1. Daemon mode (y/n)?.\r\n");
	printf("\r\nEXAMPLE\r\n");
	printf("\tEnter ATS IP address or \"COMM\":COMM\r\n");
	printf("\tEnter comm port number or device file name:/dev/tty0\r\n");
	printf("\tEnter scomm server port:20000\r\n");
	printf("\tDaemon mode (y/n)?:n\r\n\r\n");
}
