//test song is  "song.vgm"

#include <Arduino.h>
#include "SdFat.h"
#include "LTC6903.h"
#include "AY38910.h"
//#include "YM2413.h"

LTC6903 clk_AY3810(10, 831, 19); //LTC 1.79042MHz, actual clock is 1.7897725MHz (NTSC colorburst / 2)
AY38910 ay38910(&PORTF, &DDRF, 8, 9); //BDIR 8, BC1 9         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@CHANGE FOR TEENSY 3.5 UPGRADE!

SdFat SD;
File vgm;

//Buffer
const unsigned int MAX_CMD_BUFFER = 32;
unsigned char cmdBuffer[MAX_CMD_BUFFER];
uint32_t bufferPos = 0;
const unsigned int MAX_FILE_NAME_SIZE = 1024;
char fileName[MAX_FILE_NAME_SIZE];
unsigned char cmd = 0;

//Timing Variables
float singleSampleWait = 0;
const int sampleRate = 44100; //44100 standard
const float WAIT60TH = ((1000.0 / (sampleRate/(float)735))*1000);
const float WAIT50TH = ((1000.0 / (sampleRate/(float)882))*1000);
uint32_t waitSamples = 0;
unsigned long preCalced8nDelays[16];
unsigned long preCalced7nDelays[16];
unsigned long lastWaitData61 = 0;
unsigned long cachedWaitTime61 = 0;
unsigned long pauseTime = 0;
unsigned long startTime = 0;


void FillBuffer()
{
    vgm.readBytes(cmdBuffer, MAX_CMD_BUFFER);
}

unsigned char GetByte()
{
  if(bufferPos == MAX_CMD_BUFFER)
  {
    bufferPos = 0;
    FillBuffer();
  }
  return cmdBuffer[bufferPos++];
}

uint32_t ReadBuffer32() //Read 32 bit value from buffer
{
  byte v0 = GetByte();
  byte v1 = GetByte();
  byte v2 = GetByte();
  byte v3 = GetByte();
  return uint32_t(v0 + (v1 << 8) + (v2 << 16) + (v3 << 24));
}

uint32_t ReadSD32() //Read 32 bit value straight from SD card
{
  byte v0 = vgm.read();
  byte v1 = vgm.read();
  byte v2 = vgm.read();
  byte v3 = vgm.read();
  return uint32_t(v0 + (v1 << 8) + (v2 << 16) + (v3 << 24));
}

void ClearBuffers()
{
  bufferPos = 0;
  for(int i = 0; i < MAX_CMD_BUFFER; i++)
    cmdBuffer[i] = 0;
}

void RemoveSVI() //Sometimes, Windows likes to place invisible files in our SD card without asking... GTFO!
{
  File nextFile;
  nextFile.openNext(SD.vwd(), O_READ);
  char name[MAX_FILE_NAME_SIZE];
  nextFile.getName(name, MAX_FILE_NAME_SIZE);
  String n = String(name);
  if(n == "System Volume Information")
  {
      if(!nextFile.rmRfStar())
        Serial.println("Failed to remove SVI file");
  }
  SD.vwd()->rewind();
  nextFile.close();
}

void setup()
{
    clk_AY3810.Set();
    if(!SD.begin())
    {
        Serial.println("Card Mount Failed");
        return;
    }
    ClearBuffers();
    vgm.open("song.vgm", FILE_READ);
    if(!vgm)
      Serial.println("File open failed!");
    else
      Serial.println("Opened successfully...");
    FillBuffer();
    singleSampleWait = ((1000.0 / (sampleRate/(float)1))*1000);
    for(int i = 0; i<16; i++)
    {
      if(i == 0)
      {
        preCalced8nDelays[i] = 0;
        preCalced7nDelays[i] = 1;
      }
      else
      {
        preCalced8nDelays[i] = ((1000.0 / (sampleRate/(float)i))*1000);
        preCalced7nDelays[i] = ((1000.0 / (sampleRate/(float)i+1))*1000);
      }
    }
    Serial.begin(115200);
}

void loop()
{
  unsigned long timeInMicros = micros();
  if( timeInMicros - startTime <= pauseTime)
  {
    Serial.print("Wait time: ");
    Serial.println(pauseTime);
    return;
  }
  cmd = GetByte();
  Serial.println(cmd, HEX);
  switch(cmd)
  {
    case 0xA0:
    {
      unsigned char d = GetByte();
      unsigned char a = GetByte();
      ay38910.Send(a, d);
    }
    case 0x61:
    {
      //Serial.print("0x61 WAIT: at location: ");
      //Serial.print(parseLocation);
      //Serial.print("  -- WAIT TIME: ");
    uint32_t wait = 0;
    for ( int i = 0; i < 2; i++ )
    {
      wait += ( uint32_t( GetByte() ) << ( 8 * i ));
    }

    if(floor(lastWaitData61) != wait) //Avoid doing lots of unnecessary division.
    {
      lastWaitData61 = wait;
      if(wait == 0)
        break;
      cachedWaitTime61 = ((1000.0 / (sampleRate/(float)wait))*1000);
    }
    //Serial.println(cachedWaitTime61);

    startTime = timeInMicros;
    pauseTime = cachedWaitTime61;
    //delay(cachedWaitTime61);
    break;
    }
    case 0x62:
    startTime = timeInMicros;
    pauseTime = WAIT60TH;
    //delay(WAIT60TH); //Actual time is 16.67ms (1/60 of a second)
    break;
    case 0x63:
    startTime = timeInMicros;
    pauseTime = WAIT50TH;
    //delay(WAIT50TH); //Actual time is 20ms (1/50 of a second)
    break;
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F:
    {
      //Serial.println("0x7n WAIT");
      uint32_t wait = cmd & 0x0F;
      //Serial.print("Wait value: ");
      //Serial.println(wait);
      startTime = timeInMicros;
      pauseTime = preCalced7nDelays[wait];
      //delay(preCalced7nDelays[wait]);
    break;
    }
    case 0x66:
      ClearBuffers();
      FillBuffer();
      bufferPos = 0;
      // if(loopOffset == 0)
      //   loopOffset = 64;
      // loopCount++;
      // vgm.seek(loopOffset);
      // FillBuffer();
      // bufferPos = 0;
      break;
      default:
      break;
  }

}































//
