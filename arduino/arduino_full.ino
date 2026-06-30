/*
  ====================================================================================
  BẢN ĐỒ ĐẤU DÂY VÀ SƠ ĐỒ CHÂN PHẦN CỨNG TOÀN HỆ THỐNG (HARDWARE WIRING MAP)
  Mạch điều khiển trung tâm: Arduino UNO / Nano
  
  Mục đích: Khai báo toàn bộ cấu hình kết nối vật lý giữa Arduino và các module ngoại vi.
  Bao gồm xử lý Cổng chính, Garage (có công tắc hành trình), Đèn tự động và 2 cụm quét thẻ RFID.
  ====================================================================================
  
  1. GIAO TIẾP I2C (MODULE MỞ RỘNG CHÂN PCF8574 - ĐỊA CHỈ 0x20)
     - SDA (PCF8574)  --> Chân A4 của Arduino
     - SCL (PCF8574)  --> Chân A5 của Arduino
     - VCC & GND      --> 5V và GND của Arduino
     [Chức năng các chân mở rộng trên PCF8574]:
       + P0 (Output)  --> Đèn LED Cổng chính
       + P1 (Output)  --> Đèn LED Garage
       + P2 (Input)   --> Nút nhấn Đóng/Mở Cổng chính (Kích LOW)
       + P3 (Input)   --> Nút nhấn Đóng/Mở/Dừng Garage (Kích LOW)
       + P4 (Input)   --> Công tắc hành trình TRÊN của Garage (Chạm = LOW, ngắt động cơ đi lên)
       + P5 (Input)   --> Công tắc hành trình DƯỚI của Garage (Chạm = LOW, ngắt động cơ đi xuống)

  2. HỆ THỐNG ĐỘNG CƠ (SERVO) - KẾT NỐI TRỰC TIẾP ARDUINO
     - Servo Cổng (180 độ)   --> Chân D3 (Mở = 90 độ, Đóng = 0 độ)
     - Servo Garage (360 độ) --> Chân D5 (Dừng = 90, Cuốn lên = 180, Hạ xuống = 0)
     * Lưu ý: Nguồn của Servo phải được cấp từ nguồn 5V ngoài (có tụ bù áp), mass chung với Arduino.

  3. CẢM BIẾN SIÊU ÂM HC-SR04 (ĐO KHOẢNG CÁCH BẬT ĐÈN CỔNG)
     - VCC & GND      --> 5V và GND
     - TRIG (Phát)    --> Chân D6
     - ECHO (Thu)     --> Chân D7

  4. NÚT NHẤN ĐÈN GARAGE
     - Chân 1         --> Chân D8 (Sử dụng INPUT_PULLUP nội bộ của Arduino)
     - Chân 2         --> GND

  5. HỆ THỐNG ĐỌC THẺ TỪ RFID RC522 (GIAO TIẾP SPI DÙNG CHUNG BUS)
     * Các chân SPI dùng chung cho CẢ 2 MODULE RFID:
       - SCK  --> Chân D13
       - MISO --> Chân D12
       - MOSI --> Chân D11
       - VCC  --> 3.3V (Tuyệt đối không cấp 5V sẽ cháy RFID)
       - GND  --> GND
       
     * Cấu hình chân độc lập cho MODULE RFID 1 (Bảo vệ Cổng chính):
       - SDA (SS/CS)  --> Chân D10 (Kích LOW để chip 1 lắng nghe)
       - RST (Reset)  --> Chân A0 (Dùng kỹ thuật bật/tắt nguồn reset luân phiên)

     * Cấu hình chân độc lập cho MODULE RFID 2 (Bảo vệ Garage):
       - SDA (SS/CS)  --> Chân D4 (Kích LOW để chip 2 lắng nghe)
       - RST (Reset)  --> Chân A1 (Dùng kỹ thuật bật/tắt nguồn reset luân phiên)
*/

#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8574.h>

// --- CẤU HÌNH ĐỊA CHỈ PCF8574 ---
// Khởi tạo đối tượng giao tiếp I2C với IC PCF8574 tại địa chỉ phần cứng 0x20
PCF8574 pcf(0x20); 

