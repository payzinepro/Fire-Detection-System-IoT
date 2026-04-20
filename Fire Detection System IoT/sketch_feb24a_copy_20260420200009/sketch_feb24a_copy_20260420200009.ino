#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* ================= PINS ================= */
#define FIRE_SENSOR 27
#define SMOKE_SENSOR 34
#define BUZZER 26

/* ================= LCD ================= */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* ================= WIFI ================= */
const char* ssid = "CANALBOX-3D44";
const char* password = "MRLEE2022";

/* ================= IFTTT ================= */
String IFTTT_EVENT = "Home_Emergency_fire_alert_NOW";
String IFTTT_KEY = "hzJFKI5LD0sAbyAC7ddUaa873FCaPAuBbc9zXrme9Xu";

/* ================= VARIABLES ================= */
bool alertSent = false;
String lastState = "";  // To prevent LCD flickering

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  pinMode(FIRE_SENSOR, INPUT);
  pinMode(BUZZER, OUTPUT);

  Wire.begin(14, 15);   // 👈 ADD THIS LINE (SDA, SCL)
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());

  Serial.println("\nWiFi connected");
}

/* ================= LOOP ================= */
void loop() {

  int fire = digitalRead(FIRE_SENSOR);
  int smoke = analogRead(SMOKE_SENSOR);

  bool fireDetected = (fire == LOW);
  bool smokeDetected = (smoke > 2000);

  String currentState = "";

  /* ===== DETERMINE STATE ===== */
  if (fireDetected && smokeDetected) {
    currentState = "BOTH";
  } else if (fireDetected) {
    currentState = "FIRE";
  } else if (smokeDetected) {
    currentState = "SMOKE";
  } else {
    currentState = "SAFE";
  }

  /* ===== ACT BASED ON STATE ===== */
  if (currentState != lastState) {

    lcd.clear();

    if (currentState == "BOTH") {
      lcd.setCursor(0,0);
      lcd.print("FIRE+SMOKE!");
      lcd.setCursor(0,1);
      lcd.print("DANGER!!!");

      digitalWrite(BUZZER, HIGH);
    }

    else if (currentState == "FIRE") {
      lcd.setCursor(0,0);
      lcd.print("FIRE DETECTED");
      lcd.setCursor(0,1);
      lcd.print("Check Area!");

      digitalWrite(BUZZER, HIGH);
    }

    else if (currentState == "SMOKE") {
      lcd.setCursor(0,0);
      lcd.print("SMOKE ALERT!");
      lcd.setCursor(0,1);
      lcd.print("Val:");
      lcd.print(smoke);

      digitalWrite(BUZZER, HIGH);
    }

    else if (currentState == "SAFE") {
      lcd.setCursor(0,0);
      lcd.print("SYSTEM SAFE");
      lcd.setCursor(0,1);
      lcd.print("No Danger");

      digitalWrite(BUZZER, LOW);
    }

    lastState = currentState;
  }

  /* ===== SEND ALERT ONCE ===== */
  if ((currentState == "FIRE" || currentState == "SMOKE" || currentState == "BOTH") && !alertSent) {
    sendIFTTT(smoke, fire);
    alertSent = true;
  }

  if (currentState == "SAFE") {
    alertSent = false;
  }

  delay(500);
}

/* ================= IFTTT FUNCTION ================= */
void sendIFTTT(int smokeValue, int fireValue) {

  if (WiFi.status() == WL_CONNECTED) {

    WiFiClientSecure client;
    client.setInsecure();  // HTTPS fix

    HTTPClient http;

    String url = "https://maker.ifttt.com/trigger/" 
                 + IFTTT_EVENT 
                 + "/with/key/" 
                 + IFTTT_KEY
                 + "?value1=" + String(smokeValue)
                 + "&value2=" + String(fireValue);

    Serial.println("\n📡 Sending to IFTTT...");
    Serial.println(url);

    http.begin(client, url);

    int httpResponseCode = http.GET();

    Serial.print("IFTTT response: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200) {
      Serial.println("✅ Alert sent!");
    } else {
      Serial.println("❌ Failed to send alert");
    }

    http.end();
  }
}