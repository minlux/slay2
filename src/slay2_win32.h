//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Win32 implementation.

   Requires POSIX threads to be linkes (Linker option: -lpthread)
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_WIN32_H
#define SLAY2_WIN32_H

/* -- Includes ------------------------------------------------------------ */
#include <windows.h>
#include "slay2.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */
class Slay2Win32 : public Slay2
{
public:
   Slay2Win32();
   ~Slay2Win32();
   bool init(const char * serPortName, const unsigned int baudrate); //for baudrates slower than 115k2 the "SLAY2_TRANSMISSION_TIMEOUT" value must probably be adjusted (via global preprocessor define!!!)
   void shutdown(void);

   unsigned int getTime1ms(void);

   void enterCritical(void);
   void leaveCritical(void);

protected: //normally protected. for testing purpose, these functions may be made public
   unsigned int getTxCount(void);
   int transmit(const unsigned char * data, unsigned int len);
   unsigned int getRxCount(void);
   int receive(unsigned char * buffer, unsigned int size);

private:
   void flush(void);

   void * fileHandle;
   CRITICAL_SECTION critical;
};


/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
