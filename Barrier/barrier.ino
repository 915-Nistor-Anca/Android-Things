#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <time.h>


IPAddress staticIP(192, 168, 213, 208); 
IPAddress gateway(192, 168, 213, 35); 
IPAddress subnet(255, 255, 255, 0);

const char *ssid = "Dani S23";
const char *password = "testpass";

constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above
// const int trigPin = D1;
// const int echoPin = D2;
const int greenPin = D0;
const int redPin = D2;
const int buzzer = D1;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

String tag;

Servo s1;  

#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;

ESP8266WebServer server(80); // Create a web server on port 80

unsigned int servoMoveCount = 0; // Counter for servo movements
const int maxMovements = 10;
String movementTimes[maxMovements];

byte knownCard[4] = {0x13, 0x61, 0x75, 0x34};

void connectToWiFi() {
  Serial.begin(115200);
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting");
  }
  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // Print the IP address
}

// void handleRoot() {
//   String html = "<html><body><h1>Servo Move Counter</h1>";
//   html += "<p>The servo has moved ";
//   html += String(servoMoveCount);
//   html += " times.</p></body></html>";
//   server.send(200, "text/html", html);
// }

String getCurrentTime() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
  return String(buffer);
}

void handleRoot() {
  String html = "<html><head><style>";
  html += "body { font-family: 'Times New Roman', Times, serif; background-color: #f4f4f9; color: #333; margin-top: 50px; }";
  html += "h1 { color: #0066cc; text-align: center; }";
  html += "h2 { text-align: center; }";
  html += "table { margin: 0 auto; border-collapse: collapse; width: 80%; }";
  html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
  html += "th { background-color: #f2f2f2; }";
  html += "</style></head><body>";
  html += "<h1>Barrier Application</h1>";
  html += "<h2>The number and the times at which the barrier has been raised:</h2>";
  html += "<table><thead><tr><th>Move Number</th><th>Time</th></tr></thead><tbody>";

  for (unsigned int i = 0; i < servoMoveCount; i++) {
    html += "<tr><td>" + String(i + 1) + "</td><td>" + movementTimes[i] + "</td></tr>";
  }

  html += "</tbody></table></body></html>";
  
  server.send(200, "text/html", html);
}

void setup() {
  connectToWiFi();
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  s1.attach(D8);
  // pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  // pinMode(echoPin, INPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(buzzer, OUTPUT);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  server.on("/", handleRoot); // Define the handler for the root URL
  server.begin(); // Start the web server
  Serial.println("HTTP server started");
}

void printRFIDCardID(MFRC522::Uid uid) {
  Serial.print("RFID Card ID: ");
  for (byte i = 0; i < uid.size; i++) {
    Serial.print(uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(uid.uidByte[i], HEX);
  }
  Serial.println();
}

bool compareRFID(MFRC522::Uid uid, byte* knownCard) {
  if (uid.size != 4) return false; // Assuming knownCard is 4 bytes long
  for (byte i = 0; i < 4; i++) {
    if (uid.uidByte[i] != knownCard[i]) return false;
  }
  return true;
}

void loop() {
  server.handleClient(); // Handle incoming client requests
  digitalWrite(redPin, HIGH);
  

  bool rfidDetected = false;
  bool obstacleDetected = false;

  // Check for RFID card
  if (rfid.PICC_IsNewCardPresent()) {
    if (rfid.PICC_ReadCardSerial()) {
      printRFIDCardID(rfid.uid);
      if (compareRFID(rfid.uid, knownCard)) {
        rfidDetected = true;
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  // Check distance
  // digitalWrite(trigPin, LOW);
  // delayMicroseconds(2);
  // digitalWrite(trigPin, HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigPin, LOW);
  // duration = pulseIn(echoPin, HIGH);
  // distanceCm = duration * SOUND_VELOCITY / 2;

  // if (distanceCm < 5) {
  //   obstacleDetected = true;
  // }

  // If RFID detected or obstacle detected
  if (rfidDetected ){//|| obstacleDetected) {
    Serial.println("Detected");
    // Move servo up
    s1.write(0);
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);  // Turn on LED connected to GPIO9
    for(int i = 0; i < 3; i++){
      tone(buzzer, 1000);
      delay(50);
      noTone(buzzer);
      delay(50);
    }
    delay(3000); // Wait for 3 seconds
    // Move servo down
    digitalWrite(greenPin, LOW);   // Turn off LED connected to GPIO9
    tone(buzzer, 1000);
    delay(700);
    noTone(buzzer);
    s1.write(180);
    digitalWrite(redPin, HIGH);
    delay(1000);

        // Save the current time in the array
    if (servoMoveCount < maxMovements) {
      movementTimes[servoMoveCount] = getCurrentTime();
    } else {
      // Shift array elements to make space for the new time
      for (int i = 1; i < maxMovements; i++) {
        movementTimes[i - 1] = movementTimes[i];
      }
      movementTimes[maxMovements - 1] = getCurrentTime();
    }

    // Increment the servo move counter
    servoMoveCount++;
  }

  // Reset flags
  rfidDetected = false;
  obstacleDetected = false;

  delay(1000); // Add a delay to prevent rapid reading
}
