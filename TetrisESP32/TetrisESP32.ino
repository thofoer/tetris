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

const int scores[4] = { 1, 3, 6, 10 };

void setup() {
  delay( 1000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
    
  Serial.begin(115200);
  SerialBT.begin("Tetris"); // Bluetooth-Name des ESP32
  Serial.println("Der ESP32 ist bereit. Verbinde dich nun über Bluetooth.");  

  resetMatrix();
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
  copyTileToMatrix();
  removeCompleteRows();
  nextTile();
}

void removeCompleteRows() {
  int rowsRemoved = 0;
  
  for (int y=0; y<HEIGHT-1; y++) {
    rowsRemoved += testAndRemoveRow(y); 
  }
  if (rowsRemoved) {
    sendScore(scores[rowsRemoved-1]);
  }
}

int testAndRemoveRow(int row){
  //dumpMatrix();
  // Testen, ob Zeile vollständig gefüllt.
   for (int x=1; x<WIDTH-1; x++) {
     if (!matrix[x][row]) {       
      //Serial.printf("zeile %d nicht voll\n", row);
       return 0; // nein
     }
   }
   // Zeile löschen
   Serial.printf("zeile %d löschen\n", row);
   for (int y=row-1; y>=0; y--) {
     for (int x=1; x<WIDTH-1; x++) {
       matrix[x][y+1] = matrix[x][y];
     }
   }
   for (int x=1; x<WIDTH-1; x++) {
     matrix[x][0] = 0;
   }
   return 1;
}

void gameOver() {
  status = STATUS_WAIT;   
  sendGameOver();
}

void copyTileToMatrix() {
  for (int y=0; y<4; y++) {
    for (int x=0; x<4; x++) {
      int pixel = tiles[rot][tileId][y][x] * (tileId+1);
      if (pixel){
        matrix[posX+x][posY+y] = pixel;   
      }
    }
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


void dumpMatrix() {
  for (int y=0; y<HEIGHT; y++) {
    for (int x=0; x<WIDTH; x++) {
      Serial.printf("%d", matrix[x][y]);
    }
    Serial.printf("\n");
  }
   Serial.printf("----------------\n");
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
  rot = tileId<=4 ? 0 : 1;  // Teile L und J können um 90° gedreht erscheinen, um Platz zu sparen.
  posX = (WIDTH-4)/2;  
  posY = -1; 
  
  if (isCollision(0, 0, 0)) {
    gameOver();
  }
  else {
    sendNextTile();
  }
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
  rot = (rot + 1) & 0x3;
}

void down() {
  fastDown = true;
  fallCounter = FAST_DOWN_SPEED;
}

void start() {
  resetMatrix();
  tileId = random8() % 7;
  nextTileId = random8() % 7;
  sendNextTile();
  status = STATUS_GAME;
}

void reset() {
  resetMatrix();
  status = STATUS_WAIT;
}

void sendScore(int points) {
   uint8_t b[2];
   b[0] = MSG_SCORE;
   b[1] = points;
   Serial.printf("send score %d\n", points);
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

void sendGameOver() {
  Serial.printf("Game over\n");
  uint8_t b[2];
   b[0] = MSG_GAME_OVER;
   b[1] = 0;
   SerialBT.write(b, 2);   
}

void resetMatrix() {   
  for (int x=1; x<WIDTH-1; x++) {
    for (int y=0; y<HEIGHT-1; y++) {
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
