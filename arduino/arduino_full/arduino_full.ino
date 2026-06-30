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



// Chân Nút bật/tắt đèn Garage

#define NUT_DEN_GARAGE     8



// Chân cho 2 module RFID

#define RFID_1_SS          10

#define RFID_1_RST         A0

#define RFID_2_SS          4

#define RFID_2_RST         A1



// --- KHỞI TẠO ĐỐI TƯỢNG ---

Servo servoCong;

Servo servoGarage;

MFRC522 rfid1(RFID_1_SS, RFID_1_RST);

MFRC522 rfid2(RFID_2_SS, RFID_2_RST);



// --- BIẾN TRẠNG THÁI ĐỘC LẬP ---

// 1. Trạng thái Cổng

bool trangThaiCong = false;



// 2. Trạng thái Garage

enum TrangThaiGarage { STOP, DANG_LEN, DANG_XUONG };

TrangThaiGarage trangThaiGarage = STOP;

bool huongTiepTheoLaXuong = false;

bool trangThaiDenGarage = false;



// Biến chống dội nút nhấn

bool lastNutCong = HIGH;

bool lastNutGarage = HIGH;

bool lastNutDenGarage = HIGH;



const int NGUONG_KHOANG_CACH = 20;



void setup() {

  Serial.begin(9600);

  Wire.begin();

  SPI.begin();

 

  // Thiết lập chân RST làm Output để bật/tắt nguồn 2 con RFID

  pinMode(RFID_1_RST, OUTPUT);

  pinMode(RFID_2_RST, OUTPUT);

 

  pcf.begin();

 

  pcf.write(P_NUT_CONG, HIGH);

  pcf.write(P_NUT_GARAGE, HIGH);

  pcf.write(P_CTHT_GARAGE_LEN, HIGH);

  pcf.write(P_CTHT_GARAGE_XUONG, HIGH);

 

  // TẮT ĐÈN LÚC MỚI KHỞI ĐỘNG (Theo logic của bạn)

  pcf.write(P_LED_CONG, LOW);    

  pcf.write(P_LED_GARAGE, HIGH);



  pinMode(TRIG_CONG, OUTPUT);

  pinMode(ECHO_CONG, INPUT);

  pinMode(NUT_DEN_GARAGE, INPUT_PULLUP);



  servoCong.attach(SERVO_CONG_PIN);

  servoCong.write(0);

 

  servoGarage.attach(SERVO_GARAGE_PIN);

  servoGarage.write(90);



  Serial.println("He thong da san sang! (Phien ban 2 RFID hoat dong doc lap)");

}



void loop() {

  // ----------------------------------------------------

  // 1. ĐÈN TỰ ĐỘNG CỔNG (CẢM BIẾN SIÊU ÂM)

  // ----------------------------------------------------

  int kcCong = docKhoangCach(TRIG_CONG, ECHO_CONG);

  if (kcCong > 0 && kcCong < NGUONG_KHOANG_CACH) {

    pcf.write(P_LED_CONG, HIGH);

  } else {

    pcf.write(P_LED_CONG, LOW);  

  }



  // ----------------------------------------------------

  // 2. NÚT NHẤN BẬT/TẮT ĐÈN GARAGE

  // ----------------------------------------------------

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



  // ====================================================

  // PHÂN HỆ ĐỘC LẬP 1: CỔNG CHÍNH (THẺ 1 & NÚT CỔNG)

  // ====================================================

 

  // ---> Đánh thức Cổng, Ru ngủ Garage

  digitalWrite(RFID_2_RST, LOW);  

  digitalWrite(RFID_1_RST, HIGH);

  delay(30);        // Chờ 30ms cho chip ổn định điện áp

  rfid1.PCD_Init(); // Khởi tạo sóng ăng-ten

 

  if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {

    trangThaiCong = !trangThaiCong; // Đảo trạng thái Cổng

    if (trangThaiCong) {

      servoCong.write(90);

      Serial.println("Quet the 1 -> MO Cong Chinh");

    } else {

      servoCong.write(0);

      Serial.println("Quet the 1 -> DONG Cong Chinh");

    }

    rfid1.PICC_HaltA();

  }



  // Xử lý nút bấm trong cổng

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



  // ====================================================

  // PHÂN HỆ ĐỘC LẬP 2: GARAGE (THẺ 2, CTHT & NÚT GARAGE)

  // ====================================================

 

  // ---> Đánh thức Garage, Ru ngủ Cổng

  digitalWrite(RFID_1_RST, LOW);  

  digitalWrite(RFID_2_RST, HIGH);

  delay(30);        // Chờ 30ms cho chip ổn định điện áp

  rfid2.PCD_Init(); // Khởi tạo sóng ăng-ten



  if (rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {

    // Logic Đảo chiều / Dừng y hệt nút nhấn

    if (trangThaiGarage == STOP) {

      if (!huongTiepTheoLaXuong) {

        servoGarage.write(180);

        trangThaiGarage = DANG_LEN;

        Serial.println("Quet the 2 -> Garage di LEN");

      } else {

        servoGarage.write(0);

        trangThaiGarage = DANG_XUONG;

        Serial.println("Quet the 2 -> Garage di XUONG");

      }

    } else {

      // Đang chạy mà quét thẻ thì dừng lại

      servoGarage.write(90);

      if (trangThaiGarage == DANG_LEN) huongTiepTheoLaXuong = true;

      else huongTiepTheoLaXuong = false;

      trangThaiGarage = STOP;

      Serial.println("Quet the 2 -> DUNG Garage khan cap!");

    }

    rfid2.PICC_HaltA();

  }



  // Xử lý Công tắc hành trình Garage

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



  // Xử lý nút nhấn Garage

  if (nutGarage == LOW && lastNutGarage == HIGH) {

    if (trangThaiGarage == STOP) {

      if (!huongTiepTheoLaXuong) {

        servoGarage.write(180);

        trangThaiGarage = DANG_LEN;

      } else {

        servoGarage.write(0);

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



// Hàm đo khoảng cách cho cảm biến siêu âm Cổng

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