#include <HardwareSerial.h>

HardwareSerial MySerial(2); //particular to the esp32


//
//send payload is: 0x7C, 0xFF, 0xFF, command, action, length, checksum
//response payload is: 0xCC, 0xFF, 0xFF, command, action, length, antenna number(discard), [12 bytes of UID], checksum

//commands: EPCREAD,INFO,PARAMETER,

//EPCREAD:
//send: 0x7C, 0xFF, 0xFF, 0x10, 0x32, 00, 0xNN
//receive: 0xCC, 0xFF, 0xFF, 0x10, 0x00, 0x0D, 0x01, [12 bytes UID, MSB], checksum

//PARAMETER:
//list order: power(0-32 decimal), frequency hop engage, fixed frequency val, frequency hopping val [48 bit], work mode (0x01 = command, 02 = active, 03 = passive,
//read interval time, trigger engage, output mode (0x02 = RS485, 01=rs232), 32 bit auto read mode, antenna select, read type (0x10), SI?, buzzer engage (0x00 = disabled), UD? 24 bits,
//encryption enable, encryption password (16 bit),

//work mode should be active for testing, command for the future since it turns antenna off

struct Settings { //28 bytes
  byte power = 0x1E; //from 0 to 32 decimal
  byte freqHop = 0x01; //0x01 = enable
  byte fixedFreq = 0x6E; //915MHz
  byte FHV1 = 0x54;
  byte FHV2 = 0x5D;
  byte FHV3 = 0x66;
  byte FHV4 = 0x6F;
  byte FHV5 = 0x78;
  byte FHV6 = 0x82;
  byte workMode = 0x01; //0x01 = command, 0x02 = automatic, 0x03 = passive
  byte readInterval = 0x0A; //0x0A = 10ms read interval
  byte triggerEnable = 0x00; //0x01 = active
  byte outputMode = 0x02; //0x01 = RS232, 0x02 = RS485
  byte autoReadOffset = 0x00;
  byte autoReadInterval = 0x1E;
  byte autoReadWidth = 0x0A;
  byte autoReadPeriod = 0x0F;
  byte antenna = 0x01;
  byte tagType = 0x10; //0x10 = single GEN2 tag
  byte SI = 0x01;
  byte buzzerEnable = 0x00; //0x01 = enabled
  byte extraMemory = 0x00; //0x00 FU, 0x01 EPC, 0x02 TID, 0x03 USER -> used to auto read memory section
  byte startingAddress = 0x00;
  byte dataLength = 0x00;
  byte enableEncryption = 0x00; //0x01 = active
  int encryptionPassword = 0x00;
  byte maxTagReadCount = 0x40; //decimal
} defaultSettings;

byte lastEPCtag[12];

//commands
#define ISOREAD 0X01
#define ISOMEMORY 0x02
#define EPCREAD 0x10
#define EPCMULTIREAD 0x11
#define EPCMEMORY 0x12
#define PARAMETER 0x81
#define INFO 0x82
#define RESET 0x8F
#define ENCRYPT 0x30
#define TCPIP 0xB9
#define IOOUTPUT 0xBB

//actions
#define SET 0x31
#define GET 0x32
#define SETS 0x21
#define GETS 0x22

//responses
#define SUCCESS 0x00
#define FAIL 0x01
#define AUTO 0x32

byte lastTag[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //12-byte EPC tag ID

byte responseBuffer[35];

void setup() {
  MySerial.begin(57600, SERIAL_8N1, 18, 5); //particular to esp32, replace with desired serial port ex. Serial1.begin(57600);
  Serial.begin(9600); //for logging/debugging
}

void loop() {
  getTagUID();
  delay(100);

}

void getTagUID() {
  byte buffer[23];
  byte bufferI = 0;

  byte getIDcommand[] = {0x7C, 0xFF, 0xFF, 0x20, 0x00, 0x00, 0x66};

  while (MySerial.available()) MySerial.read();

  MySerial.write(getIDcommand, 7);
  delay(120); //actually takes ~90ms for a response
  byte bytesAvailable = MySerial.available();
  //tag hit: CC FF FF 20 02 10 00 34 00 E2 00 47 13 20 A0 64 26 56 C9 01 0F D7 44
  //no tag: CCFFFF201115FF



  for (int i = 0; i < 23; i++) {
    buffer[i] = MySerial.read();
  }
  if (bytesAvailable == 0 ) {
    Serial.println("RFID READER NON RESPONSIVE");
  }
  if (buffer[4] == 0x02) {
    Serial.print("TAG FOUND: ");
    for (int i = 9; i < 21; i++) {
      //Serial.print(buffer[i], HEX);
      lastTag[i-9] = buffer[i];
    }
    for (int i = 0; i < 12; i++) {
      Serial.print(lastTag[i], HEX);Serial.print(" ");
    }
    Serial.println();
  }
  else {
    Serial.print("NO TAG FOUND. RESPONSE: ");
    for (int i = 0; i < bytesAvailable; i++) {
      Serial.print(buffer[i], HEX);
      if (buffer[i] == 0x00) Serial.print("0");
    }
    Serial.println();
  }

  for (int i = 0; i < 9; i++) {
    MySerial.read(); //capture remaining response
  }
}
