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

CClient::CClient(int file_description, int con)
{
	state = true;
	memset(recvBuf, 0, sizeof(recvBuf));
	recvLen = 0;
	this->con = con;
	fBinaryRead = false;
	fBinaryWrite = false;
	this->fd = file_description;
	memset (IP,0,IP_STR_LEN);
}


CClient::~CClient()
{
	close(fd);
}

void CClient::ReceivePacket(unsigned char *buf, short size)
{
	//printf("Client%d -> PC bytes:%d\n",con,size);
	WritePacket((unsigned char)con, buf, size);
}

void CClient::SendPacket(unsigned char *buff, short size)
{
	unsigned char tmp_buff[1024];
	char ip_message[50];
	bool del = false;
	if (!fBinaryWrite)//Esli klientu poka peredajotsia ne GCSP 
	{
		memmove(tmp_buff, buff, size);
        	memmove(tmp_buff + size, "\n\r", 2);
		if (!strncmp((char*)buff, "BINARYMODE-OK", 10 + 3) && size == 10 + 3) fBinaryWrite = true;
		if (!strncmp((char *)buff, "BINARYMODE-ER", 10 + 3) && size == 10 + 3)
        	{
			del = true;
			//Loger("Client","BINARYMODE-ER","",con);
        	}
    	} 
	else //Peredajom clientu dlinnu GCSP i GCSP
	{
		memmove(tmp_buff, &size, sizeof(size));
		memmove(tmp_buff + sizeof(size), buff, size);
    	}
	//otpravka
	struct timeval tv;
	fd_set wset;	
	int retval;
	do 
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&wset);
		FD_SET(fd, &wset);
    		if ((retval = select(fd + 1, NULL, &wset, NULL, &tv)) > 0) 
		{
       			int ERROR=-1;
			socklen_t opt_size = sizeof(ERROR);
			getsockopt(fd,SOL_SOCKET,SO_ERROR,&ERROR,&opt_size);
			if(ERROR == 0)
			{
				int n;
				n = write(fd, tmp_buff, size + 2);
				//printf("Client%d <- PC bytes:%d\n",con,n);
			}
			else
			{
				del = true;
				//Loger("Client","socket error!","",con);
				break;	
			}
    		}
		else if(retval == 0)//Ne vozmozhno esli rabotaet real_timer 
		{ 
			//Loger("Client","socket is not ready to write!","",con);
		}
		else if(errno != EINTR)
		{
			del = true;
			//Loger("Client","socket internal error!","",con);
			break;
		}
	} while(retval<=0);
	if (del) state = false;
}


void CClient::OnReceive( unsigned char *buff, short size )
{
	memmove(recvBuf + recvLen, buff, size);//peremestili v bufer prijema klienta recvBuf
	recvLen += size;//Uvelichili objom dannih v recvBuf
    	if (!fBinaryRead)//Esli klient peredajot poka ne v binarnom rezhime
    	{
        	int done = 0;//chislo obrabotannih dannih
        	for (int p = 0; p < recvLen && !fBinaryRead;)//poka est nebinarnije dannie v recvBuf 
        	{
            		if (recvBuf[p] == 0xA)//esli vstretili 0xA virezaem ego 
            		{
				memmove(recvBuf + p, recvBuf + p + 1, recvLen - p - 1);
                		recvLen --;//teper b recvBuf menshe na 1 bait
                		continue;//proveriajem zanovo
            		}
            		if (recvBuf[p] == 0xD)//esli vstretili 0xD peredajom stroku do nego v txBuf 
            		{
                		ReceivePacket(recvBuf + done, p - done);
				//esli poluchennaja stroka "BINARYMODE-XXXXXX" 
				//budem interpretirovat ostalnije bajti ot klianta kak binarnije
				if ( (p - done == 10+6+1) && !strncmp((char*)recvBuf + done, "BINARYMODE-", 10+1) )
				fBinaryRead = true;
                		p ++;
                		if (recvBuf[p] == 0xA) p ++;//dlia skorosti i esli srabotalo fBinaryRead = true
				done = p;//fiksiruem kolichestvo obrabotannih simvolov
			}
            		else p ++;
        	}
        	if (done < recvLen) //esli pereshli v binarnij rezhim, a v recvBuf ostalis neobrabotannije dannije 
        	{
            		memmove(recvBuf, recvBuf  + done, recvLen - done);//sdvigajem ih v nachalo recvBuf 
            		recvLen -= done;
        	}
        	else recvLen = 0;
    	}
	if (fBinaryRead)
    	{
		while (recvLen >= 2)
        	{
			unsigned short size = (unsigned short)(*recvBuf) + ((unsigned short)*(recvBuf+1)<<8);//poluchajem dlinnu GCSP
            		if (size > MAX_FRAME)//esli GCSP bolshe dlinni freima
            		{
                		recvLen = 0;
                		return;
            		}
            		if (size + 2 > recvLen) return;//Esli v recvBuf poka net zajavlennih dannih
            		ReceivePacket(recvBuf + 2, size);//kidaem GCSP v kolco txBuf 
            		if (recvLen > size + 2)
            		{
                		memmove(recvBuf, recvBuf + size + 2, recvLen - size - 2);
                		recvLen -= size + 2;
            		}
            		else recvLen = 0;
        	}
    	}
}
int CClient::Socket()
{
	return fd;
}
char* CClient::Get_ip_str()
{
	return IP;
}
void CClient::Set_ip_str(const char* ip)
{
	strcpy(IP, ip);
}
bool CClient::Get_state()
{
	return state;
}

