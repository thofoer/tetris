#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("Tetris"); //Name des ESP32
  Serial.println("Der ESP32 ist bereit. Verbinde dich nun über Bluetooth.");
}

void loop() {
  char s[16];
  int i=0;
  if (SerialBT.available()) {

     while (i<15 && SerialBT.available()) {
      s[i++] = (char) SerialBT.read();     
     }
    
     Serial.printf("Empfangen: %s\n", s);
     i=0;
  }
  delay(10);
}

void mitInt() {
    if (SerialBT.available()) {
      int input = SerialBT.read();
      Serial.write(input);    
    }
    delay(5);
}

char* readString() {
  char result[16];
  int i=0;
  while (i<15 && SerialBT.available()) {
    result[i] = (char) SerialBT.read();
    Serial.print(result[i]);
    i++;
  }
  result[i] = (char)0;

  return result;
}



void mitString() {
  if (SerialBT.available()) {
    String s = SerialBT.readStringUntil('\n');
    Serial.write(">");
    Serial.print(s);
    s.toUpperCase();
    SerialBT.print(s);
    Serial.write("<\n");
  }
  delay(5);
}
