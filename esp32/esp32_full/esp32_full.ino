/*
  ====================================================================================
  BẢN ĐỒ ĐẤU DÂY VÀ SƠ ĐỒ CHÂN PHẦN CỨNG TOÀN HỆ THỐNG (HARDWARE WIRING MAP)
  Mạch điều khiển trung tâm: ESP32 Dev Module
  ====================================================================================
  
  1. MÀN HÌNH LCD 1602 (I2C): VCC -> 5V | GND -> GND | SDA -> GPIO 21 | SCL -> GPIO 22
  2. BÀN PHÍM KEYPAD 4x3: 
     - Hàng (R1->R4): GPIO 13, 12, 14, 27
     - Cột (C1->C3): GPIO 26, 25, 33
  3. BÁO CHÁY: 
     - Còi Buzzer (+): GPIO 15 | (-) GND
     - Cảm biến MQ-2: VCC -> 3.3V | GND -> GND | AO -> GPIO 34
  4. ĐÈN LED & NÚT NHẤN:
     - LED (+): GPIO 4 (qua trở 220 ohm) | (-) GND
     - Nút đèn: GPIO 18 & GND
  5. SERVO CỬA & NÚT NHẤN:
     - Servo Cổng (180): VCC -> 5V | GND -> GND | SIG -> GPIO 5
     - Nút cửa: GPIO 19 & GND
  6. SÀO PHƠI ĐỒ & CẢM BIẾN MƯA:
     - Servo Sào (360): VCC -> 5V | GND -> GND | SIG -> GPIO 23
     - Cảm biến mưa LM393: VCC -> 3.3V | GND -> GND | D0 -> GPIO 35
  7. KẾT NỐI ARDUINO (WIFI/APP):
     - ESP32 TX2 (GPIO 17) ---> (Mạch chia áp) ---> Arduino RX (D2)
     - Chung GND
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h> 
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ================= CẤU HÌNH GIAI ĐOẠN 1: LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= CẤU HÌNH GIAI ĐOẠN 2: KEYPAD 4x3 =================
const byte ROWS = 4; 
const byte COLS = 3; 
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33};     
Keypad matrixKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String ma_PIN_dung = "1234";
String pin_nhap = "";

// ================= CẤU HÌNH GIAI ĐOẠN 3: BÁO CHÁY =================
#define BUZZER_PIN 15 
#define MQ2_PIN 34
#define NGUONG_BAO_CHAY 1800 
bool dang_bao_chay = false;

// ================= CẤU HÌNH GIAI ĐOẠN 4: ĐÈN LED =================
#define LED_PIN 4
#define NUT_DEN_PIN 18 
bool trangThaiDen = false; 
bool lastNutDenState = HIGH;

// ================= CẤU HÌNH GIAI ĐOẠN 5: SERVO CỬA TRƯỚC =================
#define SERVO_CONG_PIN 5
#define NUT_CONG_PIN 19
Servo servoCong;
bool trangThaiCong = false; 
bool lastNutCongState = HIGH;

// ================= CẤU HÌNH GIAI ĐOẠN 6: CẢM BIẾN MƯA & SERVO 360 =================
#define SERVO_360_PIN 23 
#define RAIN_PIN 35      

Servo servo360;
bool dangCoMua = false;        
bool dangDiChuyen = false;     
unsigned long thoiGianBatDauQuay = 0; 


void setup() {
  Serial.begin(115200);
  
  // Tắt mạch chống sụt áp nguồn (Brownout Detector)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Khởi tạo các chân tín hiệu I/O
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(NUT_DEN_PIN, INPUT_PULLUP);
  pinMode(NUT_CONG_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT); 

  digitalWrite(BUZZER_PIN, LOW); 
  digitalWrite(LED_PIN, LOW); 

  // Khởi tạo Servo Cửa Trước (180 độ) - Dải xung giữ nguyên
  servoCong.setPeriodHertz(50); 
  servoCong.attach(SERVO_CONG_PIN, 500, 2400); 
  servoCong.write(0); 

  // ---> ÁP DỤNG CÁCH 1: SỬA DẢI XUNG CHO SERVO 360 ĐỘ <---
  servo360.setPeriodHertz(50);
  servo360.attach(SERVO_360_PIN, 1000, 2000); // Đã thu hẹp lại chuẩn 1000 - 2000
  servo360.write(90); // Gửi lệnh đứng im

  // Khởi tạo LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  hienThiTrangThaiBanDau();
}

void loop() {
  // ---------------- GIAI ĐOẠN 6: XỬ LÝ SÀO PHƠI ĐỒ (SERVO 360) ----------------
  if (!dang_bao_chay) {
    bool phatHienMua = (digitalRead(RAIN_PIN) == LOW); 

    // BẮT ĐẦU MƯA
    if (phatHienMua && !dangCoMua) {
      dangCoMua = true;
      dangDiChuyen = true;
      thoiGianBatDauQuay = millis();
      
      servo360.write(180); // Quay kéo đồ vào
      Serial.println(">> TROI MUA! Keo sao phoi do vao trong...");
    }
    // HẾT MƯA
    else if (!phatHienMua && dangCoMua) {
      dangCoMua = false;
      dangDiChuyen = true;
      thoiGianBatDauQuay = millis();
      
      servo360.write(0); // Quay đẩy đồ ra
      Serial.println(">> TROI QUANG! Day sao phoi do ra ngoai...");
    }

    // TỰ ĐỘNG DỪNG KHI QUAY ĐỦ 5 GIÂY
    if (dangDiChuyen && (millis() - thoiGianBatDauQuay >= 5000)) {
      servo360.write(90); // Lệnh đứng im
      dangDiChuyen = false;
      Serial.println(">> Servo 360: Da quay du 5 giay -> DUNG.");
    }
  }

  // ---------------- GIAI ĐOẠN 3: KIỂM TRA CHÁY NỔ ----------------
  int muc_khi_gas = analogRead(MQ2_PIN);

  if (muc_khi_gas > NGUONG_BAO_CHAY) {
    if (!dang_bao_chay) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("!!! FIRE !!!");
      lcd.setCursor(0, 1);
      lcd.print("DOOR OPENING");
      
      servoCong.write(90); 
      trangThaiCong = true;
      
      servo360.write(90); // Khi cháy thì dừng Servo 360
      dangDiChuyen = false; 
      
      dang_bao_chay = true;
    }
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100); 
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    return; // Dừng các lệnh bên dưới khi đang cháy
  } 
  else {
    if (dang_bao_chay) {
      digitalWrite(BUZZER_PIN, LOW); 
      dang_bao_chay = false;
      hienThiTrangThaiBanDau(); 
    }
  }

  // ---------------- GIAI ĐOẠN 4: NÚT BẤM ĐÈN LED ----------------
  bool currentNutDenState = digitalRead(NUT_DEN_PIN);
  if (lastNutDenState == HIGH && currentNutDenState == LOW) {
    trangThaiDen = !trangThaiDen; 
    digitalWrite(LED_PIN, trangThaiDen ? HIGH : LOW);
    delay(200);
  }
  lastNutDenState = currentNutDenState;

  // ---------------- GIAI ĐOẠN 5: NÚT BẤM CỬA TRƯỚC ----------------
  bool currentNutCongState = digitalRead(NUT_CONG_PIN);
  if (lastNutCongState == HIGH && currentNutCongState == LOW) {
    trangThaiCong = !trangThaiCong; 
    if (trangThaiCong) {
      servoCong.write(90);
    } else {
      servoCong.write(0);
    }
    delay(200); 
  }
  lastNutCongState = currentNutCongState;

  // ---------------- GIAI ĐOẠN 2: XỬ LÝ NHẬP MÃ PIN (TÍCH HỢP LOGIC MỚI) ----------------
  char key = matrixKeypad.getKey();

  if (key) {
    if (key == '*') {
      pin_nhap = "";
      lcd.setCursor(0, 1);
      lcd.print("Nhap PIN:       "); 
    } 
    else if (key == '#') {
      lcd.setCursor(0, 1);
      if (pin_nhap == ma_PIN_dung) {
        
        // --- LOGIC MỚI: ĐẢO TRẠNG THÁI CỬA ---
        trangThaiCong = !trangThaiCong;
        if (trangThaiCong) {
          servoCong.write(90);            
          lcd.print("Access: MO cua  "); // 16 ký tự vừa khít màn hình
        } else {
          servoCong.write(0);             
          lcd.print("Access: DONG cua");
        }

        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
        // --------------------------------------
        
      } else {
        lcd.print("Wrong PIN!      ");
        for(int i=0; i<3; i++) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(100);
          digitalWrite(BUZZER_PIN, LOW);
          delay(100);
        }
      }
      
      delay(2000); 
      pin_nhap = "";
      hienThiTrangThaiBanDau(); // Tái sử dụng hàm vẽ lại màn hình cho gọn
    } 
    else {
      if (pin_nhap.length() < 4) {
        pin_nhap += key;
        lcd.setCursor(10, 1); 
        lcd.print(pin_nhap);
      }
    }
  }
}

void hienThiTrangThaiBanDau() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home Ready");
  lcd.setCursor(0, 1);
  lcd.print("Nhap PIN:       ");
  if (pin_nhap != "") {
    lcd.setCursor(10, 1); 
    lcd.print(pin_nhap);
  }
}