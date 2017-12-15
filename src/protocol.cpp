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
pthread_mutex_t resourse = PTHREAD_MUTEX_INITIALIZER;
unsigned char buffer[LENSTRBUF * 10 + 100];
unsigned char txBuf [ LENSTRBUF];
unsigned char frameBuf[MAX_FRAME*2 + 10];
short txHead, txTail, txSend;
short LastCode;
short cTry;
unsigned short curInfo;

int frameLen;
int read_bad_frames;

int tmA;
int tmB;
int tmC;

unsigned short nFisCode;

bool fOk;
bool fFisWaitAccept;
bool fMustReinitPort;
bool link_state;
bool setcom9600 = false;

unsigned short read_word ( short &pos )
{
	unsigned short ret;
	ret = txBuf[pos ++];
	if (pos >= LENSTRBUF) pos -= LENSTRBUF;
	ret += (txBuf[pos ++]) << 8;
    	if (pos >= LENSTRBUF) pos -= LENSTRBUF;
    	return ret;
}

void write_word ( short& pos, unsigned short w )
{
	txBuf[pos ++] = (unsigned char)w;
	if (pos >= LENSTRBUF) pos -= LENSTRBUF;
	txBuf[pos ++] = (unsigned char)(w >> 8);
	if (pos >= LENSTRBUF) pos -= LENSTRBUF;
}

void add ( short& pos, short del )
{
	pos += del;
	if (pos >= LENSTRBUF) pos -= LENSTRBUF;
}

void copyto(unsigned char* data, unsigned short len)
{
	if (txHead + len <= LENSTRBUF)
	{
		memcpy(txBuf + txHead, data, len);
		add(txHead, len);
	}
	else
	{
		short del = LENSTRBUF - txHead;
		memcpy(txBuf + txHead, data, del);
		add(txHead, len);
		memcpy(txBuf, data + del, txHead);
	}
}

void copy_from(unsigned char* data, unsigned short len)
{
	if (txTail + len <= LENSTRBUF)
	{
		memcpy(data, txBuf + txTail, len);
	}
	else
	{
		short del = LENSTRBUF - txTail;
		memcpy(data, txBuf + txTail, del);
		memcpy(data + del, txBuf, len - del);
	}
}

void Write_bytes_to_ATS(unsigned char *data, short size)
{

    	struct timeval tv;
    	fd_set wset;
	int retval;
	do
	{
		tv.tv_sec = 1;
    		tv.tv_usec = 0;
		FD_ZERO(&wset);
    		FD_SET(ATS_fd, &wset);
    		if ((retval = select(ATS_fd + 1, NULL, &wset, NULL, &tv)) > 0)//vsio ravno prerviomsia signalom
		{
			if (fcomm)
			{
				int n = write(ATS_fd, data, size);
				if (n>0) printf("PC -> ATS bytes:%d\n",n);
				else
				{
					Loger("COMM PORT ERROR!!!");
					Reinit_ATS_Connection();
				}
			}
			else
			{
				int ERROR = -1;
				socklen_t opt_size = sizeof(ERROR);
				getsockopt(ATS_fd,SOL_SOCKET,SO_ERROR,&ERROR,&opt_size); 
				if(ERROR == 0)
				{
					int n = write(ATS_fd, data, size);
					printf("PC -> ATS bytes:%d\n",n);
				}
				else
				{
					Loger("ATS SOCKET ERROR!!!");
					Reinit_ATS_Connection();
				}
			}
    		}
		else if(retval == 0)//Ne vozmozhno esli rabotaet real_timer 
		{
			Loger("ATS socket is not ready to write!");
		}
		else if (errno != EINTR)
		{
			Loger("ATS socket internal error!!!");
			break;
		}
	} while (retval <=0);
}
void write_frame(unsigned char *data, short len)
{
	unsigned char buf[MAX_FRAME * 2 + 10];
    	unsigned int sum = 0;
    	unsigned char t1 = 0, t2 = 0;
    	unsigned short t3 = 0;
    	if (len > MAX_FRAME) return;

    	int out = 0;
    	buf[out ++] = 0xAA;
    	buf[out ++] = 0x11;
    	for (int in = 0; in < len; in ++)
    	{
        	t1 ^= data[in];
        	t2 += 1 + data[in];
        	t3 += (data[in] + 1) * (data[in] + 2) * (in + 3);
        	if (data[in] == 0xAA)
        	{
            		buf[out ++] = 0xAA;
            		buf[out ++] = 0x33;
        	}
        	else buf[out ++] = data[in];
    	}
    	sum = ((unsigned int)t1) + (((unsigned int)t2) << 8) + (((unsigned int)t3) << 16);
    	sum += SCOMVERSION;
    	buf[out ++] = 0xAA;
    	buf[out ++] = 0x22;
	buf[out ++] = (unsigned char)sum;
	buf[out ++] = (unsigned char)(sum >> 8);
	buf[out ++] = (unsigned char)(sum >> 16);
	buf[out ++] = (unsigned char)(sum >> 24);
   	Write_bytes_to_ATS(buf, out);

}