// --- ĐỊNH NGHĨA CÁC CHÂN IC PCF8574 ---
#define P_LED_CONG         0 // Chân P0: Cấp nguồn cho LED Cổng
#define P_LED_GARAGE       1 // Chân P1: Cấp nguồn cho LED Garage
#define P_NUT_CONG         2 // Chân P2: Đọc tín hiệu nút nhấn Cổng
#define P_NUT_GARAGE       3 // Chân P3: Đọc tín hiệu nút nhấn Garage
#define P_CTHT_GARAGE_LEN  4 // Chân P4: Đọc công tắc hành trình giới hạn chiều LÊN
#define P_CTHT_GARAGE_XUONG 5 // Chân P5: Đọc công tắc hành trình giới hạn chiều XUỐNG

// --- ĐỊNH NGHĨA CÁC CHÂN TRỰC TIẾP TRÊN ARDUINO ---
#define SERVO_CONG_PIN     3 // Chân băm xung PWM cho Servo Cổng (Loại 180 độ)
#define SERVO_GARAGE_PIN   5 // Chân băm xung PWM cho Servo Garage (Loại 360 độ - Tời kéo)

#define TRIG_CONG          6 // Chân kích phát sóng siêu âm
#define ECHO_CONG          7 // Chân nhận sóng siêu âm dội lại

// Chân Nút bật/tắt đèn Garage
#define NUT_DEN_GARAGE     8 // Đọc nút bấm đèn, tích hợp điện trở kéo lên

// Chân chọn chip (SS/CS) và Reset (RST) cho 2 module RFID hoạt động song song
#define RFID_1_SS          10 // Chip Select Module 1 (Cổng)
#define RFID_1_RST         A0 // Reset Module 1 (Cổng) - Dùng chân Analog làm Digital Output
#define RFID_2_SS          4  // Chip Select Module 2 (Garage)
#define RFID_2_RST         A1 // Reset Module 2 (Garage) - Dùng chân Analog làm Digital Output

// --- KHỞI TẠO ĐỐI TƯỢNG NGOẠI VI ---
Servo servoCong;
Servo servoGarage;
MFRC522 rfid1(RFID_1_SS, RFID_1_RST); // Cấu hình đối tượng RFID số 1
MFRC522 rfid2(RFID_2_SS, RFID_2_RST); // Cấu hình đối tượng RFID số 2

// --- BIẾN TRẠNG THÁI ĐỘC LẬP ---
// 1. Trạng thái Cổng
bool trangThaiCong = false; // false = Cổng đang đóng, true = Cổng đang mở

// 2. Trạng thái Garage (Sử dụng máy trạng thái - State Machine)
enum TrangThaiGarage { STOP, DANG_LEN, DANG_XUONG }; // 3 trạng thái của cửa cuốn Garage
TrangThaiGarage trangThaiGarage = STOP; // Mặc định đứng im
bool huongTiepTheoLaXuong = false; // Ghi nhớ hướng đi để đảo chiều ở lần bấm nút tiếp theo
bool trangThaiDenGarage = false; 

// Biến chống dội nút nhấn (Debounce) lưu lại lịch sử bấm phím
bool lastNutCong = HIGH;
bool lastNutGarage = HIGH;
bool lastNutDenGarage = HIGH;

// Cờ bắt sườn thẻ RFID: chỉ xử lý khi thẻ VỪA áp vào.
// Giữ thẻ yên -> chỉ đọc 1 lần; phải nhấc thẻ ra xa rồi áp lại mới đọc lần nữa.
bool theHienDien1 = false; int demVang1 = 0; // cho đầu đọc Cổng
bool theHienDien2 = false; int demVang2 = 0; // cho đầu đọc Garage

