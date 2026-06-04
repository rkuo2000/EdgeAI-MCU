/*
 Example guide:
 https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/ameba_pro2/amb82-mini/Example_Guides/WiFi/Simple%20Http%20Server%20to%20Receive%20Data.html
*/

#include <WiFi.h>

#include "DHT.h"
// The digital pin we're connected to.
#define DHTPIN 8

// Uncomment whatever type you're using!
#define DHTTYPE DHT11    // DHT 11
// #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// #define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

char ssid[] = "TCFSTWIFI.ALL";    // your network SSID (name)
char pass[] = "035623116";        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;     // Indicator of Wifi status

WiFiServer server(80);
void setup()
{
    // Initialize serial and wait for port to open:
    Serial.begin(115200);
    while (!Serial) {
        ;    // wait for serial port to connect. Needed for native USB port only
    }

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    dht.begin();

    // server.setBlocking();
    server.begin();
    // you're connected now, so print out the status:
    printWifiStatus();
}

void loop()
{ 
    // listen for incoming clients
    WiFiClient client = server.available();
    if (client) {
        // an http request ends with a blank line
        Serial.println("new client");
        String currentLine = "";
        Serial.println("--- Start of Client Request ---");    // Added marker
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        float t = dht.readTemperature(); 
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
                // if you've gotten to the end of the line (received a newline
                // character) and the line is blank, the http request has ended,
                // so you can send a reply
                if (c == '\n') {
                    // Serial.print("Line received: "); // Indicate a new line
                    // Serial.println(currentLine);
                    if (currentLine.length() == 0) {
                        Serial.println("Blank line detected - Sending HTML response.");
                        String htmlContent = "";
                        htmlContent += "Temperature: " + String(t) + "C <br />";
                        htmlContent += "Humidity:    " + String(h) + "%  <br />";                       
                        htmlContent += "</body></html>";
                        String response = "HTTP/1.1 200 OK\r\n";
                        response += "Content-Type: text/html\r\n";
                        response += "Connection: close\r\n";
                        response += "Content-Length: " + String(htmlContent.length()) + "\r\n";    // Calculate and set Content-Length
                        response += "\r\n";
                        response += htmlContent;

                        client.print(response);
                        Serial.println("--- HTML Response Sent ---");
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        Serial.println("--- End of Client Request (Connection Closed) ---");    // Added marker
        // give the web browser time to receive the data
        delay(1);

        // close the connection:
        // client.stop(); // remove this line since destructor will be called automatically
        Serial.println("client disconnected");
    }
    // continue with user code in WiFi server non-blocking mode
    Serial.println("User code implementing here...");
    delay(5000);
}

void printWifiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.println();
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}
