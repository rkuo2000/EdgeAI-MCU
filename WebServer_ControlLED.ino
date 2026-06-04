/*

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-ameba-web-server/
 */

#include <WiFi.h>
char ssid[] = "TCFSTWIFI.ALL";  // your network SSID (Home WiFi or Smartphone Hotspot)
char pass[] = "035623116";   // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;     // Indicator of Wifi status

WiFiServer server(80);

#define LED_PIN LED_BUILTIN

String buttonState = "off";

void setup()
{
    Serial.begin(115200);        // initialize serial communication
    pinMode(LED_PIN, OUTPUT);    // set the LED pin mode

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to Network named: ");
        Serial.println(ssid);    // print the network name (SSID);

        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection:
        delay(10000);
    }
    server.begin();       // start the web server on port 80
    printWifiStatus();    // you're connected now, so print out the status
}


void loop()
{
    WiFiClient client = server.available();    // listen for incoming clients

    if (client) {                          // if you get a client,
        Serial.println("new client");      // print a message out the serial port
        String currentLine = "";           // make a String to hold incoming data from the client
        while (client.connected()) {       // loop while the client's connected
            if (client.available()) {      // if there's bytes to read from the client,
                char c = client.read();    // read a byte, then
                Serial.write(c);           // print it out the serial monitor
                if (c == '\n') {           // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");                        
                        client.println();
                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        // CSS style
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; border-radius: 16px}");
                        client.println(".button2 {background-color: #555555;} </style></head>");
                        // web header
                        client.println("<body><h1>AMB82-mini Web Server</h1>");

                        // Display current state, and ON/OFF buttons for GPIO 26  
                        client.println("<p>LED : " + buttonState + "</p>");
                        // If the output26State is off, it displays the ON button       
                        if (buttonState=="off") {
                          client.println("<p><a href=\"/H\"><button class=\"button\">ON</button></a></p>");
                        } else {
                          client.println("<p><a href=\"/L\"><button class=\"button button2\">OFF</button></a></p>");
                        }

                        // The HTTP response ends with another blank line:
                        client.println();
                        // break out of the while loop:
                        break;
                    } else {    // if you got a newline, then clear currentLine:
                        currentLine = "";
                    }
                } else if (c != '\r') {    // if you got anything else but a carriage return character,
                    currentLine += c;      // add it to the end of the currentLine
                }

                // Check to see if the client request was "GET /H" or "GET /L":
                if (currentLine.endsWith("GET /H")) {
                    digitalWrite(LED_PIN, HIGH);    // GET /H turns the LED on
                    buttonState = "on";
                }
                if (currentLine.endsWith("GET /L")) {
                    digitalWrite(LED_PIN, LOW);    // GET /L turns the LED off
                    buttonState = "off";
                }
            }
        }
        // close the connection:
        client.stop();
        Serial.println("client disconnected");
    }
}

void printWifiStatus()
{
    // print the SSID of the network you're attached to:
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
    // print where to go in a browser:
    Serial.print("To see this page in action, open a browser to http://");
    Serial.println(ip);
}
