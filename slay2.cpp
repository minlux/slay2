//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol.
*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
// #include <iostream>
#include "slay2.h"


/* -- Defines ------------------------------------------------------------- */
// using namespace std;


/* -- Types --------------------------------------------------------------- */

/* -- (Module) Global Variables ------------------------------------------- */

/* -- Module Global Function Prototypes ----------------------------------- */


/* -- Implementation ------------------------------------------------------ */

Slay2::Slay2()
{
   for (unsigned int channel = 0; channel < SLAY2_NUM_CHANNELS; ++channel)
   {
      this->channels[channel] = NULL;
   }
   syncSent = false;
   syncCount = 0;
   nextExpRxSeqNr = 0;
}


Slay2::~Slay2()
{
   //close all channels
   for (unsigned int channel = 0; channel < SLAY2_NUM_CHANNELS; ++channel)
   {
      Slay2Channel * const ch = this->channels[channel];
      if (ch != NULL)
      {
         this->channels[channel] = NULL;
         delete ch;
      }
   }
}


void Slay2::task(void)
{
   //on startup
   if (syncSent == false)
   {
      const unsigned char syncSequence[5] = { SLAY2_SYNC, SLAY2_SYNC, SLAY2_SYNC, SLAY2_SYNC, SLAY2_SYNC };
      if (transmit(syncSequence, 5) >= 5) //send 5 sync chars to get in synchronisation with the remote endpoint
      {
         txScheduler.reset();
         rxAckDecoder.flush();
         rxDataDecoder.flush();
         nextExpRxSeqNr = 0;
         syncSent = true;
      }
   }
   doReception();
   doTransmission();
}


void Slay2::doReception(void)
{
   unsigned char rxBuffer;
   while (this->receive(&rxBuffer, 1) > 0)
   {
      //SYN
      if (rxBuffer == SLAY2_SYNC)
      {
         ++syncCount;
         if (syncCount >= 3)
         {
            syncCount = 0;
            //a consecutive receive sequence of 3 or more SYNC chars, leads to clear the "receive sequence lock"
            //as a consequence of that, the receiver does not longer expects the next frame to has a sequence
            //number of "one more than the previous".
            //this is required to get in sync with the remote station.
            //at startup a remote station shall tranmit 5 (or more) SYNC chars for synchronisation;
            txScheduler.reset();
            rxAckDecoder.flush();
            rxDataDecoder.flush();
            nextExpRxSeqNr = 0;
         }
         continue;
      }
      syncCount = 0;

      //ACK
      if (Slay2AckDecodingBuffer::isAck(rxBuffer))
      {
         rxAckDecoder.pushAck(rxBuffer);
         continue;
      }
      if (Slay2AckDecodingBuffer::isEndOfAck(rxBuffer))
      {
         if (rxAckDecoder.getCrc32() == 0) //CRC of valid frames is 0!
         {
            const unsigned char * ackBuffer = rxAckDecoder.getBuffer();
            unsigned int ackLen = rxAckDecoder.getCount();
            if (ackLen == 5) //length of ACK frames is 5 (1 byte seqNr, 4 byte CRC)
            {
               const unsigned char seqNr = ackBuffer[0]; //1st byte is expected to be the sequence number
               txScheduler.acknowledgeXfer(seqNr);
            }
         }
         rxAckDecoder.flush();;
         continue;
      }

      //DATA
      if (Slay2DataDecodingBuffer::isData(rxBuffer))
      {
         rxDataDecoder.pushData(rxBuffer);
         continue;
      }
      if (Slay2DataDecodingBuffer::isEndOfData(rxBuffer))
      {
         if (rxDataDecoder.getCrc32() == 0) //CRC of valid frames is 0!
         {
            const unsigned char * dataBuffer = rxDataDecoder.getBuffer();
            unsigned int dataLen = rxDataDecoder.getCount();
            if (dataLen > 6) //length of DATA frames is 6+X (1 byte seqNr, 1 byte channel number, X byte payload, 4 byte CRC)
            {
               const unsigned char seqNr = dataBuffer[0]; //1st byte is expected to be the sequence number
               txScheduler.scheduleAck(seqNr);
               if (seqNr == nextExpRxSeqNr)
               {
                  const unsigned char channel = dataBuffer[1];
                  Slay2Receiver receiver = channels[channel]->receiver;
                  if (receiver != NULL)
                  {
                     receiver(channels[channel]->receiverObj, &dataBuffer[2], dataLen - 6);
                  }
                  ++nextExpRxSeqNr;
               }
            }
         }
         rxDataDecoder.flush();;
         continue;
      }
      //just drop unexpected chars
      continue;
   }
}


void Slay2::doTransmission(void)
{
   if (getTxCount() <= 24) //if less/equal than 24 chars in TX buffer -> add another frame (Note: 24 ^= 3 ACK frames)
   {
      const unsigned long time1ms = getTime1ms();
      Slay2Buffer * txBuffer = txScheduler.getNextXfer(time1ms, channels, SLAY2_NUM_CHANNELS);
      if (txBuffer != NULL)
      {
         //whenever i am going to start a new transmission, i have to flush the rxAckDecoder...
         rxAckDecoder.flush();
         transmit(txBuffer->getBuffer(), txBuffer->getCount());
      }
   }
}


Slay2Channel * Slay2::open(const unsigned int channel)
{
   if (channel < SLAY2_NUM_CHANNELS)
   {
      if (this->channels[channel] == NULL)
      {
         Slay2Channel * ch = new Slay2Channel(channel);
         if (ch != NULL)
         {
            this->channels[channel] = ch;
            return ch;
         }
      }
   }
   return NULL;
}


void Slay2::close(Slay2Channel * const ch)
{
   if (ch != NULL)
   {
      const unsigned int channel = ch->channel;
      if (channel < SLAY2_NUM_CHANNELS)
      {
         this->channels[channel] = NULL;
      }
      delete ch; //delete channel
   }
}






Slay2Channel::Slay2Channel(const unsigned int channel)
{
   this->channel = channel;
   this->receiver = NULL;
   this->receiverObj = NULL;
   this->txMore = false;
}


void Slay2Channel::setReceiver(const Slay2Receiver receiver, void * const obj)
{
   this->receiver = receiver;
   this->receiverObj = obj;
}


int Slay2Channel::send(const unsigned char * data, const unsigned int len, const bool more)
{
   unsigned int count;
   bool success;

   //push data into txFifo
   for (count = 0; count < len; ++count)
   {
      success = txFifo.push(*data);
      if (success == true)
      {
         ++data;
         continue;
      }
      break;
   }
   //set more (data will follow) flag
   this->txMore = more;
   return (int)count;
}

