
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

CPPP::CPPP()
{
    frameLen = 0;

    txHead = 0;
    txTail = 0;
    txSend = 0;
    LastCode = -1;
    LastSize = 0;
    fOk = false;

    tmA = 200;
    tmB = 300;
    tmC = 500;

    TimerA = -1;
    TimerB = 300;               //Srazu zavodim 2 taimera
    TimerC = 500;

    cTry = 0;
    nWaitFE = 0;

    curInfo = (unsigned short) -1;
    fFisWaitAccept = false;

    m_dwReadBytes = 0;
    m_dwWriteBytes = 0;
    m_dwReadGoodFrames = 0;
    m_dwReadBadFrames = 0;
    m_dwDevReinits = 0;
    m_dwCounterLinkDown = 0;
}

CPPP::~CPPP(void)
{
}

unsigned short CPPP::read_word(short &pos)
{
    unsigned short ret;

    ret = txBuf[pos++];
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
    ret += (txBuf[pos++]) << 8;
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
    return ret;
}

void CPPP::write_word(short &pos, unsigned short w)
{
    txBuf[pos++] = (unsigned char) w;
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
    txBuf[pos++] = (unsigned char) (w >> 8);
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
}

void CPPP::add(short &pos, short del)
{
    pos += del;
    if (pos >= LENSTRBUF)
        pos -= LENSTRBUF;
}

void CPPP::copyto(unsigned char *data, unsigned short len)
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

void CPPP::copy_from(unsigned char *data, unsigned short len)
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

void CPPP::SendBytesToATS(unsigned char *data, short size)
{
    struct timeval tv;
    fd_set wset;
    int retval;
    int count = 0;

    do
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&wset);
        FD_SET(ATS_fd, &wset);
        if ((retval = select(ATS_fd + 1, NULL, &wset, NULL, &tv)) > 0)  //vsio ravno prerviomsia signalom
        {
            if (parser.fComm)
            {
                int n = write(ATS_fd, data, size);

                if (n > 0)
                {
                    printf("PC -> ATS bytes:%d\n", n);
                    m_dwWriteBytes += n;
                }
                else
                {
                    TRACE("COMM port error!\n");
                    Loger("Reconnecting...\n");
                    Reinit_ATS_Connection();
                }
            }
            else
            {
                int ERROR = -1;
                socklen_t opt_size = sizeof(ERROR);

                getsockopt(ATS_fd, SOL_SOCKET, SO_ERROR, &ERROR, &opt_size);
                if (ERROR == 0)
                {
                    int n = write(ATS_fd, data, size);

                    printf("PC -> ATS bytes:%d\n", n);
                    m_dwWriteBytes += n;
                }
                else
                {
                    TRACE("ATS socket error!\n");
                    Loger("Reconnecting...\n");
                    Reinit_ATS_Connection();
                }
            }
        }
        else if (retval < 0 && errno != EINTR)
        {
            TRACE("ATS socket internal error!\n");
            Loger("Reconnecting...\n");
            Reinit_ATS_Connection();
            break;
        }
        count++;
    } while (retval < 0 && count < 2);
}

void CPPP::SendFrameToATS(unsigned char *data, short len)
{
    unsigned char buf[MAX_FRAME * 2 + 10];
    unsigned int sum = 0;
    unsigned char t1 = 0, t2 = 0;
    unsigned short t3 = 0;

    if (len > MAX_FRAME)
        return;

    int out = 0;

    buf[out++] = 0xAA;
    buf[out++] = 0x11;
    for (int in = 0; in < len; in++)
    {
        t1 ^= data[in];
        t2 += 1 + data[in];
        t3 += (data[in] + 1) * (data[in] + 2) * (in + 3);
        if (data[in] == 0xAA)
        {
            buf[out++] = 0xAA;
            buf[out++] = 0x33;
        }
        else
            buf[out++] = data[in];
    }
    sum =
        ((unsigned int) t1) + (((unsigned int) t2) << 8) +
        (((unsigned int) t3) << 16);
    sum += parser.ProtV;
    buf[out++] = 0xAA;
    buf[out++] = 0x22;
    buf[out++] = (unsigned char) sum;
    buf[out++] = (unsigned char) (sum >> 8);
    buf[out++] = (unsigned char) (sum >> 16);
    buf[out++] = (unsigned char) (sum >> 24);
    SendBytesToATS(buf, out);

}

