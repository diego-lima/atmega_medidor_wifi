#include <ESP8266WiFi.h>
#include<inttypes.h>
#include<SPI.h>

// Wi-Fi Settings
const char* ssid = "motog"; // your wireless network name (SSID)
const char* password = "bialinda"; // your Wi-Fi network password

const int postingInterval = 10000; // post data every 20 seconds

int8_t storage [4];
uint8_t buff;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
    // Measure Signal Strength (RSSI) of Wi-Fi connection
  long rssi = WiFi.RSSI();
    
  Serial.print("RSSI: ");
  Serial.println(rssi);

  int8_t potencia = int(rssi);

  storage [4] = 0;
  digitalWrite(SS, LOW);
  memcpy(storage, &potencia, 4);

  SPI.transfer(storage, sizeof storage );
  digitalWrite(SS, HIGH);

  // wait and then post again
  delay(1000);
}
