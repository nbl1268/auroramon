
/***************************************************************************
 *   Copyright (C) 2012 by Jonathan Duddington                             *
 *   email: jonsd@users.sourceforge.net                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see:                                 *
 *               <http://www.gnu.org/licenses/>.                           *
 ***************************************************************************/

/*  Special thanks to Curt Blank (http://www.curtronics.com/) for his "aurora"
 *  command-line program which shows how to communicate with, and retrieve
 *    information from, the Aurora inverters
 */


#ifdef __WXMSW__
//#include <windows.h>
//#include <wx/msw/registry.h>
#else
#include <termios.h>
#endif

#include <errno.h>
#include <wx/filename.h>
#include <wx/thread.h>
#include "auroramon.h"


#pragma region Declarations
#define STATE_OK   0

#define opGetDSP   59
#define opGetCE    78

#define TimeBase      946684800

int energy_today_adjust = 0;   // use if the inverter starts with a spurious non-zero energy-today value

int big_endian = 0;
int SerialBlocking = 2;             // x10mS serial port timeout for read (Linux)
int max_send_attempts = 3;
#ifdef __WXMSW__
static int max_read_attempts = 6;
#else
static int max_read_attempts = 5;
#endif

static int comms_error = 0;
static int transmission_state_error = 0;
static int serial_port_error = 0;
static int comms_inverter = 0;
#define N_CLR_ATTEMPTS 1000

#define N_SERIALBUF 11
static char SerialBuf[N_SERIALBUF];
static const char *SerialBufSpaces = "          ";   // fill SerialBuf with 10 spaces and /0

int command_inverter = 0;
int command_type = 0;
int command_queue = 0;

void SendCommand(int inv, int type);

INVERTER_RESPONSE inverter_response[N_INV];
INVERTER_RESPONSE *ir;

#define TENSEC_BASE  0x000a
#define TENSEC_MAX     8640
#define DAILY_BASE   0x438c
#define DAILY_NDAYS     366

#define RETRIEVE_10SEC       2
#define RETRIEVE_DAILY       3

int retrieving_data = 0;
int retrieving_inv = 0;
int retrieving_progress = 0;
int retrieving_finished = 0;
int retrieving_pvoutput;
wxString retrieving_fname = wxEmptyString;
wxString retrieve_message = wxEmptyString;
int settime_offset = 0;

static int CE_start = 0;
static int CE_next = 0;
static int CE_count = 0;
static int CE_errors = 0;
static int CE_inv_addr = 0;
static int CE_inv = 0;
static double CE_scale = 1;
static int CE_today_ix = 0;
static int CE_verbose = 0;
static unsigned short *CE_data = NULL;

#pragma endregion

// Done 09/08/2022 - fixed bug in QueueCommand() where inv parameter was missing from LogMessage resulting in an 'ArgType' error when Inverter 'Retrieve Daily Energy' and 'Retrieve 10sec Energy' requests were made
// Done 09/08/2022 - expanded logging to capture details for Communicat::Transmission_state_error message

void LogCommMsg(wxString string)
{//==============================
    FILE *f;
    char buf[256];
    struct tm *btime;
    time_t current_time;

    //return;

    if((inverters[comms_inverter].alive == 0) && (inverters[comms_inverter].fails > 3))
        return;

    time(&current_time);
    btime = localtime(&current_time);

    strncpy0(buf, wxString(data_dir+_T("/system/com_log.txt")).mb_str(wxConvLocal), sizeof(buf));
    if((f = fopen(buf, "a")) != NULL)
    {
        strncpy0(buf, string.mb_str(wxConvLocal), sizeof(buf));
        fprintf(f, "%.2d:%.2d:%.2d  %s\n", btime->tm_hour, btime->tm_min, btime->tm_sec, buf);
        fclose(f);
    }
}


class InverterThread: public wxThread
{//============================
    public:
        InverterThread(void);
        virtual void *Entry();

    private:
        int addr;
        int type;
};

InverterThread::InverterThread(void)
        : wxThread()
{//=================================
    type = 0;
}

InverterThread *inverter_thread;


int QueueCommand(int inv, int type)
{//================================
    wxString message = wxEmptyString;

    if(command_queue != 0)
        return(-1);

    command_queue = (type << 8) + inv;

    if((type == cmdInverter10SecEnergy) || (type == cmdInverterDailyEnergy))
    {
        message = _T("Retrieve ") + retrieve_message;
    }
    if(message != wxEmptyString)
    {
        if(inverter_address[1] != 0)
            LogMessage(wxString::Format(_T("Inverter %d: %s"), inv, message.c_str()), 1);

        else
            LogMessage(message, 1);
    }

    return(0);
}


#ifdef __WXMSW__
static HANDLE fd_serial = INVALID_HANDLE_VALUE;

#ifdef deleted
void FindSerialPorts()
{//===================
    int ix;
    size_t nSubKeys;
    wxString strTemp;
    wxRegKey *pRegKey = new wxRegKey("HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM");

    if(!pRegKey->Exists())
    {
    }

    //Retrive the number of SubKeys and enumerate them
    pRegKey->GetKeyInfo(&nSubKeys,NULL,NULL,NULL);

    pRegKey->GetFirstKey(strTemp,1);
    for(int ix=0;ix<nSubKeys;ix++)
    {
         wxMessageBox(strTemp,"SubKey Name",0,this);
         pRegKey->GetNextKey(strTemp,1);
    }
}
#endif


void SerialClose()
{//===============
    HANDLE handle;

    if(fd_serial != INVALID_HANDLE_VALUE)
    {
        handle = fd_serial;
        fd_serial = INVALID_HANDLE_VALUE;
        CloseHandle(handle);
    }
}