void CPPP::ReadBytesFtomATS(unsigned char chr) //Kombinirujem baiti v freim, videliajem paket
{
    if (frameLen >= MAX_FRAME * 2 + 10)
        frameLen = 0;           //esli previsili MAX_FRAME * 2 + 10 copim snachala 

    frameBuf[frameLen++] = chr; //ne kopitsia esli ne vstretili priznak nachala mod.paketa

    if (frameLen == 1 && !(frameBuf[0] == 0xAA))
        frameLen = 0;           // esli ne vstretili priznak vozmozhnogo nachala mod.paketa

    if (frameLen == 2 && !(frameBuf[0] == 0xAA && frameBuf[1] == 0x11))
        frameLen = 0;           // esli ne vstretili priznak nachala mod.paketa

    if (frameLen >= 8 && (frameBuf[frameLen - 6] == 0xAA && frameBuf[frameLen - 5] == 0x22))    //esli smogli nakopit paket
    {
        int in, out;

        for (in = 0, out = 0; in < frameLen;)
        {
            if (frameBuf[in] == 0xAA)
            {
                in++;           //Analizirujem sledujushij za 0xAA bait
                switch (frameBuf[in++])
                {
                  case 0x11:   //Nachalo mod.paketa
                      out = 0;
                      break;
                  case 0x22:   //Konec mod.paketa - proveriajem Controlnuju Summu
                  {
                      unsigned int sum1 =
                          (unsigned int) *(frameBuf + in) +
                          ((unsigned int) *(frameBuf + in + 1) << 8) +
                          ((unsigned int) *(frameBuf + in + 2) << 16) +
                          ((unsigned int) *(frameBuf + in + 3) << 24);
                      in += 4;
                      unsigned int sum2 = 0;
                      unsigned char t1 = 0, t2 = 0;
                      unsigned short t3 = 0;

                      for (int p = 0; p < out; p++)
                      {
                          t1 ^= frameBuf[p];
                          t2 += 1 + frameBuf[p];
                          t3 +=
                              (frameBuf[p] + 1) * (frameBuf[p] + 2) * (p + 3);
                      }
                      sum2 =
                          ((unsigned int) t1) + (((unsigned int) t2) << 8) +
                          (((unsigned int) t3) << 16);
                      sum2 += parser.ProtV;
                      if (in != frameLen)       //ignoriruem paket esli v freime kontrolnaja summa ne na meste
                      {
                          TRACE("PC <- ATS BAD LEN\n");
                          frameLen = 0;
                          m_dwReadBadFrames++;
                          return;
                      }
                      if (sum1 != sum2)
                      {
                          TRACE("PC <- ATS BAD CRC\n");
                          frameLen = 0;
                          m_dwReadBadFrames++;
                          return;
                      }
                      else
                      {
                          printf("PC <- ATS bytes:%d\n", frameLen);
                          m_dwReadGoodFrames++;
                      }
                  }
                      break;
                  case 0x33:   //Esli v mod.pakete vstretilos 0xAA
                      frameBuf[out++] = 0xAA;
                      break;
                  case 0x44:   // AA
                      TRACE("PC <- ATS AA44\n");
                      // ignoring xAA x44
                      break;

                  default:
                      TRACE("PC <- ATS BAD FRAME\n");
                      frameLen = 0;
                      m_dwReadBadFrames++;
                      return;
                }
            }
            else
                frameBuf[out++] = frameBuf[in++];
        }
        frameLen = 0;
        ProcessPacketFromATS(frameBuf, out);
    }

}



