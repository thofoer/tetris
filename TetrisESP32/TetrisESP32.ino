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
#define FAST_DOWN_SPEED 10

#define STATUS_WAIT 0
#define STATUS_GAME 1

CRGB leds[NUM_LEDS];

BluetoothSerial SerialBT;

int status = STATUS_WAIT;
int level = 1;
int tileId = 1;
int nextTileId;
int posX = 2;
int posY = 0;
int rot = 0;

int speed = 200;
int fallCounter = speed;
boolean fastDown = false;

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
  if (status == STATUS_GAME) {
    moveDown();
    showTile();
  }
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
      touchDown();      
    }
    fallCounter = fastDown ? FAST_DOWN_SPEED : speed;
  }
}

void touchDown() {
  fastDown = false;
  posY = 0;
  posX = (WIDTH-4)/2;
  nextTile();
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
 // Serial.printf("-----------------\nrot=%d, coord=%d/%d\n", rot, posX, posY);
 // dumpTile();
 // Serial.printf("--\n");
//  dumpMatrix(posX+dX, posY+dY);
  boolean collision = false;
  for (int x=0; x<4 && !collision; x++) {
    for (int y=0; y<4 && !collision; y++) {
       int tilePixel = tiles[(rot+dRot)&0x3][tileId][y][x];
       if (tilePixel) {
         int backgroundPixel = matrix[posX+x+dX][posY+y+dY];
         if (backgroundPixel) {
           collision=true;
          // Serial.printf("Kollision: %d/%d - %d\n", x, y, backgroundPixel);
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
   if (!isCollision(0, 1, 0)) {
     posX+=1;
   }
}

void left() {
   if (!isCollision(0, -1, 0)) {
     posX-=1;
   }
}

void nextTile() {
  tileId = nextTileId;
  nextTileId = random8() % 7;
  sendNextTile();
}

void turn() {
  
  if (isCollision(1, 0, 0)) {    
    Serial.printf("posX %d\n", posX);
    if (posX<=0) {
      
      for (int x=1; x<4; x++) {
        if (!isCollision(1, x, 0)) {
          Serial.printf("rechts %d\n", x);
           posX+=x;
           rot = (rot + 1) & 0x3;
           return;
        }
      }      
    }
    else if (posX>=WIDTH-4) {
      for (int x=1; x<4; x++) {
        if (!isCollision(1, -x, 0)) {
          Serial.printf("links %d\n", x);
           posX-=x;
           rot = (rot + 1) & 0x3;
           return;
        }
      }     
    }
     return;
  }
   Serial.printf("keine kollision\n");
  rot = (rot + 1) & 0x3;
}

void down() {
  fastDown = true;
  fallCounter = FAST_DOWN_SPEED;
}

void start() {
  tileId = random8() % 7;
  nextTileId = random8() % 7;
  sendNextTile();
  status = STATUS_GAME;
}

void reset() {
  
}

void sendScore(int points) {
   uint8_t b[2];
   b[0] = MSG_SCORE;
   b[1] = points;
   SerialBT.write(b, 2);  
}

void sendLevel() {
   uint8_t b[2];
   b[0] = MSG_LEVEL;
   b[1] = level;
   SerialBT.write(b, 2);   
}

void sendNextTile() {
   uint8_t b[2];
   b[0] = MSG_NEXT_TILE;
   b[1] = nextTileId;
   SerialBT.write(b, 2);   
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
