#include <BluetoothSerial.h>
#include <FastLED.h>
#include "definitions.h"

#define WIDTH 16
#define HEIGHT 16
#define NUM_LEDS (WIDTH*HEIGHT)

#define LED_PIN     5
#define BRIGHTNESS  255
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define FRAME_TIME_MS 10
#define FAST_DOWN_SPEED 3
#define LEVEL_TIME_TICKS 30000
#define PHASE_TIME_TICKS 20000
#define START_SPEED 180
#define AUTO_MOVE_SPEED 6

#define STATUS_WAIT 0
#define STATUS_GAME 1
#define STATUS_AUTO 2

CRGB leds[NUM_LEDS];

BluetoothSerial SerialBT;

int status = STATUS_WAIT;
int level = 1;
int tileId = 1;
int nextTileId;
int posX = 2;
int posY = 0;
int rot = 0;
int moveSpeed = 12;

int targetRot = -1;
int targetX = -1;

int speed = START_SPEED;
int fallCounter = speed;
boolean fastDown = false;

TickType_t levelTimestamp;
TickType_t phaseTimestamp;
 
int matrix[WIDTH][HEIGHT];
int shadowMatrix[WIDTH][HEIGHT];
int dumpMatrix[WIDTH][HEIGHT];

const int scores[4] = { 1, 3, 6, 10 };