void down_read_frame( unsigned char* data, short len )
{
	unsigned short Type = (unsigned short)*data + ((unsigned short)*(data+1)<<8);
    	unsigned short Code = (unsigned short)*(data+2) + ((unsigned short)*(data+3)<<8);
	switch (Type)
    	{
    		case FA://ack of connection test
        	{
            		if (Code == nFisCode)fFisWaitAccept = false;
        	}
        	break;
    		case XA://message
        	{
			buffer[0] = (unsigned char)YW;
            		buffer[1] = (unsigned char)(YW >> 8);
			buffer[2] = (unsigned char)Code;
            		buffer[3] = (unsigned char)(Code >> 8);
			write_frame(buffer, 4);
            		if (LastCode != Code)//Esli paket dejstvitelno novij
            		{
                		LastCode = Code;
                		int pos = 4;

                		while (pos < len)
                		{
					unsigned short s = (unsigned short)*(data+pos) + ((unsigned short)*(data+pos+1)<<8) - 2;
					pos += 2;
                    			if (s <= MAXSIZEONEMESS) RecvData(data + pos, s); //partim paket na mesagi
                    			else
                    			{
                        			printf("WARNING declared message lengh is greater than availible!\r\n");
                        			return;
                    			}
                    			pos += s;
                		}
                		if (pos != len)
                    		printf("WARNING possible error of message lengh declaration!\r\n");
            		}
            		else //Esli povtorno prishol paket kotorij mi uzhe uspeshno obrabotali
            		{
            		}
        	}
        	break;
    		case YA://ACK of message
        	{
			pthread_mutex_lock(&resourse);
            		if (Code == txTail)//Esli prishlo podtverzhdenije na poslednij otpravlennij paket
            		{//mi ne shliom paketov s uprezhdenijem
                		txTail = txSend;//smeschajemsia v kolce txBuf
                		if (txTail != txHead)
                		{
					*buffer = (unsigned char)XW;
            				*(buffer+1) = (unsigned char)(XW >> 8);
					*(buffer+2) = (unsigned char)txTail;
            				*(buffer+3) = (unsigned char)(txTail >> 8);

                    			len = txHead - txTail;//opredelim dopustimuju dlinnu paketa
                    			add(len, LENSTRBUF);//Uchtiom zakolcovannoct txBuf dlia dlinni otpravliaemogo freima
					if (len > MAX_FRAME  - 6) len = MAX_FRAME - 6;
					short pos = txSend, s = 0, ss;
                    			while (s <= len)//nabiraem messagi v freim chtob v niom nebilo fragmentov messag
                    			{
                        			txSend = pos;//smeshaem ukuzatel otpravlennogo
                        			if (txSend == txHead) break; // esli novoe uvelichenije tochno dast bolshe
                        			s += (ss = read_word(pos));//kopim dlinnu
                        			add(pos, ss - 2);//smeshiajem ukazatel na dlinnu mesagi
                    			}
                    			len = txSend - txTail; 
                    			add(len, LENSTRBUF);//istinnaja dlinna
                    			if (!len)
		    			{
						printf("FATAL ERROR!!!");
						exit(1);
		    			}
                    			copy_from(buffer + 4, len);//copiruem iz txBuf nakoplennoe
                    			write_frame(buffer, len + 4);
                    			set_timer_a(tmA + len/10);
                    			cTry = 0;
                		}
                		else set_timer_a(0);//Paket ne podtverdilsia i mi zapuskajem ego povtornuju otpravku
            		}
			pthread_mutex_unlock(&resourse);
        	}
        	break;
	}

}


