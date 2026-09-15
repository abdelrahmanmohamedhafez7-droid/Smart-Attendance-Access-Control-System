#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>
#include <PCF8574.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= PCF8574 =================
PCF8574 pcf8574(0x20);

// ================= SERVO =================
Servo doorMotor;

// ================= FINGERPRINT =================
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ================= ULTRASONIC =================
int trigPin = A2;
int echoPin = A3;

long duration;
int distance;

// ================= SYSTEM =================
String input = "";
String adminKey = "2005";
String adminInput = "";
bool adminMode = false;

int greenLED = 11;
int redLED = 10;

int buzzer = 13;

int resetPin = 5;
int adminBtn = 6;

int wrongCount = 0;
bool locked = false;

unsigned long lastBtnPress = 0;

bool buzzerMuted = false;
bool personDetected = false;

// ================= USERS =================
#define USERS_COUNT 5

String userIDs[USERS_COUNT] = {
  "4231",
  "0129",
  "0053",
  "0123",
  "0012"
};

String userNames[USERS_COUNT] = {
  "Eng Bakinam",
  "Abdelrahman",
  "AhmedWalied",
  "Yousef Mohamed",
  "Mohamed Tamer"
};

// fingerprint IDs
int fingerIDs[USERS_COUNT] = {
  1,
  2,
  3,
  4,
  5
};

// attendance tracking
int accessCount[USERS_COUNT] = {0,0,0,0,0};

int currentUser = -1;

// ================= KEYPAD =================
char keys[4][4] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

int rowPins[4] = {0,1,2,3};
int colPins[4] = {4,5,6,7};

char getKey() {

  for (int r = 0; r < 4; r++) {

    for (int i = 0; i < 4; i++) {
      pcf8574.write(rowPins[i], HIGH);
    }

    pcf8574.write(rowPins[r], LOW);

    for (int c = 0; c < 4; c++) {

      if (pcf8574.read(colPins[c]) == LOW) {

        delay(200);

        while (pcf8574.read(colPins[c]) == LOW);

        return keys[r][c];
      }
    }
  }

  return '\0';
}

// ================= FIND USER =================
int getUserByFingerprint(int fingerID) {

  for (int i = 0; i < USERS_COUNT; i++) {

    if (fingerIDs[i] == fingerID) {
      return i;
    }
  }

  return -1;
}

// ================= GET FINGERPRINT =================
int getFingerprintID() {

  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK)
    return -1;

  p = finger.fingerFastSearch();

  if (p != FINGERPRINT_OK)
    return -1;

  return finger.fingerID;
}

// ================= RESET SESSION =================
void resetSession() {

  input = "";
  currentUser = -1;

  lcd.clear();
  lcd.print("Waiting...");
}

