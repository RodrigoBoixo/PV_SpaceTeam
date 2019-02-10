String _fromAndroid = "";
bool _startSending = false;
int axis[6] = {0};
int timeStamp[3] = {0};
#include <stdarg.h>
void p(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}
void loop() {
  // put your main code here, to run repeatedly:
  while(Serial.available()>0){
    char input = (char)Serial.read();
    _fromAndroid += input;
    if(_fromAndroid.indexOf("START") >= 0){
      _startSending = true;
      _fromAndroid = "";
    }
    if(_fromAndroid.indexOf("STOP") >= 0){
      _startSending = false;
      _fromAndroid = "";
    }
  }
  if(_startSending){
    for(int i=0;i<6;i++){
      axis[i] = rand()%100;
    }
    unsigned long curTime = millis();
    timeStamp[0] = curTime/(1000*60*24);
    timeStamp[1] = curTime/(1000*60);
    timeStamp[2] = curTime/1000;
    p("START,%d,%d,%d,%d,%d,%d,%d,%d,%d,END\n",axis[0],axis[1],axis[2],axis[3],axis[4],axis[5],timeStamp[0],timeStamp[1],timeStamp[2]);
    delay(200);
  }
  
}
