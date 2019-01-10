#include <iostream>
#include "slay2_buffer.h"

using namespace std;



int main(int argc, char * argv[])
{
   Slay2AckEncodingBuffer ackEncoder;
   Slay2AckDecodingBuffer ackDecoder;

   cout << "ACK Endoder / Decoder Test" << endl;
   cout << "Initial Ack-Encoder Length: " << ackEncoder.getCount() << endl;

   ackEncoder.pushAck('A');
   ackEncoder.pushAck('c');
   ackEncoder.pushAck('k');
   ackEncoder.pushAck('n');
   ackEncoder.pushAck('o');
   ackEncoder.pushAck(0);
   ackEncoder.pushAckBig32(ackEncoder.getCrc32());
   ackEncoder.pushEndOfAck();

   const unsigned int ackCount = ackEncoder.getCount();
   const unsigned char * ackBuffer = ackEncoder.getBuffer();
   cout << "Encoded Ack Length: " << ackCount << endl;
   for (int i = 0; i < ackCount; ++i)
   {
      const unsigned char c = *ackBuffer++;
      if (Slay2AckDecodingBuffer::isAck(c))
      {
         ackDecoder.pushAck(c);
         cout << ".";
         continue;
      }
      if (Slay2AckDecodingBuffer::isEndOfAck(c))
      {
         cout << "End Of Ack Frame reached" << endl;
         break;
      }
      cout << "Nicht erwarteter char" << endl;
   }
   cout << "Decoded Ack Length: " << ackDecoder.getCount() << endl;
   cout << "Crc of decodec Ack: " << ackDecoder.getCrc32() << endl; //0 expected!!!
   cout << (char *)ackDecoder.getBuffer() << endl;
   cout << endl << endl << endl;



   Slay2DataEncodingBuffer dataEncoder;
   Slay2DataDecodingBuffer dataDecoder;
   cout << "DATA Encoder / Decoder Test" << endl;
   cout << "Initial Date-Encoder Length: " << dataEncoder.getCount() << endl;

   static const char * const dataString = "0123456789 Das sind die Payload Daten der Uebertragung";
   const unsigned char * dataChar = (const unsigned char *)dataString;
   unsigned char dc;
   do
   {
      dc = *dataChar++;
      dataEncoder.pushData(dc);
   } while (dc != 0);
   dataEncoder.pushDataBig32(dataEncoder.getCrc32());
   dataEncoder.pushEndOfData();

   const unsigned int dataCount = dataEncoder.getCount();
   const unsigned char * dataBuffer = dataEncoder.getBuffer();
   cout << "Encoded Data Length: " << dataCount << endl;
   for (int i = 0; i < dataCount; ++i)
   {
      const unsigned char c = *dataBuffer++;
      if (Slay2DataDecodingBuffer::isData(c))
      {
         dataDecoder.pushData(c);
         // printf("0x%02X ", c);
         cout << ".";
         continue;
      }
      if (Slay2DataDecodingBuffer::isEndOfData(c))
      {
         cout << "End Of Data Frame reached" << endl;
         break;
      }
      cout << "Nicht erwarteter char" << endl;
   }
   cout << "Payload Byte 0: "  << Slay2DataDecodingBuffer::decodeData(dataEncoder.getBuffer(), 0)  << endl; //'0' expected
   cout << "Payload Byte 1: "  << Slay2DataDecodingBuffer::decodeData(dataEncoder.getBuffer(), 1)  << endl; //'1' expected
   cout << "Payload Byte 50: " << Slay2DataDecodingBuffer::decodeData(dataEncoder.getBuffer(), 50) << endl; //'g' expected


   cout << "Decoded Data Length: " << dataDecoder.getCount() << endl;
   cout << "Crc of decodec DATA: " << dataDecoder.getCrc32() << endl; //0 expected!!!
   cout << (char *)dataDecoder.getBuffer() << endl;
   cout << endl << endl << endl;



   cout << "Test Ende" << endl;
   return 0;
}