void Read_ATS_Packet(unsigned char chr)
{
	if (frameLen >= MAX_FRAME * 2 + 10) frameLen = 0;//esli previsili MAX_FRAME * 2 + 10 copim snachala 
	
	frameBuf[frameLen++] = chr;//ne kopitsia esli ne vstretili priznak nachala mod.paketa

	if (frameLen == 1 && !(frameBuf[0] == 0xAA)) frameLen = 0; // esli ne vstretili priznak vozmozhnogo nachala mod.paketa

	if (frameLen == 2 && !(frameBuf[0] == 0xAA && frameBuf[1] == 0x11)) frameLen = 0;// esli ne vstretili priznak nachala mod.paketa

	if (frameLen >= 8 && (frameBuf[frameLen - 6] == 0xAA && frameBuf[frameLen - 5] == 0x22))//esli smogli nakopit paket
    	{
		int in, out;
        	for (in = 0, out = 0; in < frameLen; )
        	{
            		if (frameBuf[in] == 0xAA)
            		{
                		in ++;//Analizirujem sledujushij za 0xAA bait
                		switch (frameBuf[in ++])
                		{
                			case 0x11://Nachalo mod.paketa
                    			out = 0;
                    			break;
                			case 0x22://Konec mod.paketa - proveriajem Controlnuju Summu
                    			{
						unsigned int sum1 = (unsigned int)*(frameBuf+in) + ((unsigned int)*(frameBuf+in+1)<<8)+
						((unsigned int)*(frameBuf+in+2)<<16) + ((unsigned int)*(frameBuf+in+3)<<24);
						in += 4;
                        			unsigned int sum2 = 0;
                        			unsigned char t1 = 0, t2 = 0;
                        			unsigned short t3 = 0;
                        			for (int p = 0; p < out; p ++)
                        			{
                            				t1 ^= frameBuf[p];
                            				t2 += 1 + frameBuf[p];
                            				t3 += (frameBuf[p] + 1) * (frameBuf[p] + 2) * (p + 3);
                        			}
                        			sum2 = ((unsigned int)t1) + (((unsigned int)t2) << 8) + (((unsigned int)t3) << 16);
                        			sum2 += SCOMVERSION;
                        			if (in != frameLen)//ignoriruem paket esli v freime kontrolnaja summa ne na meste
                        			{
                            				frameLen = 0;
                            				return;
                        			}
                        			if (sum1 != sum2)
                        			{
                            				frameLen = 0;
                            				read_bad_frames++;
                            				return;
                        			}
						else printf("PC <- ATS bytes:%d\r\n",frameLen);
                    			}
                    			break;
                			case 0x33:  //Esli v mod.pakete vstretilos 0xAA
                    			frameBuf[out ++] = 0xAA;
                    			break;
                			default:
                    			frameLen = 0;
                    			return;
                		}
            		}
            		else frameBuf[out ++] = frameBuf[in ++];
        	}
        	frameLen = 0;
		down_read_frame(frameBuf, out);
	}

}

void LinkUp( void )
{
	Loger("LinkUp!");
	link_state = true;
}

void LinkDown(void)
{
	Loger("LinkDown!");
	link_state = false;//dlia otboja klientov ozhidajushih soedinenija
	if(setcom9600)
		Reinit_ATS_Connection();
}

void OnOpenChannel(unsigned char ch)
{
}

void OnCloseChannel(unsigned char ch)
{
}

