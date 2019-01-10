//---------------------------------------------------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol
*/
//---------------------------------------------------------------------------------------------------------------------
#ifndef SLAY2_H
#define SLAY2_H

/* -- Includes ------------------------------------------------------------ */
#include <string.h>
#include "slay2_buffer.h"
#include "slay2_scheduler.h"

/* -- Defines ------------------------------------------------------------- */
#define SLAY2_NUM_CHANNELS    (8)

/* -- Types --------------------------------------------------------------- */
typedef void (*Slay2Receiver)(void * const obj, const unsigned char * const data, const unsigned int len);


class Slay2Channel; //forward declaration


class Slay2
{
public:
   Slay2();
   ~Slay2();
   void task(void);

   Slay2Channel * open(const unsigned int channel);
   void close(Slay2Channel * const channel);

   //this function must be implemented (in a derived class)
   virtual unsigned long getTime1ms(void) = 0; //utility function. probably others can utilize it too

protected:
   //this functions must be implemented (in a derived class) to connect to a hardware/plattform...
   virtual unsigned int getTxCount(void) = 0;
   virtual int transmit(const unsigned char * data, unsigned int len) = 0;
   //virtual unsigned int getRxCount(void) = 0;
   virtual int receive(unsigned char * buffer, unsigned int size) = 0;

private:
   void doReception(void);
   void doTransmission(void);

   Slay2Channel * channels[SLAY2_NUM_CHANNELS];
   bool syncSent;
   unsigned int syncCount;
   Slay2TxScheduler txScheduler;
   Slay2AckDecodingBuffer rxAckDecoder;
   Slay2DataDecodingBuffer rxDataDecoder;
   unsigned char nextExpRxSeqNr;  //expected sequence number of next received data frame!
};



class Slay2Channel
{
   friend class Slay2; //Slay2 is my friend. so this class is allowed to access my private methods/members
   friend class Slay2TxScheduler; //Slay2TxScheduler is my friend. so this class is allowed to access my private methods/members

public:
   void setReceiver(const Slay2Receiver receiver, void * const obj=NULL);
   int send(const unsigned char * data, const unsigned int len, const bool more=false);

private:
   //private constructor and destructor to prevent user from dynamic creaton/deletion of Slay2ch objects
   Slay2Channel(const unsigned int channel);
   unsigned int channel;
   Slay2Receiver receiver;
   void * receiverObj;
   Slay2Fifo txFifo;
   bool txMore;
};






/* -- Global Variables ---------------------------------------------------- */

/* -- Function Prototypes ------------------------------------------------- */

/* -- Implementation ------------------------------------------------------ */



#endif
