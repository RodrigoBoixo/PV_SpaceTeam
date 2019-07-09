#include "SoftwareSerial.h"
#include "SparkFunMPL3115A2.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
byte inByte; //byte read from terminal
bool sdInitSuccess = false; //card init status
File myFile;
String readText; //text read from file
char readCharArray[128]; //buffer for reading from file
unsigned long fileSize; //size of opened file
unsigned long filePos = 0;
MPL3115A2 myPressure;
const int chipSelect = 10;
SoftwareSerial K_30_Serial(4,7);  //Sets up a virtual serial port. Default is Using pin 12 for Rx and pin 13 for Tx. We use 7 for TXD and 4 for RXD                               
byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};  //Command packet to read Co2 (see app note)
byte response[] = {0,0,0,0,0,0,0};  //create an array to store the response
int valMultiplier = 1; //multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB

//---------------------------------------------------------------//
void setup() {
  Serial.begin(9600);  //Opens the virtual serial port with a baud of 9600  
  while (!Serial) {
    ; //wait for the serial port to connect.
  }
  
  Serial.begin(9600);         //Opens the main serial port to communicate with the computer
  
  K_30_Serial.begin(9600);    //Opens the virtual serial port with a baud of 9600  
  myPressure.begin(); // Get sensor online
  myPressure.setModeAltimeter(); // Measure altitude above sea level in meters
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags 
}
void sendRequest(byte packet[])
{
  while(!K_30_Serial.available())  //keep sending request until we start to get a response
  {
    K_30_Serial.write(readCO2,7);
    delay(50);
  }
  
  int timeout=0;  //set a timeoute counter
  while(K_30_Serial.available() < 7 ) //Wait to get a 7 byte response
  {
    timeout++;  
    if(timeout > 10)    //if it takes to long there was probably an error
      {
        while(K_30_Serial.available())  //flush whatever we have
          K_30_Serial.read();
          
          break;                        //exit and try again
      }
      delay(50);
  }
  
  for (int i=0; i < 7; i++)
  {
    response[i] = K_30_Serial.read();
  }  
}
unsigned long getValue(byte packet[])
{
    int high = packet[3];                        //high byte for value is 4th byte in packet in the packet
    int low = packet[4];                         //low byte for value is 5th byte in the packet
   unsigned long val = high*256 + low;                //Combine high byte and low byte with this formula to get value
    return val* valMultiplier;
}

//---------------------------------------------------------------//
void loop() {
 

      if (sdInitSuccess) { //check if card is initialized already
        Serial.println("Already initialized.\n");
      }
      else if (!sdInitSuccess) { //if not already initialized
        Serial.println("Initializing SD Card..");
        if (!SD.begin(10)) { //using pin 10 (SS)
          Serial.println("Initialization failed!\n");
          sdInitSuccess = false; //failure
          return;
        }
        else {
          Serial.println("Intitialization success.");
          Serial.println();
          sdInitSuccess = true;
        }
    
     Serial.print("running");
     Serial.print("YES, it works");
     while(1==1){
    sendRequest(readCO2);
    unsigned long valCO2 = getValue(response);
    Serial.println(valCO2);
    if (valCO2 > 1) {
      if (sdInitSuccess) { //proceed only if card is initialized
        myFile = SD.open("Datalog.txt", FILE_WRITE);
        if (myFile) {
          Serial.println("File opened successfully.");
          Serial.println("Writing to DataLog.text");
          Serial.println("It's still working");
          myFile.println(valCO2);
          myFile.close(); //this writes to the card
          Serial.println("Done");
          delay(2000);
        }
        else { //else show error
          Serial.println("Error opeing file.\n");
        }
      }
      else {
        Serial.println("SD Card not initialized.");
      }
    }   
   }
  }  
}
