#include <SPI.h>
#include <WiFiNINA.h>
#include <FlashStorage.h>

//VARS
int led =  LED_BUILTIN;
char c;
String currentLine;
// int status = WL_IDLE_STATUS;
WiFiServer server(80);

// Flash
typedef struct {
  boolean notBlank;
  char ssid[50];
  char pwd[50];
  IPAddress ip;
 } WiFiDataType;
WiFiDataType wifiData;
FlashStorage(flash_store, WiFiDataType);

// AP
char apssid[] = "MY_SETUP_10_0_0_10";
IPAddress apIPAdress = IPAddress(10,0,0,10);

// WiFi Client
WiFiClient client;
char ssid[50];     
char pwd[50];
IPAddress clientIPAdress;

// List networks
int ginumSsid;
typedef struct {
  String ssid;
  long lsignal;
  int encrypt;
} networkData;
networkData networks[100];


// ************************* SETUP **********************************

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  start_serial(1000);
  Serial.println("Startup");
  check_HW();

  read_flash(); 
  if (wifiData.notBlank) {
    connect_to_network();
  }

// test
/*  strcpy(ssid,"espeland_ac");     
 strcpy(pwd,"espelandnett");  
 clientIPAdress = IPAddress(192,168,232,11);;
 write_flash();   */
}
// **************************** MAIN LOOP *****************************************


void loop() {
  Serial.println("Main Loop");


  if (WiFi.status() != WL_CONNECTED) {
    get_networks();
    start_AP();
  }
  Serial.print("WiFi Status:");
  Serial.println(WiFi.status());

  if (WiFi.status() == WL_CONNECTED or (WiFi.status() == WL_AP_CONNECTED)) {
    Serial.println("Starting Webserver");
    server.begin(); // start the web server on port 80
    Serial.print("WiFi Status After webserver start:");
    Serial.println(WiFi.status());
  }

  while ((WiFi.status() == WL_CONNECTED) or (WiFi.status() == WL_AP_LISTENING) or (WiFi.status() == WL_AP_CONNECTED)) {
    client = server.available();   // listen for incoming clients
    if (client) {           
      prosess_web_connection();  
    }
    // No web berowser connected
    // *** DO your normal main loop stuff here ***

  }
  // Lost connection, must try connecting to network again
  Serial.print("Lost Connection WiFi Status:");
  Serial.println(WiFi.status());
  connect_to_network();
}

// **************************** ROUTINES *****************************************

// -----------------------------------------------------
void prosess_web_connection() {
  boolean configPage;
  
  // Read one line
  currentLine = "";                 
  c=0;
  while (client.connected() && (c != '\n') ) {
    if (client.available()) {
      c = client.read();       
      if (c!='\n' and c!='\r') {
        currentLine += c;       
      }
    }    
  }
  Serial.print("Received line: ");
  Serial.println(currentLine);

  // if on AP ask for WiFi parameters only
  if (WiFi.status() == WL_AP_CONNECTED) {
    configPage = true;
  } 
  if (currentLine.startsWith("Referer: http://10.0.0.1/config")) {  // If asked for "config"
    configPage = true;
  } 
  if (currentLine.startsWith("GET /")) {
    check_for_parameters();
  } else {
    // ignore line
  }

  if (currentLine=="") {
    Serial.println("Received Last Line");
    if (configPage) {
      Serial.println("Sender configpage to browser");
      sendConfigPage();
    } else {
      Serial.println("Sender datapage to browser");
      sendDataPage();
    }
    client.stop();
  }
}

// -----------------------------------------------------
void check_for_parameters() {
  String s;

  char ssid[50];     
  char pwd[50];
  IPAddress clientIPAdress;

  s=getVerb(currentLine,"ssid="); 
  if (s!="") {
    s.toCharArray(ssid,s.length());
  }
  s=getVerb(currentLine,"pwd="); 
  if (s!="") {
    s.toCharArray(pwd,s.length());
  }
/*  s=getVerb(currentLine,"ip="); 
  if (s!="") {
  s.replace(".",","); ?
    clientIPAdress = IPAddress(s)
  } */
  // Close connection to try connecting with current data
  WiFi.end();
}

