/*
  ====================================================================================
  BẢN ĐỒ ĐẤU DÂY (CẬP NHẬT GIAO TIẾP 2 CHIỀU)
  ====================================================================================
  [Giữ nguyên các kết nối Cảm biến, Servo, PCF8574, RFID như cũ]
  
  [GIAO TIẾP VỚI ESP32]:
     - Chân D2 (RX Arduino) <-- Nối với chân TX2 (GPIO 17) của ESP32
     - Chân D9 (TX Arduino) --> (Qua trở hạ áp) --> Nối với chân RX2 (GPIO 16) của ESP32
     - GND                  <-- Nối chung với GND của ESP32
  ====================================================================================
*/

#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8574.h>
#include <SoftwareSerial.h> 

// --- ĐỔI CHÂN TX SANG D9 ĐỂ TRÁNH TRÙNG VỚI SERVO CỔNG Ở D3 ---
SoftwareSerial espSerial(2, 9); // RX = 2, TX = 9

// --- CẤU HÌNH ĐỊA CHỈ PCF8574 ---
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

// =================================================================
// [PHẦN MỚI] HÀM HỖ TRỢ: BÁO CÁO TRẠNG THÁI CHO ESP32
// =================================================================
void baoCaoTrangThaiCua(char thietBi, bool trangThaiHienTai) {
  if (thietBi == 'C') { // Thiết bị: Cổng chính
    if (trangThaiHienTai) espSerial.write('U'); // Ký hiệu U: Đã Mở
    else espSerial.write('u');                  // Ký hiệu u: Đã Đóng
  } 
  else if (thietBi == 'G') { // Thiết bị: Garage
    if (trangThaiHienTai) espSerial.write('V'); // Ký hiệu V: Đã Mở
    else espSerial.write('v');                  // Ký hiệu v: Đã Đóng
  }
}
// =================================================================

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600); 
  
  Wire.begin(); 
  SPI.begin();  
  
  pinMode(RFID_1_RST, OUTPUT);
  pinMode(RFID_2_RST, OUTPUT);
  
  pcf.begin(); 
  
  pcf.write(P_NUT_CONG, HIGH);
  pcf.write(P_NUT_GARAGE, HIGH);
  pcf.write(P_CTHT_GARAGE_LEN, HIGH);
  pcf.write(P_CTHT_GARAGE_XUONG, HIGH);
  
  pcf.write(P_LED_CONG, LOW);    
  pcf.write(P_LED_GARAGE, HIGH); 

  pinMode(TRIG_CONG, OUTPUT);
  pinMode(ECHO_CONG, INPUT);
  pinMode(NUT_DEN_GARAGE, INPUT_PULLUP); 

  servoCong.attach(SERVO_CONG_PIN);
  servoCong.write(0); 
  
  servoGarage.attach(SERVO_GARAGE_PIN);
  servoGarage.write(90); 

  Serial.println("He thong da san sang! (Da tich hop giao tiep 2 chieu)");
}

