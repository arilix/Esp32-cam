/*
 * ESP32-CAM Object Follower with Direct Motor Control
 * Uses FOMO Object Detection to track objects and control motors directly
 * Author: arilix
 * Date: 2026-01-18
 * Purpose: ESP32-CAM with integrated motor control (MX1508 driver)
 */

#include <FOMO_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "esp_camera.h"

// ========== PIN MX1508 MOTOR DRIVER ==========
#define IN1 12   // Motor A (KANAN)
#define IN2 13
#define IN3 14   // Motor B (KIRI)
#define IN4 15

// ========== CAMERA MODEL ==========
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#endif

// ========== CAMERA SETTINGS ==========
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

static bool debug_nn = false;
static bool is_initialised = false;
uint8_t *snapshot_buf;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_QVGA,
    .jpeg_quality = 12,
    .fb_count = 1,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

// ========== OBJECT FOLLOWING PARAMETERS ==========
const int FRAME_CENTER_X = EI_CLASSIFIER_INPUT_WIDTH / 2;
const int FRAME_CENTER_Y = EI_CLASSIFIER_INPUT_HEIGHT / 2;

// Zona deteksi (dalam pixel) - Optimized untuk deteksi person
const int DEAD_ZONE = 40;           // Zona tengah dimana robot tidak perlu belok
const int MIN_BOX_SIZE = 2000;      // Ukuran minimum box untuk dianggap "terlalu dekat"
const int MAX_BOX_SIZE = 20000;     // Ukuran maksimum box untuk mundur (person besar)
const int TARGET_BOX_SIZE = 8000;   // Ukuran ideal bounding box untuk person

// Threshold confidence
const float MIN_CONFIDENCE = 0.6;   // Minimum confidence untuk detection (person only)

// Command timing
unsigned long lastCommandTime = 0;
const int COMMAND_INTERVAL = 100;   // Kirim command setiap 100ms
String lastCommand = "";

// ========== FUNCTION DECLARATIONS ==========
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
String analyzeObject(ei_impulse_result_bounding_box_t bb);

// Motor control functions
void motorStop();
void motorMaju();
void motorMundur();
void motorKiri();
void motorKanan();
void motorMajuKiri();
void motorMajuKanan();
void executeCommand(String command);

// ========== SETUP ==========
void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32-CAM Object Follower with Motor Control ===");
    
    // Initialize Motor Pins
    Serial.println("Initializing motor pins...");
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    motorStop();  // Start with motors stopped
    Serial.println("✓ Motor pins initialized");
    
    // Initialize Camera
    Serial.println("Initializing camera...");
    if (ei_camera_init() == false) {
        Serial.println("ERROR: Failed to initialize Camera!");
        while(1);
    }
    Serial.println("✓ Camera initialized successfully");
    
    Serial.println("\n=== System Ready ===");
    Serial.println("Starting object tracking...");
    
    delay(1000);
}

// ========== MAIN LOOP ==========
void loop()
{
    // Allocate snapshot buffer
    snapshot_buf = (uint8_t*)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * 
                                     EI_CAMERA_RAW_FRAME_BUFFER_ROWS * 
                                     EI_CAMERA_FRAME_BYTE_SIZE);
    
    if(snapshot_buf == nullptr) {
        Serial.println("ERROR: Failed to allocate snapshot buffer!");
        delay(1000);
        return;
    }
    
    // Prepare signal
    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;
    
    // Capture image
    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, 
                          (size_t)EI_CLASSIFIER_INPUT_HEIGHT, 
                          snapshot_buf) == false) {
        Serial.println("ERROR: Failed to capture image");
        free(snapshot_buf);
        delay(100);
        return;
    }
    
    // Run classifier
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    
    if (err != EI_IMPULSE_OK) {
        Serial.printf("ERROR: Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return;
    }
    
    // Process detection results
#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    
    // Find best "person" detection (highest confidence)
    ei_impulse_result_bounding_box_t bestBox;
    float bestConfidence = 0.0;
    bool objectFound = false;
    
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        
        // Filter hanya untuk "person" dengan confidence tinggi
        if (bb.value >= MIN_CONFIDENCE && bb.value > bestConfidence) {
            // Optional: Uncomment baris dibawah jika ingin strict filter "person" saja
            // if (String(bb.label) != "person") continue;
            
            bestConfidence = bb.value;
            bestBox = bb;
            objectFound = true;
        }
    }
    
    // Generate command based on detection
    String command = "";
    
    if (objectFound) {
        command = analyzeObject(bestBox);
        
        // Print detection info
        Serial.printf("[PERSON DETECTED] Conf:%.2f [x:%u y:%u w:%u h:%u] Area:%d -> %s\n",
                     bestBox.value,
                     bestBox.x,
                     bestBox.y,
                     bestBox.width,
                     bestBox.height,
                     bestBox.width * bestBox.height,
                     command.c_str());
    } else {
        command = "STOP";
        // Print hanya setiap 2 detik untuk tidak spam
        if (millis() % 2000 < 100) {
            Serial.println("[NO PERSON] -> STOP");
        }
    }
    
    // Execute command (with rate limiting)
    if (millis() - lastCommandTime >= COMMAND_INTERVAL) {
        executeCommand(command);
        lastCommandTime = millis();
    }
    