// -----------------------------------------------------
void start_serial(int timeout) {
  int ticks;
    Serial.begin(9600);
    ticks=0;
    while (!Serial and ticks<timeout) {
    ; // wait for serial port to connect. Needed for native USB port only
    delay(1);
    ticks+=1;
  }
}

// -----------------------------------------------------
void read_flash() {
  wifiData = flash_store.read();
  if (wifiData.notBlank) {
    Serial.println("Data Read from Flash");
    Serial.print("notBlank:");
    Serial.println(wifiData.notBlank);
    Serial.print("ssid:");
    Serial.println(wifiData.ssid);
    Serial.print("pwd:");
    Serial.println(wifiData.pwd);
    Serial.print("IP:");
    Serial.println(wifiData.ip);

    strcpy(ssid,wifiData.ssid);
    strcpy(pwd,wifiData.pwd);
    clientIPAdress = wifiData.ip;
  } else {
    Serial.println("No Data in Flash");
  }
}

// -----------------------------------------------------
void write_flash() {
  wifiData.notBlank= true;
  strcpy(wifiData.ssid,ssid);
  strcpy(wifiData.pwd,pwd);
  wifiData.ip=clientIPAdress;
  flash_store.write(wifiData);  
}

// -----------------------------------------------------
void connect_to_network() {

  WiFi.setTimeout(3000);

  WiFi.config(clientIPAdress);
  Serial.print("Attempting to connect to Network named: ");
  Serial.println(ssid); 
  WiFi.begin(ssid, pwd);
  delay(1000); // wait 1 seconds for connection:
}

// -----------------------------------------------------
void start_AP() {
  Serial.print("Creating access point named: ");
  Serial.println(apssid);

  WiFi.config(apIPAdress);
  
  if (WiFi.beginAP(apssid) != WL_AP_LISTENING ) {
    Serial.println("Creating access point failed");
    blink_error_forever();
  }
  delay(1000);
}

// -----------------------------------------------------
void get_networks() {
  Serial.println("Reading available networks");

  ginumSsid = WiFi.scanNetworks();
  Serial.print("Found networks: ");
  Serial.println(ginumSsid);
  for (int thisNet = 0; thisNet < ginumSsid; thisNet++) {
    networks[thisNet].ssid=  WiFi.SSID(thisNet);
    networks[thisNet].lsignal= WiFi.RSSI(thisNet);
    networks[thisNet].encrypt= WiFi.encryptionType(thisNet);
    Serial.println(networks[thisNet].ssid);
  } 
}

// -----------------------------------------------------
void blink_error_forever() {
  while(true) {
    digitalWrite(LED_BUILTIN, HIGH);   
    delay(1000);                       
    digitalWrite(LED_BUILTIN, LOW);     
    delay(1000); 
    Serial.println("Error");
  }                     
}

// -----------------------------------------------------
void check_HW() {
    if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    blink_error_forever();
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
    blink_error_forever();
  }
}

// -----------------------------------------------------
void sendDataPage() {
  String s;
  int dummyTemp = 27;
  int dummySpeed = 50;
  int dummyValve = 180;
  
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println(); 

    s="<form>";
    s=s + "Temp: " +  dummySpeed + " <br /> "; 
    s=s + "Speed  <input name=\"speed\" type=\"\" value=\""  +  dummySpeed  + "\" /><br /> "; 
    s=s + "Valve  <input name=\"valve\" type=\"\" value=\""  + dummyValve  + "\" /><br><br> "; 
    s=s + "<button type=\"submit\" value=\"Submit\">Submit</button>";
    s=s + "</form>";
    client.print(s);
  
    // The HTTP response ends with another blank line:
    client.println();
}