/*
void CPPP::ProcessPacketFromATS(unsigned char *data, short len)
{
    unsigned short Type =
        (unsigned short) *data + ((unsigned short) *(data + 1) << 8);
    unsigned short Code =
        (unsigned short) *(data + 2) + ((unsigned short) *(data + 3) << 8);
    switch (Type)
    {
      case FA:                 //ack of connection test
      {
          if (Code == nFisCode)
              fFisWaitAccept = false;
      }
          break;
      case XA:                 //message
      {
          buffer[0] = (unsigned char) YW;
          buffer[1] = (unsigned char) (YW >> 8);
          buffer[2] = (unsigned char) Code;
          buffer[3] = (unsigned char) (Code >> 8);
          write_frame(buffer, 4);
          if (LastCode != Code) //Esli paket dejstvitelno novij
          {
              LastCode = Code;
              int pos = 4;

              while (pos < len)
              {
                  unsigned short s =
                      (unsigned short) *(data + pos) +
                      ((unsigned short) *(data + pos + 1) << 8) - 2;
                  pos += 2;
                  if (s <= MAXSIZEONEMESS)
                      RecvData(data + pos, s);  //partim paket na mesagi
                  else
                  {
                      Loger("Message lengh is greater than availible!\n");
                      return;
                  }
                  pos += s;
              }
              if (pos != len)
                  Loger("Error of message lengh declaration!\n");
          }
          else                  //Esli povtorno prishol paket kotorij mi uzhe uspeshno obrabotali
          {
          }
      }
          break;
      case YA:                 //ACK of message
      {
          pthread_mutex_lock(&resourse);
          if (Code == txTail)   //Esli prishlo podtverzhdenije na poslednij otpravlennij paket
          {                     //mi ne shliom paketov s uprezhdenijem
              txTail = txSend;  //smeschajemsia v kolce txBuf
              if (txTail != txHead)
              {
                  *buffer = (unsigned char) XW;
                  *(buffer + 1) = (unsigned char) (XW >> 8);
                  *(buffer + 2) = (unsigned char) txTail;
                  *(buffer + 3) = (unsigned char) (txTail >> 8);

                  len = txHead - txTail;        //opredelim dopustimuju dlinnu paketa
                  add(len, LENSTRBUF);  //Uchtiom zakolcovannoct txBuf dlia dlinni otpravliaemogo freima
                  if (len > MAX_FRAME - 6)
                      len = MAX_FRAME - 6;
                  short pos = txSend, s = 0, ss;

                  while (s <= len)      //nabiraem messagi v freim chtob v niom nebilo fragmentov messag
                  {
                      txSend = pos;     //smeshaem ukuzatel otpravlennogo
                      if (txSend == txHead)
                          break;        // esli novoe uvelichenije tochno dast bolshe
                      s += (ss = read_word(pos));       //kopim dlinnu
                      add(pos, ss - 2); //smeshiajem ukazatel na dlinnu mesagi
                  }
                  len = txSend - txTail;
                  add(len, LENSTRBUF);  //istinnaja dlinna
                  if (!len)
                  {
                      Loger("Buffer error!\n");
                  }
                  copy_from(buffer + 4, len);   //copiruem iz txBuf nakoplennoe
                  write_frame(buffer, len + 4);
                  set_timer_a(tmA + len / 10);
                  cTry = 0;
              }
              else
                  set_timer_a(0);       //Paket ne podtverdilsia i mi zapuskajem ego povtornuju otpravku
          }
          pthread_mutex_unlock(&resourse);
      }
          break;
    }

}
*/

