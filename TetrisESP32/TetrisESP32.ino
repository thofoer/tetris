#include <BluetoothSerial.h>
#include <FastLED.h>

#define CMD_RIGHT 'r'
#define CMD_LEFT 'l'
#define CMD_TURN 't'
#define CMD_DOWN 'd'
#define CMD_START 's'
#define CMD_RESET 'x'

#define MSG_LEVEL 'L'
#define MSG_SCORE 'S'

#define tileO 0
#define tileI 1
#define tileT 2
#define tileZ 3
#define tileS 4
#define tileL 5
#define tileJ 6

#define WIDTH 8
#define HEIGHT 8
#define NUM_LEDS (WIDTH*HEIGHT)

#define LED_PIN     5
#define BRIGHTNESS  32
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB


CRGB leds[NUM_LEDS];

BluetoothSerial SerialBT;

int level = 1;
int tileId = 0;
int posX = 2;
int posY = 0;
int rot = 0;

int matrix[WIDTH][HEIGHT];

const int tiles[4][7][4][4] ={ 
  
                       // 0°
                      { 
                      { {0, 0, 0, 0},    // O 
                        {0, 1, 1, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // I
                        {1, 1, 1, 1},
                        {0, 0, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // T
                        {1, 1, 1, 0},
                        {0, 1, 0, 0},
                        {0, 0, 0, 0} },
            
                      { {0, 0, 0, 0},    // Z
                        {1, 1, 0, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // S
                        {0, 1, 1, 0},
                        {1, 1, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 0, 0},    // L
                        {0, 1, 0, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 1, 0},    // J
                        {0, 0, 1, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} }
                     },
                     
                     // 90°
                     {   
                      { {0, 0, 0, 0},    // O 
                        {0, 1, 1, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 0, 0},    // I
                        {0, 1, 0, 0},
                        {0, 1, 0, 0},
                        {0, 1, 0, 0} },

                      { {0, 1, 0, 0},    // T
                        {1, 1, 0, 0},
                        {0, 1, 0, 0},
                        {0, 0, 0, 0} },
            
                      { {0, 0, 1, 0},    // Z
                        {0, 1, 1, 0},
                        {0, 1, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 0, 0},    // S
                        {0, 1, 1, 0},
                        {0, 0, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // L
                        {1, 1, 1, 0},
                        {1, 0, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // J
                        {1, 0, 0, 0},
                        {1, 1, 1, 0},
                        {0, 0, 0, 0} }
                     },
                     // 180°
                     {
                      { {0, 0, 0, 0},    // O 
                        {0, 1, 1, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // I
                        {1, 1, 1, 1},
                        {0, 0, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 0, 0},    // T
                        {1, 1, 1, 0},
                        {0, 0, 0, 0},
                        {0, 0, 0, 0} },
            
                      { {0, 0, 0, 0},    // Z
                        {1, 1, 0, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // S
                        {0, 1, 1, 0},
                        {1, 1, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 1, 0},    // L
                        {0, 0, 1, 0},
                        {0, 0, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 1, 0},    // J
                        {0, 1, 0, 0},
                        {0, 1, 0, 0},
                        {0, 0, 0, 0} },
                     },
                   // 270°
                     { 
                      { {0, 0, 0, 0},    // O 
                        {0, 1, 1, 0},
                        {0, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 0, 0},    // I
                        {0, 1, 0, 0},
                        {0, 1, 0, 0},
                        {0, 1, 0, 0} },

                      { {0, 1, 0, 0},    // T
                        {0, 1, 1, 0},
                        {0, 1, 0, 0},
                        {0, 0, 0, 0} },
            
                      { {0, 0, 1, 0},    // Z
                        {0, 1, 1, 0},
                        {0, 1, 0, 0},
                        {0, 0, 0, 0} },

                      { {0, 1, 0, 0},    // S
                        {0, 1, 1, 0},
                        {0, 0, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // L
                        {0, 0, 1, 0},
                        {1, 1, 1, 0},
                        {0, 0, 0, 0} },

                      { {0, 0, 0, 0},    // J
                        {1, 1, 1, 0},
                        {0, 0, 1, 0},
                        {0, 0, 0, 0} }

                     }};

// Anzahl freie Spalten von Links für jedes Teil für alle 4 Rotationen

const int offsetLeft[4][7] = { {1, 0, 0, 0, 0, 1, 1 },
                               {1, 1, 0, 1, 1, 0, 0 },
                               {1, 0, 0, 0, 0, 1, 1 },
                               {1, 1, 1, 1, 1, 0, 0 } };

                           
 
const CRGB colors[7] = { CRGB(127, 127, 0), CRGB::MidnightBlue, CRGB::Green, CRGB::Indigo, CRGB::MediumTurquoise, CRGB::Crimson, CRGB::DarkOrange };

void setup() {
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
    
  Serial.begin(115200);
  SerialBT.begin("Tetris"); //Name des ESP32
  Serial.println("Der ESP32 ist bereit. Verbinde dich nun über Bluetooth.");

  for (int x=0; x<WIDTH; x++) {
    for (int y=0; y<HEIGHT; y++) {
      matrix[x][y] = 0;
      leds[x*WIDTH+y] = CRGB::Black;
    }
  }
}

void clearLeds() {
  for (int x=0; x<WIDTH; x++) {
    for (int y=0; y<HEIGHT; y++) {   
      leds[x*WIDTH+y] = CRGB::Black;
    }
  }
}

void showTile() {
  setLed(0,0, CRGB::DarkGrey);
  setLed(7,0, CRGB::DarkBlue);
  for (int x=0; x<4; x++) {
    for (int y=0; y<4; y++) {
      int pixel = tiles[rot][tileId][y][x];
      if (pixel) {
        setLed(x+posX, y+posY, colors[tileId]);
      }
    }
  }
}

void loop() {
  
  char input;
  
  if (SerialBT.available()) {
    input = (char) SerialBT.read();
    switch (input) {
      case CMD_RIGHT: right(); break;
      case CMD_LEFT: left(); break;
      case CMD_TURN: turn(); break;
    }
  }
  delay(100);
  clearLeds();
  showTile();
  FastLED.show();
}

void right() {
   Serial.println("rechts");
   uint8_t b[2];
   b[0] = MSG_LEVEL;
   b[1] = level;
   SerialBT.write(b, 2);   
   level++;
}

void left() {
   Serial.println("links");
   uint8_t b[2];
   b[0] = MSG_SCORE;
   b[1] = 3;
   SerialBT.write(b, 2);   
   tileId = (tileId+1) %7;
}

void turn() {
  rot = (rot +1 ) % 4;
}

void setLed(int x, int y, CRGB color) {
  int i = y*WIDTH;
  i += (y&1) ? x : (WIDTH-x-1);
  leds[i] = color;
}