void RecvData (unsigned char* data, short len)
{
	if (len < 1)
	{
		//WARNING
		printf("WARNING message lengh error\r\n");
		return;
	}
	unsigned char to = *data;// beriom pervij simvol mesagi
	data ++;
	len --;
	if (to == 0xFB)// esli FB
	{
		if (fOk)//Esli do etogo kanal bil rabochij to teper kanal so stanciej ne rabochij
		{
			fOk = false;
			curInfo = (unsigned short) -1;
			LinkDown();//otbivaem klientskije soedinenija
		}
	}
	else if (to == 0xFF && len == 2) //esli FF
	{
		if (!fOk)//Esli kanal ne rabochij 
		{
			WritePacket(0xFB, NULL, 0);//shliom FB
		}
		//a zatem FE c s poluchennim <n><n>
		WritePacket(0xFE, data, sizeof(unsigned short)); 
		curInfo = (unsigned short)(*data) + ((unsigned short)*(data+1)<<8);
	}
	else if (to == 0xFE && len == 2) //esli FE
	{
		unsigned short info = (unsigned short)*data + ((unsigned short)*(data+1)<<8);//schitivaem parametr <k><k>
		unsigned char id;
		id = 0;
		if (info == ((id) + (id << 8)))// esli sovpalo s PC
		{
			
			if (!fOk && curInfo != (unsigned short)-1)
			{
				fOk = true;
				LinkUp();
			}
		}
		else if (fOk)
		{
			fOk = false;
			curInfo = (unsigned short)-1;
			LinkDown();
		}
	}
	else if (!fOk) return;
	else if (to == 0xFD && len == 1) // etu hren ne dolzhna slat stancija
	{
		unsigned char ch = *data;
		if (ch < MAX_CLIENT) OnOpenChannel(ch);
	}
	else if (to == 0xFC && len == 1) // etu hren ne dolzhna slat stancija
	{
		unsigned char ch = *data;
		if (ch < MAX_CLIENT) OnCloseChannel(ch);
	}
	else if (to == 0xF9) // esli mp
	{
		if (fcomm)
		{
			struct termios tty;
			if (tcgetattr(ATS_fd, &tty) < 0) Loger("Can't get COMM info to set 9600!");
			else
			{
				cfsetospeed(&tty, B9600);
				cfsetispeed(&tty, B9600);
				cfmakeraw(&tty);
				if (tcsetattr(ATS_fd, TCSANOW, &tty) < 0) Loger("Can't set COMM 9600!");
				else
				{ 
					setcom9600 = true;
					Loger("COMM set to 9600!");
				}
			}
		}
		else Loger("Can't set COMM 9600 - scomm conected to ATS via ethernet!!!");
	}
	else if (to < MAX_CLIENT)//otdajem klientu
	{
		ReadPacket(to, data, len);
	}
	else
	{
		Loger("DATA ERROR!!!");
	}
}

void WritePacket (unsigned char ch, unsigned char* data, short len )
{
	unsigned char buf[MAXSIZEONEMESS];
	if (len + 1 > MAXSIZEONEMESS)
	{
        	Loger("WARNING paket is to big!\r\n");
        	return;
	}
	*buf = ch;
	memcpy(buf + 1, data, len);
	SendData(buf, len + 1);
}

void ReadPacket (unsigned char ch, unsigned char* data, short len)
{

	if ((Client[ch]!=NULL) && ch < MAX_CLIENT) Client[ch]->SendPacket(data, len);
}