void CPPP::ProcessPacketFromATS(unsigned char *data, short len)
{

    unsigned short Type =
        (unsigned short) *data + ((unsigned short) *(data + 1) << 8);
    unsigned short Code =
        (unsigned short) *(data + 2) + ((unsigned short) *(data + 3) << 8);

    switch (Type)
    {
      case FA:
      {
          if (Code == nFisCode) // все ОК, фисушка потвердилась
              fFisWaitAccept = false;
      }
          break;
      case XA:
      {
          short WaitCode = LastCode + LastSize;

          if (WaitCode >= LENSTRBUF)
              WaitCode -= LENSTRBUF;

          short del = WaitCode - Code;

          if (del < 0)
              del += LENSTRBUF;

          if (!LastSize || del < (LENSTRBUF / 2 - LENSTRBUF / 50))
          {
              buffer[0] = (unsigned char) YW;
              buffer[1] = (unsigned char) (YW >> 8);
              buffer[2] = (unsigned char) Code;
              buffer[3] = (unsigned char) (Code >> 8);
              SendFrameToATS(buffer, 4);
          }
          else
              break;

          if (!LastSize)
              WaitCode = Code;

          int pos = 4;
          short CurCode = Code;
          bool fCatch = false;

          while (pos < len)
          {
              unsigned short s =
                  (unsigned short) *(data + pos) +
                  ((unsigned short) *(data + pos + 1) << 8) - 2;
              pos += 2;
              if (s <= MAXSIZEONEMESS + 1)
              {
                  // proverim novij li eto packet
                  if (fCatch || CurCode == WaitCode)
                  {
                      fCatch = true;
                      ProcessMessFromATS(data + pos, s);
                  }
              }
              else
              {
                  TRACE("Message lengh is greater than availible!\n");
                  return;
              }
              pos += s;

              CurCode += (s + 2);
              if (CurCode >= LENSTRBUF)
                  CurCode -= LENSTRBUF;
          }
          if (pos != len)
              TRACE("Error of message lengh declaration!\n");

          if (fCatch)
          {
              LastCode = Code;
              if (fOk)
                  LastSize = len - 4;
              else
                  LastSize = 0;
          }
          else if (fOk)
          {
              int del = CurCode - WaitCode;

              if (del > (LENSTRBUF / 2 - LENSTRBUF / 50))
                  del -= LENSTRBUF;
              if (del > 0)
                  TRACE("OSTATOK ERROR\n");
          }
      }
          break;
      case YA:                 //ACK of message
      {
          pthread_mutex_lock(&resourse);
          if (Code == txTail)   //Esli prishlo podtverzhdenije na poslednij otpravlennij paket
          {                     //mi ne shliom paketov s uprezhdenijem
              txTail = txSend;  //smeschajemsia v kolce txBuf
              if (txTail != txHead)
              {
                  *buffer = (unsigned char) XW;
                  *(buffer + 1) = (unsigned char) (XW >> 8);
                  *(buffer + 2) = (unsigned char) txTail;
                  *(buffer + 3) = (unsigned char) (txTail >> 8);

                  len = txHead - txTail;        //opredelim dopustimuju dlinnu paketa
                  add(len, LENSTRBUF);  //Uchtiom zakolcovannoct txBuf dlia dlinni otpravliaemogo freima
                  if (len > MAX_FRAME - 6)
                      len = MAX_FRAME - 6;
                  short pos = txSend, s = 0, ss;

                  while (s <= len)      //nabiraem messagi v freim chtob v niom nebilo fragmentov messag
                  {
                      txSend = pos;     //smeshaem ukuzatel otpravlennogo
                      if (txSend == txHead)
                          break;        // esli novoe uvelichenije tochno dast bolshe
                      s += (ss = read_word(pos));       //kopim dlinnu
                      add(pos, ss - 2); //smeshiajem ukazatel na dlinnu mesagi
                  }
                  len = txSend - txTail;
                  add(len, LENSTRBUF);  //istinnaja dlinna
                  if (!len)
                  {
                      TRACE("Buffer error!\n");
                  }
                  copy_from(buffer + 4, len);   //copiruem iz txBuf nakoplennoe
                  SendFrameToATS(buffer, len + 4);
                  set_timer_a(tmA + len / 10);
                  cTry = 0;
              }
              else
                  set_timer_a(0);       //Nechego otsilat. Vikliuchajem taimer
          }
          pthread_mutex_unlock(&resourse);
      }
          break;
    }
}

void CPPP::LinkUp(void)
{
    Loger("LinkUp!\n");
    link_state = true;
}

void CPPP::LinkDown(void)
{
    Loger("LinkDown!\n");
    link_state = false;         //dlia otboja klientov ozhidajushih soedinenija
    m_dwCounterLinkDown++;
    sleep(1);
    Reinit_ATS_Connection();
}

void CPPP::OnOpenChannel(unsigned char ch)
{
}

void CPPP::OnCloseChannel(unsigned char ch)
{
}

