// ======================================================
//  SMART HOME - CUM TRUNG TAM (ESP32 Gateway)
//  Phong khach: den, quat, bao chay (MQ2 + coi).
//  Cua TRUOC: keypad mat khau + LCD + khoa dien (solenoid).
//  Cua SAU : co khoa dien rieng, mo bang nut ben trong.
//  Sao phoi do tu dong theo cam bien mua + 2 cong tac hanh trinh.
//
//  QUY TAC LINH KIEN (theo yeu cau):
//   - Linh kien Wokwi co san  -> dung that (LCD, MQ2, relay, keypad, buzzer).
//   - Linh kien Wokwi KHONG co -> dung LED thay the:
//       * Motor DC + L298N (sao phoi) -> 2 LED chi chieu KEO VAO / DAY RA.
//   - Cam bien khong co trong Wokwi (mua) la tin hieu DAU VAO -> dung NUT
//     bam gia lap (LED khong nhap tin hieu vao duoc).
//   - Cong tac hanh trinh = nut bam (CTHT thuc chat la 1 cong tac).
//
//  Luu y chan: keypad dung 4x3 (bo cot A/B/C/D) de nhuong 1 chan cho CTHT;
//  mat khau chi gom so + */# nen khong can A/B/C/D.
// ======================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ---------- RELAY: chon kieu kich ----------
// Module relay that la "kich Low" => doi RELAY_ACTIVE_LOW thanh 1.
// Trong Wokwi relay kich High nen de 0 cho mo phong sang dung.
#define RELAY_ACTIVE_LOW 0
#define RELAY_ON   (RELAY_ACTIVE_LOW ? LOW : HIGH)
#define RELAY_OFF  (RELAY_ACTIVE_LOW ? HIGH : LOW)

// ==========================================
// KHAI BAO CHAN ESP32
// ==========================================
// -- Ngo ra qua RELAY (BOM: 1 module 1 KENH + 1 module 4 KENH) --
//   Relay 1 kenh -> khoa cua TRUOC.
//   Relay 4 kenh -> CH1 khoa cua SAU, CH2 den, CH3 quat, CH4 du phong.
#define RELAY_LOCK_FRONT 4
#define RELAY_LOCK_BACK  12
#define RELAY_LIGHT      5
#define RELAY_FAN        18

#define BUZZER_PIN       19

// -- Motor sao phoi (DC + L298N) -> 2 LED chi chieu --
#define CLOTHES_IN_LED   23  // LED: dang KEO sao VAO (mua)
#define CLOTHES_OUT_LED  25  // LED: dang DAY sao RA (tanh)

// -- Ngo vao --
#define GAS_PIN          34  // MQ2 (analog)
#define RAIN_BTN         35  // nut gia lap MUA  (input-only + tro keo len 10k)
#define BTN_DOOR_BACK    36  // nut mo cua SAU   (input-only + tro keo len 10k)
#define LIMIT_CL_OUT     39  // CTHT sao da day RA het (input-only + tro keo len 10k)
#define LIMIT_CL_IN      2   // CTHT sao da keo VAO het (INPUT_PULLUP)
#define BTN_LIGHT        26
#define BTN_FAN          27
#define BTN_DOOR_FRONT   32

// ==========================================
// KEYPAD 4x3 & MAT KHAU
// ==========================================
const byte ROWS = 4, COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 14, 33, 17};
byte colPins[COLS] = {15, 16, 0};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputPassword = "";
String correctPassword = "1234";

// ==========================================
// DOI TUONG & NGUONG
// ==========================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int GAS_THRESHOLD = 1500;
const unsigned long UNLOCK_MS = 3000;

// ==========================================
// BIEN TRANG THAI
// ==========================================
bool lightOn = false, fanOn = false;
unsigned long unlockFrontUntil = 0, unlockBackUntil = 0;

bool lastBtnLight = HIGH, lastBtnFan = HIGH;
bool lastBtnFront = HIGH, lastBtnBack = HIGH;

// ------------------------------------------
void beep(int ms) { digitalWrite(BUZZER_PIN, HIGH); delay(ms); digitalWrite(BUZZER_PIN, LOW); }

