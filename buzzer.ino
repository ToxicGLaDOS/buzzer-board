#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3c

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO

#define NUM_PLAYERS 16
#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8
#define NUM_BUTTONS 4

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int16_t scores[NUM_PLAYERS] = {};


enum scrollState{WAITING, SCROLLING_UP, SCROLLING_DOWN};
enum buttonState{PRESSED, RELEASED};
enum gameState{BUZZERS_LOCKED, BUZZERS_UNLOCKED, PLAYER_BUZZED};
enum mode{GAME, EDIT, SCORES};

struct scrollData {
  int16_t yOffset = 0;
  int16_t waitTimer = 0;
  const int16_t pauseTime = 50;
  const int16_t scrollSpeed = 1;
  scrollState state = WAITING;

} scrollData;

struct button {
  buttonState state = RELEASED; // Current button state
  bool pressed = false; // Pressed this "frame"
  uint8_t pin;
};

struct editModeData {
  uint16_t selectedPlayer = 0;
  uint16_t blinkTimer = 0;
  uint16_t yOffset = 0;
  bool visible = true;
  const uint16_t blinkDuration = 10;
} editModeData;

struct gameModeData {
  gameState state = BUZZERS_LOCKED;
  uint16_t activePlayer = 0; // The player who is currently buzzed in
} gameModeData;

button whiteBtn;
button greenBtn;
button blueBtn;
button redBtn;

button* buttons[NUM_BUTTONS] = {&whiteBtn, &greenBtn, &blueBtn, &redBtn};

mode currentMode = GAME;

void drawString(int16_t x, int16_t y, String text, uint16_t foregroundColor, uint16_t backgroundColor, u_int16_t size) {
  uint16_t charSize = CHAR_WIDTH * size;
  for (uint16_t i = 0; i < text.length(); i++){
    char c = text[i];
    display.drawChar(x + i*charSize, y, c, foregroundColor, backgroundColor, size);
  }
}

void showScores() {
  char scoreBuffer[15];
  const int16_t scrollMax = CHAR_HEIGHT*(NUM_PLAYERS) - SCREEN_HEIGHT;


  drawString(63, 0, "SCORES", MONOOLED_WHITE, MONOOLED_BLACK, 1);
  drawString(63, CHAR_HEIGHT*2, "BL: EXIT", MONOOLED_WHITE, MONOOLED_BLACK, 1);

  for (uint16_t i = 0; i < NUM_PLAYERS; i++) {
    sprintf(scoreBuffer, "p%d: %d", i, scores[i]);
    drawString(0, CHAR_HEIGHT*i - scrollData.yOffset, scoreBuffer, MONOOLED_WHITE, MONOOLED_BLACK, 1);
  }

  switch (scrollData.state){
    case WAITING:
      scrollData.waitTimer++;
      if (scrollData.waitTimer >= scrollData.pauseTime){
        scrollData.waitTimer = 0;
        if (scrollData.yOffset <= 0) {
          scrollData.state = SCROLLING_DOWN;
        }
        else {
          scrollData.state = SCROLLING_UP;
        }
      }
      break;
    case SCROLLING_DOWN:
      if(scrollData.yOffset >= scrollMax) {
        scrollData.state = WAITING;
      }
      else {
        scrollData.yOffset += scrollData.scrollSpeed;
      }
      break;
    case SCROLLING_UP:
      if(scrollData.yOffset <= 0) {
        scrollData.state = WAITING;
      }
      else {
        scrollData.yOffset -= scrollData.scrollSpeed;
      }
      break;
  }

  if (blueBtn.pressed) {
    currentMode = GAME;
  }
}