void CPPP::ProcessMessFromATS(unsigned char *data, short len)
{
    if (len < 1)
    {
        TRACE("Message lengh error!\n");
        return;
    }
    unsigned char to = *data;   // beriom pervij simvol mesagi

    data++;
    len--;
    if (to == 0xFB)             // esli FB
    {
        if (fOk)                //Esli do etogo kanal bil rabochij to teper kanal so stanciej ne rabochij
        {
            fOk = false;
            LastSize = 0;
            curInfo = (unsigned short) -1;
            LinkDown();         //otbivaem klientskije soedinenija
            LastSize = 0;
        }
    }
    else if (to == 0xFF && len == 2)    //esli FF
    {
        if (!fOk)               //Esli kanal ne rabochij 
        {
            PutMessToBuff(0xFB, NULL, 0); //shliom FB
        }
        //a zatem FE c s poluchennim <n><n>
        PutMessToBuff(0xFE, data, sizeof(unsigned short));
        curInfo =
            (unsigned short) (*data) + ((unsigned short) *(data + 1) << 8);
    }
    else if (to == 0xFE && len == 2)    //esli FE
    {
        unsigned short info = (unsigned short) *data + ((unsigned short) *(data + 1) << 8);     //schitivaem parametr <k><k>
        unsigned char id;

        nWaitFE = 0;
        id = 0;
        if (info == ((id) + (id << 8))) // esli sovpalo s PC
        {

            if (!fOk && curInfo != (unsigned short) -1)
            {
                fOk = true;
                LinkUp();
                LastSize = 0;
            }
        }
        else if (fOk)
        {
            fOk = false;
            LastSize = 0;
            curInfo = (unsigned short) -1;
            LinkDown();
            LastSize = 0;
        }
    }
    else if (!fOk)
        return;
    else if (to == 0xFD && len == 1)    // etu hren ne dolzhna slat stancija
    {
        unsigned char ch = *data;

        if (ch < MAX_CLIENT)
            OnOpenChannel(ch);
    }
    else if (to == 0xFC && len == 1)    // etu hren ne dolzhna slat stancija
    {
        unsigned char ch = *data;

        if (ch < MAX_CLIENT)
            OnCloseChannel(ch);
    }
    else if (to == 0xF9)        // esli mp
    {
        if (parser.fComm)
        {
            struct termios tty;

            if (tcgetattr(ATS_fd, &tty) < 0)
                TRACE("Can't get COMM info to set 9600!\n");
            else
            {
                cfsetospeed(&tty, B9600);
                cfsetispeed(&tty, B9600);
                cfmakeraw(&tty);
                if (tcsetattr(ATS_fd, TCSANOW, &tty) < 0)
                    TRACE("Can't set COMM 9600!\n");
                else
                {
                    Loger("COMM set to 9600!\n");
                }
            }
        }
        else
            TRACE("Can't set COMM 9600 - conected via ethernet!\n");
    }
    else if (to < MAX_CLIENT)   //otdajem klientu
    {
        SendMessToClient(to, data, len);
    }
    else
    {
        TRACE("Data error!\n");
    }
}

void CPPP::PutMessToBuff(unsigned char ch, unsigned char *data, short len)
{
    unsigned char buf[MAXSIZEONEMESS];

    if (len + 1 > MAXSIZEONEMESS)
    {
        TRACE("WARNING paket is to big!\n");
        return;
    }
    *buf = ch;
    memcpy(buf + 1, data, len);
    PutDataToBuff(buf, len + 1);
}

void CPPP::SendMessToClient(unsigned char ch, unsigned char *data, short len)
{
    if(ch < MAX_CLIENT)
    {
        //pthread_mutex_lock(&client_mutex[ch]);
        if (Client[ch])
            Client[ch]->SendPacket(data, len);
        //pthread_mutex_unlock(&client_mutex[ch]);
    }
}


void CPPP::PutDataToBuff(unsigned char *data, short len)
{
    short used;

    len += 2;
    if (((unsigned short) len) > MAXSIZEONEMESS)
    {
        TRACE("Warning message lengh error!\n");
        return;
    }
    pthread_mutex_lock(&resourse);
    used = txHead - txTail;
    add(used, LENSTRBUF);
    if (used + len >= LENSTRBUF)        //perekritije dannih
    {
        TRACE("Warning buffer owerflow!\n");
        txHead = txTail = txSend = 0;
        pthread_mutex_unlock(&resourse);
        return;
    }
    write_word(txHead, len);    //zapisali dlinnu v kolco
    len -= 2;
    copyto(data, len);          //perenesli mesagu v kolco

    if (txTail == txSend && txTail != txHead)   // esli vsio podtverdilos i est chto otpravit otpravliajem
    {
        *buffer = (unsigned char) XW;
        *(buffer + 1) = (unsigned char) (XW >> 8);
        *(buffer + 2) = (unsigned char) txTail;
        *(buffer + 3) = (unsigned char) (txTail >> 8);
        len = txHead - txTail;
        add(len, LENSTRBUF);
        if (len > MAX_FRAME - 6)
            len = MAX_FRAME - 6;
        short pos = txSend, s = 0, ss;

        while (s <= len)
        {
            txSend = pos;
            if (txSend == txHead)
                break;
            s += (ss = read_word(pos));
            add(pos, ss - 2);
        }
        len = txSend - txTail;
        add(len, LENSTRBUF);
        if (!len)
        {
            TRACE("Buffer error!\n");
        }
        copy_from(buffer + 4, len);

        SendFrameToATS(buffer, len + 4);
        set_timer_a(tmA + len / 10);
        cTry = 0;
    }
    pthread_mutex_unlock(&resourse);
}

