/*
  ====================================================================================
  Mạch điều khiển trung tâm: ESP32 Dev Module (PHIÊN BẢN LƯU TRỮ NỘI BỘ NVS)
  ====================================================================================
*/

#define BLYNK_TEMPLATE_ID "TMPL6dffk8LTe"
#define BLYNK_TEMPLATE_NAME "SMART HOUSE ANPHUCDATKHOA"
#define BLYNK_AUTH_TOKEN "PqARr5-XhktZ9M_LFw7R101R2TEIysMq"
#define BLYNK_PRINT Serial 

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h> 
#include <Preferences.h> 
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "abcxyz";
char pass[] = "1303040506";     

#define TX2_PIN 17
#define RX2_PIN 16 

LiquidCrystal_I2C lcd(0x27, 16, 2);

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

#define BUZZER_PIN 15 
#define MQ2_PIN 34
#define NGUONG_BAO_CHAY 3000
bool dang_bao_chay = false;

#define LED_PIN 4
#define NUT_DEN_PIN 18 
bool trangThaiDen = false; 
bool lastNutDenState = HIGH;

#define SERVO_CONG_PIN 5
#define NUT_CONG_PIN 19
Servo servoCong;
bool trangThaiCong = false; // false = Đóng, true = Mở
bool lastNutCongState = HIGH;

#define SERVO_360_PIN 23 
#define RAIN_PIN 35      
Servo servo360;
bool dangCoMua = false;        
bool dangDiChuyen = false;     
unsigned long thoiGianBatDauQuay = 0; 

Preferences prefs;
String ma_PIN_dung = "1234"; 
String rfid_list = "";       
String the_la_gan_nhat = ""; 
String pin_nhap = "";

WidgetTerminal terminal(V3); 

BLYNK_WRITE(V1) {
  if (param.asInt() == 1) { Serial2.write('1'); } 
  else { Serial2.write('0'); }
}
BLYNK_WRITE(V2) {
  if (param.asInt() == 1) { Serial2.write('O'); } else { Serial2.write('C'); }
}
BLYNK_WRITE(V4) {
  String newPass = param.asStr();
  if (newPass.length() > 0) {
    ma_PIN_dung = newPass;
    prefs.putString("pass", ma_PIN_dung); 
    terminal.println("=> Da doi mat khau thanh: " + ma_PIN_dung);
    terminal.flush();
  }
}
BLYNK_WRITE(V5) {
  String uidCanXoa = param.asStr();
  uidCanXoa.toUpperCase();
  if (uidCanXoa == "CLEAR") {
    rfid_list = "";
    prefs.putString("rfids", rfid_list);
    terminal.clear();
    terminal.println("=> DA DON SACH TOAN BO THE VA RAC BO NHO!");
  } 
  else if (uidCanXoa.length() > 0 && rfid_list.indexOf(uidCanXoa) >= 0) {
    rfid_list.replace(uidCanXoa + ",", ""); 
    prefs.putString("rfids", rfid_list);
    terminal.println("=> Da XOA the: " + uidCanXoa);
  } else {
    terminal.println("=> Khong tim thay the: " + uidCanXoa);
  }
  terminal.flush();
  capNhatThongTinLenBlynk(); 
}
BLYNK_WRITE(V7) {
  if (param.asInt() == 1 && the_la_gan_nhat != "") {
    if (rfid_list.indexOf(the_la_gan_nhat) == -1) { 
      rfid_list += the_la_gan_nhat + ",";
      prefs.putString("rfids", rfid_list);
      terminal.println("=> Da THEMM the moi: " + the_la_gan_nhat);
      terminal.flush();
      the_la_gan_nhat = "";
      Blynk.virtualWrite(V6, "Trống"); 
    }
  }
}
BLYNK_WRITE(V8) {
  if (param.asInt() == 1) { capNhatThongTinLenBlynk(); }
}
BLYNK_WRITE(V9) {
  trangThaiDen = param.asInt();
  digitalWrite(LED_PIN, trangThaiDen ? HIGH : LOW);
  terminal.println(trangThaiDen ? "=> Blynk: BAT den phong khach" : "=> Blynk: TAT den phong khach");
  terminal.flush();
}
BLYNK_WRITE(V10) {
  if (param.asInt() == 1) { Serial2.write('L'); terminal.println("=> Blynk: BAT den Gara"); } 
  else { Serial2.write('l'); terminal.println("=> Blynk: TAT den Gara"); }
  terminal.flush();
}

