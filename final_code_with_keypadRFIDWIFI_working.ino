#define BLYNK_TEMPLATE_ID "TMPL3jQOZ95Wg"
#define BLYNK_TEMPLATE_NAME "SmartLock"
#define BLYNK_AUTH_TOKEN "2wL6RuM8EAJqHKhZVfrk4qNLDsVBTZ-g"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

// Wi-Fi credentials
char ssid[] = "Euphoria";
char pass[] = "qenb7354";
// char ssid[] = "The Nest 4th floor";
// char pass[] = "9560524023";
// char ssid[] = "NeelRay28";
// char pass[] = "28Niellr@y13";

// Blynk Timer
BlynkTimer timer;

// Servo setup
Servo lockServo;
const int servoPin = 15;
bool isUnlocked = false;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID setup
#define RST_PIN 4
#define SS_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Registered RFID UID (replace with actual UID)
byte registeredUID[] = {0x63, 0x66, 0x69, 0x14};

// Keypad setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {33, 32, 25, 26};
byte colPins[COLS] = {27, 14, 12};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

char password[5];  
byte pwdindex = 0;
const char correctPassword[] = "7777";

// ==================== UNLOCK FUNCTION ====================
void unlockLock() {
  if (!isUnlocked) {
    lockServo.write(90);  // Unlock
    lcd.clear(); lcd.setCursor(0, 0);
    lcd.print("Unlocked!");
    isUnlocked = true;
    Serial.println("[INFO] Unlocked!");
    timer.setTimeout(5000L, autoLock); // Auto-lock after 5s
  }
}

void autoLock() {
  lockServo.write(0);  // Lock
  lcd.clear(); lcd.setCursor(0, 0);
  lcd.print("Locked Again");
  Serial.println("[INFO] Locked Again!");
  delay(1000);
  lcd.clear(); lcd.print("Scan or Enter");
  isUnlocked = false;
}

// ==================== BLYNK HANDLER ====================
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1) {
    unlockLock();
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init(); lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Connecting...");

  // Setup servo
  lockServo.attach(servoPin);
  lockServo.write(0);

  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n[INFO] WiFi Connected");

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // RFID Setup
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("[INFO] RFID Initialized");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan or Enter");
}

// ==================== LOOP ====================
void loop() {
  Blynk.run();
  timer.run();
  handleKeypad();
  handleRFID();
}

// ==================== KEYPAD ====================
void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    if (key == '*') {
      pwdindex = 0;
      memset(password, 0, sizeof(password));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Password:");
    } 
    else if (pwdindex < 4 && isDigit(key)) {
      password[pwdindex++] = key;
      lcd.setCursor(pwdindex - 1, 1);
      lcd.print("*");

      if (pwdindex == 4) {
        password[pwdindex] = '\0'; // Null-terminate string
        delay(200); 

        Serial.print("[INFO] Entered Password: ");
        Serial.println(password);  

        if (strcmp(password, correctPassword) == 0) {
          unlockLock();
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong Password");
          Serial.println("[ERROR] Wrong Password!");
          delay(1500);
          lcd.clear();
          lcd.print("Scan or Enter");
        }

        pwdindex = 0;
        memset(password, 0, sizeof(password));
      }
    }
  }
}

// ==================== RFID ====================
void handleRFID() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;  
  if (!mfrc522.PICC_ReadCardSerial()) return;  

  Serial.println("[INFO] Card Detected!");

  bool match = true;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
    if (mfrc522.uid.uidByte[i] != registeredUID[i]) {
      match = false;
    }
  }
  Serial.println();

  if (match) {
    Serial.println("[INFO] Access Granted!");
    unlockLock();
  } else {
    lcd.clear(); lcd.print("Access Denied");
    Serial.println("[ERROR] Access Denied!");
    delay(1500);
    lcd.clear(); lcd.print("Scan or Enter");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(500);  // Allow RFID to reset before next scan
}
