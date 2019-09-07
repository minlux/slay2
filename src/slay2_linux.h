//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol. Linux implementation.
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_LINUX_H
#define SLAY2_LINUX_H

/* -- Includes ------------------------------------------------------------ */
#include <pthread.h>
#include "slay2.h"

/* -- Defines ------------------------------------------------------------- */

/* -- Types --------------------------------------------------------------- */
class Slay2Linux : public Slay2
{
public:
   Slay2Linux();
   ~Slay2Linux();
   bool init(const char * dev, const unsigned int baudrate); //for baudrates slower than 115k2 the "SLAY2_TRANSMISSION_TIMEOUT" value must probably be adjusted (via global preprocessor define!!!)
   void shutdown(void);

   unsigned int getTime1ms(void);

   void enterCritical(void);
   void leaveCritical(void);

// protected: //normally protected. for testing purpose, these functions may be made public
   unsigned int getTxCount(void);
   int transmit(const unsigned char * data, unsigned int len);
   unsigned int getRxCount(void);
   int receive(unsigned char * buffer, unsigned int size);

private:
   int setInterfaceAttribs(unsigned int baudrate);
   unsigned int encodeBaudrate(unsigned int baudrate);
   void flush(void);

   int fileDesc;
   pthread_mutex_t mutex;
};


/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
