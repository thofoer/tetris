
// von TetrisControl empfangene Kommandos
#define CMD_RIGHT 'r'
#define CMD_LEFT 'l'
#define CMD_TURN 't'
#define CMD_DOWN 'd'
#define CMD_START 's'
#define CMD_RESET 'x'

// an TetrisControl gesendete Messages
#define MSG_LEVEL 'L'
#define MSG_SCORE 'S'
#define MSG_NEXT_TILE 'N'
#define MSG_GAME_OVER 'O'

// Pixel ID in matrix-Array
#define  EMPTY_PIX  0
#define  TILE_O_PIX  1
#define  TILE_I_PIX  2
#define  TILE_T_PIX  3
#define  TILE_Z_PIX  4
#define  TILE_S_PIX  5
#define  TILE_L_PIX  6
#define  TILE_J_PIX  7
#define  FRAME_PIX   8

// Indizes der Teile im tiles Array
#define  TILE_O  0
#define  TILE_I  1
#define  TILE_T  2
#define  TILE_Z  3
#define  TILE_S  4
#define  TILE_L  5
#define  TILE_J  6

// Farben für Teile und Rahmen
#define COLOR_FRAME  CRGB(28, 11, 3)
#define COLOR_TILE_O CRGB(127, 127, 0)
#define COLOR_TILE_I CRGB::MidnightBlue
#define COLOR_TILE_T CRGB::Green
#define COLOR_TILE_Z CRGB::Indigo
#define COLOR_TILE_S CRGB::MediumTurquoise
#define COLOR_TILE_L CRGB::Crimson
#define COLOR_TILE_J CRGB::DarkOrange


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

const CRGB colors[9] = {CRGB::Black, COLOR_TILE_O, COLOR_TILE_I, COLOR_TILE_T, COLOR_TILE_Z, COLOR_TILE_S, COLOR_TILE_L, COLOR_TILE_J, COLOR_FRAME };