void SendData ( unsigned char* data, short len )
{
	short used;
	len += 2;
	if ((( unsigned  short)len) > MAXSIZEONEMESS)
    	{
        	printf("Warning message lengh error!\r\n");
        	return;
    	}
	pthread_mutex_lock(&resourse);
    	used = txHead - txTail;
    	add(used, LENSTRBUF);
    	if (used + len >= LENSTRBUF)//perekritije dannih
    	{
		printf("Warning BUFFER OWERFLOW!!!");
        	txHead = txTail = txSend = 0;
        	return;
		pthread_mutex_unlock(&resourse);
    	}
	write_word(txHead, len);//zapisali dlinnu v kolco
	len -= 2;
	copyto(data, len);//perenesli mesagu v kolco
	
	if (txTail == txSend && txTail != txHead) // esli vsio podtverdilos i est chto otpravit otpravliajem
    	{
		*buffer = (unsigned char)XW;
            	*(buffer+1) = (unsigned char)(XW >> 8);
		*(buffer+2) = (unsigned char)txTail;
            	*(buffer+3) = (unsigned char)(txTail >> 8);
		len = txHead - txTail;
        	add(len, LENSTRBUF);
        	if (len > MAX_FRAME  - 6) len = MAX_FRAME  - 6;
		short pos = txSend, s = 0, ss;
        	while (s <= len)
        	{
            		txSend = pos;
            		if (txSend == txHead) break;
            		s += (ss = read_word(pos));
            		add(pos, ss - 2);
		}
        	len = txSend - txTail;
        	add(len, LENSTRBUF);
        	if (!len) 
		{
			printf("FATAL ERROR!!!");
			exit(1);
        	}
		copy_from(buffer + 4, len);

        	write_frame(buffer, len + 4);
		set_timer_a(tmA + len/10);
        	cTry = 0;
    	}
	pthread_mutex_unlock(&resourse);
}

void OpenChannel(unsigned char ch)
{
	unsigned char buf[10];
	*buf = 0xFD;
	*(buf + 1) = ch;
	SendData (buf, 2);
}

void CloseChannel(unsigned char ch)
{
	unsigned char buf[10];
	*buf = 0xFC;
	*(buf + 1) = ch;
	SendData (buf, 2);
}

void set_timer_a(int timer)
{
	TimerA = timer;
}


void set_timer_b(int timer)
{
	TimerB = timer;
}

void set_timer_c(int timer)
{
	TimerC = timer;
}

void Timer_A(void)
{
	short len;
	pthread_mutex_lock(&resourse);
    	if (txSend != txTail)
    	{
        	if (cTry >= 10)
        	{
           		cTry = 0;
            		txHead = 0;
            		txTail = 0;
            		txSend = 0;
			pthread_mutex_unlock(&resourse);
            		if (fOk)
            		{
                		fOk = false;
                		curInfo = (unsigned short)-1;
                		LinkDown();
            		}
            		return;
        	}
		*buffer = (unsigned char)XW;
            	*(buffer+1) = (unsigned char)(XW >> 8);
		*(buffer+2) = (unsigned char)txTail;
            	*(buffer+3) = (unsigned char)(txTail >> 8);
        	len = txSend - txTail;//starij paket
        	add(len, LENSTRBUF);//korrektirovka dlinni

        	copy_from(buffer + 4, len);//kopiruem iz txBuff starij paket

        	write_frame(buffer, len + 4);
        	set_timer_a(tmA + len/10 );
        	cTry ++;//uvelichivaem kol-vo otpravok
	}
	pthread_mutex_unlock(&resourse);
}


void Timer_B(void)
{
	unsigned char buf[4];
	*buf = (unsigned char)FW;
        *(buf+1) = (unsigned char)(FW >> 8);
	nFisCode++;
	*(buf+2) = (unsigned char)nFisCode;
	*(buf+3) = (unsigned char)(nFisCode >> 8);
	if (fFisWaitAccept)
	{
		fMustReinitPort = true;
		write_frame(buf, 4);
		fFisWaitAccept = false;
		set_timer_b(tmB);
		return;
	}
	//write_frame(buf,4);//visilaem neskolko raz
	//write_frame(buf,4);//FW dlia bolshej uverennosti
	write_frame(buf,4);//ibo esli etot paket nedojdiot sviazi pizdec
	fFisWaitAccept = true;
	set_timer_b(tmB);
}

void Timer_C(void)
{
	unsigned char info[2] = {0,0};
	WritePacket(0xFF, info, sizeof(unsigned short));//otpravka b ochered FF00
	set_timer_c(tmC);
}

