// ======================================================
//  SMART HOME - CUM CONG & GARA (Arduino UNO Node)
//  Cong truoc & Gara, moi cum:
//   - Quet the RFID (RC522) o mat ngoai  -> mo cua
//   - Nut bam ben trong                  -> mo cua
//   - Radar phat hien nguoi              -> tu dong bat den
//
//  QUY TAC LINH KIEN (theo yeu cau):
//   - Wokwi co san -> dung that: RFID RC522 (board-mfrc522).
//   - Wokwi KHONG co -> dung LED thay the:
//       * Motor DC + L298N (cong, gara) -> 2 LED chi chieu MO / DONG.
//   - Radar RCWL & cong tac hanh trinh la tin hieu DAU VAO -> dung NUT bam.
//
//  RANG BUOC CHAN: UNO chi 18 chan dung duoc. Da gan 2 dau doc RFID that
//  (chiem 6 chan) nen chi du cho do CONG TAC HANH TRINH o CONG (2 cai).
//  Gara dung dem thoi gian. Tren mach that, cac CTHT con lai dau qua
//  IC mo rong PCF8574 (co trong BOM).
// ======================================================

#include <SPI.h>
#include <MFRC522.h>

// ---- 2 dau doc RFID dung chung SCK/MOSI/MISO/RST, RIENG chan SS ----
#define RFID_RST   9
#define SS_GATE    10
#define SS_GARAGE  8
MFRC522 rfidGate(SS_GATE, RFID_RST);
MFRC522 rfidGarage(SS_GARAGE, RFID_RST);

// ---- Motor (DC + L298N) -> 2 LED chi chieu ----
#define GATE_OPEN_LED    2
#define GATE_CLOSE_LED   3
#define GARAGE_UP_LED    4
#define GARAGE_DOWN_LED  5

// ---- Den cong / gara (radar bat) ----
#define GATE_LIGHT       6
#define GARAGE_LIGHT     7

// ---- Nut gia lap radar, nut mo cua trong, cong tac hanh trinh ----
#define RADAR_GATE       A0
#define RADAR_GARAGE     A1
#define BTN_INT_GATE     A2
#define BTN_INT_GARAGE   A3
#define LIMIT_GATE_OPEN  A4   // CTHT cong da MO het
#define LIMIT_GATE_CLOSE A5   // CTHT cong da DONG kin

#define NO_LIMIT 255          // cum khong gan CTHT (dung dem thoi gian)

const unsigned long MOVE_MS   = 2000; // thoi gian motor chay (mo phong/fallback)
const unsigned long OPEN_HOLD = 3000; // giu mo bao lau roi tu dong dong

// Trang thai moi cua: 0 IDLE, 1 OPENING, 2 OPEN, 3 CLOSING
byte gateState = 0,   garageState = 0;
unsigned long gateT = 0, garageT = 0;

// ------------------------------------------------------
bool cardScanned(MFRC522 &reader) {
  if (!reader.PICC_IsNewCardPresent()) return false;
  if (!reader.PICC_ReadCardSerial())   return false;
  reader.PICC_HaltA();
  return true;
}

// Cham CTHT (neu co) HOAC het thoi gian chay -> coi nhu da toi noi.
bool reachedEnd(byte limitPin, unsigned long t) {
  if (limitPin != NO_LIMIT && digitalRead(limitPin) == LOW) return true;
  return (millis() - t > MOVE_MS);
}

void runDoor(bool trigger, byte openLed, byte closeLed,
             byte limOpen, byte limClose,
             byte &state, unsigned long &t, const char *name) {
  switch (state) {
    case 0: // IDLE
      if (trigger) {
        state = 1; t = millis();
        digitalWrite(openLed, HIGH);
        Serial.print(name); Serial.println(": dang MO");
      }
      break;
    case 1: // OPENING -> dung khi cham CTHT mo (hoac het gio)
      if (reachedEnd(limOpen, t)) {
        digitalWrite(openLed, LOW);
        state = 2; t = millis();
        Serial.print(name); Serial.println(": da mo het");
      }
      break;
    case 2: // OPEN (giu)
      if (millis() - t > OPEN_HOLD) {
        digitalWrite(closeLed, HIGH);
        state = 3; t = millis();
        Serial.print(name); Serial.println(": dang DONG");
      }
      break;
    case 3: // CLOSING -> dung khi cham CTHT dong (hoac het gio)
      if (reachedEnd(limClose, t)) {
        digitalWrite(closeLed, LOW);
        state = 0;
        Serial.print(name); Serial.println(": da dong kin");
      }
      break;
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfidGate.PCD_Init();
  rfidGarage.PCD_Init();

  pinMode(GATE_OPEN_LED, OUTPUT);  pinMode(GATE_CLOSE_LED, OUTPUT);
  pinMode(GARAGE_UP_LED, OUTPUT);  pinMode(GARAGE_DOWN_LED, OUTPUT);
  pinMode(GATE_LIGHT, OUTPUT);     pinMode(GARAGE_LIGHT, OUTPUT);

  pinMode(RADAR_GATE, INPUT_PULLUP);   pinMode(RADAR_GARAGE, INPUT_PULLUP);
  pinMode(BTN_INT_GATE, INPUT_PULLUP); pinMode(BTN_INT_GARAGE, INPUT_PULLUP);
  pinMode(LIMIT_GATE_OPEN, INPUT_PULLUP); pinMode(LIMIT_GATE_CLOSE, INPUT_PULLUP);

  Serial.println("ARDUINO NODE: Cong & Gara san sang!");
}

void loop() {
  // ===== 1. RADAR -> TU DONG BAT DEN (giu nut = co nguoi) =====
  digitalWrite(GATE_LIGHT,   digitalRead(RADAR_GATE)   == LOW ? HIGH : LOW);
  digitalWrite(GARAGE_LIGHT, digitalRead(RADAR_GARAGE) == LOW ? HIGH : LOW);

  // ===== 2. CONG: quet the HOAC nut trong (co 2 CTHT) =====
  bool gateTrig = cardScanned(rfidGate) || digitalRead(BTN_INT_GATE) == LOW;
  runDoor(gateTrig, GATE_OPEN_LED, GATE_CLOSE_LED,
          LIMIT_GATE_OPEN, LIMIT_GATE_CLOSE, gateState, gateT, "CONG");

  // ===== 3. GARA: quet the HOAC nut trong (dem thoi gian) =====
  bool garageTrig = cardScanned(rfidGarage) || digitalRead(BTN_INT_GARAGE) == LOW;
  runDoor(garageTrig, GARAGE_UP_LED, GARAGE_DOWN_LED,
          NO_LIMIT, NO_LIMIT, garageState, garageT, "GARA");

  delay(20);
}