// [ĐÃ SỬA] ĐỒNG BỘ LOGIC BLYNK CỬA PHÒNG KHÁCH
BLYNK_WRITE(V11) {
  trangThaiCong = (param.asInt() == 1);
  lcd.clear(); lcd.setCursor(0, 0);
  if (trangThaiCong) {
    servoCong.write(0); // Góc Mở
    lcd.print("Blynk: MO CUA");
    terminal.println("=> Blynk: MO cua phong khach");
  } else {
    servoCong.write(90); // Góc Đóng
    lcd.print("Blynk: DONG CUA");
    terminal.println("=> Blynk: DONG cua phong khach");
  }
  terminal.flush();
  delay(1000);
  hienThiTrangThaiBanDau();
}

BLYNK_WRITE(V12) {
  int lenhSaoDo = param.asInt();
  dangDiChuyen = true;
  thoiGianBatDauQuay = millis(); 
  if (lenhSaoDo == 1) {
    servo360.write(0); 
    terminal.println("=> Blynk: KEO SAO DO RA NGOAI");
  } else {
    servo360.write(180); 
    terminal.println("=> Blynk: KEO SAO DO VAO TRONG");
  }
  terminal.flush();
}

void capNhatThongTinLenBlynk() {
  terminal.clear();
  terminal.println("=== HE THONG SMART HOME ===");
  terminal.println("Mat khau: " + ma_PIN_dung);
  terminal.println("Danh sach RFID:");
  if (rfid_list.length() > 0) { terminal.println(rfid_list); } 
  else { terminal.println("(Chua co the nao)"); }
  terminal.flush();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  prefs.begin("smarthome", false);
  ma_PIN_dung = prefs.getString("pass", "1234"); 
  rfid_list = prefs.getString("rfids", "");      

  Serial.println("Dang ket noi den Blynk Cloud...");
  Blynk.begin(auth, ssid, pass);

  pinMode(BUZZER_PIN, OUTPUT); pinMode(LED_PIN, OUTPUT);
  pinMode(NUT_DEN_PIN, INPUT_PULLUP); pinMode(NUT_CONG_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT); 

  digitalWrite(BUZZER_PIN, LOW); digitalWrite(LED_PIN, LOW); 

  servoCong.setPeriodHertz(50); 
  servoCong.attach(SERVO_CONG_PIN, 500, 2400); 
  servoCong.write(90); // Khởi động mặc định cửa Đóng (90 độ)

  servo360.setPeriodHertz(50);
  servo360.attach(SERVO_360_PIN, 1000, 2000); 
  servo360.write(90); 

  Wire.begin(21, 22);
  lcd.init(); lcd.backlight();
  hienThiTrangThaiBanDau();
  
  capNhatThongTinLenBlynk(); 
  Blynk.virtualWrite(V6, "Trống");
}

