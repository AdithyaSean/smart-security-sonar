#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";
const char* host = "RASPBERRY_PI_IP";
const int port = "RASPBERRY_PI_PORT";

// Ultrasonic sensor pins (for future use)
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
  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.print("Connection failed to ");
    Serial.println(host);
    return;
  }

  String request = "GET /data?value=1 HTTP/1.1\r\n";
  request += "Host: " + String(host) + "\r\n";
  request += "Connection: close\r\n\r\n";
  
  client.print(request);
  Serial.println("Sent: 1");
  digitalWrite(LED_BUILTIN, HIGH);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Client Timeout!");
      client.stop();
      return;
    }
  }
  client.stop();
  digitalWrite(LED_BUILTIN, LOW);
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