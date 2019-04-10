//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Win32 implementation.

   See https://www.xanthium.in/Serial-Port-Programming-using-Win32-API for
   further information.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "slay2_win32.h"
#include "slay2_buffer.h"


/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */

/* -- Implementation ------------------------------------------------------ */

Slay2Win32::Slay2Win32()
{
   fileHandle = INVALID_HANDLE_VALUE;
   InitializeCriticalSection(&critical); //init critical section for thread synchronization
}

Slay2Win32::~Slay2Win32()
{
   shutdown();
}


bool Slay2Win32::init(const char * serPortName)
{
   //open serial COM port
   fileHandle = CreateFile(
      serPortName,                  //COM1-9
      GENERIC_READ | GENERIC_WRITE, //Read/Write
      0,                            //No Sharing
      NULL,                         //No Security
      OPEN_EXISTING,                //Open existing port only
      0,                            //Non overlapped I/O
      NULL);

   //configure serial settings
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      DCB dcbSerialParams = { 0 };
      COMMTIMEOUTS timeouts = { 0 };

      dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
      //get current settings
      GetCommState(fileHandle, &dcbSerialParams);
      //adjust settings
      dcbSerialParams.BaudRate = CBR_115200;
      dcbSerialParams.ByteSize = 8;
      dcbSerialParams.StopBits = ONESTOPBIT;
      dcbSerialParams.Parity   = NOPARITY;
     //control flags
      dcbSerialParams.fBinary = 1;
      dcbSerialParams.fParity = 0;
      dcbSerialParams.fOutxCtsFlow = 0;
      dcbSerialParams.fOutxDsrFlow = 0;
      dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE; //0x01: enable the DTR line when the device is opened and leaves it on
      dcbSerialParams.fDsrSensitivity = 0;
      dcbSerialParams.fOutX = 0; //don't use sw flow control
      dcbSerialParams.fInX = 0; //don't use sw flow control
      dcbSerialParams.fErrorChar = 0;
      dcbSerialParams.fNull = 0; //don't discard NULL bytes
      dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE; //0x01: enables the RTS line when the dives is opend and leaves it on
      dcbSerialParams.fAbortOnError = 0;

      //write back settings
      SetCommState(fileHandle, &dcbSerialParams);

      //set timeouts
      // Look up SetCommTimeouts and the COMMTIMEOUTS structure in the API reference.
      // The description of the ReadIntervalTimeout structure member contains the following paragraph:
         // A value of MAXDWORD, combined with zero values for both the  ReadTotalTimeoutConstant and
         // ReadTotalTimeoutMultiplier members, specifies that the read operation is to return immediately with
         // the number of characters that have already been received, even if no characters have been received.
      timeouts.ReadIntervalTimeout = MAXDWORD;
      SetCommTimeouts(fileHandle, &timeouts);
      return true; //success
   }

   //failed to init
   return false;
}


void Slay2Win32::shutdown(void)
{
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      CloseHandle(fileHandle);
      fileHandle = INVALID_HANDLE_VALUE;
   }
}


unsigned int Slay2Win32::getTime1ms(void)
{
   return GetTickCount();
}


void Slay2Win32::enterCritical(void)
{
   EnterCriticalSection(&critical);
}

void Slay2Win32::leaveCritical(void)
{
   LeaveCriticalSection(&critical);
}


unsigned int Slay2Win32::getTxCount(void)
{
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      unsigned int success;
      unsigned long errors;
      COMSTAT status;
      //retrieve status information, using the clear comm error function
      success = ClearCommError(fileHandle, &errors, &status);
      if (success)
      {
         return status.cbOutQue;
      }
   }
   return 0;
}

int Slay2Win32::transmit(const unsigned char * data, unsigned int len)
{
   unsigned long wr = 0;
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      WriteFile(fileHandle, data, len, &wr, NULL);
   }
   return wr;
}


unsigned int Slay2Win32::getRxCount(void)
{
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      unsigned int success;
      unsigned long errors;
      COMSTAT status;
      //retrieve status information, using the clear comm error function
      success = ClearCommError(fileHandle, &errors, &status);
      if (success)
      {
         return status.cbInQue;
      }
   }
   return 0;
}

int Slay2Win32::receive(unsigned char * buffer, unsigned int size)
{
   unsigned long rd = 0;
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      ReadFile(fileHandle, buffer, size, &rd, NULL);
   }
   return rd;
}


void Slay2Win32::flush(void)
{
   if (fileHandle != INVALID_HANDLE_VALUE)
   {
      PurgeComm(fileHandle, PURGE_TXCLEAR | PURGE_RXCLEAR);
   }
}