void loop() {
  Blynk.run(); 

  if (Serial2.available() > 0) {
    char dataTuArduino = Serial2.read();
    
    if (dataTuArduino == 'U') { Blynk.virtualWrite(V1, 1); } 
    else if (dataTuArduino == 'u') { Blynk.virtualWrite(V1, 0); }
    else if (dataTuArduino == 'V') { Blynk.virtualWrite(V2, 1); } 
    else if (dataTuArduino == 'v') { Blynk.virtualWrite(V2, 0); }

    // [BỔ SUNG] Lắng nghe trạng thái Đèn Gara từ nút bấm vật lý
    else if (dataTuArduino == 'W') { 
      Blynk.virtualWrite(V10, 1); 
      terminal.println("=> Nut nhan (Arduino): BAT den Gara"); 
      terminal.flush();
    }
    else if (dataTuArduino == 'w') { 
      Blynk.virtualWrite(V10, 0); 
      terminal.println("=> Nut nhan (Arduino): TAT den Gara"); 
      terminal.flush();
    }
    
    // --- XỬ LÝ THẺ TỪ CỔNG CHÍNH ---
    else if (dataTuArduino == 'R') {
      String uid_quet_duoc = Serial2.readStringUntil('\n'); 
      uid_quet_duoc.trim(); 
      
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Main Gate Card..");
      
      if (rfid_list.indexOf(uid_quet_duoc) >= 0) {
        // Gửi lệnh lật trạng thái cửa Cổng Chính (Servo Arduino)
        Serial2.write('T'); // Quy ước thêm lệnh T lật trạng thái cổng chính
        lcd.setCursor(0, 1); lcd.print("Access Granted  ");
        terminal.println("-> Kich hoat CONG CHINH bang the: " + uid_quet_duoc);
        digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);
      } else {
        lcd.setCursor(0, 1); lcd.print("Access Denied!  ");
        the_la_gan_nhat = uid_quet_duoc; 
        Blynk.virtualWrite(V6, the_la_gan_nhat); 
        Blynk.logEvent("unknown_tag", String("Phát hiện thẻ lạ quét vào cổng: ") + the_la_gan_nhat);
        terminal.println("-> TU CHOI the lạ (Cổng): " + uid_quet_duoc);
        for(int i=0; i<3; i++) { digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100); }
      }
      terminal.flush();
      delay(2000); hienThiTrangThaiBanDau();
    }
    
    // [PHẦN MỚI] XỬ LÝ THẺ TỪ GARA TỪ ARDUINO GỬI SANG
    else if (dataTuArduino == 'G') {
      String uid_quet_duoc = Serial2.readStringUntil('\n'); 
      uid_quet_duoc.trim(); 
      
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("Garage Card...");
      
      if (rfid_list.indexOf(uid_quet_duoc) >= 0) {
        Serial2.write('Y'); // Gửi lệnh lật trạng thái Gara sang Arduino
        lcd.setCursor(0, 1); lcd.print("Access Granted  ");
        terminal.println("-> Kich hoat GARA bang the: " + uid_quet_duoc);
        digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);
      } else {
        lcd.setCursor(0, 1); lcd.print("Access Denied!  ");
        the_la_gan_nhat = uid_quet_duoc; 
        Blynk.virtualWrite(V6, the_la_gan_nhat); 
        Blynk.logEvent("unknown_tag", String("Thẻ lạ quét vào Gara: ") + the_la_gan_nhat);
        terminal.println("-> TU CHOI the Gara lạ: " + uid_quet_duoc);
        for(int i=0; i<3; i++) { digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100); }
      }
      terminal.flush();
      delay(2000); hienThiTrangThaiBanDau();
    }
  }

  // --- CẢM BIẾN MƯA ---
  if (!dang_bao_chay) {
    bool phatHienMua = (digitalRead(RAIN_PIN) == LOW); 
    
    // Khi bắt đầu mưa -> Kéo vào trong (Góc 180)
    if (phatHienMua && !dangCoMua) { 
      dangCoMua = true; 
      dangDiChuyen = true; 
      thoiGianBatDauQuay = millis(); 
      servo360.write(180); 
      
      // Bổ sung đồng bộ lên App
      Blynk.virtualWrite(V12, 0); // Tắt nút V12
      terminal.println("=> Cam bien: Troi MUA, tu dong KEO SAO DO VAO");
      terminal.flush();
    }
    // Khi tạnh mưa -> Đẩy ra ngoài (Góc 0)
    else if (!phatHienMua && dangCoMua) { 
      dangCoMua = false; 
      dangDiChuyen = true; 
      thoiGianBatDauQuay = millis(); 
      servo360.write(0); 
      
      // Bổ sung đồng bộ lên App
      Blynk.virtualWrite(V12, 1); // Bật nút V12
      terminal.println("=> Cam bien: TANH MUA, tu dong DAY SAO DO RA");
      terminal.flush();
    }
    
    // Tự động ngắt động cơ sau 5 giây
    if (dangDiChuyen && (millis() - thoiGianBatDauQuay >= 5000)) { 
      servo360.write(90); 
      dangDiChuyen = false; 
    }
  }

  // --- BÁO CHÁY ---
  int muc_khi_gas = analogRead(MQ2_PIN);
  if (muc_khi_gas > NGUONG_BAO_CHAY) {
    if (!dang_bao_chay) {
      lcd.clear(); lcd.setCursor(0, 0); lcd.print("!!! FIRE !!!"); lcd.setCursor(0, 1); lcd.print("DOOR OPENING");
      servoCong.write(0); trangThaiCong = true; // Mở cửa thoát hiểm
      Blynk.virtualWrite(V11, 1);
      servo360.write(90); dangDiChuyen = false; dang_bao_chay = true;
      
      // Bổ sung ghi log khi phát hiện cháy
      terminal.println("!!! KHAN CAP: PHAT HIEN CHAY NO !!!");
      terminal.println("=> He thong: Tu dong MO TOANG cua phong khach de thoat hiem.");
      terminal.flush();
    }
    digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100);
    return; 
  } 
  else {
    if (dang_bao_chay) { 
      digitalWrite(BUZZER_PIN, LOW); 
      dang_bao_chay = false; 
      
      // Bổ sung ghi log khi an toàn trở lại
      terminal.println("=> He thong: Nong do khoi da an toan. Ngung bao dong.");
      terminal.flush();
      
      hienThiTrangThaiBanDau(); 
    }
  }

  // --- NÚT ĐÈN PHÒNG KHÁCH ---
  bool currentNutDenState = digitalRead(NUT_DEN_PIN);
  if (lastNutDenState == HIGH && currentNutDenState == LOW) {
    trangThaiDen = !trangThaiDen; digitalWrite(LED_PIN, trangThaiDen ? HIGH : LOW); delay(200);
    Blynk.virtualWrite(V9, trangThaiDen ? 1 : 0); 
  }
  lastNutDenState = currentNutDenState;

  // [ĐÃ SỬA] NÚT CỬA PHÒNG KHÁCH 
  bool currentNutCongState = digitalRead(NUT_CONG_PIN);
  if (lastNutCongState == HIGH && currentNutCongState == LOW) {
    trangThaiCong = !trangThaiCong; 
    lcd.clear(); lcd.setCursor(0, 0);
    if (trangThaiCong) { 
      servoCong.write(0); 
      lcd.print("Nut Nhan: MO CUA"); 
    } else { 
      servoCong.write(90); 
      lcd.print("Nut Nhan: DONG"); 
    }
    Blynk.virtualWrite(V11, trangThaiCong ? 1 : 0);
    delay(1000); 
    hienThiTrangThaiBanDau();
  }
  lastNutCongState = currentNutCongState;

  // [ĐÃ SỬA] NHẬP KEYPAD CỬA PHÒNG KHÁCH
  char key = matrixKeypad.getKey();
  if (key) {
    if (key == '*') {
      pin_nhap = ""; lcd.setCursor(0, 1); lcd.print("Nhap PIN:       "); 
    } 
    else if (key == '#') {
      lcd.setCursor(0, 1);
      if (pin_nhap == ma_PIN_dung) {
        trangThaiCong = !trangThaiCong;
        if (trangThaiCong) { 
          servoCong.write(0); 
          lcd.print("Access: MO cua  "); 
        } else { 
          servoCong.write(90);  
          lcd.print("Access: DONG cua"); 
        }
        Blynk.virtualWrite(V11, trangThaiCong ? 1 : 0);
        digitalWrite(BUZZER_PIN, HIGH); delay(150); digitalWrite(BUZZER_PIN, LOW);
      } else {
        lcd.print("Wrong PIN!      ");
        for(int i=0; i<3; i++) { digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); delay(100); }
      }
      delay(2000); pin_nhap = ""; hienThiTrangThaiBanDau(); 
    } 
    else {
      if (pin_nhap.length() < 10) { pin_nhap += key; lcd.setCursor(10, 1); lcd.print(pin_nhap); }
    }
  }
}

void hienThiTrangThaiBanDau() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home Ready");
  lcd.setCursor(0, 1);
  lcd.print("Nhap PIN:       ");
  if (pin_nhap != "") { lcd.setCursor(10, 1); lcd.print(pin_nhap); }
}