// Ngưỡng khoảng cách phát hiện vật cản của cảm biến siêu âm (Đơn vị: cm)
const int NGUONG_KHOANG_CACH = 20; 

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Khởi động bus I2C (Cho PCF8574)
  SPI.begin();  // Khởi động bus SPI (Cho 2 module RFID)
  
  // Thiết lập chân RST làm Output để có thể ngắt/mở nguồn 2 con RFID luân phiên (Chống nhiễu SPI)
  pinMode(RFID_1_RST, OUTPUT);
  pinMode(RFID_2_RST, OUTPUT);
  
  pcf.begin(); // Bắt đầu kết nối với IC PCF8574
  
  // Khởi tạo các chân Input trên PCF8574 ở mức HIGH (Bật điện trở kéo lên ảo)
  pcf.write(P_NUT_CONG, HIGH);
  pcf.write(P_NUT_GARAGE, HIGH);
  pcf.write(P_CTHT_GARAGE_LEN, HIGH);
  pcf.write(P_CTHT_GARAGE_XUONG, HIGH);
  
  // TẮT ĐÈN LÚC MỚI KHỞI ĐỘNG 
  pcf.write(P_LED_CONG, LOW);    
  pcf.write(P_LED_GARAGE, HIGH); 

  // Cấu hình in/out cho các chân trên main Arduino
  pinMode(TRIG_CONG, OUTPUT);
  pinMode(ECHO_CONG, INPUT);
  pinMode(NUT_DEN_GARAGE, INPUT_PULLUP); // Bật điện trở nội bảo vệ chân D8

  // Khởi tạo Servo Cổng và khóa mặc định ở 0 độ
  servoCong.attach(SERVO_CONG_PIN);
  servoCong.write(0); 
  
  // Khởi tạo Servo Garage (360) và ép trục dừng im ở xung 90
  servoGarage.attach(SERVO_GARAGE_PIN);
  servoGarage.write(90); 

  Serial.println("He thong da san sang! (Phien ban 2 RFID hoat dong doc lap)");
}