void editScores() {
  char scoreBuffer[15];
  
  editModeData.blinkTimer++;
  if (editModeData.blinkTimer > editModeData.blinkDuration) {
    editModeData.blinkTimer = 0;
    editModeData.visible = !editModeData.visible;
  }

  drawString(63, 0, "EDIT MODE", MONOOLED_WHITE, MONOOLED_BLACK, 1);
  drawString(63, CHAR_HEIGHT*2, "GREEN: +1", MONOOLED_WHITE, MONOOLED_BLACK, 1);
  drawString(63, CHAR_HEIGHT*3, "RED: -1", MONOOLED_WHITE, MONOOLED_BLACK, 1);
  drawString(63, CHAR_HEIGHT*4, "WHITE: NEXT", MONOOLED_WHITE, MONOOLED_BLACK, 1);
  drawString(63, CHAR_HEIGHT*5, "BLUE: EXIT", MONOOLED_WHITE, MONOOLED_BLACK, 1);

  for (uint16_t i = 0; i < NUM_PLAYERS; i++) {
    sprintf(scoreBuffer, "p%d: %d", i, scores[i]);
    uint8_t color = editModeData.selectedPlayer != i || editModeData.visible ? MONOOLED_WHITE : MONOOLED_BLACK;
    drawString(0, CHAR_HEIGHT*i - editModeData.yOffset, scoreBuffer, color, MONOOLED_BLACK, 1);
  }

  if (whiteBtn.pressed) {
    editModeData.selectedPlayer++;
    editModeData.selectedPlayer %= NUM_PLAYERS;
    // A little hacky, but probably good enough
    if (editModeData.selectedPlayer == 0) {
      editModeData.yOffset = 0;
    }
    if (editModeData.selectedPlayer > 7) {
      editModeData.yOffset += CHAR_HEIGHT;
    }
  }
  if(greenBtn.pressed) {
    scores[editModeData.selectedPlayer]++;
  }
  if(redBtn.pressed) {
    scores[editModeData.selectedPlayer]--;
  }
  if(blueBtn.pressed) {
    currentMode = GAME;
  }
}

// Poll all the daughter boards
void checkBuzzers() {
  if (whiteBtn.pressed) {
    gameModeData.state = PLAYER_BUZZED;
    gameModeData.activePlayer = random(0, NUM_PLAYERS);
  }
}

void game() {
  drawString(63, 0, "GAME MODE", MONOOLED_WHITE, MONOOLED_BLACK, 1);

  switch (gameModeData.state)
  {
  case BUZZERS_LOCKED:
    drawString(63, CHAR_HEIGHT*2, "WH: UNLOCK", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(63, CHAR_HEIGHT*3, "GR: SCORES", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(63, CHAR_HEIGHT*4, "BL: EDIT", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, 0, "Buzzers", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, CHAR_HEIGHT, "locked", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    if (whiteBtn.pressed) {
      gameModeData.state = BUZZERS_UNLOCKED;
    }
    if (greenBtn.pressed) {
      currentMode = SCORES;
    }
    if (blueBtn.pressed) {
      currentMode = EDIT;
    }
    break;

  case BUZZERS_UNLOCKED:
    drawString(63, CHAR_HEIGHT*2, "BL: EDIT", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(63, CHAR_HEIGHT*3, "RE: LOCK &", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(63, CHAR_HEIGHT*4, "    RESET", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, 0, "Buzzers", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, CHAR_HEIGHT, "ready!", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, CHAR_HEIGHT*3, "Waiting", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, CHAR_HEIGHT*4, "for buzz in.", MONOOLED_WHITE, MONOOLED_BLACK, 1);

    checkBuzzers();

    if (blueBtn.pressed) {
      currentMode = EDIT;
    }
    if (redBtn.pressed) {
      gameModeData.state = BUZZERS_LOCKED;
    }
    break;
  
  case PLAYER_BUZZED:
    char buffer[5];
    sprintf(buffer, "p%d", gameModeData.activePlayer);
    drawString(63, CHAR_HEIGHT*2, "GREEN: YES", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(63, CHAR_HEIGHT*3, "RED: NO", MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, 0, buffer, MONOOLED_WHITE, MONOOLED_BLACK, 1);
    drawString(0, CHAR_HEIGHT, "buzzed in", MONOOLED_WHITE, MONOOLED_BLACK, 1);

    if (greenBtn.pressed) {
      scores[gameModeData.activePlayer]++;
      gameModeData.state = BUZZERS_LOCKED;
    }
    if (redBtn.pressed) {
      gameModeData.state = BUZZERS_UNLOCKED;
    }
    break;

  default:
    break;
  }
}

// Sets up button states
void updateButtons() {
  for (uint16_t i = 0; i < NUM_BUTTONS; i++) {
    button* button = buttons[i];
    buttonState state = digitalRead(button->pin) == LOW ? PRESSED : RELEASED;

    button->pressed = false; // Reset back to false every "frame"
    if (state == PRESSED && state != button->state){
      button->pressed = true;
    }
    button->state = state;
  }
}

void setup() {
  display.begin(i2c_Address, true); // Address 0x3C default
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(25, INPUT_PULLUP);
  pinMode(18, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);

  whiteBtn.pin = 25;
  greenBtn.pin = 18;
  blueBtn.pin = 32;
  redBtn.pin = 23;
  
}

void loop() {
  updateButtons();
  
  display.clearDisplay();

  switch (currentMode)
  {
  case GAME:
    game();
    break;
  case EDIT:
    editScores();
    break;
  case SCORES:
    showScores();
    break;
  default:
    break;
  }
  display.display();
}