#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8574.h>
#include <SoftwareSerial.h> 

SoftwareSerial espSerial(2, 9); // RX = 2, TX = 9
PCF8574 pcf(0x20); 

#define P_LED_CONG         0 
#define P_LED_GARAGE       1 
#define P_NUT_CONG         2 
#define P_NUT_GARAGE       3 
#define P_CTHT_GARAGE_LEN  4 
#define P_CTHT_GARAGE_XUONG 5 

#define SERVO_CONG_PIN     3 
#define SERVO_GARAGE_PIN   5 
#define TRIG_CONG          6 
#define ECHO_CONG          7 
#define NUT_DEN_GARAGE     8 

#define RFID_1_SS          10 
#define RFID_1_RST         A0 
#define RFID_2_SS          4  
#define RFID_2_RST         A1 

const int TOC_DO_LEN = 180;    
const int TOC_DO_XUONG = 50;   

Servo servoCong;
Servo servoGarage;
MFRC522 rfid1(RFID_1_SS, RFID_1_RST); 
MFRC522 rfid2(RFID_2_SS, RFID_2_RST); 

bool trangThaiCong = false; 

enum TrangThaiGarage { STOP, DANG_LEN, DANG_XUONG }; 
TrangThaiGarage trangThaiGarage = STOP; 
bool huongTiepTheoLaXuong = false; 
bool trangThaiDenGarage = false; 

bool lastNutCong = HIGH;
bool lastNutGarage = HIGH;
bool lastNutDenGarage = HIGH;

bool theHienDien1 = false; int demVang1 = 0; 
bool theHienDien2 = false; int demVang2 = 0; 

const int NGUONG_KHOANG_CACH = 20; 

void baoCaoTrangThaiCua(char thietBi, bool trangThaiHienTai) {
  if (thietBi == 'C') { 
    if (trangThaiHienTai) espSerial.write('U'); 
    else espSerial.write('u');                  
  } 
  else if (thietBi == 'G') { 
    if (trangThaiHienTai) espSerial.write('V'); 
    else espSerial.write('v');                  
  }
}

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600); 
  
  Wire.begin(); SPI.begin();  
  
  pinMode(RFID_1_RST, OUTPUT); pinMode(RFID_2_RST, OUTPUT);
  pcf.begin(); 
  pcf.write(P_NUT_CONG, HIGH); pcf.write(P_NUT_GARAGE, HIGH);
  pcf.write(P_CTHT_GARAGE_LEN, HIGH); pcf.write(P_CTHT_GARAGE_XUONG, HIGH);
  pcf.write(P_LED_CONG, LOW); pcf.write(P_LED_GARAGE, LOW); 

  pinMode(TRIG_CONG, OUTPUT); pinMode(ECHO_CONG, INPUT); pinMode(NUT_DEN_GARAGE, INPUT_PULLUP); 

  servoCong.attach(SERVO_CONG_PIN); servoCong.write(0); 
  servoGarage.attach(SERVO_GARAGE_PIN); servoGarage.write(90); 
}

