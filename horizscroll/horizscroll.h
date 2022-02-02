typedef struct {
  byte x; 		// Hero x coordinate
  byte y; 		// Hero y coordinate
  byte dir;		// Hero direction
  int collided:1;	// Hero collided value
  word bit1;		// Hero score    0-9
  word bit2;		// Hero score   10-99
  word bit3;		// Hero score  100-999
  word bit4;		// Hero score 1000-9999

} Hero;

typedef struct {
  byte x;		// Heart x coordinate
  byte y;		// Heart y coordinate

} Heart;


//Prototypes;
void play(void);
void init_game(void);
void game_over(void);
void start_game(void);
void main_scroll(void);
void clrscrn(void);
void add_point(Hero*);
void title_screen(void);