#include <RadioLib.h>
#include <Queue.h>
#include <string>

#define BLOCK_LENGTH 200
#define BUFFER_LENGTH 50
#define TIMEOUT 1*1000
#define SEMTECH  //EBYTE



typedef struct {
  char data[255];
  int length;
} message;

std::queue<message> queue;
std::queue<std::string> streamQueue;

long int id = 0;



void setup() {
  Serial.begin(115200);
    delay(200);
  Serial1.begin(115200);
  delay(500);
  sendSerial("INIT"); //to synchronise Arduino and app
  delay(500);

}



bool waitForBytes(int count) {
  unsigned long start = millis();
  while (Serial1.available() < count) {
    if (millis() - start > TIMEOUT) {
      return false;
    }
  }
  return true;
}

bool readUntil(char terminator, byte* buffer) {
  for (int i = 0; i < BUFFER_LENGTH; i++) {
    bool ok = waitForBytes(1);
    if (!ok) return false;

    byte b = Serial1.read();
    if (b == terminator) {
      buffer[i] = 0;
      return true;
    }
    else {
      if (i == BUFFER_LENGTH - 1) {
        return false;
      }
      else {
        buffer[i] = b;
      }
    }
  }
}

void sendSerial(const char* cmd) {
  Serial.println(cmd);
  Serial1.print(id);
  Serial1.flush();
  Serial1.print(",");
  Serial1.flush();
  Serial1.println(cmd);
  Serial1.flush();
  delayMicroseconds(500);
  id++;
}

void push_string(long id, char type, byte* str) {
  message m;
  m.data[0] = id >> 8;
  m.data[1] = id;
  m.data[2] = type;
  strcpy(m.data+3, (char*)str);
  int len = strlen((char*)str) + 4;
 
  m.length = len - 1;

  if (queue.size() < 400) {
    queue.push(m);
  }
  else {
    Serial1.println("Queue full!!!");
  }
}

void push_stream(long id, byte* fileName, int block, int len, bool hasMore, byte* data) {
  message m;
  m.data[0] = id >> 8;
  m.data[1] = id;
  m.data[2] = 'S';
  m.data[3] = block >> 8;
  m.data[4] = block;
  m.data[5] = len;
  m.data[6] = hasMore;
  strcpy(m.data+7, (char*) fileName);
  int fnLen = strlen((char*) fileName);
  m.data[7 + fnLen] = 0;
  memcpy(m.data + fnLen + 8, data, len);
 
  m.length = len + fnLen + 8;

 
  if (queue.size() < 400) {
    queue.push(m);
  }
  else {
    Serial1.println("Queue full!!!");
  }
}

void onInitAvailable(int id) {
  byte buff[BUFFER_LENGTH];
  bool ok = readUntil('\n', buff);
  if (!ok) return;

  push_string(id, 'I', (byte*)"INIT");
}

void onTimeAvailable(int id) {
  byte timeStr[BUFFER_LENGTH];
  bool ok = readUntil('\n', timeStr);
  if (!ok) return;

  push_string(id, 'T', timeStr);
}

void onGpsAvailable(int id) {
  byte gpsStr[BUFFER_LENGTH];
  bool ok = readUntil('\n', gpsStr);
  if (!ok) return;

  push_string(id, 'G', gpsStr);
}

void onPhotoAvailable(int id) {
  byte fileName[BUFFER_LENGTH];
  byte fileSizeStr[BUFFER_LENGTH];
  long fileSize;
  bool ok = readUntil(',', fileName);
  if (!ok) return;
  ok = readUntil('\n', fileSizeStr);
  if (!ok) return;
  sscanf((char*)fileSizeStr, "%ld", &fileSize);

  char msg[BUFFER_LENGTH];
  sprintf(msg, "%s,%d", fileName, fileSize);
  push_string(id, 'P', (byte*)msg);

  /*char cmd[BUFFER_LENGTH];
  sprintf(cmd, "STREAM(%s,%d)", fileName, 0);
  sendSerial(cmd);*/
}

long waitingForAck;
bool waitForAck;
void onStreamAvailable(int id) {
  byte fileName[BUFFER_LENGTH];
  bool ok = readUntil(',', fileName);
  if (!ok) return;
  ok = waitForBytes(4);
  if (!ok) return;
  long block = (Serial1.read() << 8) + Serial1.read();
  unsigned char len = Serial1.read();
  byte hasMore = Serial1.read();
  byte data[len+1];   //NEW
  data[len]=0;        //NEW   Need this so on print doesnt overwrite
  for (int i = 0; i < len; i++) {
    ok = waitForBytes(1);
    if (!ok) return;
    data[i] = Serial1.read();
  }

  push_stream(id, fileName, block, len, hasMore, data);

  Serial.print((char *)fileName);
  Serial.print(",");
  Serial.print(block);
  Serial.print(",");
  Serial.print(int(len));
  Serial.print(",");
  Serial.print(hasMore);
  Serial.print(",");
  Serial.println((char *)data);  //Data is un printable  The characters are mostly not visible
   
 
  if (hasMore == 3) {   //Adds the next one to the queue that gets sent to the phone so the phone returns the next chunk of the picture
    char cmd[50];
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "STREAM(%s,%ld+)", fileName, block+1);
    //sendSerial(cmd);
    streamQueue.push(cmd);
  }

  if (streamQueue.size() > 0) {

    sendSerial(streamQueue.front().c_str());
    streamQueue.pop();
  }
}

unsigned long lastSerial = millis(), lastTx = millis(), lastDebug = millis();
long int lastPacketTOA = 0;

unsigned long loopcount=0;
void loop() {
  if (loopcount==100)   sendSerial("CTIME");
  loopcount++;

  if (Serial1.available() > 0) {
    byte msgIdStr[BUFFER_LENGTH];
    int msgId;
    char msgType;
    bool ok = readUntil(',', msgIdStr);
    if (!ok) return;
    sscanf((char*)msgIdStr, "%d", &msgId);
    ok = waitForBytes(1);
    if (!ok) return;
    msgType = Serial1.read();
 
    switch (msgType) {
      case 'I':
        onInitAvailable(msgId);
        break;
      case 'T':
        onTimeAvailable(msgId);  
        break;
      case 'G':
        onGpsAvailable(msgId);
        break;
      case 'P':
        onPhotoAvailable(msgId);
        break;
      case 'S':
        onStreamAvailable(msgId);
        break;
    }
 
  }



  //push_string(0, 'T', (byte*)"testtesttest");

  /*if (millis() < 15*1000) {
    sendSerial("TIME");
    sendSerial("GPS");
    sendSerial("PHOTO(B,50%,100,250,CLOUDY)");
    sendSerial("PHOTO(F,50%,100,100,AUTO)");
  }*/

  /*if (millis() - lastSerial > (unsigned long)10*1000) {
    lastSerial = millis();
   
    sendSerial("TIME");
    sendSerial("GPS");
    //Front/Back, Quality, ISO, Shutter speed, White Balance
    sendSerial("PHOTO(B,50%,100,250,CLOUDY)");
    //sendSerial("PHOTO(F,50%,100,100,AUTO)");
    //sendSerial("BURST(B,50%,100,10,CLOUDY)");
    //sendSerial("BURST(F,50%,100,500,CLOUDY)");
  }*/
}