// ================= SETUP =================
void setup() {

  Wire.begin();

  pcf8574.begin();

  for (int i = 0; i < 4; i++) {
    pcf8574.write(rowPins[i], HIGH);
    pcf8574.write(colPins[i], HIGH);
  }

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  pinMode(buzzer, OUTPUT);

  pinMode(resetPin, INPUT);
  pinMode(adminBtn, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  lcd.init();
  lcd.backlight();

  doorMotor.attach(7);
  doorMotor.write(0);

  // fingerprint
  finger.begin(57600);

  lcd.setCursor(0,0);
  lcd.print("Waiting...");
}

// ================= LOOP =================
void loop() {

  // ===== Ultrasonic =====
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  distance = duration * 0.034 / 2;

  personDetected = (distance < 20);

  // ===== IF PERSON LEFT =====
  if (!personDetected) {

    input = "";
    currentUser = -1;

    lcd.setCursor(0,0);
    lcd.print("Waiting...      ");

    lcd.setCursor(0,1);
    lcd.print("                ");

    return;
  }

  // ===== RESET BUTTON =====
  if (digitalRead(resetPin) == HIGH && millis() - lastBtnPress > 300) {

    lastBtnPress = millis();

    if (!locked) {

      if (input.length() > 0)
        input.remove(input.length() - 1);

      lcd.setCursor(0,1);
      lcd.print("                ");

      lcd.setCursor(0,1);
      lcd.print(input);

    } else {

      noTone(buzzer);
      buzzerMuted = true;
    }
  }

  // ===== ADMIN MODE =====
  if (digitalRead(adminBtn) == HIGH && locked) {

    adminMode = true;
    adminInput = "";

    lcd.clear();
    lcd.print("Admin Key:");

    delay(300);
  }

  if (adminMode) {

    char key = getKey();

    if (key) {

      if (key == '*') {

        adminInput = "";

      } else if (key == '#') {

        if (adminInput == adminKey) {

          locked = false;
          wrongCount = 0;
          buzzerMuted = false;

          digitalWrite(redLED, LOW);

          noTone(buzzer);

          lcd.clear();
          lcd.print("Unlocked");

          delay(1500);

          resetSession();

          adminMode = false;

        } else {

          lcd.clear();
          lcd.print("Wrong Key");

          delay(1000);

          lcd.clear();
          lcd.print("Try Again:");

          adminInput = "";
        }

      } else {

        if (adminInput.length() < 4) {
          adminInput += key;
        }
      }

      lcd.setCursor(0,1);
      lcd.print("                ");

      lcd.setCursor(0,1);
      lcd.print(adminInput);
    }

    return;
  }

  // ===== LOCK =====
  if (locked) {

    digitalWrite(redLED, HIGH);

    if (!buzzerMuted)
      tone(buzzer, 1000);

    lcd.setCursor(0,0);
    lcd.print("SYSTEM LOCKED! ");

    return;
  }

  // ===== MAIN =====
  lcd.setCursor(0,0);
  lcd.print("Put Finger...  ");

  int fingerID = getFingerprintID();

  // no finger
  if (fingerID == -1)
    return;

  currentUser = getUserByFingerprint(fingerID);

  // wrong fingerprint
  if (currentUser == -1) {

    lcd.clear();
    lcd.print("Unauthorized");

    tone(buzzer, 1000);

    delay(1500);

    noTone(buzzer);

    resetSession();

    return;
  }

  // already used twice
  if (accessCount[currentUser] >= 2) {

    lcd.clear();
    lcd.print("Access Denied");

    delay(2000);

    resetSession();

    return;
  }

  // ===== PASSWORD =====
  input = "";

  lcd.clear();
  lcd.print("Enter Password");

  while (input.length() < 4) {

    // if user moved away
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);

    distance = duration * 0.034 / 2;

    personDetected = (distance < 20);

    if (!personDetected) {

      resetSession();
      return;
    }

    // backspace
    if (digitalRead(resetPin) == HIGH && millis() - lastBtnPress > 300) {

      lastBtnPress = millis();

      if (input.length() > 0)
        input.remove(input.length() - 1);

      lcd.setCursor(0,1);
      lcd.print("                ");

      lcd.setCursor(0,1);
      lcd.print(input);
    }

    char key = getKey();

    if (key) {

      input += key;

      lcd.setCursor(0,1);
      lcd.print("                ");

      lcd.setCursor(0,1);
      lcd.print(input);
    }
  }

  // ===== CHECK PASSWORD =====
  if (input == userIDs[currentUser]) {

    digitalWrite(greenLED, HIGH);

    digitalWrite(redLED, LOW);

    noTone(buzzer);

    lcd.clear();

    // first time = attendance
    if (accessCount[currentUser] == 0) {

      lcd.print("Welcome");

    }
    // second time = leaving
    else if (accessCount[currentUser] == 1) {

      lcd.print("Thank You");
    }

    lcd.setCursor(0,1);
    lcd.print(userNames[currentUser]);

    doorMotor.write(90);

    delay(2000);

    doorMotor.write(0);

    delay(1000);

    digitalWrite(greenLED, LOW);

    wrongCount = 0;

    accessCount[currentUser]++;

  } else {

    wrongCount++;

    digitalWrite(redLED, HIGH);

    tone(buzzer, 1000);

    lcd.clear();
    lcd.print("Wrong Password");

    delay(1500);

    noTone(buzzer);

    digitalWrite(redLED, LOW);

    if (wrongCount >= 3)
      locked = true;
  }

  input = "";

  delay(1000);

  resetSession();
}