#else
    Serial.println("ERROR: This model is not an object detection model!");
    sendCommand("STOP");
#endif
    
    // Free memory
    free(snapshot_buf);
    
    // Small delay
    delay(50);
}

// ========== OBJECT ANALYSIS ==========
String analyzeObject(ei_impulse_result_bounding_box_t bb) {
    // Calculate object center
    int objectCenterX = bb.x + (bb.width / 2);
    int objectCenterY = bb.y + (bb.height / 2);
    
    // Calculate bounding box area
    int boxArea = bb.width * bb.height;
    
    // Calculate horizontal offset from center
    int offsetX = objectCenterX - FRAME_CENTER_X;
    
    // Determine command based on position and size
    String command = "";
    
    // Check if object is too close (box too large)
    if (boxArea > MAX_BOX_SIZE) {
        command = "MUNDUR";  // Back up
    }
    // Check if object is in acceptable range
    else if (boxArea > MIN_BOX_SIZE) {
        // Object is close enough, just adjust position
        if (abs(offsetX) <= DEAD_ZONE) {
            command = "STOP";  // Object centered and close enough
        } else if (offsetX < -DEAD_ZONE) {
            command = "KIRI";  // Turn left
        } else {
            command = "KANAN";  // Turn right
        }
    }
    // Object is far, move forward while adjusting
    else {
        if (abs(offsetX) <= DEAD_ZONE) {
            command = "MAJU";  // Go straight forward
        } else if (offsetX < -DEAD_ZONE) {
            command = "MAJU_KIRI";  // Forward-left
        } else {
            command = "MAJU_KANAN";  // Forward-right
        }
    }
    
    return command;
}

// ========== MOTOR CONTROL FUNCTIONS ==========
void motorStop() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void motorMaju() {
    // MAJU (berdasarkan contoh user - versi dibalik)
    digitalWrite(IN1, LOW);   // Kanan maju
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);   // Kiri maju
    digitalWrite(IN4, HIGH);
}

void motorMundur() {
    // MUNDUR (kebalikan dari maju)
    digitalWrite(IN1, HIGH);  // Kanan mundur
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);  // Kiri mundur
    digitalWrite(IN4, LOW);
}

void motorKiri() {
    // BELOK KIRI (kanan maju, kiri mundur)
    digitalWrite(IN1, LOW);   // Kanan maju
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);  // Kiri mundur
    digitalWrite(IN4, LOW);
}

void motorKanan() {
    // BELOK KANAN (kanan mundur, kiri maju)
    digitalWrite(IN1, HIGH);  // Kanan mundur
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);   // Kiri maju
    digitalWrite(IN4, HIGH);
}

void motorMajuKiri() {
    // MAJU sambil belok KIRI (kiri pelan, kanan cepat)
    // Simplified: kanan maju, kiri stop
    digitalWrite(IN1, LOW);   // Kanan maju
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);   // Kiri stop
    digitalWrite(IN4, LOW);
}

void motorMajuKanan() {
    // MAJU sambil belok KANAN (kanan pelan, kiri cepat)
    // Simplified: kiri maju, kanan stop
    digitalWrite(IN1, LOW);   // Kanan stop
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);   // Kiri maju
    digitalWrite(IN4, HIGH);
}

void executeCommand(String command) {
    if (command != lastCommand) {
        lastCommand = command;
        Serial.println("→ Command: " + command);
        
        if (command == "STOP") {
            motorStop();
        } else if (command == "MAJU") {
            motorMaju();
        } else if (command == "MUNDUR") {
            motorMundur();
        } else if (command == "KIRI") {
            motorKiri();
        } else if (command == "KANAN") {
            motorKanan();
        } else if (command == "MAJU_KIRI") {
            motorMajuKiri();
        } else if (command == "MAJU_KANAN") {
            motorMajuKanan();
        } else {
            motorStop();  // Default: stop jika command tidak dikenal
        }
    }
}

// ========== CAMERA FUNCTIONS ==========
bool ei_camera_init(void) {
    if (is_initialised) return true;

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) { 
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, 0);
    }

    is_initialised = true;
    return true;
}

void ei_camera_deinit(void) {
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK) {
        ei_printf("Camera deinit failed\n");
        return;
    }
    is_initialised = false;
}

bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    bool do_resize = false;

    if (!is_initialised) {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
        ei_printf("Camera capture failed\n");
        return false;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
    esp_camera_fb_return(fb);

    if(!converted){
        ei_printf("Conversion failed\n");
        return false;
    }

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
        || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        do_resize = true;
    }

    if (do_resize) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }

    return true;
}

static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16) + 
                               (snapshot_buf[pixel_ix + 1] << 8) + 
                               snapshot_buf[pixel_ix];
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    
    return 0;
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif
