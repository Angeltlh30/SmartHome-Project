/*
  ====================================================================================
  BẢN ĐỒ ĐẤU DÂY VÀ SƠ ĐỒ CHÂN PHẦN CỨNG TOÀN HỆ THỐNG (HARDWARE WIRING MAP)
  Mạch điều khiển trung tâm: Arduino UNO / Nano
  ====================================================================================
  
  1. GIAO TIẾP I2C (MODULE MỞ RỘNG CHÂN PCF8574 - ĐỊA CHỈ 0x20)
     - SDA (PCF8574)  --> Chân A4 của Arduino
     - SCL (PCF8574)  --> Chân A5 của Arduino
     - VCC & GND      --> 5V và GND chung
     [Phân bổ các chân mở rộng trên IC PCF8574]:
       + P0 (Output)  --> Nối cực Dương (+) của Đèn LED Cổng chính
       + P1 (Output)  --> Nối cực Dương (+) của Đèn LED Garage
       + P2 (Input)   --> Nút nhấn Mở/Đóng Cổng chính (Chân còn lại nối GND)
       + P3 (Input)   --> Nút nhấn Mở/Dừng/Đóng Garage (Chân còn lại nối GND)
       + P4 (Input)   --> Công tắc hành trình TRÊN (Chạm = LOW, ngắt động cơ kéo Lên)
       + P5 (Input)   --> Công tắc hành trình DƯỚI (Chạm = LOW, ngắt động cơ kéo Xuống)

  2. HỆ THỐNG ĐỘNG CƠ SERVO (KẾT NỐI TRỰC TIẾP ARDUINO)
     - Servo Cổng (Loại 180 độ):
       + Dây Cam/Vàng (Tín hiệu) --> Chân D3
       + Dây Đỏ (VCC)            --> 5V (Nên dùng nguồn ngoài hoặc cắm tụ bù áp)
       + Dây Nâu/Đen (GND)       --> GND chung
     - Servo Garage (Loại 360 độ - Tời kéo):
       + Dây Cam/Vàng (Tín hiệu) --> Chân D5
       + Dây Đỏ (VCC)            --> 5V
       + Dây Nâu/Đen (GND)       --> GND chung

  3. CẢM BIẾN SIÊU ÂM HC-SR04 (ĐO KHOẢNG CÁCH BẬT ĐÈN CỔNG)
     - VCC            --> 5V
     - GND            --> GND chung
     - TRIG (Phát)    --> Chân D6
     - ECHO (Thu)     --> Chân D7

  4. NÚT NHẤN BẬT/TẮT ĐÈN GARAGE (ĐỘC LẬP)
     - Chân 1         --> Chân D8 của Arduino (Sử dụng INPUT_PULLUP nội bộ)
     - Chân 2         --> GND chung

  5. HỆ THỐNG ĐỌC THẺ TỪ RFID RC522 (GIAO TIẾP SPI DÙNG CHUNG BUS)
     ⚠️ CẢNH BÁO: Module RC522 CHỈ CHỊU ĐƯỢC 3.3V. TUYỆT ĐỐI KHÔNG CẤP 5V.
     
     * Cụm chân SPI dùng chung (Mắc song song cho CẢ 2 MODULE):
       - SCK          --> Chân D13
       - MISO         --> Chân D12
       - MOSI         --> Chân D11
       - VCC          --> Chân 3.3V của Arduino
       - GND          --> GND chung
       
     * Chân điều khiển ĐỘC LẬP cho MODULE RFID 1 (Cổng chính):
       - SDA (SS/CS)  --> Chân D10
       - RST (Reset)  --> Chân A0 (Dùng kỹ thuật bật/tắt nguồn chống nhiễu)

     * Chân điều khiển ĐỘC LẬP cho MODULE RFID 2 (Garage):
       - SDA (SS/CS)  --> Chân D4
       - RST (Reset)  --> Chân A1 (Dùng kỹ thuật bật/tắt nguồn chống nhiễu)
  ====================================================================================
*/

#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8574.h>

// --- CẤU HÌNH ĐỊA CHỈ PCF8574 ---
PCF8574 pcf(0x20); 

// --- ĐỊNH NGHĨA CÁC CHÂN IC PCF8574 ---
#define P_LED_CONG         0 
#define P_LED_GARAGE       1 
#define P_NUT_CONG         2 
#define P_NUT_GARAGE       3 
#define P_CTHT_GARAGE_LEN  4 
#define P_CTHT_GARAGE_XUONG 5 

// --- ĐỊNH NGHĨA CÁC CHÂN TRỰC TIẾP TRÊN ARDUINO ---
#define SERVO_CONG_PIN     3 
#define SERVO_GARAGE_PIN   5 
#define TRIG_CONG          6 
#define ECHO_CONG          7 
#define NUT_DEN_GARAGE     8 

// Chân cho 2 module RFID
#define RFID_1_SS          10 
#define RFID_1_RST         A0 
#define RFID_2_SS          4  
#define RFID_2_RST         A1 

// ================= CẤU HÌNH TỐC ĐỘ SERVO GARAGE =================
// Chuẩn của Servo 360: 90 là đứng im. Càng xa 90 quay càng nhanh.
const int TOC_DO_LEN = 180;    // Quay nhanh tối đa chiều lên
const int TOC_DO_XUONG = 50;   // Quay nhanh chiều xuống
// ================================================================

// --- KHỞI TẠO ĐỐI TƯỢNG NGOẠI VI ---
Servo servoCong;
Servo servoGarage;
MFRC522 rfid1(RFID_1_SS, RFID_1_RST); 
MFRC522 rfid2(RFID_2_SS, RFID_2_RST); 

// --- BIẾN TRẠNG THÁI ĐỘC LẬP ---
bool trangThaiCong = false; 

enum TrangThaiGarage { STOP, DANG_LEN, DANG_XUONG }; 
TrangThaiGarage trangThaiGarage = STOP; 
bool huongTiepTheoLaXuong = false; 
bool trangThaiDenGarage = false; 

bool lastNutCong = HIGH;
bool lastNutGarage = HIGH;
bool lastNutDenGarage = HIGH;

// Cờ bắt sườn thẻ RFID (Chống kẹt thẻ)
bool theHienDien1 = false; int demVang1 = 0; 
bool theHienDien2 = false; int demVang2 = 0; 

const int NGUONG_KHOANG_CACH = 20; 

void setup() {
  Serial.begin(9600);
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

  Serial.println("He thong da san sang! (Da tang MAX toc do Servo)");
}

void loop() {
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
        Serial.println("Quet the 1 -> MO Cong Chinh");
      } else {
        servoCong.write(0);
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
      Serial.println("Nut nhan -> MO Cong Chinh");
    } else {
      servoCong.write(0);
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
          Serial.println("Quet the 2 -> Garage di LEN (Nhanh nhat)");
        } else {
          servoGarage.write(TOC_DO_XUONG); 
          trangThaiGarage = DANG_XUONG;
          Serial.println("Quet the 2 -> Garage di XUONG (Nhanh nhat)");
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
      } else {
        servoGarage.write(TOC_DO_XUONG); 
        trangThaiGarage = DANG_XUONG;
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