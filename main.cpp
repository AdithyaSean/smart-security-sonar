#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";
const char* host = "RASPBERRY_PI_IP";
const int port = 2003;  // Default to first preferred port

// Ultrasonic sensor pins
const int TRIG_PIN = D1;
const int ECHO_PIN = D2;
const int DISTANCE_THRESHOLD = 50; // Distance threshold in cm

// Operation mode
const bool SIMULATION_MODE = true; // Set to false when using real sensor

// Simulation variables
unsigned long simulationStartTime = 0;
const unsigned long SIMULATION_DURATION = 5000; // 5 seconds
bool simulationActive = false;

void setup() {
    Serial.begin(115200);
    
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    
    pinMode(LED_BUILTIN, OUTPUT);
    
    if (!SIMULATION_MODE) {
        pinMode(TRIG_PIN, OUTPUT);
        pinMode(ECHO_PIN, INPUT);
    }
}

void sendData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        WiFiClient client;
        
        // Construct the URL with port
        String url = String("http://") + host + ":" + String(port) + "/sensor_data";
        
        http.begin(client, url);
        http.addHeader("Content-Type", "application/json");
        
        // Create simple JSON string
        String jsonString = "{\"value\":1}";
        
        // Send POST request
        int httpResponseCode = http.POST(jsonString);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Response: " + response);
            digitalWrite(LED_BUILTIN, HIGH);  // Indicate successful send
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
        } else {
            Serial.println("Error on sending POST: " + String(httpResponseCode));
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
    // Send 1s for 5 seconds straight
    unsigned long currentTime = millis();
    
    if (!simulationActive) {
        simulationStartTime = currentTime;
        simulationActive = true;
        for (int i = 0; i < 5; i++) {  // Send 5 times during the 5 seconds
            sendData();
            delay(1000);
        }
        simulationActive = false;
        delay(5000);  // Wait 5 seconds before next cycle
    }
}

void handleRealSensor() {
    float distance = getDistance();
    
    if (distance < DISTANCE_THRESHOLD) {
        sendData();
        Serial.printf("Object detected at %.2f cm\n", distance);
    }
    
    delay(100);  // Small delay to prevent flooding
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (SIMULATION_MODE) {
            handleSimulation();
        } else {
            handleRealSensor();
        }
    } else {
        Serial.println("WiFi Disconnected");
        delay(1000);
    }
}