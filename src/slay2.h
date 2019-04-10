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
#define SLAY2_NUM_CHANNELS    (8) //up to 256 channels are possible

/* -- Types --------------------------------------------------------------- */
typedef void (*Slay2Receiver)(void * const obj, const unsigned char * const data, const unsigned int len);


class Slay2Channel; //forward declaration


class Slay2
{
public:
   Slay2();
   ~Slay2();        //this also delets all open channels
   void task(void); //must be called cyclically
   void setVerbose(void);

   Slay2Channel * open(const unsigned int channel); //returns NULL, if channel number of of range, or channel is already open
   void close(Slay2Channel * const channel); //this deletes the object pointed by channel

   //this function must be implemented (in a derived class)
   virtual unsigned int getTime1ms(void) = 0; //public utility function. probably others can utilize it too
   //synchronization primitives. must have recursive ownership feature. must be implemented in a derived class
   virtual void enterCritical(void) = 0;
   virtual void leaveCritical(void) = 0;

protected:
   //this functions must be implemented (in a derived class) to connect to a hardware/plattform...
   virtual unsigned int getTxCount(void) = 0; //return number of bytes in TX buffer
   virtual int transmit(const unsigned char * data, unsigned int len) = 0; //return number of written bytes
   //virtual unsigned int getRxCount(void) = 0; //return number of bytes in RX buffer
   virtual int receive(unsigned char * buffer, unsigned int size) = 0; //return number of read bytes

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
   bool verbose;
};



class Slay2Channel
{
   friend class Slay2; //Slay2 is my friend. so this class is allowed to access my private methods/members
   friend class Slay2TxScheduler; //Slay2TxScheduler is my friend. so this class is allowed to access my private methods/members

public:
   void setReceiver(const Slay2Receiver receiver, void * const obj=NULL);
   int send(const unsigned char * data, const unsigned int len, const bool more=false);
   unsigned int getTxBufferSize();
   unsigned int getTxBufferSpace();
   void flushTxBuffer();
   //synchronization primitives
   void enterCritical();
   void leaveCritical();

private:
   //private constructor to prevent user from dynamic creaton of Slay2Channel objects (Slay2.open shall be used therefore)
   Slay2Channel(Slay2 * const slay2, const unsigned int channel);
   Slay2 * slay2;
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