int SerialOpen()
{//=============
    // Windows
	int err =0;
    wxString dev_name;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};

    dev_name = serial_port.mb_str(wxConvLocal);
    if(dev_name.empty())
        return(-1);   // no serial port set

    fd_serial = CreateFile(dev_name.fn_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if(fd_serial == INVALID_HANDLE_VALUE)
    {
        if(GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            // serial port does not exist
			err = -1;
        }
		else
		{
			// some other error
			err = -2;
		}
		fd_serial = INVALID_HANDLE_VALUE;
		return(err);
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if(!GetCommState(fd_serial, &dcbSerialParams))
    {
        // error getting state
		err = -3;
    }
    else
    {
        dcbSerialParams.BaudRate = CBR_19200;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if(!SetCommState(fd_serial, &dcbSerialParams))
        {
            // error setting serial port state
            err = -4;
        }
        else
        {
            timeouts.ReadIntervalTimeout = 50;
            timeouts.ReadTotalTimeoutConstant = 80;
            timeouts.ReadTotalTimeoutMultiplier = 10;
            timeouts.WriteTotalTimeoutConstant = 50;
            timeouts.WriteTotalTimeoutMultiplier = 10;

            if(!SetCommTimeouts(fd_serial, &timeouts))
            {
                // error in setting timeouts
                err = -5;
            }
        }
    }

    if(err != 0)
    {
        SerialClose();
    }

	return(err);
}


#else  // LINUX

static struct termios old_terminal;    // saved previous serial device configuration
static int fd_serial = -1;

int SerialConfig()
{//===============
    struct termios terminal;    // serial device configuration

    tcgetattr(fd_serial, &old_terminal);     // save previous port settings

    memset(&terminal, 0, sizeof(terminal));  // no parity, one stop bit, no hangup
    terminal.c_cflag &= ~PARENB;	    // no parity
    terminal.c_cflag &= ~CSTOPB;		// one stop bit
    terminal.c_cflag &= ~CSIZE;			// character size mask
    terminal.c_cflag &= ~HUPCL;			// no hangup
//    if (bRTSCTS)
//        terminal.c_cflag |= CRTSCTS;             /* enable hardware flow control */
//    else
        terminal.c_cflag &= ~CRTSCTS;             /* disable hardware flow control */
    terminal.c_cflag |= CS8 | CLOCAL | CREAD;	// 8 bit - ignore modem control lines - enable receiver
//    if (bXonXoff)
//        terminal.c_iflag |= (IXON | IXOFF);		/* enable XON/XOFF flow control on output & input */
//    else
        terminal.c_iflag &= ~(IXON | IXOFF);		/* disable XON/XOFF flow control on output & input*/
    terminal.c_iflag |= IGNBRK | IGNPAR;		/* ignore BREAK condition on input & framing errors & parity errors */
    terminal.c_oflag = 0;	    			/* set serial device input mode (non-canonical, no echo,...) */
    terminal.c_oflag &= ~OPOST;			/* disable output processing */
    terminal.c_lflag = 0;
    terminal.c_cc[VTIME]    = SerialBlocking;		/* timeout in 1/10 sec intervals */
    terminal.c_cc[VMIN]     = 0;			/* block until char or timeout */

    if(cfsetospeed(&terminal, B19200) != 0)
    {
        // failed to set output speed
        return(-3);
    }

    if(cfsetispeed(&terminal, B19200) != 0)
    {
        // failed to set input speed
        return(-4);
    }

    if(tcflush(fd_serial, TCIFLUSH) != 0)
    {
    }
    if(tcsetattr(fd_serial, TCSANOW, &terminal) != 0)
    {
        return(-5);
    }

    if(tcflush(fd_serial, TCIOFLUSH) != 0)
    {
        return(-6);
    }

    return(0);
}


int SerialOpen()
{//==============
    // Linux
    int result;
    char dev_name[20];
	struct stat statbuf;

    strncpy0(dev_name, serial_port.mb_str(wxConvLocal), sizeof(dev_name));
    if(dev_name[0] == 0)
        return(-1);   // no serial port set

	if(stat(dev_name,&statbuf) != 0)
	{
	    if(strcmp(dev_name, "/dev/ttyUSB0") == 0)
	    {
	        // Try a different ttyUSB port
            strcpy(dev_name, "/dev/ttyUSB1");

            if(stat(dev_name,&statbuf) != 0)
                return(-1);
	    }
	    else
	    {
            return(-1);
	    }
	}

    // open the serial port
    if((fd_serial = open(dev_name, O_RDWR | O_NOCTTY )) < 0)
    {
        // failed to open serial port device
        return(-2);
    }

    if((result = SerialConfig()) < 0)
    {
        close(fd_serial);
        fd_serial = -1;
        return(result);
    }

    return(0);
}


void SerialClose()
{//===============
    int handle;

    if(fd_serial < 0)
        return;

    handle = fd_serial;
    fd_serial = -1;

    if(tcsetattr(handle, TCSANOW, &old_terminal) != 0)
    {
        return;
    }
    if(tcflush(handle, TCIOFLUSH) != 0)
    {
    }
    close(handle);
}
#endif


/*--------------------------------------------------------------------------
    crc16
                                         16   12   5
    this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
    This is 0x1021 when x is 2, but the way the algorithm works
    we use 0x8408 (the reverse of the bit pattern).  The high
    bit is always assumed to be set, thus we only use 16 bits to
    represent the 17 bit value.
----------------------------------------------------------------------------*/

#define POLY 0x8408   /* 0x1021 bit reversed */

unsigned short crc16(char *data_p, unsigned short length)
{//======================================================
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
        return (~crc);
    do
    {
        for (i=0, data=(unsigned int)0xff & *data_p++;
                i < 8;
                i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else
                crc >>= 1;
        }
    } while (--length);

    crc = ~crc;
    return (crc);
}



int ReadNextChar(char *out, int timeout)
{//=====================================

#ifdef __WXMSW__
    unsigned long n_read;
    if(!ReadFile(fd_serial, out, 1, &n_read, NULL))
    {
    }
#else
    int n_read;
    errno = 0;
    n_read = read(fd_serial, out, 1);

// we get result=0 if inverter is not alive
    if(errno != 0)
    {
    }
    if(n_read == -1)
    {
        // failed to read from serial device
    }
#endif
    return(n_read);
}

int ReadToBuffer(char *out, int n_chars)
{//=====================================
    int ix;
    char ch;
    int result;
    int retry = 0;

    ix = 0;
    while(ix < n_chars)
    {
        while((result = ReadNextChar(&ch, 0)) == 0)
        {
            retry++;
            if(retry > max_read_attempts)
            {
                return(0);
            }
        }

        if(result == 1)
        {
            out[ix++] = ch;
        }
        else
            return(result);

    }

    if(retry > 1)
    {
        LogCommMsg(wxString::Format(_T("   Wait %d"), retry));
    }
    return(ix);  // OK
}

int Communicate(int check_state)
{//=============================
    int count = 0;
    unsigned long nchars = 0;
    int result = 0;
    int crc_ok = 0;
    int crcValue;
    char ch;
    int max_attempts;
    int attempts;
    char SerialBufSave[N_SERIALBUF];
    int ix;

    //wxStartTimer();

    memcpy(SerialBufSave, SerialBuf, N_SERIALBUF);

    max_attempts = max_send_attempts;
    if(inverters[comms_inverter].alive == 0)
    {
        // This inverter is not alive.  Don't do repeat attempts, and so don't slow down the polling rate of the other inverter too much.
        max_attempts = 1;

        if((inverters[comms_inverter].fails > 3) && (inverters[comms_inverter ^ 1].alive == 0))
        {
            // The other inverter is also not alive. Reduce the polling rate.
            inverter_thread->Sleep(300);
            max_attempts = 2;
        }
    }

    for(attempts = 1; (crc_ok == 0) && (attempts <= max_attempts); attempts++)
    {
        if(attempts > 1)
        {
            SerialClose();
            SerialOpen();
            inverters[comms_inverter].comms_error = 1;
        }
        memcpy(SerialBuf, SerialBufSave, N_SERIALBUF);

        while((ReadNextChar(&ch, 0) != 0) && (count < N_CLR_ATTEMPTS))
        {
            count++;
        }

        crcValue = crc16(SerialBuf, 8);
        SerialBuf[8] = crcValue & 0xff;
        SerialBuf[9] = (crcValue >> 8) & 0xff;
        SerialBuf[10] = 0;

#ifdef __WXMSW__
        WriteFile(fd_serial, SerialBuf, 10, &nchars, NULL);

#else
        if(tcflush(fd_serial, TCIOFLUSH) != 0)
        {
            result = 1;
        }
        nchars = write(fd_serial, SerialBuf, 10);   // should we repeat until all chars have been sent?
        if(tcdrain(fd_serial) != 0)
        {
            result = 2;
        }
#endif

        if(nchars != 10)
        {
            LogCommMsg(wxString::Format(_T("%d  Only written %d chars"), attempts, nchars));
            if(nchars <= 0)
                continue;   // we havent sent anything, so no point in looking for a reply
        }

        strcpy(SerialBuf, SerialBufSpaces);

        result = ReadToBuffer(SerialBuf, 8);

        if(result == 8)
        {
            crcValue = crc16(SerialBuf, 6);
            if(((unsigned char)SerialBuf[6] != (crcValue & 0xff)) || ((unsigned char)SerialBuf[7] != ((crcValue >> 8) & 0xff)))
            {
                // crc error
                LogCommMsg(wxString::Format(_T("%d  CRC error, nchars %d opcode %2d %2d"), attempts, result, SerialBufSave[1], SerialBufSave[2]));
            }
            else
            {
                crc_ok = 1;
            }
        }
        else
        if(result == 0)
        {
            inverter_thread->Sleep(100);  // thread sleep
            if(attempts >= max_attempts)
                LogCommMsg(wxString::Format(_T("%d  Wait %d timeout - fail"), attempts, max_read_attempts+1));
            else
                LogCommMsg(wxString::Format(_T("%d  Wait %d timeout, retry"), attempts, max_read_attempts+1));
        }
        else
        {
            // failed to read from serial device
            LogCommMsg(wxString::Format(_T("%d  ReadToBuffer %d chars"), attempts, result));
        }
    }

    if(crc_ok == 0)
    {
        if(result > 0)
        {
            LogCommMsg(_T("CRC error, failed\n"));
        }
        result = -1;
    }

    if((check_state==1) && (SerialBuf[0] != STATE_OK))
    {
        transmission_state_error = SerialBuf[0];
        if(transmission_state_error != 0x20)
        {
            // 0x20 is the space character we put into SerialBuf[0], unchanged
            LogMessage(wxString::Format(_T("----")), 1);
            LogMessage(wxString::Format(_T("Transmission state error %d  buf[0]=%d cmd=%d"), transmission_state_error, SerialBuf[0], SerialBufSave[1]),1);
            LogCommMsg(wxString::Format(_T("Transmission state error %d  buf[0]=%d cmd=%d\n"), transmission_state_error, SerialBuf[0], SerialBufSave[1]));
            
            // list out the buffer contents here
            for (ix = 0;  SerialBuf[ix] !=0; ix++)
            {
                LogMessage(wxString::Format(_T("Transmission state error - buffer content buf[%d]=%d"), ix, SerialBuf[ix]),1);
                LogCommMsg(wxString::Format(_T("Transmission state error - buffer content buf[%d]=%d\n"), ix, SerialBuf[ix]));
            }
            LogMessage(wxString::Format(_T("Transmission state error - End")), 1);
            LogMessage(wxString::Format(_T("----")), 1);
        }
        result = -2;
    }
    if(result <= 0)
    {
        comms_error |= 1;
    }
    else
    {
        //LogCommMsg(wxString::Format(_T("OK   opcode %2d %2d  result %.2x %.2x %.2x %.2x %.2x %.2x"), SerialBufSave[1], SerialBufSave[2],
        //                             SerialBuf[0], SerialBuf[1], SerialBuf[2]&0xff, SerialBuf[3]&0xff, SerialBuf[4]&0xff, SerialBuf[5]&0xff));
    }

    //LogCommMsg(wxString::Format(_T("  time %d"), wxGetElapsedTime()));
    return(result);
}


unsigned long ConvertLong(char *buf)
{//=================================
    unsigned long *p;
    unsigned char buf2[4];

    if(big_endian)
    {
        buf2[0] = buf[0];
        buf2[1] = buf[1];
        buf2[2] = buf[2];
        buf2[3] = buf[3];
    }
    else
    {
        buf2[0] = buf[3];
        buf2[1] = buf[2];
        buf2[2] = buf[1];
        buf2[3] = buf[0];
    }

    p = (unsigned long *)buf2;
    return(*p & 0xffffffff);
}

unsigned short ConvertShort(char *buf)
{//=================================
    unsigned short *p;
    unsigned char buf2[4];

    if(big_endian)
    {
        buf2[0] = buf[0];
        buf2[1] = buf[1];
    }
    else
    {
        buf2[0] = buf[1];
        buf2[1] = buf[0];
    }

    p = (unsigned short *)buf2;
    return(*p);
}

float ConvertFloat(char *buf)
{//==========================
    float *p;
    unsigned char buf2[4];

    if(big_endian)
    {
        buf2[0] = buf[0];
        buf2[1] = buf[1];
        buf2[2] = buf[2];
        buf2[3] = buf[3];
    }
    else
    {
        buf2[0] = buf[3];
        buf2[1] = buf[2];
        buf2[2] = buf[1];
        buf2[3] = buf[0];
    }

    p = (float *)buf2;
    return(*p);
}


int GetCEdata(int addr, int param)
{//===============================
    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = opGetCE;
    SerialBuf[2] = param;
    SerialBuf[3] = 0;

    if(Communicate(1) <= 0)
    {
        return(-1);  // failed
    }

    return(ConvertLong(&SerialBuf[2]));
}

float GetDSPdata(int addr, int param)
{//==================================
    float value;
    int result;

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = opGetDSP;
    SerialBuf[2] = param;
    SerialBuf[3] = 0;

    if((result = Communicate(1)) <= 0)
    {
        return(-97999);  // big negative number indicates fail
    }

    value = ConvertFloat(&SerialBuf[2]);
    return(value);
}

typedef struct {
   int  value;
   const char *name;
} NAME_TABLE;

typedef struct {
   int  value;
   double scale;  // for 10sec data. -1 means "these statistics are not available for this model of Inverter at the moment"
   const char *name;
} MODEL_TABLE;

// eg. PVI-4.2-OUTD-UK-W
MODEL_TABLE model_names[] = {
{'1', 0.0970019, "PVI-3.0-OUTD"},
{'2', 0.1617742, "PVI-3.3-OUTD"},
{'3', 0.1617742, "PVI-3.6-OUTD"},
{'4', 0.1617742, "PVI-4.2-OUTD"},
{'5', 0.1617742, "PVI-5000-OUTD"},
{'6', 0.1617742, "PVI-6000-OUTD"},
{'A', 0.1617742, "PVI-CENTRAL-350"},
{'B', 0.1617742, "PVI-CENTRAL-350"},
{'C', 0.1617742, "PVI-MODULE-50"},
{'D', 0.5320955, "PVI-12.5-OUTD"},
{'G', -1,        "UNO-2.5-I"},
{'H', -1,        "PVI-4.6-I-OUTD"},
{'I', 0.1004004, "PVI-3600"},
{'L', 0.1617742, "PVI-CENTRAL-350"},
{'M', 0.5320955, "PVI-CENTRAL-250"},
{'O', 0.1004004, "PVI-3600-OUTD"},
{'P', 0.1617742, "3-PHASE-INTERFACE"},
{'T', 0.5320955, "PVI-12.5-I-OUTD"},  // scale ? (output 480 VAC)
{'U', 0.5320955, "PVI-12.5-I-OUTD"},  // scale ? (output 208 VAC)
{'V', 0.5320955, "PVI-12.5-I-OUTD"},  // scale ? (output 380 VAC)
{'X', 0.5320955, "PVI-10.0-OUTD"},
{'Y', 0.5320955, "PVI-TRIO-30-OUTD"}, // scale ?
{'Z', 0.5320955, "PVI-12.5-I-OUTD"},  // scale ? (output 600 VAC)
{'g', -1,        "UNO-2.0-I"},
{'h', -1,        "PVI-3.8-I"},
{'i', 0.0557842, "PVI-2000"},
{'o', 0.0557842, "PVI-2000-OUTD"},
{'t', 0.5320955, "PVI-10.0-I-OUTD"},  // scale ? (output 480 VAC)
{'u', 0.5320955, "PVI-10.0-I-OUTD"},  // scale ? (output 208 VAC)
{'v', 0.5320955, "PVI-10.0-I-OUTD"},  // scale ? (output 380 VAC)
{'w', 0.5320955, "PVI-10.0-I-OUTD"},  // scale ? (output 480 VAC current limit 12 A)
{'y', 0.5320955, "PVI-TRIO-20-OUTD"}, // scale ?
{'z', 0.5320955, "PVI-10.0-I-OUTD"},  // scale ? (output 600 VAC)
{-1, -1, NULL}
};

NAME_TABLE standard_names[] = {
{'A', "US UL1741"},
{'B', "BE VDE Belgium Model"},
{'C', "CZ Czech Republic"},
{'E', "DE VDE0126"},
{'F', "FR VDE0126"},
{'G', "GR VDE Greece Model"},
{'H', "HU Hungary"},
{'I', "IT ENEL DK 5950"},
{'K', "AU AS 4777"},
{'O', "KR Korea"},
{'P', "PT Portugal"},
{'Q', "CN China"},
{'R', "IE EN50438"},
{'S', "ES DR 1663/2000"},
{'T', "TW Taiwan"},
{'U', "UK G83"},
{'W', "DE BDEW"},

{'a', "US UL1741 Vout = 208 single phase"},
{'b', "US UL1741 Vout = 240 single phase"},
{'c', "US UL1741 Vout = 277 single phase"},
{'e', "   VDE AR-N-4105"},
{'k', "IL Isreal - Derived from AS"},
{'o', "   Corsica"},
{'u', "UK G59"},
{-1, NULL}
};

const char *LookupName(NAME_TABLE *t, int value)
{//=============================================
    while(t->name != NULL)
    {
        if(t->value == value)
            return(t->name);
        t++;
    }
    return(NULL);
}

MODEL_TABLE *LookupModel(MODEL_TABLE *t, int value)
{//================================================
    while(t->name != NULL)
    {
        if(t->value == value)
            return(t);
        t++;
    }
    return(NULL);
}


time_t GetInverterTime(int addr, time_t *timeval, int *timediff)
{//=============================================================
    time_t computer_time;
    time_t inverter_time;
    int diff;

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 70;
    SerialBuf[2] = 0;

    if(Communicate(1) <= 0)
        return(-1);

    computer_time = time(NULL);

    inverter_time = ConvertLong(&SerialBuf[2]);
    inverter_time += (time_t)TimeBase;
    inverter_time -= GetGmtOffset(0);

    diff = inverter_time - computer_time;

    if(timeval != NULL)
        *timeval = inverter_time;
    if(timediff != NULL)
        *timediff = diff;
    return(0);
}


int ResetPartial(int inv)
{//======================
    // reset the partial energy total and partial time counter
    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = inverter_address[inv];
    SerialBuf[1] = 80;  // get counter
    SerialBuf[2] = 3;
    if(Communicate(1) <= 0) return(-1);
    return(0);
}

int GetInverterInfo(int inv)
{//=========================
// -fgmnpv GetVerFW  GetMfgDate  GetConf  GetSN  GetPN  GetVer
    FILE *f_inv;
    wxString fname;
    int addr;
    int ix;
    const char *pc, *ps, *pt, *pw;
    MODEL_TABLE *pm;
    char firmware[4] = {' ',' ',' ',' '};
    char serial_number[7] = {0};
    char part_number[7] = {0};
    char model_name_buf[50];
    char standard_name_buf[50];
    char country_letters[4];
    static const char *unknown = "unknown";
    static char manufacture_week[3] = "  ";
    static char manufacture_year[3] = "  ";

    addr = inverter_address[inv];
    comms_error = 0;
    pc = unknown;

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 72;  // firmware release
    SerialBuf[2] = 0;
    if(Communicate(1) <= 0) return(-1);
    firmware[0] = SerialBuf[2];
    firmware[1] = SerialBuf[3];
    firmware[2] = SerialBuf[4];
    firmware[3] = SerialBuf[5];

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 65;  // manufacturing week and year
    SerialBuf[2] = 0;
    if(Communicate(1) <= 0) return(-1);
    manufacture_week[0] = SerialBuf[2];
    manufacture_week[1] = SerialBuf[3];
    manufacture_year[0] = SerialBuf[4];
    manufacture_year[1] = SerialBuf[5];

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 77;  // system configuration
    SerialBuf[2] = 0;
    if(Communicate(1) <= 0) return(-1);

    switch(SerialBuf[2])
    {
        case 0: pc = "System operating with both strings."; break;
        case 1: pc = "String 1 connected, String 2 disconnected."; break;
        case 2: pc = "String 2 connected, String 1 disconnected."; break;
        default: pc = unknown;
    }

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 63;  // serial number
    SerialBuf[2] = 0;
    if(Communicate(0) <= 0) return(-1);   // note, buf[0] is used to return the serial number, not the transmission state
    strncpy(serial_number, SerialBuf, 6);
    serial_number[6] = 0;

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 52;  // part number
    SerialBuf[2] = 0;
    if(Communicate(0) <= 0) return(-1);
    strncpy(part_number, SerialBuf, 6);
    part_number[6] = 0;

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 58;  // inverter version
    SerialBuf[2] = 0;
    if(Communicate(1) <= 0) return(-1);

    // Log Inverter Details
    // LogMessage(wxString::Format(_T("Inverter %d: %s"), message.c_str()), 1);
    // LogCommMsg(wxString::Format(_T("Transmission state error %d  buf[0]=%d cmd=%d\n"), transmission_state_error, SerialBuf[0], SerialBufSave[1]));

    LogCommMsg(wxString::Format(_T("Part Number SerialBuf[2]=%d\n"), SerialBuf[2]));
    pm = LookupModel(model_names, SerialBuf[2] & 0xff);
    if(pm == NULL)
    {
        sprintf(model_name_buf, "Aurora ??? (%.2x)", SerialBuf[2] & 0xff);
    }
    else
    {
        strcpy(model_name_buf, pm->name);
    }

    LogCommMsg(wxString::Format(_T("Look Up Name SerialBuf[3]=%d\n"), SerialBuf[3]));
    ps = LookupName(standard_names, SerialBuf[3] & 0xff);
    if(ps == NULL)
    {
        sprintf(standard_name_buf, "   Standard ?? (%.2x)", SerialBuf[3] & 0xff);
        ps = standard_name_buf;
    }

    country_letters[0] = 0;
    if(ps[0] > ' ')
    {
        country_letters[0] = '-';
        country_letters[1] = ps[0];
        country_letters[2] = ps[1];
        country_letters[3] = 0;
    }

    switch(SerialBuf[4])
    {
        case 'T': pt = "Transformer"; break;
        case 't': pt = "Transformer HF"; break;
        case 'N': pt = "Transformerless"; break;
        default:  pt = unknown;
    }
    switch(SerialBuf[5])
    {
        case 'W': pw = "Wind"; break;
        case 'N': pw = "Photovoltaic"; break;
        default:  pw = unknown;
    }

    fname = data_system_dir + wxString::Format(_T("/InverterInfo%d"), addr);
    if((f_inv = fopen(fname.mb_str(wxConvLocal), "w")) != NULL)
    {
        fprintf(f_inv, "\nPart Number:  %s\n\nSerial Number:  %s\n\n"
                "Firmware:  %c.%c.%c.%c\n\nManufacturing Date:  20%s Week %s\n\n"
                "Inverter Version:  -- %s%s --\n     -- %s, %s version --\n\nReference Standard:  %s\n\n%s\n\n",
                part_number, serial_number, firmware[0], firmware[1], firmware[2], firmware[3],
                manufacture_year, manufacture_week, model_name_buf, country_letters, pt, pw, &ps[3], pc);
        fclose(f_inv);
    }

    for(ix=0; ix<3; ix++)
    {
        // Note: read counter #3 resets 'partial energy'
        strcpy(SerialBuf, SerialBufSpaces);
        SerialBuf[0] = addr;
        SerialBuf[1] = 80;  // get counter
        SerialBuf[2] = ix;
        if(Communicate(1) <= 0) return(-1);

        inverters[inv].counters[ix] = ConvertLong(&SerialBuf[2]);
    }

    if(GetInverterTime(addr, NULL, &inverters[inv].time_offset) < 0)
        return(-1);
    return(0);
}

int GetState(int addr)
{//===================
    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 50;  // state request
    SerialBuf[2] = 0;
    if(Communicate(0) <= 0) return(-1);
    memcpy(ir->state, &SerialBuf[1], 5);
    return(0);
}

int SetInverterTime(int addr, int offset)
{//========================================
    time_t computer_time;
    time_t inverter_time;
    int diff;

    LogCommMsg(_T("Get time"));
    computer_time = time(NULL);

    inverter_time = computer_time + offset - (time_t)TimeBase;
    inverter_time += GetGmtOffset(0);

    inverter_time++;   // add 1 second for latency

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = addr;
    SerialBuf[1] = 71;   // set time/date
    SerialBuf[2] = inverter_time >> 24;
    SerialBuf[3] = inverter_time >> 16;
    SerialBuf[4] = inverter_time >> 8;
    SerialBuf[5] = inverter_time;
    SerialBuf[6] = 0;

    LogCommMsg(_T("Set time"));
    if(Communicate(1) <= 0)
    {
    LogCommMsg(_T("set time failed"));
        GetInverterTime(addr, NULL, NULL);
        return(-1);
    }
    LogCommMsg(_T("set time done"));

    GetInverterTime(addr, NULL, &diff);
    LogCommMsg(wxString::Format(_T("difference is now %d"), diff));
    return(0);
}

int GetAllDsp(int addr)
{//====================
    int ix;
    int dsp;
    FILE *f;
    const char *name;
    wxString fname = data_system_dir + _T("/read_all_dsp.txt");

    if((f = fopen(fname.mb_str(wxConvLocal),"w")) == NULL)
        return(-1);

    for(dsp=0; dsp<N_DSP; dsp++)
    {
        name = NULL;
        for(ix=1; ix<n_graphtypes; ix++)
        {
            if((dsp > 0) && (graph_types[ix].dsp_code == dsp))
            {
                name = graph_types[ix].name;
                break;
            }
        }

        for(ix=1; (name == NULL) && (extra_reading_types[ix].name != NULL); ix++)
        {
            if(extra_reading_types[ix].dsp_code == dsp)
                name = extra_reading_types[ix].name;
        }
        if(name == NULL)
            name = "";

        fprintf(f, "%2d  value %11f  %s\n", dsp, GetDSPdata(addr, dsp), name);
    }
    fclose(f);
    return(0);
}

int GetPowerIn(int addr)
{//=====================
    float p;
    float pw_in = 0;

    ir->vt[0] = GetDSPdata(addr, 1);
    if(comms_error != 0)
        return(-1);

    ir->vt[1] = GetDSPdata(addr, 23);
    ir->ct[1] = GetDSPdata(addr, 25);
    ir->pw[1] = ir->vt[1]*ir->ct[1];
    if(comms_error != 0)
        return(-1);

    ir->vt[2] = GetDSPdata(addr, 26);
    ir->ct[2] = GetDSPdata(addr, 27);
    ir->pw[2] = ir->vt[2]*ir->ct[2];
    if(comms_error != 0)
        return(-1);

    if((p = ir->vt[1] * ir->ct[1]) > 0)
        pw_in = p;
    if((p = ir->vt[2] * ir->ct[2]) > 0)
        pw_in += p;

    if(pw_in == 0)
        ir->effic = 0;
    else
        ir->effic = (ir->pw[0] * 100)/ pw_in;
    return(0);
}

int GetEnergy(int addr)
{//====================
    int value;
    value = GetCEdata(addr, 0) - energy_today_adjust;
    if(comms_error != 0)
        return(-1);
    ir->energy[0] = (double)value / 1000.0;

    ir->energy[1] = (double)GetCEdata(addr, 1) / 1000.0;
    if(comms_error != 0)
        return(-1);

    ir->energy[2] = (double)GetCEdata(addr, 3) / 1000.0;
    if(comms_error != 0)
        return(-1);

    ir->energy[3] = (double)GetCEdata(addr, 4) / 1000.0;
    if(comms_error != 0)
        return(-1);

    ir->energy[4] = (double)GetCEdata(addr, 5) / 1000.0;
    if(comms_error != 0)
        return(-1);

    ir->energy[5] = (double)GetCEdata(addr, 6) / 1000.0;
    if(comms_error != 0)
        return(-1);

    return(0);
}


void Energy10Sec_Finish()
{//======================
    int ix;
    FILE *f_out;
    FILE *f_10sec = NULL;
    time_t timeval = 0;
    struct tm *btime;
    double value;
    double total = 0;
    int n_values = 0;
    int prev_yday = -1;
    wxString date;
    wxString fname;
    char buf[100];

    f_out = fopen(retrieving_fname.mb_str(wxConvLocal), "w");
    if(f_out == NULL)
    {
        wxLogError(_T("Can't write to file: ") + retrieving_fname);
        return;
    }

    fname = data_dir  + _T("/system/energy_10sec");
    wxRemoveFile(fname);
    f_10sec = fopen(fname.mb_str(wxConvLocal), "w");

    for(ix=0; ix < CE_count; ix++)
    {
        if(CE_data[ix] == 0xffff)
        {
            // time follows in the next 2 entries
            timeval = (CE_data[ix+1] << 16) + CE_data[ix+2];
            timeval += TimeBase;
            timeval -= GetGmtOffset(timeval);
            timeval -= inverters[retrieving_inv].time_offset;

            if(retrieving_pvoutput)
            {
                btime = localtime(&timeval);
                if(btime->tm_yday != prev_yday)
                {
                    date = MakeDateString(wxString::Format(_T("%.4d%.2d%.2d"), btime->tm_year+1900, btime->tm_mon+1, btime->tm_mday));
                    strcpy(buf, date.mb_str(wxConvLocal));
                    fprintf(f_out, "\n%s\n", buf);
 //                   fprintf(f_out, "\n%.2d/%.2d/%.2d\n", btime->tm_mday, btime->tm_mon+1, btime->tm_year % 100);
                    prev_yday = btime->tm_yday;
                }
            }
            else
            {
                fprintf(f_out, "\n");
            }
            total = 0;
            n_values = 0;
            ix += 2;
            continue;
        }

        if(timeval > 0)
        {
            value = (double)CE_data[ix] * CE_scale / 1.003;
            total += value;
            n_values++;

            btime = localtime(&timeval);


            if(f_10sec != NULL)
            {
                fprintf(f_10sec, "%.4d%.2d%.2d %.2d:%.2d:%.2d %10.1f\n",
                        btime->tm_year+1900, btime->tm_mon+1, btime->tm_mday,
                        btime->tm_hour, btime->tm_min, btime->tm_sec,
                        value);
            }

            if(retrieving_pvoutput)
            {
                if(((btime->tm_min % 5) == 0) && (n_values > 6))
                {
                    // every 5 minutes, give the averaged power for the 5 minute period
                    fprintf(f_out, "%.2d:%.2d,,%6d\n",
                        btime->tm_hour, btime->tm_min, (int)(total/n_values));
                    total = 0;
                    n_values = 0;
                }
            }
            else
            {
                fprintf(f_out, "%.4d%.2d%.2d-%.2d:%.2d:%.2d %10.1f\n",
                        btime->tm_year+1900, btime->tm_mon+1, btime->tm_mday,
                        btime->tm_hour, btime->tm_min, btime->tm_sec,
                        value);
            }
            timeval += 10;
        }
    }

    fclose(f_out);
    if(f_10sec != NULL)
        fclose(f_10sec);
}

int Energy10Sec_Continue(int n)
{//============================
    int count;

    for(count=0; count < n; count++)
    {
        strcpy(SerialBuf, SerialBufSpaces);
        SerialBuf[0] = CE_inv_addr;
        SerialBuf[1] = 79;   // get Cumulated energy
        SerialBuf[2] = CE_next >> 8;
        SerialBuf[3] = CE_next;
        SerialBuf[4] = 0;
        if(Communicate(1) <= 0)
        {
            CE_errors++;
            return(-1);
        }

        CE_data[CE_count++] = ConvertShort(&SerialBuf[2]);
        CE_next += 2;
        if(CE_next < (TENSEC_BASE + TENSEC_MAX*2))
        {
            CE_data[CE_count++]  = ConvertShort(&SerialBuf[4]);
            CE_next += 2;
        }

        if(CE_next >= (TENSEC_BASE + TENSEC_MAX*2))
        {
            CE_next = TENSEC_BASE;
        }
        if(CE_count >= TENSEC_MAX)
        {
            retrieving_data = 0;
            Energy10Sec_Finish();
            retrieving_finished = 1;
            break;
        }
        retrieving_progress = 1 + (99 * CE_count) / TENSEC_MAX;
    }
    return(0);
}

int Energy10Sec_Start(int inv)
{//===========================
    MODEL_TABLE *pm;

    retrieving_inv = inv;
    CE_inv_addr = inverter_address[inv];
    CE_inv = inv;

    if(GetInverterTime(CE_inv_addr, NULL, &inverters[inv].time_offset) < 0)
        return(-1);

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = CE_inv_addr;
    SerialBuf[1] = 58;  // inverter version
    SerialBuf[2] = 0;
    if(Communicate(1) <= 0) return(-1);
    pm = LookupModel(model_names, SerialBuf[2] & 0xff);
    if((pm == NULL) || (pm->scale < 0))
    {
        return(-10);  // not for this model
    }
    CE_scale = pm->scale;

    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = CE_inv_addr;
    SerialBuf[1] = 75;
    SerialBuf[2] = 1;
    if(Communicate(1) <= 0) return(-1);
    CE_start = ConvertShort(&SerialBuf[2]);

    CE_data = (unsigned short *)realloc(CE_data, (TENSEC_MAX+1) * sizeof(unsigned short));
    CE_next = CE_start;
    CE_count = 0;
    retrieving_data = RETRIEVE_10SEC;
    CE_errors = 0;

    Energy10Sec_Continue(10);
    return(0);
}


void EnergyDaily_Finish()
{//======================
// all the daily energy data has been retrieved
    int ix;
    FILE *f_out;
    FILE *f_daily = NULL;
    int j_today;      // modified julian date for "today"
    int date_offset;  // offset to convert inverter date-codes to a modified julian date
    int j_date;
    int count;
    int value;
    int datecode_today;  // the inverter's date-code for "today"
    int datecode_prev;
    wxDateTime today;
    wxDateTime date;
    wxString fname;
    char date_string[40];
    char value_string[20];
    int jdates[DAILY_NDAYS];

    memset(jdates, 0, sizeof(jdates));

    // open the output text file
    f_out = fopen(retrieving_fname.mb_str(wxConvLocal), "w");
    if(f_out == NULL)
    {
        wxLogError(_T("Can't write to file: ") + retrieving_fname);
        return;
    }

    datecode_today = datecode_prev = CE_data[CE_today_ix];

    today = wxDateTime::Now();
    j_today = (int)today.GetModifiedJulianDayNumber();
    date_offset = datecode_today - j_today;  // offset to convert inverter date-codes to a modified julian date

    if(CE_verbose)
        fprintf(f_out, "Date        Value    Addr   Date_code\n");


    ix = CE_today_ix;
    for(count=0; count < DAILY_NDAYS; count++, ix-=2)
    {
        if(ix < 0)
        {
            // wrap around to the end of the memory area
            ix += DAILY_NDAYS*2;
            if(CE_verbose)
                fprintf(f_out, "#                    ====\n");
        }

        if((value = CE_data[ix+1]) == 0xffff)
            strcpy(value_string, "    -1");
        else
            sprintf(value_string, "%6d", value);


        j_date = CE_data[ix] - date_offset;
        date = wxDateTime((double)j_date + 2400000.5);   // 2400000.5 converts a modified julian date to a julian date
        strcpy(date_string, date.FormatISODate().mb_str(wxConvLocal));   // make date string as ISO  yyyy-mm-dd

        if((CE_data[ix] == 0xffff) || (CE_data[ix] > datecode_today) || (CE_data[ix+1] == 0) || ((datecode_prev - CE_data[ix]) > 50) || (value == 0xffff))
        {
            // date is later than today, or has no reading, or is more than 50 days earlier than the last reading
            if(CE_verbose)
            {
                fprintf(f_out,"           %s    %.4x  %5d", value_string, DAILY_BASE+ix*2, CE_data[ix]);
                if(CE_data[ix] <= datecode_today)
                    fprintf(f_out, " (%s)", date_string);
                fputc('\n', f_out);
            }
            continue;
        }

        jdates[ix/2] = j_date;
        datecode_prev = CE_data[ix];

        if(CE_verbose)
            fprintf(f_out,"%s %s    %.4x  %5d\n", date_string, value_string, DAILY_BASE+ix*2, CE_data[ix]);
        else
            fprintf(f_out,"%s  %7.3f kWh\n", date_string, (double)value/1000);

    }

    fclose(f_out);


    // write a list of dates and energies in ascending order (excluding today)
    fname = data_dir  + _T("/system/energy_daily");
    wxRemoveFile(fname);
    f_daily = fopen(fname.mb_str(wxConvLocal), "w");

    if(f_daily != NULL)
    {
        ix = CE_today_ix + 2;
        for(count=1; count < DAILY_NDAYS; count++, ix+=2)
        {
            if(ix >= DAILY_NDAYS*2)
                ix = 0;

            if(jdates[ix/2] > 0)
            {
                date = wxDateTime((double)jdates[ix/2] + 2400000.5);   // 2400000.5 converts a modified julian date to a julian date
                strcpy(date_string, date.FormatISODate().mb_str(wxConvLocal));   // make date string as ISO  yyyy-mm-dd
                fprintf(f_daily,"%s  %7.3f\n", date_string, (double)CE_data[ix+1]/1000);
            }
        }
        fclose(f_daily);
    }
}

int EnergyDaily_Continue()
{//=======================
    int count;

    // read a batch of 10 messages, and then return to allow
    // the regular inverter readings to continue
    for(count=0; count < 10; count++)
    {
        if(CE_next >= (DAILY_BASE + DAILY_NDAYS*4))
        {
            retrieving_data = 0;
            EnergyDaily_Finish();
            retrieving_finished = 1;
            break;
        }

        // each message reads one pair of date-code and energy value
        strcpy(SerialBuf, SerialBufSpaces);
        SerialBuf[0] = CE_inv_addr;
        SerialBuf[1] = 79;   // get Cumulated energy
        SerialBuf[2] = CE_next >> 8;
        SerialBuf[3] = CE_next;
        SerialBuf[4] = 0;
        if(Communicate(1) <= 0)
        {
            CE_errors++;
            return(-1);
        }

        // store the data, to be processed later by EnergyDaily_Finished()
        CE_data[CE_count] = ConvertShort(&SerialBuf[2]);    // date-code
        CE_data[CE_count+1] = ConvertShort(&SerialBuf[4]);  // energy

        if(CE_next == CE_start)
        {
            CE_today_ix = CE_count;
        }
        CE_next += 4;
        CE_count += 2;
        retrieving_progress = (100 * CE_count) / (DAILY_NDAYS*2);  // increment percentage value for the progress indicator
    }
    return(0);
}

int EnergyDaily_Start(int inv)
{//===========================
    retrieving_inv = inv;
    CE_inv_addr = inverter_address[inv];

    // find the start address in the memory, for todays value
    strcpy(SerialBuf, SerialBufSpaces);
    SerialBuf[0] = CE_inv_addr;
    SerialBuf[1] = 75;
    SerialBuf[2] = 2;
    if(Communicate(1) <= 0) return(-1);
    CE_start = ConvertShort(&SerialBuf[2]);
    CE_verbose = dlg_retrieve_energy->option_verbose;

    CE_data = (unsigned short *)realloc(CE_data, (DAILY_NDAYS*2) * sizeof(unsigned short));
    CE_next = DAILY_BASE;
    CE_count = 0;
    retrieving_data = RETRIEVE_DAILY;
    CE_errors = 0;

    EnergyDaily_Continue();
    return(0);
}


int GetInverterData(int inv, int type)
{//===================================
    int ix;
    int dsp_param;
    int n_readings;
    int addr;

    addr = inverter_address[inv];
    comms_error = 0;
    ir->flags = 0;

    // always get the power output
    ir->pw[0] = GetDSPdata(addr, 3);
    if(comms_error != 0)
        return(-1);

    if(retrieving_data == RETRIEVE_10SEC)
    {
        if(type != cmdExtraDsp)
        {
            // only read the 10-second energy data, (except on 10 second periods)
            return(Energy10Sec_Continue(12));
        }
    }
    else
    if(retrieving_data == RETRIEVE_DAILY)
    {
        EnergyDaily_Continue();
    }

    switch(type)
    {
    case 0:
        if(GetPowerIn(addr) < 0)
            break;
        ir->tempr[0] = GetDSPdata(addr, 21);  // Booster temperature (usually higher than Inverter temperature)
        GetState(addr);
        ir->flags = IR_HAS_POWERIN | IR_HAS_TEMPR | IR_HAS_STATE;
        break;

    case 1:
        if(GetPowerIn(addr) < 0)
            break;
        ir->tempr[1] = GetDSPdata(addr, 22);
        GetState(addr);
        ir->flags = IR_HAS_POWERIN | IR_HAS_TEMPR | IR_HAS_STATE;
        break;

    case 2:
        if(GetEnergy(addr) < 0)
            break;
        ir->tempr[0] = GetDSPdata(addr, 21);
        GetState(addr);
        ir->flags = IR_HAS_ENERGY | IR_HAS_ENERGY_TODAY | IR_HAS_TEMPR | IR_HAS_STATE;

        if(inverters[inv].need_time_offset == 1)
        {
            if(GetInverterTime(addr, NULL, &inverters[inv].time_offset) == 0)
            {
                ir->flags |= IR_HAS_TIME;
                inverters[inv].need_time_offset = 0;
            }
        }
        break;

    case 3:
        if(GetPowerIn(addr) < 0)
            break;
        ir->tempr[1] = GetDSPdata(addr, 22);
        GetState(addr);
        ir->flags = IR_HAS_POWERIN | IR_HAS_TEMPR | IR_HAS_STATE;
        break;

    case 4:
        if(GetPowerIn(addr) < 0)
            break;
        GetState(addr);
        ir->energy[0] = (double)(GetCEdata(addr, 0) - energy_today_adjust) / 1000.0;  // today energy
        ir->flags = IR_HAS_POWERIN | IR_HAS_STATE | IR_HAS_ENERGY_TODAY;
        break;

    case 5:
        GetState(addr);
        inverter_thread->Sleep(200);
        ir->flags = IR_HAS_STATE;
        break;

    case cmdExtraDsp:  // get extra DSP data, or every 10 seconds while retrieving 10_second energy data
    case cmdExtraDsp2: // every 1 minute
        ir->flags = IR_HAS_EXTRA;
        n_readings = 0;
        for(ix=0; ix < N_EXTRA_READINGS; ix++)
        {
            if((dsp_param = extra_readings[ix].dsp_code) != 0)
            {
                if((extra_readings[ix].status_slot != 0) || (extra_readings[ix].graph_slot != 0))
                {
                    n_readings++;
                    if(dsp_param == 64)
                    {
                        // energy total for today
                        ir->energy[0] = (double)(GetCEdata(addr, 0) - energy_today_adjust)/ 1000.0;  // today energy
                        ir->dsp[64] = ir->energy[0];
                        ir->flags |= IR_HAS_ENERGY_TODAY;
                    }
                    else
                    {
                        ir->dsp[dsp_param] = GetDSPdata(addr, dsp_param);
                    }
                }
            }
        }

        if(type == cmdExtraDsp2)
        {
            // read the Peak Power Today every 1 minute
            ir->dsp[DSP_PEAK_TODAY] = GetDSPdata(addr, DSP_PEAK_TODAY);
            ir->flags |= IR_HAS_PEAK;
            n_readings++;
        }

        if(retrieving_data == RETRIEVE_10SEC)
        {
            // retrieving 10_second energy data,  Read all every 10 seconds
            if(GetPowerIn(addr) < 0)
                break;
            ir->flags |= IR_HAS_POWERIN;
            n_readings = 0;
        }

        if(n_readings < 7)
        {
            GetState(addr);
            ir->flags |= IR_HAS_STATE;
        }
        if(n_readings < 6)
        {
            ir->tempr[0] = GetDSPdata(addr, 21);
            ir->flags |= IR_HAS_TEMPR;
        }
        if(n_readings < 5)
        {
            ir->tempr[1] = GetDSPdata(addr, 22);
        }
        if((n_readings < 4) && !(ir->flags & IR_HAS_ENERGY_TODAY))
        {
            ir->energy[0] = (double)(GetCEdata(addr, 0) - energy_today_adjust)/ 1000.0;  // today energy
            ir->flags |= IR_HAS_ENERGY_TODAY;
        }
        break;
    }

    if(comms_error != 0)
    {
        ir->flags = 0;
        return(-1);
    }
    return(0);
}


void UpdatePowerData2(const char *fname, char *periods, float *powers)
{//===================================================================
// Append retrieved power data to a data file
    int ix;
    int k;
    FILE *f_out;
    int first_period = N_10SEC;
    int prev_period = 0;
    int next_period = 0;

    if(fname[0] == 0)
        return;

    if((f_out = fopen(fname, "a")) == NULL)
        return;

    ix = 0;
    while(ix < N_10SEC)
    {
        if(periods[ix] == 1)
        {
            // This period has original data
            if(ix < first_period)
                first_period = ix;
            prev_period = ix;
        }

        if(periods[ix] == 2)
        {
            // we have retrieved data, but no original data, for this 10-second period
            // Fill the hole with the retrieved data, but leave a gap at each side

            next_period = N_10SEC;
            for(k=ix; k<N_10SEC; k++)
            {
                if(periods[k] == 1)
                {
                    next_period = k;
                    break;
                }
            }

            while(ix < (next_period-(MAX_GAP/2)))
            {
                if((periods[ix] == 2) && ((ix > (prev_period+(MAX_GAP/2))) || (ix < first_period)))
                {
                    fprintf(f_out, "%.2d:%.2d:%.2d\t%6.1f\n", ix/360, (ix/6) % 60, (ix % 6)*10, powers[ix]);
                }
                ix++;
            }
        }
        ix++;
    }

    fclose(f_out);
}

void UpdatePowerData(wxString fname10sec)
{//======================================
    // check whether the 10sec data contains power readings which are missing from the d*_yyyymmdd file

    FILE *f_10sec;
    FILE *f_in;
    int hour, min, sec;
    int t_hour, t_min, t_sec;
    int period;
    float power;
    char date[20];
    char year[5];
    char timestr[12];
    char fname[200];
    char current_date[20];
    char buf[80];
    char periods[N_10SEC];
    float powers[N_10SEC];

    if((f_10sec = fopen(fname10sec.mb_str(wxConvLocal), "r")) == NULL)
        return;

    fname[0] = 0;
    current_date[0] = 0;

    // close today's data log while we update
    if(inverters[CE_inv].f_log != NULL)
    {
        fclose(inverters[CE_inv].f_log);
        inverters[CE_inv].f_log = NULL;
    }

    // read the 10-second retrieved energy
    while(fgets(buf, sizeof(buf), f_10sec) != NULL)
    {
        if(sscanf(buf, "%s%d:%d:%d %f", date, &hour, &min, &sec, &power) != 5)
            continue;

        if(strcmp(date, current_date) != 0)
        {
            // a change in date, update the data file for the previous data (if any)
            UpdatePowerData2(fname, periods, powers);
            strcpy(current_date, date);

            // read the original data for this new date
            memcpy(year, date, 4);
            year[4] = 0;

 // NOTE: sprintf(fname, "%ls", data_dir.c_str()) doesn't work on Windows
            strcpy(fname, data_dir.mb_str(wxConvLocal));
            sprintf(&fname[strlen(fname)], "/%s/d%d_%s.txt", year, CE_inv_addr, date);

            memset(periods, 0, sizeof(periods));
            if((f_in = fopen(fname, "r")) != NULL)
            {
                while(fgets(timestr, sizeof(timestr), f_in) != NULL)
                {
                    if(sscanf(timestr, "%d:%d:%d", &t_hour, &t_min, &t_sec) == 3)
                    {
                        period = t_hour*360 + t_min*6 + t_sec/10;
                        periods[period] = 1;
                    }
                }
                fclose(f_in);
            }
            else
            {
                // create an empty data file for this date
                if((f_in = fopen(fname, "w")) != NULL)
                {
                    fprintf(f_in, "AUR1 %s #%d\n", date, inverter_address[CE_inv]);
                    fclose(f_in);
                }
            }
        }

        period = hour*360 + min*6 + sec/10;
        if(periods[period] == 0)
        {
            // there was no original data for this period, but we have retrieved data
            periods[period] = 2;
            powers[period] = power;
        }
    }
    fclose(f_10sec);
    UpdatePowerData2(fname, periods, powers);

    // read and display today's data
    OpenLogFiles(-1, 1);
}


void Mainframe::OnInverterEvent(wxCommandEvent &event)
{//===================================================
    int data_ok;
    int inv_info = -1;
    int period_type = 0;
    wxString fname;
    static int next_command[N_INV] = {0, 0};

    if(retrieving_finished != 0)
    {
        LogMessage(wxString::Format(_T("Retrieve %s complete"), retrieve_message.c_str()), 1);

        fname = data_dir + _T("/system/energy_10sec");
        if(wxFileExists(fname))
        {
            // 10-second energy data has been retrieved from an inverter
            UpdatePowerData(fname);
            wxRemoveFile(fname);
        }

        fname = data_dir + _T("/system/energy_daily");
        if(wxFileExists(fname))
        {
            // daily energy data has been retrieved from an inverter
            UpdateDailyData(CE_inv, fname);
            wxRemoveFile(fname);
        }
    }

    if(progress_retrieval != NULL)
    {
        if(retrieving_data != 0)
        {
            if(progress_retrieval->Update(retrieving_progress) == false)
            {
                retrieving_data = 0;  // abort
                delete progress_retrieval;
                progress_retrieval = NULL;
            }
        }
        else
        if(retrieving_finished != 0)
        {
            delete progress_retrieval;
            progress_retrieval = NULL;
            retrieving_finished = 0;
        }
    }
    else
    {
        if(retrieving_data != 0)
        {
            LogMessage(wxString::Format(_T("Retrieving %s %2d%%%%"), retrieve_message.c_str(), retrieving_progress), 0);
        }
        else
        if(retrieving_finished != 0)
        {
            retrieving_finished = 0;
        }
    }

    data_ok = 0;
    switch(event.GetId())
    {
    case idInverterData:
        data_ok = 1;
        if(inverters[command_inverter].alive == 0)
        {
            // start with a request which reads temperature (so we don't start with an unknown temperature value)
            next_command[0] = next_command[1] = 0;
        }
        inverters[command_inverter].alive = 1;
        break;

    case idInverterModelError:
        wxLogError(_T("Sorry, these statistics are not available for this model of Inverter at the moment."));
        break;

    case idInverterFail:
        inverters[command_inverter].alive = 0;

        if((command_type == cmdSetInvTime2) || (command_type == cmdResetPartial))
        {
            wxLogError(wxString::Format(_T("No response from inverter #%d"), inverter_address[command_inverter]));
        }
        break;
    }

    if(command_type == cmdInverterInfo)
    {
        inv_info = command_inverter;
    }
    else
    {
        period_type = DataResponse(data_ok, &inverter_response[command_inverter]);
        if(period_type > 0)
        {
            if((period_type > 1) || (n_extra_readings > 0) || (retrieving_data == RETRIEVE_10SEC))
            {
                // get the extra dsp readings at the start of a 10 second period
                inverters[command_inverter].get_extra_dsp = period_type;
            }
        }
    }

    DrawStatusPulse(command_inverter+1);   // set bit 0 or bit 1
    if(serial_port_error == 0)
    {
        pulsetimer.Start(160, wxTIMER_ONE_SHOT);
    }

    if(command_queue != 0)
    {
        // a special command has been requested
        SendCommand(command_queue & 0xf, command_queue >> 8);
        command_queue = 0;
    }
    else
    {
        int inv;
        // change to the next inverter
        inv = (command_inverter+1) % 2;
        if(inverter_address[inv] == 0)
            inv = command_inverter;

        if((inverters[inv].alive == 0) || (inverter_response[inv].state[1] != INVERTER_STATE_RUN))
        {
            SendCommand(inv, 5);  // start with just alarm and power-out
        }
        else
        if(inverters[inv].get_extra_dsp > 0)
        {
            if(inverters[inv].get_extra_dsp > 1)
                SendCommand(inv, cmdExtraDsp2);
            else
                SendCommand(inv, cmdExtraDsp);
            inverters[inv].get_extra_dsp = 0;
        }
        else
        {
            if(next_command[inv] >= 5)
                next_command[inv] = 0;
            SendCommand(inv, next_command[inv]);
            next_command[inv]++;
        }
    }

    if(inv_info >= 0)
    {
        // do this after sending the pulse and the next command, so the modal dialog doesn't pause them
        GotInverterInfo(inv_info, data_ok);
    }
}  // end of OnInverterEvent



void SendCommand(int inv, int type)
{//=================================
    wxThreadError error;

    command_type = type;
    command_inverter = inv;
    //return;  // TESTING
    // wxLogStatus(wxString::Format(_T("SendCommand %d  %d  live=%d"), inv, type, inverters[0].alive));
    // LogMessage(wxString::Format(_T("SendCommand %d  %d  live=%d"), inv, type, inverters[0].alive), 1);
    if((serial_port_error != 0) && (serial_port[0] != 0))
    {
        if(serial_port_error == -1)
        {
            LogMessage(wxString::Format(_T("Can't find serial port: "), serial_port_error) + serial_port, 0);
        }
        else
        if(serial_port_error == -2)
        {
            LogMessage(wxString::Format(_T("Failed to open (%d) serial port: "), serial_port_error) + serial_port, 0);
        }
   }

    inverter_thread = new InverterThread();
    error = inverter_thread->Create();
    if(error != wxTHREAD_NO_ERROR)
    {
		LogMessage(_T("Failed to create inverter thread"), 1);
    }
    error = inverter_thread->Run();
    if(error != wxTHREAD_NO_ERROR)
    {
        LogMessage(_T("Failed to run inverter thread"), 1);
    }

}

void *InverterThread::Entry()
{//==========================
    int addr;
    int type;
    int result;

    result = SerialOpen();
    serial_port_error = result;

    if(result != 0)
    {
        // pause so that we don't run continuously trying to open the port
        Sleep(2000);   // thread sleep
    }
    else
    {
        comms_inverter = command_inverter;
        addr = inverter_address[comms_inverter];
        type = command_type;

        ir = &inverter_response[comms_inverter];
        ir->flags = 0;

        switch(type)
        {
        case cmdInverterInfo:
            // write information to a temporary file
            result = GetInverterInfo(comms_inverter);
            break;

        case cmdResetPartial:
            result = ResetPartial(comms_inverter);
            break;

        case cmdInverter10SecEnergy:
            result = Energy10Sec_Start(comms_inverter);
            if(result != 0)
                retrieving_finished = 2;
            break;

        case cmdInverterDailyEnergy:
            result = EnergyDaily_Start(comms_inverter);
            if(result != 0)
                retrieving_finished = 2;
            break;

        case cmdGetInvTime:
            result = GetInverterTime(addr, &ir->time, &inverters[comms_inverter].time_offset);
            inverters[comms_inverter].need_time_offset = result;
            break;

        case cmdSetInvTime:
            result = SetInverterTime(addr, 0);
            break;

        case cmdSetInvTime2:
            result = SetInverterTime(addr, settime_offset);
            break;

        default:
            result = GetInverterData(comms_inverter, type);
            break;
        }
        SerialClose();
    }

    if(result == 0)
    {
        inverters[comms_inverter].fails = 0;
        my_event.SetId(idInverterData);
    }
    else
    if(result == -10)
    {
        my_event.SetId(idInverterModelError);
    }
    else
    {
        my_event.SetId(idInverterFail);
        inverters[comms_inverter].fails++;
    }

    wxPostEvent(mainframe->GetEventHandler(), my_event);
    return(NULL);
}

void InitComms()
{//=============

    memset(inverter_response, 0, sizeof(inverter_response));
    inverter_response[0].state[0] = 0xff;
    inverter_response[1].state[0] = 0xff;

}
