
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>


// pin connections 

// TFT display
#define TFT_SCLK D9
#define TFT_MOSI D10
#define TFT_RST  D8
#define TFT_DC   D4
#define TFT_CS   D5
#define TFT_BL   D6

// buzzer
#define BUZZER_PIN D7

// buttons
#define BUTTON_1 D3
#define BUTTON_2 D2
#define BUTTON_3 D1
#define BUTTON_4 D0


// TFT display setup

class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi,
           int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}

  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(
  TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST
);


// clock variables 


// Initial clock time: 12:00

uint32_t totalSeconds = 12UL * 60UL * 60UL;
uint32_t lastSecondTick = 0;

// Initial alarm time: 07:00

int alarmHour = 7;
int alarmMinute = 0;

bool alarmEnabled = true;
bool alarmRinging = false;



enum UiMode {
  CLOCK_MODE,
  SET_CLOCK_MODE,
  SET_ALARM_MODE
};

UiMode currentMode = CLOCK_MODE;

bool editingMinutes = false;


// button debouncing variables


const uint8_t buttonPins[4] = {
  BUTTON_1,
  BUTTON_2,
  BUTTON_3,
  BUTTON_4
};

bool lastReading[4] = {HIGH, HIGH, HIGH, HIGH};
bool stableState[4] = {HIGH, HIGH, HIGH, HIGH};

uint32_t lastDebounceTime[4] = {0, 0, 0, 0};

const uint32_t debounceDelay = 35;


// buzzer variables


uint32_t lastBuzzerToggle = 0;
bool buzzerState = false;


// display variables

uint32_t lastDisplayUpdate = 0;


// Convert the current clock time into hours.

int getHour() {
  return (totalSeconds / 3600) % 24;
}

// Convert the current clock time into minutes.

int getMinute() {
  return (totalSeconds / 60) % 60;
}

// Convert the current clock time into seconds.

int getSecond() {
  return totalSeconds % 60;
}

// Print a number with two digits.

void printTwoDigits(int number) {
  if (number < 10) {
    tft.print("0");
  }

  tft.print(number);
}


bool buttonPressed(uint8_t index) {

  bool reading = digitalRead(buttonPins[index]);

  if (reading != lastReading[index]) {
    lastDebounceTime[index] = millis();
  }

  if (millis() - lastDebounceTime[index] > debounceDelay) {

    if (reading != stableState[index]) {

      stableState[index] = reading;

      if (stableState[index] == LOW) {
        lastReading[index] = reading;
        return true;
      }
    }
  }

  lastReading[index] = reading;

  return false;
}


void adjustValue(int direction) {

  if (currentMode == SET_CLOCK_MODE) {

    int hour = getHour();
    int minute = getMinute();

    if (editingMinutes) {
      minute = (minute + direction + 60) % 60;
    } else {
      hour = (hour + direction + 24) % 24;
    }

    totalSeconds =
      (uint32_t)hour * 3600UL +
      (uint32_t)minute * 60UL;

    lastSecondTick = millis();

  }

  else if (currentMode == SET_ALARM_MODE) {

    if (editingMinutes) {
      alarmMinute =
        (alarmMinute + direction + 60) % 60;
    } else {
      alarmHour =
        (alarmHour + direction + 24) % 24;
    }

    alarmEnabled = true;
  }
}

// butoons configuration

void checkButtons() {

  // Button 1: Increase value

  if (buttonPressed(0)) {
    adjustValue(1);
  }

  // Button 2: Decrease value

  if (buttonPressed(1)) {
    adjustValue(-1);
  }

  // Button 3: Change operating mode

  if (buttonPressed(2)) {

    if (currentMode == CLOCK_MODE) {
      currentMode = SET_CLOCK_MODE;
    }

    else if (currentMode == SET_CLOCK_MODE) {
      currentMode = SET_ALARM_MODE;
    }

    else {
      currentMode = CLOCK_MODE;
    }

    editingMinutes = false;
  }

  // Button 4: Select field or dismiss alarm

  if (buttonPressed(3)) {

    if (alarmRinging) {
      alarmRinging = false;
      buzzerState = false;
      digitalWrite(BUZZER_PIN, LOW);
    }

    else if (currentMode != CLOCK_MODE) {
      editingMinutes = !editingMinutes;
    }
  }
}


// update clock


void updateTime() {

  uint32_t now = millis();

  uint32_t elapsedSeconds =
    (now - lastSecondTick) / 1000;

  if (elapsedSeconds > 0) {

    totalSeconds += elapsedSeconds;

    lastSecondTick += elapsedSeconds * 1000;
  }
}


// check alarm


uint32_t lastCheckedMinute = 0;
uint32_t lastAlarmDay = UINT32_MAX;

void checkAlarm() {

  uint32_t currentMinute = totalSeconds / 60;

  if (currentMinute != lastCheckedMinute) {

    lastCheckedMinute = currentMinute;

    int currentHour = getHour();
    int currentMinuteOfHour = getMinute();

    uint32_t currentDay = totalSeconds / 86400;

    if (
      alarmEnabled &&
      currentHour == alarmHour &&
      currentMinuteOfHour == alarmMinute &&
      currentDay != lastAlarmDay
    ) {

      alarmRinging = true;
      lastAlarmDay = currentDay;
    }
  }
}


// buzzer setup


void updateBuzzer() {

  if (alarmRinging) {

    if (millis() - lastBuzzerToggle >= 400) {

      lastBuzzerToggle = millis();

      buzzerState = !buzzerState;

      digitalWrite(
        BUZZER_PIN,
        buzzerState ? HIGH : LOW
      );
    }

  } else {

    buzzerState = false;

    digitalWrite(BUZZER_PIN, LOW);
  }
}


// update display


void updateDisplay() {

  if (millis() - lastDisplayUpdate < 500) {
    return;
  }

  lastDisplayUpdate = millis();

  tft.fillScreen(ST77XX_BLACK);

  // Current time

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(12, 5);

  printTwoDigits(getHour());
  tft.print(":");

  printTwoDigits(getMinute());
  tft.print(":");

  printTwoDigits(getSecond());

  
  tft.setTextSize(1);
  tft.setCursor(12, 35);

  tft.print("ALARM: ");

  printTwoDigits(alarmHour);
  tft.print(":");
  printTwoDigits(alarmMinute);

  tft.print(alarmEnabled ? " ON" : " OFF");

  

  tft.setCursor(12, 52);

  if (currentMode == CLOCK_MODE) {
    tft.print("CLOCK MODE");
  }

  else if (currentMode == SET_CLOCK_MODE) {
    tft.print("SETTING CLOCK ");

    tft.print(editingMinutes ? "MIN" : "HOUR");
  }

  else {
    tft.print("SETTING ALARM ");

    tft.print(editingMinutes ? "MIN" : "HOUR");
  }

  

  if (alarmRinging) {
    tft.setCursor(190, 52);
    tft.setTextColor(ST77XX_RED);
    tft.print("RINGING!");
  }
}



// SETUP

void setup() {

  Serial.begin(115200);

  // Configure buttons

  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Configure buzzer

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

 
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);

  // Initialize display

  tft.init(76, 284);

  tft.setOffsets(82, 18);

  tft.invertDisplay(false);

  tft.setRotation(1);

  tft.fillScreen(ST77XX_BLACK);

  // Initialize clock timing

  lastSecondTick = millis();

  lastCheckedMinute = totalSeconds / 60;

  Serial.println("BLARE Clock Initialized!");
}

// LOOP


void loop() {

  checkButtons();

  updateTime();

  checkAlarm();

  updateBuzzer();

  updateDisplay();

}