void unlockFront() {
  digitalWrite(RELAY_LOCK_FRONT, RELAY_ON);
  unlockFrontUntil = millis() + UNLOCK_MS;
  lcd.clear(); lcd.print("Cua truoc: MO");
}
void unlockBack() {
  digitalWrite(RELAY_LOCK_BACK, RELAY_ON);
  unlockBackUntil = millis() + UNLOCK_MS;
  lcd.clear(); lcd.print("Cua sau: MO");
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_LOCK_FRONT, OUTPUT); pinMode(RELAY_LOCK_BACK, OUTPUT);
  pinMode(RELAY_LIGHT, OUTPUT);      pinMode(RELAY_FAN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CLOTHES_IN_LED, OUTPUT);   pinMode(CLOTHES_OUT_LED, OUTPUT);
  digitalWrite(RELAY_LOCK_FRONT, RELAY_OFF); digitalWrite(RELAY_LOCK_BACK, RELAY_OFF);
  digitalWrite(RELAY_LIGHT, RELAY_OFF);      digitalWrite(RELAY_FAN, RELAY_OFF);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(CLOTHES_IN_LED, LOW);  digitalWrite(CLOTHES_OUT_LED, LOW);

  pinMode(GAS_PIN, INPUT);
  pinMode(RAIN_BTN, INPUT);          // tro keo len 10k ngoai
  pinMode(BTN_DOOR_BACK, INPUT);     // tro keo len 10k ngoai
  pinMode(LIMIT_CL_OUT, INPUT);      // tro keo len 10k ngoai
  pinMode(LIMIT_CL_IN, INPUT_PULLUP);
  pinMode(BTN_LIGHT, INPUT_PULLUP);  pinMode(BTN_FAN, INPUT_PULLUP);
  pinMode(BTN_DOOR_FRONT, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("Smart Home Ready");

  Serial.println("ESP32 Gateway: san sang (offline).");
}

void loop() {
  // ============ 1. BAO CHAY (MQ2) ============
  bool fire = analogRead(GAS_PIN) > GAS_THRESHOLD;
  if (fire) {
    digitalWrite(BUZZER_PIN, HIGH);
    lcd.setCursor(0, 1); lcd.print("FIRE ALARM!!!   ");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ============ 2. KEYPAD MO CUA TRUOC ============
  char key = keypad.getKey();
  if (key) {
    if (!fire) beep(40);
    if (key == '#') {
      if (inputPassword == correctPassword) {
        lcd.clear(); lcd.print("Access Granted!");
        unlockFront();
      } else {
        lcd.clear(); lcd.print("Wrong Password!");
        for (int i = 0; i < 3; i++) { beep(100); delay(100); }
        lcd.clear(); lcd.print("Smart Home Ready");
      }
      inputPassword = "";
    } else if (key == '*') {
      inputPassword = "";
      lcd.setCursor(0, 1); lcd.print("                ");
    } else {
      inputPassword += key;
      lcd.setCursor(0, 1); lcd.print(inputPassword);
    }
  }

  // ============ 3. NUT BAM TRONG NHA ============
  bool b;
  b = digitalRead(BTN_LIGHT);
  if (b == LOW && lastBtnLight == HIGH) { lightOn = !lightOn; digitalWrite(RELAY_LIGHT, lightOn ? RELAY_ON : RELAY_OFF); }
  lastBtnLight = b;

  b = digitalRead(BTN_FAN);
  if (b == LOW && lastBtnFan == HIGH) { fanOn = !fanOn; digitalWrite(RELAY_FAN, fanOn ? RELAY_ON : RELAY_OFF); }
  lastBtnFan = b;

  b = digitalRead(BTN_DOOR_FRONT);
  if (b == LOW && lastBtnFront == HIGH) unlockFront();
  lastBtnFront = b;

  b = digitalRead(BTN_DOOR_BACK);
  if (b == LOW && lastBtnBack == HIGH) unlockBack();
  lastBtnBack = b;

  if (unlockFrontUntil && millis() > unlockFrontUntil) {
    digitalWrite(RELAY_LOCK_FRONT, RELAY_OFF); unlockFrontUntil = 0;
    if (!fire) { lcd.clear(); lcd.print("Smart Home Ready"); }
  }
  if (unlockBackUntil && millis() > unlockBackUntil) {
    digitalWrite(RELAY_LOCK_BACK, RELAY_OFF); unlockBackUntil = 0;
  }

  // ============ 4. SAO PHOI DO TU DONG (motor = 2 LED, dung CTHT) ============
  // Mua  -> keo VAO cho den khi cham CTHT_IN  thi dung.
  // Tanh -> day RA  cho den khi cham CTHT_OUT thi dung.
  bool raining = (digitalRead(RAIN_BTN) == LOW);
  bool limIn   = (digitalRead(LIMIT_CL_IN)  == LOW);
  bool limOut  = (digitalRead(LIMIT_CL_OUT) == LOW);
  if (raining) {
    digitalWrite(CLOTHES_IN_LED,  limIn ? LOW : HIGH);
    digitalWrite(CLOTHES_OUT_LED, LOW);
  } else {
    digitalWrite(CLOTHES_OUT_LED, limOut ? LOW : HIGH);
    digitalWrite(CLOTHES_IN_LED,  LOW);
  }

  delay(20);
}
