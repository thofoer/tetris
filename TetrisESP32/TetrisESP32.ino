#include <BluetoothSerial.h>
#include <FastLED.h>
#include "definitions.h"


#define WIDTH 8
#define HEIGHT 8
#define NUM_LEDS (WIDTH*HEIGHT)

#define LED_PIN     5
#define BRIGHTNESS  32
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define FRAME_TIME_MS 10



CRGB leds[NUM_LEDS];

BluetoothSerial SerialBT;

int level = 1;
int tileId = 4;
int posX = 2;
int posY = 0;
int rot = 0;

int speed = 1;
int fallCounter = speed;

int matrix[WIDTH][HEIGHT];

void setup() {
  delay( 1000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
    
  Serial.begin(115200);
  SerialBT.begin("Tetris"); // Bluetooth-Name des ESP32
  Serial.println("Der ESP32 ist bereit. Verbinde dich nun über Bluetooth.");  
  
  for (int x=0; x<WIDTH; x++) {
    for (int y=0; y<HEIGHT; y++) {
      matrix[x][y] = EMPTY_PIX;
      leds[x*WIDTH+y] = CRGB::Black;
    }
  }  

// Frame in matrix zeichnen
  for (int i=0; i<HEIGHT; i++) {
    matrix[0][i] = FRAME_PIX;
    matrix[WIDTH-1][i] = FRAME_PIX;
  }
  for (int i=1; i<WIDTH-1; i++) {
    matrix[i][HEIGHT-1] = FRAME_PIX;
  }
}

void loop() {  
  receiveCommand();
  clearLeds();
  drawMatrix();
  //moveDown();
  showTile();
  FastLED.show();  
  delay(FRAME_TIME_MS);
}

void moveDown() {
  fallCounter--;
  if (fallCounter<=0) {
    if (!isCollision(0, 0, 1)) {
      posY++;
    }
    else {
      // unten angekommen
      posY = 0;
    }
    fallCounter = speed;
  }
}

void dumpTile() {
  for (int y=0; y<4; y++) {
    for (int x=0; x<4; x++) {
      Serial.printf("%d", tiles[rot][tileId][y][x]);
    }
    Serial.printf("\n");
  }
}

void dumpMatrix(int sx, int sy) {
  for (int y=0; y<4; y++) {
    for (int x=0; x<4; x++) {
      Serial.printf("%d", matrix[sx+x][sy+y]);
    }
    Serial.printf("\n");
  }
}

boolean isCollision(int dRot, int dX, int dY) {
  Serial.printf("-----------------\nrot=%d, coord=%d/%d\n", rot, posX, posY);
  dumpTile();
  Serial.printf("--\n");
  dumpMatrix(posX+dX, posY+dY);
  boolean collision = false;
  for (int x=0; x<4 && !collision; x++) {
    for (int y=0; y<4 && !collision; y++) {
       int tilePixel = tiles[(rot+dRot)%4][tileId][y][x];
       if (tilePixel) {
         int backgroundPixel = matrix[posX+x+dX][posY+y+dY];
         if (backgroundPixel) {
           collision=true;
           Serial.printf("Kollision: %d/%d - %d\n", x, y, backgroundPixel);
         }
       }
    }
  }
  return collision;
}


void receiveCommand() {
  char input;
  
  if (SerialBT.available()) {
    input = (char) SerialBT.read();
    switch (input) {
      case CMD_RIGHT: right(); break;
      case CMD_LEFT:  left();  break;
      case CMD_TURN:  turn();  break;
      case CMD_DOWN:  down();  break;
      case CMD_START: start(); break;
      case CMD_RESET: reset(); break;
    }
  }
}


void right() {
   Serial.println("rechts");
   uint8_t b[2];
   b[0] = MSG_LEVEL;
   b[1] = level;
   SerialBT.write(b, 2);   
   level++;
   moveDown();
}

void left() {
   Serial.println("links");
   uint8_t b[2];
   b[0] = MSG_SCORE;
   b[1] = 3;
   SerialBT.write(b, 2);   
   tileId = (tileId + 1) % 7;
}

void turn() {
  if (!isCollision(1, 0, 0)) {
    rot = (rot + 1) & 0x3;
  }
}

void down() {
  
}
void start() {
  
}
void reset() {
  
}

void showTile() {
  for (int x=0; x<4; x++) {
    for (int y=0; y<4; y++) {
      int pixel = tiles[rot][tileId][y][x];
      if (pixel) {
        setLed(x+posX, y+posY, colors[tileId+1]);
      }
    }
  }
}

void drawMatrix() {
  for (int x=0; x<WIDTH; x++) {
    for (int y=0; y<HEIGHT; y++) {
      if (matrix[x][y]) {
        setLed(x, y, colors[matrix[x][y]]);
      }
    }
  }
}

void setLed(int x, int y, CRGB color) {
  int i = y*WIDTH;
  i += (y&1) ? x : (WIDTH-x-1);
  leds[i] = color;
}

void clearLeds() {
  for (int x=0; x<WIDTH; x++) {
    for (int y=0; y<HEIGHT; y++) {   
      leds[x*WIDTH+y] = CRGB::Black;
    }
  }
}