void loop() {
  // ----------------------------------------------------
  // 1. ĐÈN TỰ ĐỘNG CỔNG (KÍCH HOẠT BỞI CẢM BIẾN SIÊU ÂM)
  // ----------------------------------------------------
  int kcCong = docKhoangCach(TRIG_CONG, ECHO_CONG); // Gọi hàm tính toán khoảng cách
  if (kcCong > 0 && kcCong < NGUONG_KHOANG_CACH) {
    pcf.write(P_LED_CONG, HIGH); // Sáng đèn khi có người/xe trong phạm vi 20cm
  } else {
    pcf.write(P_LED_CONG, LOW);  // Tắt đèn khi không có chướng ngại vật
  }

  // ----------------------------------------------------
  // 2. XỬ LÝ NÚT NHẤN BẬT/TẮT ĐÈN GARAGE
  // ----------------------------------------------------
  bool nutDenGarage = digitalRead(NUT_DEN_GARAGE);
  if (nutDenGarage == LOW && lastNutDenGarage == HIGH) { // Bắt sự kiện người dùng nhấn nút (chuyển sườn từ HIGH xuống LOW)
    trangThaiDenGarage = !trangThaiDenGarage; 
    if (trangThaiDenGarage) {
      pcf.write(P_LED_GARAGE, LOW); 
      Serial.println("BAT den Garage");
    } else {
      pcf.write(P_LED_GARAGE, HIGH); 
      Serial.println("TAT den Garage");
    }
    delay(200); // Trễ chống nhiễu phím
  }
  lastNutDenGarage = nutDenGarage;

  // ====================================================
  // PHÂN HỆ ĐỘC LẬP 1: CỬA CỔNG CHÍNH (ĐIỀU KHIỂN BẰNG THẺ 1 & NÚT CỔNG)
  // ====================================================
  
  // Kỹ thuật Time-Multiplexing: Đánh thức Cổng, Ru ngủ Garage để tránh xung đột dữ liệu trên đường truyền SPI
  digitalWrite(RFID_2_RST, LOW);  // Kéo LOW để tắt RFID 2
  digitalWrite(RFID_1_RST, HIGH); // Kéo HIGH để bật RFID 1
  delay(30);        // Chờ 30ms cho chip cấp nguồn ổn định
  rfid1.PCD_Init(); // Kích hoạt sóng ăng-ten cho đầu đọc 1
  
  // Quét xem có thẻ chạm vào đầu đọc 1 không
  if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {
    demVang1 = 0;                      // có thấy thẻ -> reset bộ đếm vắng
    if (!theHienDien1) {               // CHỈ xử lý khi thẻ VỪA áp vào (sườn lên)
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
    // Không thấy thẻ vài vòng liên tiếp -> coi như đã nhấc thẻ ra, cho phép đọc lần sau
    if (++demVang1 > 2) theHienDien1 = false;
  }

  // Xử lý đọc nút bấm mở cổng khẩn cấp từ bên trong thông qua PCF8574
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
  // PHÂN HỆ ĐỘC LẬP 2: CỬA CUỐN GARAGE (ĐIỀU KHIỂN BẰNG THẺ 2, CTHT & NÚT GARAGE)
  // ====================================================
  
  // Kỹ thuật Time-Multiplexing: Đánh thức Garage, Ru ngủ Cổng
  digitalWrite(RFID_1_RST, LOW);  // Tắt RFID 1
  digitalWrite(RFID_2_RST, HIGH); // Bật RFID 2
  delay(30);        
  rfid2.PCD_Init(); 

  // Quét xem có thẻ chạm vào đầu đọc 2 không
  if (rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {
    demVang2 = 0;                      // có thấy thẻ -> reset bộ đếm vắng
    if (!theHienDien2) {               // CHỈ xử lý khi thẻ VỪA áp vào (sườn lên)
      theHienDien2 = true;
      // Thuật toán Đảo chiều / Dừng (Hoạt động giống hệ thống cửa cuốn thực tế)
      if (trangThaiGarage == STOP) {
        if (!huongTiepTheoLaXuong) {
          servoGarage.write(180); // Cấp xung quay liên tục (Cuốn cửa lên)
          trangThaiGarage = DANG_LEN;
          Serial.println("Quet the 2 -> Garage di LEN");
        } else {
          servoGarage.write(0); // Cấp xung quay ngược liên tục (Hạ cửa xuống)
          trangThaiGarage = DANG_XUONG;
          Serial.println("Quet the 2 -> Garage di XUONG");
        }
      } else {
        // Nếu cửa ĐANG CHẠY mà có hành động quét thẻ -> Ra lệnh DỪNG KHẨN CẤP
        servoGarage.write(90); // Gửi xung 90 để phanh Servo 360 lại
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

  // Đọc các cảm biến giới hạn vật lý (Công tắc hành trình) của Garage qua PCF8574
  bool cthtLen = (pcf.read(P_CTHT_GARAGE_LEN) == LOW);     
  bool cthtXuong = (pcf.read(P_CTHT_GARAGE_XUONG) == LOW); 
  bool nutGarage = pcf.read(P_NUT_GARAGE);

  // Bảo vệ cơ khí: Tự động ngắt động cơ khi cửa cuộn chạm trần (CTHT Lên)
  if (trangThaiGarage == DANG_LEN && cthtLen) {
    servoGarage.write(90); 
    trangThaiGarage = STOP;
    huongTiepTheoLaXuong = true; // Set cờ hiệu để lần bấm sau cửa sẽ đi xuống
    Serial.println("CHAM CTHT TREN -> Dung!");
  }
  
  // Bảo vệ cơ khí: Tự động ngắt động cơ khi cửa chạm sàn (CTHT Xuống)
  if (trangThaiGarage == DANG_XUONG && cthtXuong) {
    servoGarage.write(90); 
    trangThaiGarage = STOP;
    huongTiepTheoLaXuong = false; // Set cờ hiệu để lần bấm sau cửa sẽ đi lên
    Serial.println("CHAM CTHT DUOI -> Dung!");
  }

  // Xử lý nút bấm điều khiển Garage (Logic hoạt động y hệt như lúc quét thẻ)
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

// Hàm tính toán khoảng cách vật cản dựa trên thời gian dội âm của cảm biến HC-SR04
int docKhoangCach(int trigPin, int echoPin) {
  // Tạo một xung phát sóng ngắn 10 micro giây
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Đếm thời gian sóng siêu âm đi và dội về (Timeout 30ms để chống treo vi điều khiển)
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return -1; // Trả về lỗi nếu không nhận được sóng phản xạ
  
  // Công thức: Khoảng cách (cm) = (Thời gian * Vận tốc âm thanh 0.034 cm/us) / 2 chiều
  int distance = duration * 0.034 / 2;
  return distance;
}