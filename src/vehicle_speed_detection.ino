#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// --- Credentials ---
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

#define BOTtoken "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

// --- Hardware Pins ---
#define SENSOR_ENTRY 2
#define SENSOR_EXIT 4
#define LED_GREEN 5
#define LED_RED 18
#define BUZZER 19

// --- Settings ---
float sensorDistance = 15.0;   // Distance between sensors in cm
float speedLimit = 60.0;       // Speed limit in cm/s

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  pinMode(SENSOR_ENTRY, INPUT);
  pinMode(SENSOR_EXIT, INPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  lcd.print("Init WiFi...");

  WiFi.begin(ssid, password);

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  // Wait maximum 5 seconds for Wi-Fi
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < 5000) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();

  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("Mode: ONLINE");
    bot.sendMessage(CHAT_ID,
                    "System Online - Vehicle Speed Monitoring",
                    "");
  } 
  else {
    lcd.print("Mode: OFFLINE");
  }

  delay(2000);

  lcd.clear();
  lcd.print("Scanning...");
}

void loop() {

  // Detect vehicle at first sensor
  if (digitalRead(SENSOR_ENTRY) == LOW) {

    unsigned long startTime = millis();

    // Wait for second sensor
    while (digitalRead(SENSOR_EXIT) == HIGH) {

      // Safety timeout
      if (millis() - startTime > 2000) {
        break;
      }
    }

    unsigned long endTime = millis();

    float timeTaken =
        (endTime - startTime) / 1000.0;

    float speed =
        (timeTaken > 0) ?
        (sensorDistance / timeTaken) : 0;

    processSpeed(speed);
  }
}

void processSpeed(float speed) {

  lcd.clear();

  lcd.print("Speed: ");
  lcd.print(speed);
  lcd.print(" cm/s");

  if (speed > speedLimit) {

    // Local overspeed alert
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER, HIGH);

    lcd.setCursor(0, 1);
    lcd.print("OVERSPEED!");

    // IoT alert through Telegram
    if (WiFi.status() == WL_CONNECTED) {

      String message =
          "Overspeed Alert!\nSpeed: " +
          String(speed) +
          " cm/s";

      bot.sendMessage(CHAT_ID, message, "");
    }

  } 
  else {

    // Safe speed indication
    digitalWrite(LED_GREEN, HIGH);

    lcd.setCursor(0, 1);
    lcd.print("SAFE SPEED");
  }

  delay(3000);

  resetSystem();
}

void resetSystem() {

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(BUZZER, LOW);

  lcd.clear();
  lcd.print("Scanning...");
}
