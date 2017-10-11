//
//  Based initially on the OTAA-LoRa-Seeed sketch, but I've
//  pretty much gutted it.
//
//  Copywrite (c) 2017  Todd Krein.   All Rights Reserved.
//

#define USE_GPS 1

#include "LoRaWan.h"

#ifdef USE_GPS
#include "TinyGPS++.h"
TinyGPSPlus gps;
#endif

unsigned char data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA,};
char buffer[256];

int loopcount = 0;

int sentCount;
int ackCount;

void setup(void)
{
    int i;
    char c;
#ifdef USE_GPS
    bool locked;
#endif
    
    SerialUSB.begin(115200);
    while(!SerialUSB);
    
    lora.init();
    lora.setDeviceReset();

#ifdef USE_GPS
    Serial.begin(9600);     // open the GPS
    locked = false;

    // For S&G, let's get the GPS fix now, before we start running arbitary
    // delays for the LoRa section

    while (!gps.location.isValid()) {
      while (Serial.available() > 0) {
        if (gps.encode(c=Serial.read())) {
          displayInfo();
          if (gps.location.isValid()) {
//            locked = true;
            break;
          }
        }
//        SerialUSB.print(c);
      }

//      if (locked)
//        break;
        
      if (millis() > 15000 && gps.charsProcessed() < 10)
      {
        SerialUSB.println(F("No GPS detected: check wiring."));
        SerialUSB.println(gps.charsProcessed());
        while(true);
      } 
      else if (millis() > 20000) {
        SerialUSB.println(F("Not able to get a fix in alloted time."));     
        break;
      }
    }
#endif
    
    memset(buffer, 0, 256);
    lora.getVersion(buffer, 256, 1);
    SerialUSB.print(buffer); 
    
    memset(buffer, 0, 256);
    lora.getId(buffer, 256, 1);
    SerialUSB.print(buffer);

    lora.setId(NULL, NULL, "70b3d57ed0006fcc");      // set App key, from TTN device overview

    // ???
    lora.setKey("039CBB775BFF1AC59FFFBCA9BC5DEC52",     // NetSKEY
                "9D3F0252CA11CC6920F36DE215748093",     // AppSKEY
                "ce3889feb09fa4b536ee9c41ba863364");    // AppKey
    
    if (lora.setDeviceMode(LWOTAA) == false)               // Over The Air Activation
      SerialUSB.print("Set Mode to OTAA failed.\n");
    else
      SerialUSB.print("OTAA mode set.\n");
      
//    lora.setDeciveMode(LWABP);              // pre-shared keys
    lora.setDataRate(DR0, US915HYBRID);             // This resets all the CH and RXWIN1 values
    lora.setAdaptiveDataRate(true); 

//    lora.getChannel();
//    lora.getReceiveWindowFirst();
      
    lora.setReceiveWindowSecond(923.3, DR8);      // 2.2.7  Second receive window
    lora.setPower(14);

//    lora.getChannel();
//    lora.getReceiveWindowFirst();
    
    SerialUSB.print("Starting OTTA Join.\n");
    loopcount = 0;
    while(true) {
      loopcount++;
      if (lora.setOTAAJoin(JOIN))
        break;
//      lora.getChannel();
//      lora.getReceiveWindowFirst();
    }

    SerialUSB.print("Took ");
    SerialUSB.print(loopcount);
    SerialUSB.println(" tries to join.");

    lora.getChannel();
    lora.getReceiveWindowFirst();

  loopcount = 0;
  sentCount = 0;
  ackCount = 0;
}

    
void loop(void)
{   
    bool result = false;


    if(SerialUSB.available()) {
      SerialUSB.print("--Entering Debug--\n");
      lora.loraDebug();
      SerialUSB.print("--Exit Debug--\n");
    }

    lora.transferPacket("mud", 10);
    result = lora.transferPacketWithConfirmed("Testing", 10); 
    sentCount++;
           
    if(result)
    {
        short length;
        short rssi;
        
        ackCount++;      
        memset(buffer, 0, 256);
        length = lora.receivePacket(buffer, 256, &rssi);
        
        if(length)
        {
            SerialUSB.print("Length is: ");
            SerialUSB.println(length);
            SerialUSB.print("RSSI is: ");
            SerialUSB.println(rssi);
            SerialUSB.print("Data is: ");
            for(unsigned char i = 0; i < length; i ++)
            {
                SerialUSB.print("0x");
                SerialUSB.print(buffer[i], HEX);
                SerialUSB.print(" ");
            }
            SerialUSB.println();
        }
    }
    else {
      SerialUSB.print("Send failure\n");
    }

    SerialUSB.print("Sent/ack ");
    SerialUSB.print(sentCount);
    SerialUSB.print("/");
    SerialUSB.println(ackCount);
    delay(2000);
}

void displayInfo()
{
  SerialUSB.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    SerialUSB.print(gps.location.lat(), 6);
    SerialUSB.print(F(","));
    SerialUSB.print(gps.location.lng(), 6);
  }
  else
  {
    SerialUSB.print(F("INVALID"));
  }

  SerialUSB.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    SerialUSB.print(gps.date.month());
    SerialUSB.print(F("/"));
    SerialUSB.print(gps.date.day());
    SerialUSB.print(F("/"));
    SerialUSB.print(gps.date.year());
  }
  else
  {
    SerialUSB.print(F("INVALID"));
  }

  SerialUSB.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) SerialUSB.print(F("0"));
    SerialUSB.print(gps.time.hour());
    SerialUSB.print(F(":"));
    if (gps.time.minute() < 10) SerialUSB.print(F("0"));
    SerialUSB.print(gps.time.minute());
    SerialUSB.print(F(":"));
    if (gps.time.second() < 10) SerialUSB.print(F("0"));
    SerialUSB.print(gps.time.second());
    SerialUSB.print(F("."));
    if (gps.time.centisecond() < 10) SerialUSB.print(F("0"));
    SerialUSB.print(gps.time.centisecond());
  }
  else
  {
    SerialUSB.print(F("INVALID"));
  }

  SerialUSB.println();
}
