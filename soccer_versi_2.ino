// ========== KONFIGURASI PIN (Sesuai soccer.txt) ==========
int ena_kanan = 26;
int ena_kiri = 33;
int RPWM_kanan = 13;
int LPWM_kanan = 15;
int RPWM_kiri = 5;
int LPWM_kiri = 25;

// Konfigurasi PWM
const int frequency = 30000;
const int pwm_channel0 = 0; // RPWM Kiri
const int pwm_channel1 = 1; // LPWM Kiri
const int pwm_channel2 = 2; // RPWM Kanan
const int pwm_channel3 = 3; // LPWM Kanan
const int resolution = 8;

// Set Kecepatan (0 - 255)
int kecepatan = 150; 

void setup() {
  // Inisialisasi Serial Monitor 9600
  Serial.begin(9600);
  Serial.println("--- SISTEM AKTIF: MAJU TERUS ---");

  // Konfigurasi PWM ESP32
  ledcSetup(pwm_channel0, frequency, resolution);
  ledcSetup(pwm_channel1, frequency, resolution);
  ledcSetup(pwm_channel2, frequency, resolution);
  ledcSetup(pwm_channel3, frequency, resolution);
  
  // Hubungkan Pin Fisik ke Channel PWM
  ledcAttachPin(RPWM_kiri, pwm_channel0);
  ledcAttachPin(LPWM_kiri, pwm_channel1);
  ledcAttachPin(RPWM_kanan, pwm_channel2);
  ledcAttachPin(LPWM_kanan, pwm_channel3);
  
  // Set Pin Mode untuk Driver
  pinMode(RPWM_kanan, OUTPUT);
  pinMode(LPWM_kanan, OUTPUT);
  pinMode(RPWM_kiri, OUTPUT);
  pinMode(LPWM_kiri, OUTPUT);
  pinMode(ena_kiri, OUTPUT);
  pinMode(ena_kanan, OUTPUT);

  // Aktifkan Driver (Wajib HIGH agar motor jalan)
  digitalWrite(ena_kanan, HIGH);
  digitalWrite(ena_kiri, HIGH);
  
  Serial.println("Driver Enabled. Motor siap bergerak.");
}

void loop() {
  // LOGIKA MAJU (Berdasarkan fungsi maju Anda sebelumnya)
  // Motor Kiri: RPWM = 0, LPWM = Speed
  ledcWrite(pwm_channel0, 0);
  ledcWrite(pwm_channel1, kecepatan);
  
  // Motor Kanan: RPWM = Speed, LPWM = 0
  ledcWrite(pwm_channel2, kecepatan);
  ledcWrite(pwm_channel3, 0);

  // Update Status ke Serial Monitor
  Serial.print("Status: MAJU | Speed: ");
  Serial.println(kecepatan);
  
  delay(0); // Update log setiap 0.5 detik
}