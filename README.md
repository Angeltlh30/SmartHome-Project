# Smart Home — Đồ án IOT102

Hệ thống 2 board chạy độc lập trong Wokwi (offline; Blynk/WiFi để sau):

- **`ESP32_Gateway/`** — trong nhà: keypad mật khẩu + LCD + khóa cửa trước/sau (relay), đèn/quạt (relay), báo cháy (MQ2 + buzzer), sào phơi tự động.
- **`Arduino_Node/`** — cổng & gara: RFID RC522 ×2, radar bật đèn, motor cửa.

## Nguyên tắc chọn linh kiện
- Linh kiện **Wokwi có sẵn** → dùng thật: LCD, MQ2, **relay**, **keypad**, buzzer, **RFID RC522**.
- Linh kiện **Wokwi KHÔNG có** → dùng **LED** thay (không lấy part khác):
  - **Motor DC + L298N** (cổng, gara, sào phơi) → **LED** chỉ chiều chạy (mở/đóng, kéo vào/đẩy ra).
- Cảm biến **không có trong Wokwi** mà là tín hiệu **đầu vào** (radar RCWL, cảm biến mưa) → **nút bấm giả lập** (LED không nhập tín hiệu vào được).

| BOM thật | Trong mô phỏng |
|---|---|
| Motor DC + L298N | LED (xanh = mở/kéo vào, đỏ = đóng/đẩy ra) + chú thích |
| Radar RCWL | nút "Giả lập Radar" |
| Cảm biến mưa | nút "Giả lập MUA" |
| Solenoid | hiển thị bằng đèn báo của relay |
| MQ2, RFID, keypad, LCD, buzzer, relay | part thật của Wokwi |

## Relay (đúng BOM: 1 cái 4 kênh + 1 cái 1 chân)
- **Relay 1 chân** → khóa cửa **TRƯỚC**.
- **Relay 4 kênh** → CH1 khóa cửa **SAU**, CH2 **đèn**, CH3 **quạt**, CH4 **dự phòng**.

Wokwi chỉ có module relay 1 kênh nên trong diagram dùng 4 module rời, đặt tên rõ để biết kênh nào thuộc module 4 kênh.

## Sơ đồ chân
**ESP32**: LCD 21/22 · keypad R=13,14,33,17 C=15,16,0,2 · relay: khóa trước=4, khóa sau=12, đèn=5, quạt=18 · buzzer=19 · LED sào: vào=23, ra=25 · MQ2=34 · nút giả lập mưa=35, nút cửa sau=36 (2 chân này có trở kéo lên 10k ngoài) · nút đèn=26, quạt=27, cửa trước=32.

**Arduino**: SPI 11/12/13, RST chung=9 · SS RFID cổng=10, gara=8 · LED motor: cổng mở=2/đóng=3, gara mở=4/đóng=5 · đèn cổng=6, gara=7 · nút radar cổng=A0, gara=A1 · nút trong cổng=A2, gara=A3.

## Chạy bằng VS Code
Mỗi thư mục:
```powershell
# ESP32_Gateway
arduino-cli compile --fqbn esp32:esp32:esp32 --output-dir build .
# Arduino_Node
arduino-cli compile --fqbn arduino:avr:uno --output-dir build .
```
Rồi mở `diagram.json` → F1 → `Wokwi: Start Simulator`.

## Lên mạch thật
- Relay **kích Low**: mở `ESP32_Gateway.ino` đổi `#define RELAY_ACTIVE_LOW 1`.
- Cổng/gara/sào dùng motor DC thật → thay LED bằng L298N + motor; thêm **công tắc hành trình** (BOM có) để ngắt motor — đấu qua **PCF8574** vì UNO/ESP32 đã gần hết chân.
- ESP32 logic 3.3V; module 5V cần đệm mức.
