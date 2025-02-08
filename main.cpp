#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

// Wi-Fi credentials
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";
const char* host = "RASPBERRY_PI_IP";
const int port = "RASPBERRY_PI_PORT";

// Ultrasonic sensor pins
const int TRIG_PIN = D9;
const int ECHO_PIN = D8;
const int DISTANCE_THRESHOLD = 50; // Distance threshold in cm

// Operation mode
bool SIMULATION_MODE = true; // Set to false when using real sensor

// Previous state
bool previousState = false;

// Modify constants
const int MAX_RETRIES = 3;  // Maximum HTTP request retries

// Add WebServer
ESP8266WebServer server(81);

// Simple state handler - returns current detection state
void handleStream() {
    bool currentState = detectMotion();  // Get real-time detection
    String response = String(currentState ? "1" : "0");
    server.send(200, "text/plain", response);
}

// Add timer constant
const unsigned long MODE_CHECK_INTERVAL = 10000; // 10 seconds
unsigned long lastModeCheck = 0;

void setup() {
    Serial.begin(115200);  // Make sure Serial Monitor matches this baud rate
    delay(2000);  // Wait for serial to initialize
    Serial.println("\n\nStarting Sonar Sensor...");  // Startup message
    
    // Visual feedback
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);  // Turn on LED
    
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    
    // Connect to WiFi with feedback
    Serial.printf("Connecting to WiFi: %s\n", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // Toggle LED
        delay(500);
        Serial.print(".");
    }
    digitalWrite(LED_BUILTIN, LOW);  // Turn off LED
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    // Setup web server
    server.on("/stream", handleStream);
    server.begin();
    Serial.println("HTTP server running on port 81");
}

void ensureWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.print("Reconnecting to WiFi");
        WiFi.begin(ssid, password);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();
    }
}

// Update sendData with retries
void sendData(bool detected) {
    ensureWiFiConnection();
    
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        WiFiClient client;
        int retries = 0;
        
        while (retries < MAX_RETRIES) {
            String url = String("http://") + host + ":" + String(port) + "/sensor_data";
            http.begin(client, url);
            http.addHeader("Content-Type", "application/json");
            
            String jsonString = "{\"value\":" + String(detected ? "1" : "0") + "}";
            int httpResponseCode = http.POST(jsonString);
            
            if (httpResponseCode > 0) {
                digitalWrite(LED_BUILTIN, HIGH);
                delay(100);
                digitalWrite(LED_BUILTIN, LOW);
                http.end();
                return;
            }
            
            retries++;
            delay(1000);
        }
        http.end();
    }
}

float getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    return duration * 0.034 / 2;
}

void handleSimulation() {
    static unsigned long lastStateChange = 0;
    unsigned long currentTime = millis();
    
    // Change state every 5 seconds
    if (currentTime - lastStateChange >= 5000) {
        previousState = !previousState;  // Toggle state
        sendData(previousState);
        Serial.printf("Simulation: Object %s\n", 
                     previousState ? "detected" : "removed");
        lastStateChange = currentTime;
    }
}

void handleRealSensor() {
    float distance = getDistance();
    bool currentState = (distance < DISTANCE_THRESHOLD);
    
    // Only send data when state changes
    if (currentState != previousState) {
        sendData(currentState);
        Serial.printf("Object %s at %.2f cm\n", 
                     currentState ? "detected" : "removed",
                     distance);
        previousState = currentState;
    }
    
    delay(100);  // Small delay to prevent flooding
}

// Update loop with longer interval
void loop() {
    server.handleClient();
    
    bool detected = detectMotion();
    if (detected) {
        Serial.println("Motion detected!");
        digitalWrite(LED_BUILTIN, LOW);  // LED on
    } else {
        digitalWrite(LED_BUILTIN, HIGH); // LED off
    }
    
    delay(100);  // Small delay to prevent overwhelming the serial output
}

bool detectMotion() {
    // Trigger the sonar
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Read the response
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2;
    
    return distance < DISTANCE_THRESHOLD;
}