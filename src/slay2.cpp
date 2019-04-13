//-----------------------------------------------------------------------------
/*!
   \file
   \brief Serial Layer 2 Protocol.

   To synchronice transmitter and receiver in respect to the next expected sequence number,
   each endpoint sends an SYNC sequence at stratup. A SYNC sequence consists of 5 consecutive
   SYNC bytes:

   SLAY2_SYNC (0x2C)
      +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
      | 0 | 0 | 1 | 0 | 1 | 1 | 0 | 0 |
      +---+---+---+---+---+---+---+---+


   DATA and ACK is transfered in frames. Frames are terminated with a final
   byte:

   SLAY2_END_OF_ACK (0x1)
      +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
      | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 |
      +---+---+---+---+---+---+---+---+
   SLAY2_END_OF_DATA (0x2)
      +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
      | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 |
      +---+---+---+---+---+---+---+---+


   DATA is encoded into a byte stream. Each byte of DATA stream contains
   7 payload bits. Such bytes has bit[7] = [1], bit[6:0] = [DDDDDDD].

   DATA
      +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
      | 1 | D | D | D | D | D | D | D |
      +---+---+---+---+---+---+---+---+


   Acknowledgment ACK is also encoded into a byte stream. Each byte of ACK
   stream contains 6 payload bits. Such bytes has bit[7:6] = [01], bit[5:0] = [AAAAAA].

   ACK
      +-7-+-6-+-5-+-4-+-3-+-2-+-1-+-0-+
      | 0 | 1 | A | A | A | A | A | A |
      +---+---+---+---+---+---+---+---+




   Transmission is done in units of "frames":

   DATA-FRAME
      +----+----+-------------------------------------------------------------+-------+
      |SEQ | CH |                        PAYLOAD                              | CRC32 |
      +----+----+-------------------------------------------------------------+-------+

   Assembly of DATA frames:
     -- 1st byte: sequence number of the frame
     -- 2nd byte: communication hannel number
     -- next-N bytes: Up to 256 payload data bytes (sent through the communication channel)
     -- final 4 bytes: 32-bit CRC of the entire frame (big endian)


   ACK-FRAME
      +-----+-------+
      | SEQ | CRC32 |
      +-----+-------+

   Assembly of ACK frames:
     -- 1st byte: sequence number of the frame
     -- final 4 bytes: 32-bit CRC of the entire frame (big endian)


    Note:
    For transmission, these frames are encoded and terminated with an end-of-ack resp. end-of-data byte.

*/
//-----------------------------------------------------------------------------

/* -- Includes ------------------------------------------------------------ */
#include <iostream>
#include "slay2.h"


/* -- Defines ------------------------------------------------------------- */
using namespace std;

#define SLAY2_SYNC            (0x2C)  //a bit pattern with a "special" 0-1 sequence that is considered robust against inteferences


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
   verbose = false;
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
   enterCritical();
   //on startup
   if (syncSent == false)
   {
      const unsigned char syncSequence[5] = { SLAY2_SYNC, SLAY2_SYNC, SLAY2_SYNC, SLAY2_SYNC, SLAY2_SYNC };
      if (verbose) cout << "SLAY2: Sending 5x SYNC" << endl;
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
   leaveCritical();
}


void Slay2::setVerbose(void)
{
   verbose = true;
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
         if (verbose) cout << "SLAY2: SYNC received" << endl;
         if (syncCount >= 3)
         {
            if (verbose) cout << "SLAY2: reset for synchronisation" << endl;
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
         if (verbose) cout << "SLAY2: ACK frame finished. CRC=" << rxAckDecoder.getCrc32() << endl;
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
         if (verbose) cout << "SLAY2: DATA frame finished. CRC=" << rxDataDecoder.getCrc32() << endl;
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
                  const unsigned char ch = dataBuffer[1];
                  if (ch < SLAY2_NUM_CHANNELS)
                  {
                     Slay2Channel * const channel = channels[ch];
                     if (channel != NULL)
                     {
                        Slay2Receiver receiver = channel->receiver;
                        if (receiver != NULL)
                        {
                           //force "zero termination" at the end of RX data (this overwrites one of the CRC bytes!)
                           ((unsigned char *)dataBuffer)[2 + (dataLen - 6)] = 0;
                           //callback to application
                           receiver(channel->receiverObj, &dataBuffer[2], dataLen - 6);
                        }
                     }
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
      const unsigned int time1ms = getTime1ms();
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
         Slay2Channel * ch = new Slay2Channel(this, channel);
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






Slay2Channel::Slay2Channel(Slay2 * const slay2, const unsigned int channel)
{
   this->slay2 = slay2;
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
   enterCritical();
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
   leaveCritical();
   return (int)count;
}


unsigned int Slay2Channel::getTxBufferSize()
{
   return SLAY2_FIFO_SIZE;
}

unsigned int Slay2Channel::getTxBufferSpace()
{
   return txFifo.getSpace();
}

void Slay2Channel::flushTxBuffer()
{
   enterCritical();
   txFifo.flush();
   leaveCritical();
}


void Slay2Channel::enterCritical()
{
   slay2->enterCritical();
}

void Slay2Channel::leaveCritical()
{
   slay2->leaveCritical();
}