void setup() {
  delay( 1000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
    
  Serial.begin(115200);
  SerialBT.begin("Tetris"); // Bluetooth-Name des ESP32
  Serial.println("Der ESP32 ist bereit. Verbinde dich nun über Bluetooth.");  
  
  phaseTimestamp = xTaskGetTickCount();
  resetMatrix();
}


void loop() {    
  receiveCommand();
  
  if (status == STATUS_WAIT) {
    screensaver();
    TickType_t now = xTaskGetTickCount();
    if ( (now-phaseTimestamp) >= PHASE_TIME_TICKS) {
       phaseTimestamp = now;
       levelTimestamp = now;
       moveSpeed=12;
       start();
       status = STATUS_AUTO;
    }
    return;
  }
  clearLeds();
  drawMatrix();
  
  if (status == STATUS_AUTO) {
    autoMove();
  }
  if (status != STATUS_WAIT) {
    
    moveDown();
    showTile();
    levelUp();
  }
  FastLED.show();  
  delay(FRAME_TIME_MS);
}


void autoMove() {
  if (fallCounter % moveSpeed != 0) {
    return;
  }
  if (targetRot!=rot) {
    if (!turn()) {
      down();
    }
  }
  else if (targetX>posX) {
    right();
  }
  else if (targetX<posX) {
    left();
  }  
  else {
    down();
  }
}

void calculateTarget() {

  int bestScore = -10000000;
  int destRot;
  int destX;
  
  for (int r=0; r<rotationsDegrees[tileId]; r++) {
    for (int x=-2; x<WIDTH; x++) {
      int y=0;
      memcpy(shadowMatrix, matrix, sizeof(matrix));
      while (!isCollisionAbs(r, x, y, shadowMatrix) ) {
        y++;        
      }
      y--;
     
      if (y>0) {
     //    Serial.printf("r=%d x=%d y=%d\n", r, x, y);
         int score = rate(shadowMatrix, r, x, y);
         if (score>bestScore) {
          bestScore=score;
          destRot = r;
          destX = x;
         }
      }
    }
  }
  Serial.printf("score=%d   rot=%d   x=%d\n", bestScore, destRot, destX);
  targetX = destX;
  targetRot = destRot;
  Serial.println("---------------------------------");
}



void levelUp() {
    TickType_t now = xTaskGetTickCount();
    if ( (now-levelTimestamp) >= LEVEL_TIME_TICKS) {
       level++;
       sendLevel();
       speed = (8*speed) / 10;
       levelTimestamp = now;
       Serial.printf("Levelup %d - speed: %d\n", level, speed);
       switch(level) {
         case 1:
         case 2: moveSpeed=12; break;
         case 3: 
         case 4:
         case 5: 
         case 6: moveSpeed = 12-level; break;
         case 7: moveSpeed = 5; break;
         default: moveSpeed = 4;
       }
    }
}

void moveDown() {
  fallCounter--;
  if (fallCounter<=0) {
    if (!isCollision(0, 0, 1, matrix)) {
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
  removeCompleteRows(matrix);
  nextTile();
}

void removeCompleteRows(int mat[WIDTH][HEIGHT]) {
  int rowsRemoved = 0;
  
  for (int y=0; y<HEIGHT-1; y++) {
    rowsRemoved += testAndRemoveRow(y, mat); 
  }
  if (rowsRemoved) {
    sendScore(scores[rowsRemoved-1]);
  }
}

int testAndRemoveRow(int row, int mat[WIDTH][HEIGHT]){
  //dumpMatrix();
  // Testen, ob Zeile vollständig gefüllt.
   for (int x=1; x<WIDTH-1; x++) {
     if (!mat[x][row]) {       
      //Serial.printf("zeile %d nicht voll\n", row);
       return 0; // nein
     }
   }
   // Zeile löschen
   Serial.printf("zeile %d löschen\n", row);
   for (int y=row-1; y>=0; y--) {
     for (int x=1; x<WIDTH-1; x++) {
       mat[x][y+1] = mat[x][y];
     }
   }
   for (int x=1; x<WIDTH-1; x++) {
     mat[x][0] = 0;
   }
   return 1;
}

void gameOver() {
  status = STATUS_WAIT;   
  phaseTimestamp = xTaskGetTickCount();
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


int rate(int mat[WIDTH][HEIGHT], int r, int px, int py) {
  int score = 0;
  memcpy(dumpMatrix, mat, sizeof(dumpMatrix));

 // Serial.printf("r=%d x=%d y=%d  \n", r, px, py);
  
  for (int y=0; y<4; y++) {
    for (int x=0; x<4; x++) {
      int pixel = tiles[r][tileId][y][x] * (tileId+1);
      if (pixel){
        dumpMatrix[px+x][py+y] = 9;   
      }
    }
  }
  int rowsRemoved = 0;
  
  for (int y=0; y<HEIGHT-1; y++) {
    if (countFilled(y, dumpMatrix)==WIDTH-2) {
      rowsRemoved++;
    }
  }


  int allHoles = 0;
  for (int x=1; x<WIDTH-1; x++) {
    int y=HEIGHT-2;
    int holes = 0;
    int endY = 0;
    while(dumpMatrix[x][endY]==0) {
      endY++;
    }
    while(y>=endY) {
      if (dumpMatrix[x][y]==0) {
        holes++;
      }
      y--;
    }
   // Serial.printf("HOLES=%d x=%d\n", holes, x);
    if (holes<HEIGHT-2) {
      allHoles+=holes;
    }

  }
  int highest = 0;
  while( countFilled(highest, dumpMatrix)==0) {
    highest++;
  }
  highest = HEIGHT-highest-1;
  
  /*
  for (int y=0; y<HEIGHT; y++) {
    for (int x=0; x<WIDTH; x++) {
      Serial.printf("%d", dumpMatrix[x][y]);
    }
    Serial.printf("\n");
  }
  Serial.printf("rowsRem=%d  holes=%d  highest=%d   score=%d\n",rowsRemoved, allHoles, highest, (10*rowsRemoved - allHoles*5 - highest - (HEIGHT-py)) );

  
  
  Serial.printf("----------------\n");
  */
  
    return 10*rowsRemoved - allHoles*5 - (highest*3) - (HEIGHT-py);
}

int countFilled(int row, int mat[WIDTH][HEIGHT]) {
  int filled = 0;
  for (int x=1; x<WIDTH-1; x++) {
    if (mat[x][row]!=0) {
      filled++;
    }
  }
  return filled;
}

void dumpMat(int sx, int sy) {
  for (int y=0; y<4; y++) {
    for (int x=0; x<4; x++) {
      Serial.printf("%d", matrix[sx+x][sy+y]);
    }
    Serial.printf("\n");
  }
}

boolean isCollision(int dRot, int dX, int dY, int mat[WIDTH][HEIGHT]) {
 // Serial.printf("-----------------\nrot=%d, coord=%d/%d\n", rot, posX, posY);
 // dumpTile();
 // Serial.printf("--\n");
//  dumpMatrix(posX+dX, posY+dY);
  boolean collision = false;
  for (int x=0; x<4 && !collision; x++) {
    for (int y=0; y<4 && !collision; y++) {
       int tilePixel = tiles[(rot+dRot)&0x3][tileId][y][x];
       if (tilePixel) {
         int backgroundPixel =  mat[posX+x+dX][posY+y+dY];
         if (backgroundPixel) {
           collision=true;
          // Serial.printf("Kollision: %d/%d - %d\n", x, y, backgroundPixel);
         }
       }
    }
  }
  return collision;
}

boolean isCollisionAbs(int absRot, int absX, int absY, int mat[WIDTH][HEIGHT]) {
  boolean collision = false;
  for (int x=0; x<4 && !collision; x++) {
    for (int y=0; y<4 && !collision; y++) {
       int tilePixel = tiles[absRot][tileId][y][x];
       if (tilePixel) {
         int backgroundPixel =  mat[absX+x][absY+y];
         if (backgroundPixel) {
           collision=true;          
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
      case CMD_AUTO: toggleAuto(); break;
    }
  }
}

void toggleAuto() {
  Serial.printf("toggle auto!\n");
  if (status==STATUS_GAME) {
    status=STATUS_AUTO;
    Serial.printf("auto!\n");
  }
  else if (status==STATUS_AUTO) {
    status=STATUS_GAME;
    Serial.printf("game!\n");
  }
  else {
    Serial.printf("start!\n");
     start();
     status=STATUS_AUTO;
  }
}

void right() {
   if (!isCollision(0, 1, 0, matrix)) {
     posX+=1;
   }
}

void left() {
   if (!isCollision(0, -1, 0, matrix)) {
     posX-=1;
   }
}

void nextTile() {
  tileId = nextTileId;
  nextTileId = random8() % 7;
  rot = tileId<=4 ? 0 : 1;  // Teile L und J können um 90° gedreht erscheinen, um Platz zu sparen.
  posX = (WIDTH-4)/2;  
  posY = 0; 
  
  if (isCollision(0, 0, 0, matrix)) {
    gameOver();
  }
  else {
    sendNextTile();
  }
  calculateTarget();
}

boolean turn() {
  
  if (isCollision(1, 0, 0, matrix)) {    
    //Serial.printf("posX %d\n", posX);
    if (posX<=0) {
      
      for (int x=1; x<4; x++) {
        if (!isCollision(1, x, 0, matrix)) {
          
           posX+=x;
           rot = (rot + 1) & 0x3;
           return true;
        }
      }      
    }
    else if (posX>=WIDTH-4) {
      for (int x=1; x<4; x++) {
        if (!isCollision(1, -x, 0, matrix)) {          
           posX-=x;
           rot = (rot + 1) & 0x3;
           return true;
        }
      }     
    }
     return false;
  }   
  rot = (rot + 1) & 0x3;
  return true;
}

void down() {
  fastDown = true;
  fallCounter = FAST_DOWN_SPEED;
}

void start() {
  levelTimestamp = xTaskGetTickCount();
  speed = START_SPEED;
  fallCounter = speed;
  fastDown = false;
  level = 1;
  resetMatrix();
  tileId = random8() % 7;
  nextTileId = random8() % 7;
  calculateTarget();
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

void setLed(int y, int x, CRGB color) {
  y=WIDTH-y-1;
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


// =======================================================================

void screensaver() {
    uint32_t ms = millis();
    int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / WIDTH));
    int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / HEIGHT));
    DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);

    FastLED.show();
}

void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
  byte lineStartHue = startHue8;
  for( byte y = 0; y < HEIGHT; y++) {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;      
    for( byte x = 0; x < WIDTH; x++) {
      pixelHue += xHueDelta8;
      setLed(y, x, CHSV( pixelHue, 255, 155));      
    }
  }
}