void loop() {
  // =================================================================================
  // ĐỌC LỆNH TỪ APP BLYNK (THÔNG QUA ESP32) ĐỂ KÍCH HOẠT ĐỘNG CƠ
  // =================================================================================
  if (espSerial.available() > 0) {
    char blynkCmd = espSerial.read(); 
    Serial.print(">> Nhan lenh tu App Blynk: ");
    Serial.println(blynkCmd);

    if (blynkCmd == '1') { 
      trangThaiCong = true;
      servoCong.write(90);
      Serial.println("=> Thuc thi: MO Cong Chinh");
    } 
    else if (blynkCmd == '0') { 
      trangThaiCong = false;
      servoCong.write(0);
      Serial.println("=> Thuc thi: DONG Cong Chinh");
    }
    else if (blynkCmd == 'O') { 
      servoGarage.write(TOC_DO_LEN);
      trangThaiGarage = DANG_LEN;
      Serial.println("=> Thuc thi: KEO Garage LEN");
    }
    else if (blynkCmd == 'C') { 
      servoGarage.write(TOC_DO_XUONG);
      trangThaiGarage = DANG_XUONG;
      Serial.println("=> Thuc thi: HA Garage XUONG");
    }
  }

  // ---------------- 1. ĐÈN TỰ ĐỘNG CỔNG ----------------
  int kcCong = docKhoangCach(TRIG_CONG, ECHO_CONG); 
  if (kcCong > 0 && kcCong < NGUONG_KHOANG_CACH) {
    pcf.write(P_LED_CONG, HIGH); 
  } else {
    pcf.write(P_LED_CONG, LOW);  
  }

  // ---------------- 2. NÚT NHẤN ĐÈN GARAGE ----------------
  bool nutDenGarage = digitalRead(NUT_DEN_GARAGE);
  if (nutDenGarage == LOW && lastNutDenGarage == HIGH) { 
    trangThaiDenGarage = !trangThaiDenGarage; 
    if (trangThaiDenGarage) {
      pcf.write(P_LED_GARAGE, LOW); 
      Serial.println("BAT den Garage");
    } else {
      pcf.write(P_LED_GARAGE, HIGH); 
      Serial.println("TAT den Garage");
    }
    delay(200); 
  }
  lastNutDenGarage = nutDenGarage;

  // ================= PHÂN HỆ 1: CỔNG CHÍNH =================
  digitalWrite(RFID_2_RST, LOW);  
  digitalWrite(RFID_1_RST, HIGH); 
  delay(30);        
  rfid1.PCD_Init(); 
  
  if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {
    demVang1 = 0;                      
    if (!theHienDien1) {               
      theHienDien1 = true;
      trangThaiCong = !trangThaiCong;
      
      if (trangThaiCong) {
        servoCong.write(90);
        baoCaoTrangThaiCua('C', true); // Truyền ngược về App
        Serial.println("Quet the 1 -> MO Cong Chinh");
      } else {
        servoCong.write(0);
        baoCaoTrangThaiCua('C', false); // Truyền ngược về App
        Serial.println("Quet the 1 -> DONG Cong Chinh");
      }
    }
    rfid1.PICC_HaltA(); 
  } else {
    if (++demVang1 > 2) theHienDien1 = false; 
  }

  bool nutCong = pcf.read(P_NUT_CONG);
  if (nutCong == LOW && lastNutCong == HIGH) { 
    trangThaiCong = !trangThaiCong;
    
    if (trangThaiCong) {
      servoCong.write(90);
      baoCaoTrangThaiCua('C', true); // Truyền ngược về App
      Serial.println("Nut nhan -> MO Cong Chinh");
    } else {
      servoCong.write(0);
      baoCaoTrangThaiCua('C', false); // Truyền ngược về App
      Serial.println("Nut nhan -> DONG Cong Chinh");
    }
    delay(200); 
  }
  lastNutCong = nutCong;

  // ================= PHÂN HỆ 2: GARAGE =================
  digitalWrite(RFID_1_RST, LOW);  
  digitalWrite(RFID_2_RST, HIGH); 
  delay(30);        
  rfid2.PCD_Init(); 

  if (rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {
    demVang2 = 0;                      
    if (!theHienDien2) {               
      theHienDien2 = true;
      
      if (trangThaiGarage == STOP) {
        if (!huongTiepTheoLaXuong) {
          servoGarage.write(TOC_DO_LEN); 
          trangThaiGarage = DANG_LEN;
          baoCaoTrangThaiCua('G', true); // Truyền ngược về App
          Serial.println("Quet the 2 -> Garage di LEN");
        } else {
          servoGarage.write(TOC_DO_XUONG); 
          trangThaiGarage = DANG_XUONG;
          baoCaoTrangThaiCua('G', false); // Truyền ngược về App
          Serial.println("Quet the 2 -> Garage di XUONG");
        }
      } else {
        servoGarage.write(90); 
        if (trangThaiGarage == DANG_LEN) huongTiepTheoLaXuong = true;
        else huongTiepTheoLaXuong = false;
        trangThaiGarage = STOP;
        Serial.println("Quet the 2 -> DUNG Garage khan cap!");
      }
    }
    rfid2.PICC_HaltA();
  } else {
    if (++demVang2 > 2) theHienDien2 = false;
  }

  bool cthtLen = (pcf.read(P_CTHT_GARAGE_LEN) == LOW);     
  bool cthtXuong = (pcf.read(P_CTHT_GARAGE_XUONG) == LOW); 
  bool nutGarage = pcf.read(P_NUT_GARAGE);

  if (trangThaiGarage == DANG_LEN && cthtLen) {
    servoGarage.write(90); 
    trangThaiGarage = STOP;
    huongTiepTheoLaXuong = true; 
    Serial.println("CHAM CTHT TREN -> Dung!");
  }
  
  if (trangThaiGarage == DANG_XUONG && cthtXuong) {
    servoGarage.write(90); 
    trangThaiGarage = STOP;
    huongTiepTheoLaXuong = false; 
    Serial.println("CHAM CTHT DUOI -> Dung!");
  }

  if (nutGarage == LOW && lastNutGarage == HIGH) {
    if (trangThaiGarage == STOP) {
      if (!huongTiepTheoLaXuong) {
        servoGarage.write(TOC_DO_LEN); 
        trangThaiGarage = DANG_LEN;
        baoCaoTrangThaiCua('G', true); // Truyền ngược về App
      } else {
        servoGarage.write(TOC_DO_XUONG); 
        trangThaiGarage = DANG_XUONG;
        baoCaoTrangThaiCua('G', false); // Truyền ngược về App
      }
    } else {
      servoGarage.write(90);
      if (trangThaiGarage == DANG_LEN) huongTiepTheoLaXuong = true;
      else huongTiepTheoLaXuong = false;
      trangThaiGarage = STOP;
    }
    delay(200);
  }
  lastNutGarage = nutGarage;
}

int docKhoangCach(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return -1; 
  
  int distance = duration * 0.034 / 2;
  return distance;
}