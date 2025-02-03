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
bool SIMULATION_MODE = true; // Set to false when using real sensor

// Simulation variables
unsigned long simulationStartTime = 0;
const unsigned long SIMULATION_DURATION = 5000; // 5 seconds
bool simulationActive = false;

// Previous state
bool previousState = false;

// Modify constants
const int MAX_RETRIES = 3;  // Maximum HTTP request retries

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

    // Check initial operation mode
    SIMULATION_MODE = checkOperationMode();
    Serial.println("Operation mode: " + String(SIMULATION_MODE ? "Running in simulation mode" : "Running in sensor mode"));
}

bool checkOperationMode() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        WiFiClient client;
        
        String url = String("http://") + host + ":" + String(port) + "/mode";
        
        http.begin(client, url);
        int httpResponseCode = http.GET();
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            http.end();
            return response.indexOf("simulation") > 0;
        }
        
        http.end();
    }
    return true; // Default to simulation mode if can't connect
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
    if (WiFi.status() == WL_CONNECTED) {
        if (SIMULATION_MODE) {
            handleSimulation();
        } else {
            handleRealSensor();
        }
    } else {
        ensureWiFiConnection();
    }
}