void CPPP::OpenChannel(unsigned char ch)
{
    unsigned char buf[10];

    *buf = 0xFD;
    *(buf + 1) = ch;
    PutDataToBuff(buf, 2);
}

void CPPP::CloseChannel(unsigned char ch)
{
    unsigned char buf[10];

    *buf = 0xFC;
    *(buf + 1) = ch;
    PutDataToBuff(buf, 2);
}

void CPPP::set_timer_a(int timer)
{
    TimerA = timer;
}


void CPPP::set_timer_b(int timer)
{
    TimerB = timer;
}

void CPPP::set_timer_c(int timer)
{
    TimerC = timer;
}

void CPPP::Timer_A(void)
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
            TRACE("ERROR TIMER A\n");
            if (fOk)
            {
                fOk = false;
                curInfo = (unsigned short) -1;
                LastSize = 0;
                LinkDown();
                LastSize = 0;
            }
            return;
        }
        *buffer = (unsigned char) XW;
        *(buffer + 1) = (unsigned char) (XW >> 8);
        *(buffer + 2) = (unsigned char) txTail;
        *(buffer + 3) = (unsigned char) (txTail >> 8);
        len = txSend - txTail;  //starij paket
        add(len, LENSTRBUF);    //korrektirovka dlinni

        copy_from(buffer + 4, len);     //kopiruem iz txBuff starij paket

        SendFrameToATS(buffer, len + 4);
        set_timer_a(tmA + len / 10);
        cTry++;                 //uvelichivaem kol-vo otpravok
    }
    pthread_mutex_unlock(&resourse);
}


void CPPP::Timer_B(void)
{
    unsigned char buf[4];
    static int countB = 0;

    *buf = (unsigned char) FW;
    *(buf + 1) = (unsigned char) (FW >> 8);
    nFisCode++;
    *(buf + 2) = (unsigned char) nFisCode;
    *(buf + 3) = (unsigned char) (nFisCode >> 8);
    if (fFisWaitAccept)
    {
        if (++countB > 2)
        {
            countB = 0;
            TRACE("ERROR TIMER B\n");
            Reinit_ATS_Connection();
            fFisWaitAccept = false;
            set_timer_b(tmB);
            return;
        }
    }
    else
        countB = 0;
    SendFrameToATS(buf, 4);        //visilaem neskolko raz
    SendFrameToATS(buf, 4);        //FW dlia bolshej uverennosti
    SendFrameToATS(buf, 4);        //ibo esli etot paket nedojdiot sviazi pizdec
    fFisWaitAccept = true;
    set_timer_b(tmB);
}

void CPPP::Timer_C(void)
{
    unsigned char info[2] = { 0, 0 };
    PutMessToBuff(0xFF, info, sizeof(unsigned short));    //otpravka b ochered FF00
    set_timer_c(tmC);
    if (++nWaitFE >= 5)
    {
        nWaitFE = 0;
        if (fOk)
        {
            TRACE("ERROR TIMER C\n");
            fOk = false;
            LastSize = 0;
            curInfo = (unsigned short) -1;
            LinkDown();
            LastSize = 0;
        }
    }
}

void CPPP::ProcessTimers(void)
{
    if (TimerA > 0)
    {
        TimerA--;
        if (TimerA == 0)
            RunTimerA = true;
    }
    if (TimerB > 0)
    {
        TimerB--;
        if (TimerB == 0)
            RunTimerB = true;
    }
    if (TimerC > 0)
    {
        TimerC--;
        if (TimerC == 0)
            RunTimerC = true;
    }
}

void CPPP::RunTimers(void)
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
}

void CPPP::PrintInfo(char *str)
{
    char *s = str;

    s += sprintf(str,
                 "ReadBytes:%d\r\nWriteBytes:%d\r\nReadGoodFrames:%d\r\nReadBadFrames:%d\r\nDevReinits:%d\r\nCounterLinkDown:%d\r\n",
                 m_dwReadBytes, m_dwWriteBytes, m_dwReadGoodFrames,
                 m_dwReadBadFrames, m_dwDevReinits, m_dwCounterLinkDown);
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (Client[i] && strlen(Client[i]->IP) && s)
            s += sprintf(s, "Connected:%s\r\n", Client[i]->IP);
    }
    strcat(str, isOk()? "LinkUp\r\n" : "LinkDown\r\n");
    strcat(str, "\r\n");
}
