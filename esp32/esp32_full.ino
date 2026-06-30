/*
  ====================================================================================
  BẢN ĐỒ ĐẤU DÂY VÀ SƠ ĐỒ CHÂN PHẦN CỨNG TOÀN HỆ THỐNG (HARDWARE WIRING MAP)
  Mạch điều khiển trung tâm: ESP32 Dev Module
  
  Mục đích: Khai báo toàn bộ cấu hình kết nối vật lý giữa ESP32 và các linh kiện ngoại vi.
  Đoạn mã này chuẩn hóa sơ đồ chân để bất kỳ hệ thống AI nào cũng có thể đọc hiểu cấu trúc phần cứng.
  ====================================================================================
  
  1. GIAI ĐOẠN 1: MÀN HÌNH HIỂN THỊ LCD 1602 (GIAO TIẾP I2C)
     - VCC (LCD)      --> Chân 5V (VIN) của ESP32 (Cấp nguồn 5V)
     - GND (LCD)      --> Chân GND chung
     - SDA (LCD)      --> GPIO 21 của ESP32 (Chân truyền dữ liệu I2C)
     - SCL (LCD)      --> GPIO 22 của ESP32 (Chân xung nhịp I2C)

  2. GIAI ĐOẠN 2: BÀN PHÍM MA TRẬN KEYPAD 4x3 (CỤM CHÂN GỌN LIÊN TIẾP)
     - Hàng 1 (R1)     --> GPIO 13 của ESP32
     - Hàng 2 (R2)     --> GPIO 12 của ESP32 (Lưu ý: Không nhấn giữ phím này khi cắm nguồn/nạp code)
     - Hàng 3 (R3)     --> GPIO 14 của ESP32
     - Hàng 4 (R4)     --> GPIO 27 của ESP32
     - Cột 1 (C1)     --> GPIO 26 của ESP32
     - Cột 2 (C2)     --> GPIO 25 của ESP32
     - Cột 3 (C3)     --> GPIO 33 của ESP32

  3. GIAI ĐOẠN 3: HỆ THỐNG BÁO CHÁY CHUẨN AN TOÀN
     A. Còi báo động (Buzzer):
        - Chân dương (+) --> GPIO 15 của ESP32 (Xuất tín hiệu HIGH để kích còi)
        - Chân âm (-)    --> Chân GND chung
     B. Cảm biến khói/khí gas MQ-2:
        - VCC (MQ-2)     --> Chân 3V3 của ESP32 (Bắt buộc dùng 3.3V để bảo vệ áp đầu vào Analog)
        - GND (MQ-2)     --> Chân GND chung
        - AO (Analog Out)--> GPIO 34 của ESP32 (Đọc giá trị điện áp Analog từ 0 - 4095)
        - DO (Digital Out)--> BỎ TRỐNG (Không sử dụng)

  4. GIAI ĐOẠN 4: HỆ THỐNG ĐÈN LED CHIẾU SÁNG & NÚT NHẤN TẮT/MỞ
     A. Đèn LED:
        - Chân dương (+) --> Nối qua điện trở hạn dòng (220Ω hoặc 330Ω) --> GPIO 4 của ESP32 (Đầu ra 3.3V)
        - Chân âm (-)    --> Chân GND chung
     B. Nút nhấn điều khiển đèn (Nút nhấn nhả 2 chân/4 chân):
        - Chân 1         --> GPIO 18 của ESP32 (Sử dụng điện trở kéo lên nội INPUT_PULLUP)
        - Chân 2         --> Chân GND chung

  5. GIAI ĐOẠN 5: ĐỘNG CƠ SERVO CỬA TRƯỚC (QUAY 180 ĐỘ) & NÚT BẤM CỬA
     A. Động cơ Servo 180:
        - Dây Đỏ (VCC)   --> Chân 5V (VIN) của ESP32 (Gắn kèm tụ điện hóa 1000µF song song nguồn để bù dòng)
        - Dây Nâu/Đen(GND)--> Chân GND chung
        - Dây Cam/Vàng(SIG)--> GPIO 5 của ESP32 (Xuất xung điều khiển PWM góc quay từ 0 - 180 độ)
     B. Nút nhấn đóng/mở cửa từ bên trong:
        - Chân 1         --> GPIO 19 của ESP32 (Sử dụng điện trở kéo lên nội INPUT_PULLUP)
        - Chân 2         --> Chân GND chung

  6. GIAI ĐOẠN 6: HỆ THỐNG SÀO PHƠI ĐỒ THÔNG MINH (SERVO 360 ĐỘ & CẢM BIẾN MƯA LM393)
     A. Động cơ Servo 360 độ (Quay liên tục):
        - Dây Đỏ (VCC)   --> Chân 5V (VIN) của ESP32
        - Dây Nâu/Đen(GND)--> Chân GND chung
        - Dây Cam/Vàng(SIG)--> GPIO 23 của ESP32 (Điều khiển hướng quay: 0 = Thuận, 180 = Ngược, 90 = Dừng)
     B. Cảm biến mưa (Raindrop Module):
        - Đầu có 2 chân không chú thích trên bo xử lý nhỏ màu xanh --> Nối vào 2 chân của tấm vỉ kim loại nhận nước mưa.
        - Chân VCC (Bo nhỏ) --> Chân 3V3 của ESP32 (Bảo vệ mức logic tín hiệu đầu vào)
        - Chân GND (Bo nhỏ) --> Chân GND chung
        - Chân DO (Digital Out)--> GPIO 35 của ESP32 (Đọc tín hiệu Logic kỹ thuật số: LOW = Có mưa, HIGH = Khô ráo)
        - Chân AO (Analog Out)--> BỎ TRỐNG (Không sử dụng)

  7. GIAI ĐOẠN KẾT NỐI MỞ RỘNG (GIAO TIẾP LIÊN MẠCH - TRUYỀN LỆNH BLYNK/WIFI)
     - Chân TX2       --> GPIO 17 của ESP32 --> Nối thẳng sang chân nhận dữ liệu RX (Digital 2) của Arduino UNO.
     - Chân GND chung  --> Nối chung GND giữa ESP32 và Arduino để đồng bộ mass tín hiệu.
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h> 
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ================= CẤU HÌNH GIAI ĐOẠN 1: MÀN HÌNH LCD 1602 I2C =================
// Cấu hình màn hình hiển thị: Địa chỉ I2C 0x27, kích thước 16 cột và 2 hàng
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= CẤU HÌNH GIAI ĐOẠN 2: BÀN PHÍM MA TRẬN KEYPAD 4x3 =================
const byte ROWS = 4; // Ma trận gồm 4 hàng
const byte COLS = 3; // Ma trận gồm 3 cột
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// Định nghĩa mảng chân kết nối vật lý của Keypad vào ESP32
byte rowPins[ROWS] = {13, 12, 14, 27}; // Chân hàng kết nối lần lượt vào GPIO 13, 12, 14, 27
byte colPins[COLS] = {26, 25, 33};     // Chân cột kết nối lần lượt vào GPIO 26, 25, 33
Keypad matrixKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String ma_PIN_dung = "1234"; // Chuỗi ký tự lưu mã PIN chính xác của hệ thống
String pin_nhap = "";        // Biến động lưu trữ các ký tự người dùng đang bấm nhập

// ================= CẤU HÌNH GIAI ĐOẠN 3: HỆ THỐNG CẢNH BÁO CHÁY NỔ =================
#define BUZZER_PIN 15       // Còi báo động kết nối với chân kỹ thuật số đầu ra GPIO 15
#define MQ2_PIN 34          // Chân Analog đầu vào GPIO 34 dùng để đọc cảm biến khói Gas MQ-2
#define NGUONG_BAO_CHAY 2500 // Ngưỡng kích hoạt báo động (Giá trị đọc Analog vượt quá 2500)
bool dang_bao_chay = false; // Biến trạng thái giám sát hệ thống có đang gặp hỏa hoạn hay không

// ================= CẤU HÌNH GIAI ĐOẠN 4: HỆ THỐNG ĐÈN CHIẾU SÁNG =================
#define LED_PIN 4           // Đèn LED chiếu sáng phòng kết nối chân đầu ra GPIO 4
#define NUT_DEN_PIN 18      // Nút nhấn tắt/mở đèn kết nối chân đầu vào kỹ thuật số GPIO 18
bool trangThaiDen = false;  // Biến lưu trạng thái Đèn hiện tại (false = Tắt, true = Sáng)
bool lastNutDenState = HIGH; // Biến lưu trạng thái trước đó của nút nhấn đèn phục vụ thuật toán bắt sườn xuống

// ================= CẤU HÌNH GIAI ĐOẠN 5: ĐỘNG CƠ CỬA TRƯỚC GARA (180 ĐỘ) =================
#define SERVO_CONG_PIN 5    // Chân xuất xung điều khiển Servo 180 độ kết nối với GPIO 5
#define NUT_CONG_PIN 19     // Nút nhấn đóng mở cửa bằng tay kết nối chân đầu vào GPIO 19
Servo servoCong;            // Khai báo đối tượng điều khiển động cơ Servo cửa trước
bool trangThaiCong = false; // Biến lưu trạng thái cửa hiện tại (false = Đóng 0 độ, true = Mở 90 độ)
bool lastNutCongState = HIGH; // Biến lưu trạng thái trước đó của nút bấm cửa phục vụ xử lý nhấn nhả

// ================= CẤU HÌNH GIAI ĐOẠN 6: SÀO PHƠI ĐỒ TỰ ĐỘNG (SERVO 360 ĐỘ & CẢM BIẾN MƯA) =================
#define SERVO_360_PIN 23    // Chân xuất xung điều khiển Servo 360 độ quay liên tục kết nối với GPIO 23
#define RAIN_PIN 35         // Chân kỹ thuật số đầu vào đọc trạng thái từ mạch cảm biến mưa GPIO 35
Servo servo360;             // Khai báo đối tượng điều khiển động cơ Servo 360 độ quay liên tục
bool dangCoMua = false;         // Biến lưu trạng thái thời tiết thực tế (false = Khô ráo, true = Trời mưa)
bool dangDiChuyen = false;      // Biến cờ hiệu xác định động cơ Servo 360 có đang trong tiến trình quay hay không
unsigned long thoiGianBatDauQuay = 0; // Biến lưu mốc thời gian (ms) bắt đầu kích hoạt động cơ để tính chu kỳ chạy 5 giây


void setup() {
  Serial.begin(115200); // Khởi tạo cổng truyền thông Serial truyền dữ liệu lên máy tính với Baudrate 115200
  
  // Can thiệp thanh ghi để vô hiệu hóa mạch bảo vệ sụt áp nguồn (Brownout Detector) của ESP32
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Thiết lập cấu hình chiều dữ liệu đầu vào/đầu ra cho các chân GPIO
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(NUT_DEN_PIN, INPUT_PULLUP); // Sử dụng điện trở kéo lên nội của chip ESP32 (Mặc định chân ở mức HIGH)
  pinMode(NUT_CONG_PIN, INPUT_PULLUP); // Thiết lập chế độ INPUT_PULLUP chống nhiễu thả nổi cho nút bấm cửa
  pinMode(RAIN_PIN, INPUT);           // Đọc mức logic kỹ thuật số đầu vào trực tiếp từ module cảm biến mưa

  // Trạng thái tắt mặc định của các thiết bị ngoại vi khi vừa khởi động hệ thống
  digitalWrite(BUZZER_PIN, LOW); 
  digitalWrite(LED_PIN, LOW); 

  // Khởi tạo các tham số hoạt động cho Động cơ Servo Cửa Trước (180 độ)
  servoCong.setPeriodHertz(50); // Đặt tần số điều khiển PWM chuẩn là 50Hz cho Servo
  servoCong.attach(SERVO_CONG_PIN, 500, 2400); // Đấu nối chân và thiết lập dải độ rộng xung từ 500us đến 2400us
  servoCong.write(0); // Ép trục servo quay về vị trí khóa cửa (0 độ) ngay khi khởi động

  // Khởi tạo các tham số hoạt động cho Động cơ Servo Sào Phơi (360 độ)
  servo360.setPeriodHertz(50);
  servo360.attach(SERVO_360_PIN, 1000, 2000); // Thiết lập dải xung chuẩn 1000us - 2000us tối ưu cho việc triệt tiêu trôi Servo
  servo360.write(90); // Gửi xung dừng hẳn (mức góc 90 độ tương đương xung 1500us) để giữ động cơ đứng im

  // Khởi tạo màn hình hiển thị LCD 1602
  Wire.begin(21, 22); // Thiết lập cổng giao tiếp I2C với chân dữ liệu SDA=21 và chân xung nhịp SCL=22
  lcd.init();
  lcd.backlight(); // Bật sáng đèn nền LCD
  hienThiTrangThaiBanDau(); // Gọi hàm hiển thị giao diện màn hình chờ chính
}

void loop() {
  // ---------------- GIAI ĐOẠN 6: THUẬT TOÁN ĐIỀU KHIỂN SÀO PHƠI ĐỒ THEO THỜI TIẾT ----------------
  if (!dang_bao_chay) { // Tính năng phơi đồ chỉ chạy khi ngôi nhà ở trạng thái an toàn (không có cháy)
    bool phatHienMua = (digitalRead(RAIN_PIN) == LOW); // Đọc chân GPIO 35, cảm biến trả về LOW nghĩa là bề mặt có nước mưa

    // Trường hợp 1: Trời bắt đầu đổ mưa (Chuyển đổi trạng thái từ Khô ráo sang Có nước)
    if (phatHienMua && !dangCoMua) {
      dangCoMua = true;
      dangDiChuyen = true;
      thoiGianBatDauQuay = millis(); // Ghi lại mốc thời gian hiện tại của vi điều khiển
      
      servo360.write(180); // Kích hoạt Servo 360 quay hết tốc độ về bên TRÁI để kéo sào đồ vào trong nhà
      Serial.println(">> TROI MUA! Keo sao phoi do vao trong...");
    }
    // Trường hợp 2: Trời tạnh mưa (Chuyển đổi trạng thái từ Có nước sang Khô ráo hoàn toàn)
    else if (!phatHienMua && dangCoMua) {
      dangCoMua = false;
      dangDiChuyen = true;
      thoiGianBatDauQuay = millis(); // Làm mới lại mốc thời gian bắt đầu quay
      
      servo360.write(0); // Kích hoạt Servo 360 quay hết tốc độ về bên PHẢI để đẩy sào đồ ra ngoài nắng
      Serial.println(">> TROI QUANG! Day sao phoi do ra ngoai...");
    }

    // Cơ chế an toàn đếm giờ ngầm (Non-blocking): Tự động ngắt điện dừng động cơ sau đúng 5 giây (5000ms) chạy liên tục
    if (dangDiChuyen && (millis() - thoiGianBatDauQuay >= 5000)) {
      servo360.write(90); // Gửi lệnh dừng hẳn cho động cơ Servo 360
      dangDiChuyen = false; // Hạ cờ hiệu di chuyển
      Serial.println(">> Servo 360: Da quay du 5 giay -> DUNG.");
    }
  }

  // ---------------- GIAI ĐOẠN 3: GIÁM SÁT HỎA HOẠN VÀ XỬ LÝ AN TOÀN KHẨN CẤP ----------------
  int muc_khi_gas = analogRead(MQ2_PIN); // Đọc giá trị điện áp Analog từ chân GPIO 34 (Dải đo từ 0 đến 4095)

  if (muc_khi_gas > NGUONG_BAO_CHAY) { // Nếu nồng độ khói/khí gas vượt quá ngưỡng cho phép
    if (!dang_bao_chay) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("!!! FIRE !!!");
      lcd.setCursor(0, 1);
      lcd.print("DOOR OPENING");

      servoCong.write(90); // Báo cháy: TỰ ĐỘNG MỞ cửa trước để thoát hiểm
      trangThaiCong = true;

      servo360.write(90); // Dừng động cơ phơi đồ khi có cháy (an toàn)
      dangDiChuyen = false; 
      
      dang_bao_chay = true;
    }
    // Tạo chuỗi xung nháy réo còi hú báo động liên tục gắt quãng
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100); 
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    return; // Dừng toàn bộ các tác vụ xử lý nút bấm hay nhập mã PIN bên dưới khi ngôi nhà đang cháy nổ
  } 
  else {
    // Khi nồng độ khói đã tan hoàn toàn (Hệ thống chuyển từ trạng thái báo cháy về trạng thái an toàn)
    if (dang_bao_chay) {
      digitalWrite(BUZZER_PIN, LOW); // Tắt còi hú báo động
      dang_bao_chay = false;
      hienThiTrangThaiBanDau(); // Trả lại giao diện màn hình nhập mã PIN bình thường
    }
  }

  // ---------------- GIAI ĐOẠN 4: THUẬT TOÁN XỬ LÝ NÚT NHẤN BẬT / TẮT ĐÈN LED ----------------
  bool currentNutDenState = digitalRead(NUT_DEN_PIN); // Đọc trạng thái điện áp hiện tại của chân nút nhấn đèn GPIO 18
  
  // Phát hiện hành động bấm nút: Trạng thái chuyển từ chưa nhấn (HIGH) sang nhấn giữ (LOW)
  if (lastNutDenState == HIGH && currentNutDenState == LOW) {
    trangThaiDen = !trangThaiDen; // Đảo trạng thái bóng đèn LED
    digitalWrite(LED_PIN, trangThaiDen ? HIGH : LOW); // Xuất điện áp điều khiển LED (HIGH = sáng, LOW = tắt)
    delay(200); // Hàm trễ ngắn dùng để chống dội phím cơ khí (Debounce)
  }
  lastNutDenState = currentNutDenState; // Lưu lại trạng thái để so sánh ở vòng lặp loop tiếp theo

  // ---------------- GIAI ĐOẠN 5: THUẬT TOÁN XỬ LÝ NÚT NHẤN ĐÓNG / MỞ CỬA GARA ----------------
  bool currentNutCongState = digitalRead(NUT_CONG_PIN); // Đọc trạng thái logic của nút bấm cửa tại GPIO 19
  
  // Phát hiện hành động bấm nút điều khiển cửa từ bên trong nhà
  if (lastNutCongState == HIGH && currentNutCongState == LOW) {
    trangThaiCong = !trangThaiCong; // Đảo trạng thái đóng/mở của cửa sổ
    if (trangThaiCong) {
      servoCong.write(90); // Điều khiển cánh tay servo quay ra góc 90 độ (Mở cửa)
    } else {
      servoCong.write(0);  // Điều khiển cánh tay servo quay về góc 0 độ (Đóng cửa)
    }
    delay(200); // Chống dội phím nút bấm cửa
  }
  lastNutCongState = currentNutCongState;

  // ---------------- GIAI ĐOẠN 2: THUẬT TOÁN QUÉT PHÍM VÀ BẢO MẬT MÃ PIN HỆ THỐNG CỬA ----------------
  char key = matrixKeypad.getKey(); // Thực hiện quét ma trận phím từ thư viện Keypad để nhận nút bấm hiện tại

  if (key) { // Nếu phát hiện có nút được nhấn vào từ Keypad
    // Trường hợp nút bấm là phím '*' --> Chức năng XÓA LÀM LẠI
    if (key == '*') {
      pin_nhap = ""; // Xóa sạch toàn bộ chuỗi ký tự đã nhập
      lcd.setCursor(0, 1);
      lcd.print("Nhap PIN:       "); // Ghi đè khoảng trắng để xóa dòng cũ trên màn hình LCD
    } 
    // Trường hợp nút bấm là phím '#' --> Chức năng XÁC NHẬN KIỂM TRA MẬT MÃ
    else if (key == '#') {
      lcd.setCursor(0, 1);
      if (pin_nhap == ma_PIN_dung) { // So sánh chuỗi vừa gõ với mã gốc "1234"
        // Mat khau dung -> DAO trang thai cua truoc (dang dong thi MO, dang mo thi DONG)
        trangThaiCong = !trangThaiCong;
        if (trangThaiCong) {
          servoCong.write(90);            // Mo cua truoc
          lcd.print("Access: MO cua  ");
        } else {
          servoCong.write(0);             // Dong cua truoc
          lcd.print("Access: DONG cua");
        }

        // Phát tín hiệu âm thanh "Bíp" ngắn báo hiệu thao tác thành công
        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
      } else {
        lcd.print("Wrong PIN!      "); // Hiển thị thông báo sai mật mã
        
        // Kích hoạt chuỗi còi hú dằn mặt "Bíp bíp bíp" gồm 3 tiếng liên tục
        for(int i = 0; i < 3; i++) {
          digitalWrite(BUZZER_PIN, HIGH);
          delay(100);
          digitalWrite(BUZZER_PIN, LOW);
          delay(100);
        }
      }
      
      delay(2000); // Treo thông báo kết quả nhập PIN trên màn hình trong 2 giây
      pin_nhap = ""; // Reset bộ đệm chuỗi
      lcd.setCursor(0, 1);
      lcd.print("Nhap PIN:       "); // Khôi phục lại dòng chữ hiển thị mặc định
    } 
    // Trường hợp người dùng nhấn các nút số từ 0 đến 9
    else {
      if (pin_nhap.length() < 4) { // Giới hạn chỉ cho phép nhập độ dài tối đa là 4 ký tự mã PIN
        pin_nhap += key; // Thêm ký tự vừa nhấn vào cuối chuỗi lưu trữ
        lcd.setCursor(10, 1); 
        lcd.print(pin_nhap); // In trực tiếp chuỗi số đang gõ lên sau dấu hai chấm của LCD
      }
    }
  }
}

// Hàm phụ thực thi việc vẽ lại toàn bộ giao diện màn hình chờ ban đầu của hệ thống nhà thông minh
void hienThiTrangThaiBanDau() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home Ready"); // Hàng 1 thông báo hệ thống đã sẵn sàng hoạt động
  lcd.setCursor(0, 1);
  lcd.print("Nhap PIN:       "); // Hàng 2 hiển thị nhãn hướng dẫn nhập mã khóa bảo mật
  if (pin_nhap != "") {
    lcd.setCursor(10, 1); 
    lcd.print(pin_nhap); // Giữ lại hiển thị các số đang nhập dở dang nếu màn hình bị vẽ lại
  }
}