void loop() {
  if (espSerial.available() > 0) {
    char blynkCmd = espSerial.read(); 
    if (blynkCmd == '1') { trangThaiCong = true; servoCong.write(90); baoCaoTrangThaiCua('C', true); } 
    else if (blynkCmd == '0') { trangThaiCong = false; servoCong.write(0); baoCaoTrangThaiCua('C', false); }
    else if (blynkCmd == 'O') { servoGarage.write(TOC_DO_LEN); trangThaiGarage = DANG_LEN; baoCaoTrangThaiCua('G', true); }
    else if (blynkCmd == 'C') { servoGarage.write(TOC_DO_XUONG); trangThaiGarage = DANG_XUONG; baoCaoTrangThaiCua('G', false); }
    
    // Đảo trạng thái Cổng chính (từ lệnh ESP32 sau khi quét RFID 1 hợp lệ)
    else if (blynkCmd == 'T') {
      trangThaiCong = !trangThaiCong;
      if (trangThaiCong) { servoCong.write(90); baoCaoTrangThaiCua('C', true); } 
      else { servoCong.write(0); baoCaoTrangThaiCua('C', false); }
    }
    // Đảo trạng thái Gara (từ lệnh ESP32 sau khi quét RFID 2 hợp lệ)
    else if (blynkCmd == 'Y') {
      if (trangThaiGarage == STOP) {
        if (!huongTiepTheoLaXuong) { servoGarage.write(TOC_DO_LEN); trangThaiGarage = DANG_LEN; baoCaoTrangThaiCua('G', true); } 
        else { servoGarage.write(TOC_DO_XUONG); trangThaiGarage = DANG_XUONG; baoCaoTrangThaiCua('G', false); }
      } else {
        servoGarage.write(90); 
        if (trangThaiGarage == DANG_LEN) huongTiepTheoLaXuong = true; else huongTiepTheoLaXuong = false;
        trangThaiGarage = STOP;
      }
    }
    
    // Bật tắt đèn Gara
    else if (blynkCmd == 'L') { pcf.write(P_LED_GARAGE, HIGH); trangThaiDenGarage = true; }
    else if (blynkCmd == 'l') { pcf.write(P_LED_GARAGE, LOW); trangThaiDenGarage = false; }
  }

  // --- CẢM BIẾN SIÊU ÂM BẬT ĐÈN ---
  int kcCong = docKhoangCach(TRIG_CONG, ECHO_CONG); 
  if (kcCong > 0 && kcCong < NGUONG_KHOANG_CACH) { pcf.write(P_LED_CONG, HIGH); } else { pcf.write(P_LED_CONG, LOW); }

  // --- NÚT BẤM ĐÈN GARAGE ---
  bool nutDenGarage = digitalRead(NUT_DEN_GARAGE);
  if (nutDenGarage == LOW && lastNutDenGarage == HIGH) { 
    trangThaiDenGarage = !trangThaiDenGarage; 
    if (trangThaiDenGarage) { pcf.write(P_LED_GARAGE, HIGH); } else { pcf.write(P_LED_GARAGE, LOW); }
    delay(200); 
  }
  lastNutDenGarage = nutDenGarage;

  // ====================================================================
  // PHÂN HỆ 1: CỔNG CHÍNH (ĐỌC MÃ GỬI LÊN ESP32 VỚI CHỮ 'R')
  // ====================================================================
  digitalWrite(RFID_2_RST, LOW);  digitalWrite(RFID_1_RST, HIGH); delay(30);        
  rfid1.PCD_Init(); 
  if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {
    demVang1 = 0;                      
    if (!theHienDien1) {               
      theHienDien1 = true;
      String uidStr = "";
      for (byte i = 0; i < rfid1.uid.size; i++) {
        uidStr += (rfid1.uid.uidByte[i] < 0x10 ? "0" : "");
        uidStr += String(rfid1.uid.uidByte[i], HEX);
      }
      uidStr.toUpperCase(); 
      espSerial.print('R'); espSerial.println(uidStr); 
    }
    rfid1.PICC_HaltA(); 
  } else { if (++demVang1 > 2) theHienDien1 = false; }

  // --- Nút nhấn Cổng chính ---
  bool nutCong = pcf.read(P_NUT_CONG);
  if (nutCong == LOW && lastNutCong == HIGH) { 
    trangThaiCong = !trangThaiCong;
    if (trangThaiCong) { servoCong.write(90); baoCaoTrangThaiCua('C', true); } 
    else { servoCong.write(0); baoCaoTrangThaiCua('C', false); }
    delay(200); 
  }
  lastNutCong = nutCong;

  // ====================================================================
  // PHÂN HỆ 2: GARAGE (ĐỌC MÃ GỬI LÊN ESP32 VỚI CHỮ 'G')
  // ====================================================================
  digitalWrite(RFID_1_RST, LOW);  digitalWrite(RFID_2_RST, HIGH); delay(30);        
  rfid2.PCD_Init(); 
  if (rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {
    demVang2 = 0;                      
    if (!theHienDien2) {               
      theHienDien2 = true;
      String uidStr2 = "";
      for (byte i = 0; i < rfid2.uid.size; i++) {
        uidStr2 += (rfid2.uid.uidByte[i] < 0x10 ? "0" : "");
        uidStr2 += String(rfid2.uid.uidByte[i], HEX);
      }
      uidStr2.toUpperCase();
      espSerial.print('G'); espSerial.println(uidStr2); 
    }
    rfid2.PICC_HaltA();
  } else { if (++demVang2 > 2) theHienDien2 = false; }

  // --- Công tắc hành trình ---
  bool cthtLen = (pcf.read(P_CTHT_GARAGE_LEN) == LOW);     
  bool cthtXuong = (pcf.read(P_CTHT_GARAGE_XUONG) == LOW); 
  bool nutGarage = pcf.read(P_NUT_GARAGE);

  if (trangThaiGarage == DANG_LEN && cthtLen) { servoGarage.write(90); trangThaiGarage = STOP; huongTiepTheoLaXuong = true; }
  if (trangThaiGarage == DANG_XUONG && cthtXuong) { servoGarage.write(90); trangThaiGarage = STOP; huongTiepTheoLaXuong = false; }

  // --- Nút nhấn Garage ---
  if (nutGarage == LOW && lastNutGarage == HIGH) {
    if (trangThaiGarage == STOP) {
      if (!huongTiepTheoLaXuong) { servoGarage.write(TOC_DO_LEN); trangThaiGarage = DANG_LEN; baoCaoTrangThaiCua('G', true); } 
      else { servoGarage.write(TOC_DO_XUONG); trangThaiGarage = DANG_XUONG; baoCaoTrangThaiCua('G', false); }
    } else {
      servoGarage.write(90);
      if (trangThaiGarage == DANG_LEN) huongTiepTheoLaXuong = true; else huongTiepTheoLaXuong = false;
      trangThaiGarage = STOP;
    }
    delay(200);
  }
  lastNutGarage = nutGarage;
}

int docKhoangCach(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(2); digitalWrite(trigPin, HIGH); delayMicroseconds(10); digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return -1; 
  return duration * 0.034 / 2;
}