# ESP32 Object Follower System

Sistem robot following object menggunakan 2 ESP32:
1. **ESP32-CAM** - Sebagai controller (seperti stik PS3) dengan object detection FOMO
2. **ESP32 WROOM** - Robot yang akan mengikuti object

## 📁 File Structure

```
soccer_versi_2/
├── esp32cam_object_follower_controller.ino  → Upload ke ESP32-CAM
├── robot_object_follower.ino                → Upload ke ESP32 WROOM (robot)
└── README_OBJECT_FOLLOWER.md                → Dokumentasi ini
```

## 🔧 Hardware Requirements

### ESP32-CAM (Controller)
- ESP32-CAM module
- FOMO model sudah di-deploy
- Power supply 5V

### ESP32 WROOM (Robot)
- ESP32 WROOM board
- Motor driver (L298N atau sejenisnya)
- 2x DC Motors
- Power supply untuk motor
- Chassis robot

## 📡 Komunikasi

**Bluetooth Classic (Serial):**
- ESP32-CAM broadcast sebagai: `ESP32-CAM-Controller`
- Robot broadcast sebagai: `SOCCER-v2`
- Keduanya akan auto-connect saat dihidupkan

## 🎯 Cara Kerja

### ESP32-CAM Controller
1. Melakukan object detection menggunakan FOMO
2. Menganalisis posisi bounding box:
   - **Horizontal**: Kiri, Tengah, Kanan
   - **Size**: Jauh (box kecil), Dekat (box besar)
3. Mengirim command via Bluetooth ke robot

### Robot Follower
1. Menerima command via Bluetooth
2. Menggerakkan motor sesuai command
3. Failsafe: Stop jika tidak ada command dalam 1 detik

## 🎮 Logika Following

| Kondisi Object | Area Bounding Box | Command |
|---------------|-------------------|---------|
| Object di kiri | Any | `KIRI` atau `MAJU_KIRI` |
| Object di kanan | Any | `KANAN` atau `MAJU_KANAN` |
| Object di tengah | Kecil (jauh) | `MAJU` |
| Object di tengah | Sedang (ideal) | `STOP` |
| Object di tengah | Besar (terlalu dekat) | `MUNDUR` |
| Tidak ada object | - | `STOP` |

## ⚙️ Parameter Yang Bisa Disesuaikan

### ESP32-CAM Controller (`esp32cam_object_follower_controller.ino`)

```cpp
// Zona deteksi
const int DEAD_ZONE = 30;           // Zona tengah (pixel)
const int MIN_BOX_SIZE = 1000;      // Box minimum untuk "dekat"
const int MAX_BOX_SIZE = 15000;     // Box maksimum untuk "mundur"
const int TARGET_BOX_SIZE = 5000;   // Box ideal

// Confidence threshold
const float MIN_CONFIDENCE = 0.5;   // Minimum confidence detection

// Command timing
const int COMMAND_INTERVAL = 100;   // Kirim command setiap 100ms
```

### Robot Follower (`robot_object_follower.ino`)

```cpp
// Kecepatan
int speedNormal = 120;     // Kecepatan normal
int speedTurn = 100;       // Kecepatan belok
int speedSlow = 80;        // Kecepatan lambat

// Failsafe
const int FAILSAFE_TIMEOUT = 1000;  // Timeout 1 detik
```

## 📝 Cara Upload

### 1. Upload ke ESP32-CAM
```
1. Buka esp32cam_object_follower_controller.ino di Arduino IDE
2. Pastikan library Edge Impulse sudah terinstall
3. Board: AI Thinker ESP32-CAM
4. Upload (gunakan FTDI programmer)
```

### 2. Upload ke Robot (ESP32 WROOM)
```
1. Buka robot_object_follower.ino di Arduino IDE
2. Board: ESP32 Dev Module (atau sesuai board)
3. Upload
```

## 🚀 Cara Menggunakan

1. **Nyalakan Robot terlebih dahulu**
   - Robot akan broadcast Bluetooth sebagai "SOCCER-v2"
   - LED akan mati (waiting for connection)

2. **Nyalakan ESP32-CAM**
   - ESP32-CAM akan connect ke robot
   - LED robot akan nyala (connected)

3. **Arahkan kamera ke object**
   - Robot akan otomatis mengikuti object yang terdeteksi
   - Monitor Serial untuk debug info

## 📊 Serial Monitor Output

### ESP32-CAM:
```
=== ESP32-CAM Object Follower Controller ===
Bluetooth initialized as: ESP32-CAM-Controller
Camera initialized successfully
✓ Robot connected!
Object: person (0.85) [x:120 y:80 w:60 h:80] -> CMD: MAJU
→ Sent: MAJU
```

### Robot:
```
=== Robot Object Follower ===
Bluetooth initialized as: SOCCER-v2
✓ ESP32-CAM Controller connected!
→ MAJU
→ KANAN
→ STOP
```

## 🔍 Troubleshooting

### Robot tidak connect
- Pastikan nama device sama (`SOCCER-v2`)
- Cek apakah Bluetooth sudah diaktifkan
- Restart kedua device

### Detection tidak akurat
- Tuning parameter `DEAD_ZONE`, `MIN_BOX_SIZE`, `MAX_BOX_SIZE`
- Sesuaikan `MIN_CONFIDENCE` threshold
- Pastikan pencahayaan cukup

### Robot terlalu cepat/lambat
- Sesuaikan `speedNormal`, `speedTurn` di robot
- Ubah `COMMAND_INTERVAL` di ESP32-CAM

### Robot sering stop (failsafe)
- Tingkatkan `FAILSAFE_TIMEOUT` di robot
- Kurangi `COMMAND_INTERVAL` di ESP32-CAM

## 🎛️ Advanced Tuning

### Untuk Object Lebih Kecil (misal: bola)
```cpp
const int MIN_BOX_SIZE = 500;      // Lower threshold
const int MAX_BOX_SIZE = 8000;     // Lower threshold
const int DEAD_ZONE = 20;          // Lebih sensitif
```

### Untuk Object Lebih Besar (misal: manusia)
```cpp
const int MIN_BOX_SIZE = 2000;     // Higher threshold
const int MAX_BOX_SIZE = 20000;    // Higher threshold
const int DEAD_ZONE = 40;          // Kurang sensitif
```

## 📋 Command List

Commands yang dikirim dari ESP32-CAM ke Robot:

- `MAJU` - Maju lurus
- `MUNDUR` - Mundur lurus
- `KIRI` - Belok kiri di tempat
- `KANAN` - Belok kanan di tempat
- `MAJU_KIRI` - Maju sambil belok kiri
- `MAJU_KANAN` - Maju sambil belok kanan
- `STOP` - Berhenti

## 💡 Tips

1. **Tuning Pertama Kali:**
   - Mulai dengan `speedNormal = 100` (lebih lambat)
   - Gradually tingkatkan kecepatan

2. **Stabilitas:**
   - Gunakan battery terpisah untuk motor dan ESP32
   - Tambahkan kapasitor di power motor

3. **Range:**
   - Bluetooth Classic range ~10 meter
   - Pastikan line of sight untuk hasil terbaik

## 📄 License

MIT License - Bebas digunakan dan dimodifikasi

## 👤 Author

arilix - 2026

---

**Happy Following! 🤖📷**
