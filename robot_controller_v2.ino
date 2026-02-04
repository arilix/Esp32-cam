/*
 * Robot Controller v2.0 - BLE Edition
 * Compatible with: Soccer Robot & Sumo Robot
 * Communication: Bluetooth Low Energy (BLE)
 * Author: arilix
 * Date: 2026-01-09
 * Repository: https://github.com/arilix/Soccer_v2
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ========== BLE SETUP ==========
BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
bool deviceConnected = false;
String deviceName = "SOCCER-2";  // Change to "SUMO-v1" for sumo robot

// BLE UUIDs (Nordic UART Service compatible)
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

String receivedCommand = "";

// ========== PIN CONFIGURATION ==========
int ena_kanan = 26;
int ena_kiri = 33;
int RPWM_kanan = 13;
int LPWM_kanan = 15;
int RPWM_kiri = 5;
int LPWM_kiri = 25;
int pwm_ki, pwm_ka;

// ========== PWM CONFIGURATION ==========
const int frequency = 30000;
const int pwm_channel = 0;
const int pwm_channel1 = 1;
const int pwm_channel2 = 2;
const int pwm_channel3 = 3;
const int resolution = 8;

// ========== CONTROL VARIABLES ==========
int speedNormal = 90;      // Normal speed (adjustable via Pro Mode)
int speedFast = 255;       // Fast/Boost speed (adjustable via Pro Mode)
int currentSpeed = 90;     // Current active speed
bool isBoostMode = false;  // Boost active flag

// ========== FAILSAFE ==========
unsigned long lastCommandTime = 0;
const int FAILSAFE_TIMEOUT = 2000;  // 2 seconds (adjustable via Pro Mode)

// ========== MOTOR CONTROL FUNCTIONS ==========
void kanan() {
  ledcWrite(pwm_channel, 0);
  ledcWrite(pwm_channel1, pwm_ki);
  ledcWrite(pwm_channel2, 0);
  ledcWrite(pwm_channel3, pwm_ka);
}

void kiri() {
  ledcWrite(pwm_channel, pwm_ki);
  ledcWrite(pwm_channel1, 0);
  ledcWrite(pwm_channel2, pwm_ka);
  ledcWrite(pwm_channel3, 0);
}

void maju() {
  ledcWrite(pwm_channel, 0);
  ledcWrite(pwm_channel1, pwm_ki);
  ledcWrite(pwm_channel2, pwm_ka);
  ledcWrite(pwm_channel3, 0);
}

void mundur() {
  ledcWrite(pwm_channel, pwm_ki);
  ledcWrite(pwm_channel1, 0);
  ledcWrite(pwm_channel2, 0);
  ledcWrite(pwm_channel3, pwm_ka);
}

void maju_kiri() {
  ledcWrite(pwm_channel, 0);
  ledcWrite(pwm_channel1, pwm_ki / 2);
  ledcWrite(pwm_channel2, pwm_ka);
  ledcWrite(pwm_channel3, 0);
}

void maju_kanan() {
  ledcWrite(pwm_channel, 0);
  ledcWrite(pwm_channel1, pwm_ki);
  ledcWrite(pwm_channel2, pwm_ka / 2);
  ledcWrite(pwm_channel3, 0);
}

void mundur_kiri() {
  ledcWrite(pwm_channel, pwm_ki);
  ledcWrite(pwm_channel1, 0);
  ledcWrite(pwm_channel2, 0);
  ledcWrite(pwm_channel3, pwm_ka / 2);
}

void mundur_kanan() {
  ledcWrite(pwm_channel, pwm_ki / 2);
  ledcWrite(pwm_channel1, 0);
  ledcWrite(pwm_channel2, 0);
  ledcWrite(pwm_channel3, pwm_ka);
}

void stopped() {
  ledcWrite(pwm_channel, 0);
  ledcWrite(pwm_channel1, 0);
  ledcWrite(pwm_channel2, 0);
  ledcWrite(pwm_channel3, 0);
}

// ========== COMMAND PARSER ==========
void parseCommand(String cmd) {
  cmd.trim();  // Remove whitespace
  
  // Update last command time (for failsafe)
  lastCommandTime = millis();
  
  // Set speed based on mode
  if (isBoostMode) {
    pwm_ki = speedFast;
    pwm_ka = speedFast;
  } else {
    pwm_ki = currentSpeed;
    pwm_ka = currentSpeed;
  }
  
  // Movement commands
  if (cmd == "MAJU") {
    maju();
    Serial.println("CMD: MAJU");
  }
  else if (cmd == "MUNDUR") {
    mundur();
    Serial.println("CMD: MUNDUR");
  }
  else if (cmd == "KIRI") {
    kiri();
    Serial.println("CMD: KIRI");
  }
  else if (cmd == "KANAN") {
    kanan();
    Serial.println("CMD: KANAN");
  }
  else if (cmd == "MAJU_KIRI") {
    maju_kiri();
    Serial.println("CMD: MAJU_KIRI");
  }
  else if (cmd == "MAJU_KANAN") {
    maju_kanan();
    Serial.println("CMD: MAJU_KANAN");
  }
  else if (cmd == "MUNDUR_KIRI") {
    mundur_kiri();
    Serial.println("CMD: MUNDUR_KIRI");
  }
  else if (cmd == "MUNDUR_KANAN") {
    mundur_kanan();
    Serial.println("CMD: MUNDUR_KANAN");
  }
  else if (cmd == "STOP") {
    stopped();
    Serial.println("CMD: STOP");
  }
  
  // Speed mode commands
  else if (cmd == "SPEED_NORMAL") {
    currentSpeed = speedNormal;
    isBoostMode = false;
    Serial.println("CMD: SPEED_NORMAL");
  }
  else if (cmd == "SPEED_FAST") {
    currentSpeed = speedFast;
    isBoostMode = false;
    Serial.println("CMD: SPEED_FAST");
  }
  else if (cmd == "BOOST_ON") {
    isBoostMode = true;
    Serial.println("CMD: BOOST_ON");
  }
  else if (cmd == "BOOST_OFF") {
    isBoostMode = false;
    Serial.println("CMD: BOOST_OFF");
  }
  
  // Special action commands (for Soccer robot)
  else if (cmd == "KICK") {
    // TODO: Implement kick mechanism if hardware available
    Serial.println("CMD: KICK");
  }
  
  // Pro Mode tuning commands
  else if (cmd.startsWith("TUNING:")) {
    parseTuning(cmd);
  }
  
  // Ping command (for latency test)
  else if (cmd == "PING") {
    sendBLE("PONG");
  }
  
  // Unknown command
  else {
    Serial.println("CMD: UNKNOWN - " + cmd);
  }
}

// ========== BLE SEND FUNCTION ==========
void sendBLE(String message) {
  if (deviceConnected && pTxCharacteristic != NULL) {
    pTxCharacteristic->setValue(message.c_str());
    pTxCharacteristic->notify();
  }
}

// ========== PRO MODE TUNING PARSER ==========
void parseTuning(String data) {
  // Format: "TUNING:pwmMax,frequency,speedNormal,speedFast,failsafeTimeout"
  // Example: "TUNING:255,30000,90,255,2000"
  
  data.replace("TUNING:", "");
  
  int values[5];
  int index = 0;
  int lastPos = 0;
  
  for (int i = 0; i < data.length(); i++) {
    if (data.charAt(i) == ',' || i == data.length() - 1) {
      if (i == data.length() - 1) i++;
      values[index++] = data.substring(lastPos, i).toInt();
      lastPos = i + 1;
      if (index >= 5) break;
    }
  }
  
  // Apply tuning values
  // values[0] = pwmMax (not used, always 255 for 8-bit)
  // values[1] = frequency (Hz)
  // values[2] = speedNormal
  // values[3] = speedFast
  // values[4] = failsafeTimeout (ms)
  
  if (values[1] > 0) {
    ledcSetup(pwm_channel, values[1], resolution);
    ledcSetup(pwm_channel1, values[1], resolution);
    ledcSetup(pwm_channel2, values[1], resolution);
    ledcSetup(pwm_channel3, values[1], resolution);
    Serial.print("Frequency updated: ");
    Serial.println(values[1]);
  }
  
  speedNormal = constrain(values[2], 0, 255);
  speedFast = constrain(values[3], 0, 255);
  
  Serial.println("TUNING: Applied successfully");
  sendBLE("TUNING_OK");
}

// ========== BLE CALLBACKS ==========
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
      // Restart advertising
      BLEDevice::startAdvertising();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        receivedCommand = "";
        for (int i = 0; i < rxValue.length(); i++) {
          receivedCommand += rxValue[i];
        }
        // Process command
        parseCommand(receivedCommand);
      }
    }
};

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  
  // BLE initialization
  Serial.println("Starting BLE...");
  BLEDevice::init(deviceName.c_str());
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX Characteristic (for sending data to phone)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create RX Characteristic (for receiving data from phone)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                                           CHARACTERISTIC_UUID_RX,
                                           BLECharacteristic::PROPERTY_WRITE
                                         );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE initialized: " + deviceName);
  Serial.println("Waiting for connection...");
  
  // PWM setup
  ledcSetup(pwm_channel, frequency, resolution);
  ledcSetup(pwm_channel1, frequency, resolution);
  ledcSetup(pwm_channel2, frequency, resolution);
  ledcSetup(pwm_channel3, frequency, resolution);
  
  // Pin attachments
  ledcAttachPin(RPWM_kiri, pwm_channel);
  ledcAttachPin(LPWM_kiri, pwm_channel1);
  ledcAttachPin(RPWM_kanan, pwm_channel2);
  ledcAttachPin(LPWM_kanan, pwm_channel3);
  
  // Pin modes
  pinMode(RPWM_kanan, OUTPUT);
  pinMode(LPWM_kanan, OUTPUT);
  pinMode(RPWM_kiri, OUTPUT);
  pinMode(LPWM_kiri, OUTPUT);
  pinMode(ena_kiri, OUTPUT);
  pinMode(ena_kanan, OUTPUT);
  
  // Initial state: motors disabled
  digitalWrite(ena_kanan, LOW);
  digitalWrite(ena_kiri, LOW);
  
  Serial.println("Setup complete!");
}

// ========== MAIN LOOP ==========
void loop() {
  // Check BLE connection
  if (deviceConnected) {
    // Enable motors when connected
    digitalWrite(ena_kanan, HIGH);
    digitalWrite(ena_kiri, HIGH);
    
    // Failsafe: Stop if no command received for timeout period
    if (millis() - lastCommandTime > FAILSAFE_TIMEOUT) {
      stopped();
      // Don't spam serial
      if ((millis() / 1000) % 5 == 0) {  // Print every 5 seconds
        Serial.println("FAILSAFE: No command, motors stopped");
      }
    }
  } 
  else {
    // BLE disconnected: disable motors
    stopped();
    digitalWrite(ena_kanan, LOW);
    digitalWrite(ena_kiri, LOW);
    
    // Reset command time
    lastCommandTime = 0;
  }
  
  // Small delay to prevent overwhelming the loop
  delay(10);
}