// -----------------------------------------------------
void sendConfigPage() {
  String s;
  
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println(); 

    s="<form>";
    s=s + "<label for=\"ssid\">SSID:</label>"; 
    s=s + "<select name=\"ssid\" id=\"ssid\">";
    s=s + "<option value=\"-1\">Please Select</option>";
    for (int thisNet = 0; thisNet < ginumSsid; thisNet++) {
      s=s + "<option value=\"" + thisNet + "\">" + networks[thisNet].ssid + "</option>";
    }
    s=s + "</select><br>";
    s=s + "Password  <input name=\"pwd\" type=\"\"text\" value=\"Enter Password\" /><br>"; 
    s=s + "IP  <input name=\"ip\" type=\"\"text\" value=\"Enter IP Address\" /><br>"; 
    s=s + "<button type=\"submit\" value=\"Submit\">Submit</button>";
    s=s + "</form>";
    client.print(s);
  
    // The HTTP response ends with another blank line:
    client.println();
}

// -----------------------------------------------------
String getVerb(String s, String  sVerb) {
  int p1;
  int p2;

  Serial.print("Leter Etter: ");
  Serial.println(sVerb);
  Serial.print("I: ");
  Serial.println(s);

  p1=s.indexOf(sVerb);
  if (p1==-1) {
    return "";
  } else {
    p2=s.indexOf("&",p1+1); 
    if (p2==-1) {
      p2=s.indexOf(" ",p1+1); // try " " if last parameter
      if (p2==-1) {
        // no value after verb
        return "";
      } else {
        return s.substring(p1 + sVerb.length(),p2);
      }
    } else {
      return s.substring(p1 + sVerb.length(),p2);
    }
  }
}

// ---------------------------################-------------

// -----------------------------------------------------
/*void ask_for_networkparameters() {
  boolean foundGET = false;
  String s ="";
  Serial.println("Ask for Net Parameters");

  while (true) {
    client = server.available();   // listen for incoming clients

    if (client) {           
      // Read noe line
      currentLine = "";                 
      c=0;
      while (client.connected() && (c != '\n') ) {             
        if (client.available()) { 
          c = client.read();       
          if (c!='\n' and c!='\r') {
            currentLine += c;       
          }
        }    
      }

      Serial.print("Read line: ");
      Serial.println(currentLine);
          
      if (currentLine.startsWith("GET /")) {
          Serial.print("Funnet: ");
          Serial.println(currentLine);
          s=getVerb(currentLine,"speed=");
          // Set parameters                  
          Serial.print("Funnet: '");
          Serial.print(s);
          Serial.println("'");
      }
      if (currentLine=="") {
          Serial.println("mottat siste linje");
        if (s!="") {
          client.stop();
          Serial.println("client disonnected");
          return;
        }  
        Serial.println("Sender data");
        send2client(1,2);
        client.stop();
      }
    }
  }
}
*/

// -----------------------------------------------------
void parseGet(String  s) {
  int p1;
  int p2;
  String sVerb = "speed=";
  int i;
  String ss;
  
//  Serial.println(s.indexOf("speed="));
  
  p1=s.indexOf(sVerb);
  if (p1 > -1 ) {
    p2=s.indexOf("&",p1+1); 
    ss=s.substring(p1 + sVerb.length(),p2);
    i=ss.toInt();
//    fanSpeed(i);
  } else {
    Serial.println("not Found" + sVerb);
  }

  sVerb = "valve=";
  p1=s.indexOf(sVerb);
  if (p1 > -1 ) {
    p2=s.indexOf(" ",p1+1); 
    ss=s.substring(p1 + sVerb.length(),p2);
    i=ss.toInt();
//    moveto_slow(i,50);
//    moveto(i);
  } else {
    Serial.println("not Found" + sVerb);
  }
}
