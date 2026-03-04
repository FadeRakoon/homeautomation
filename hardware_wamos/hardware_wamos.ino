
#include <SoftwareSerial.h>
// IMPORT ALL REQUIRED LIBRARIES

#include <math.h>
#include <stdio.h>
#include <ArduinoJson.h>
   
//**********ENTER IP ADDRESS OF SERVER******************//

#define HOST_IP     "localhost"       // REPLACE WITH IP ADDRESS OF SERVER ( IP ADDRESS OF COMPUTER THE BACKEND IS RUNNING ON) 
#define HOST_PORT   "8080"            // REPLACE WITH SERVER PORT (BACKEND FLASK API PORT)
#define route       "api/update"      // LEAVE UNCHANGED 
#define idNumber    "620171573"       // REPLACE WITH YOUR ID NUMBER 

// WIFI CREDENTIALS
#define SSID        "YOUR WIFI"      // "REPLACE WITH YOUR WIFI's SSID"   
#define password    "YOUR PASSWORD"  // "REPLACE WITH YOUR WiFi's PASSWORD" 

#define stay        100
 
//**********PIN DEFINITIONS******************//

#define espRX         10
#define espTX         11
#define espTimeout_ms 300
//ultrasonic sensor pins
#define TRIG_PIN 4
#define ECHO_PIN 3

// Tank geometry and measurement constants (inches)
#define SENSOR_HEIGHT_IN 94.5
#define MAX_WATER_HEIGHT_IN 77.763
#define TANK_DIAMETER_IN 61.5
#define GALLON_CUBIC_IN 231.0

// HC-SR04 timing
#define PULSE_TIMEOUT_US 30000UL

// Invalid measurement sentinel
#define INVALID_READING -1.0

 
 
/* Declare your functions below */
double readRadarInches();
double clampDouble(double value, double lower, double upper);
double computeWaterHeight(double radarIn);
double computeReserveGallons(double waterHeightIn);
double computePercentage(double waterHeightIn);
 

SoftwareSerial esp(espRX, espTX); 
 

void setup(){

  Serial.begin(115200); 
  // Configure GPIO pins here
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

 

  espInit();  
 
}

void loop(){ 
   
  // send updates with schema ‘{"id": "student_id", "type": "ultrasonic", "radar": 0, "waterheight": 0, "reserve": 0, "percentage": 0}’
  double radar = readRadarInches();

  if (radar == INVALID_READING) {
    Serial.println("Radar read failed (timeout/invalid). Skipping update.");
    delay(1000);
    return;
  }

  double waterHeight = computeWaterHeight(radar);
  double reserve = computeReserveGallons(waterHeight);
  double percentage = computePercentage(waterHeight);
  bool isOverflow = (waterHeight > MAX_WATER_HEIGHT_IN);

  Serial.print("radar(in): ");
  Serial.print(radar, 2);
  Serial.print(" | waterheight(in): ");
  Serial.print(waterHeight, 2);
  Serial.print(" | reserve(gal): ");
  Serial.print(reserve, 2);
  Serial.print(" | percentage(%): ");
  Serial.print(percentage, 2);
  Serial.print(" | overflow: ");
  Serial.println(isOverflow ? "YES" : "NO");

  StaticJsonDocument<256> doc;
  char message[290] = {0};

  doc["id"] = idNumber;
  doc["type"] = "ultrasonic";
  doc["radar"] = radar;
  doc["waterheight"] = waterHeight;
  doc["reserve"] = reserve;
  doc["percentage"] = percentage;

  serializeJson(doc, message, sizeof(message));

  espUpdate(message);



  delay(1000);  
}

 
void espSend(char command[] ){   
    esp.print(command); // send the read character to the esp    
    while(esp.available()){ Serial.println(esp.readString());}    
}


void espUpdate(char mssg[]){ 
    char espCommandString[50] = {0};
    char post[290]            = {0};

    snprintf(espCommandString, sizeof(espCommandString),"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",HOST_IP,HOST_PORT); 
    espSend(espCommandString);    //starts the connection to the server
    delay(stay);

    // GET REQUEST 
    // snprintf(post,sizeof(post),"GET /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n",route,HOST_IP,strlen(mssg),mssg);

    // POST REQUEST
    snprintf(post,sizeof(post),"POST /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n",route,HOST_IP,strlen(mssg),mssg);
  
    snprintf(espCommandString, sizeof(espCommandString),"AT+CIPSEND=%d\r\n", strlen(post));
    espSend(espCommandString);    //sends post length
    delay(stay);
    Serial.println(post);
    espSend(post);                //sends POST request with the parameters 
    delay(stay);
    espSend("AT+CIPCLOSE\r\n");   //closes server connection
   }

void espInit(){
    char connection[100] = {0};
    esp.begin(115200); 
    Serial.println("Initiallizing");
    esp.println("AT"); 
    delay(1000);
    esp.println("AT+CWMODE=1");
    delay(1000);
    while(esp.available()){ Serial.println(esp.readString());} 

    snprintf(connection, sizeof(connection),"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,password);
    esp.print(connection);

    delay(3000);  //gives ESP some time to get IP

    if(esp.available()){   Serial.print(esp.readString());}
    
    Serial.println("\nFinish Initializing");    
   
}

//***** Design and implement all util functions below ******
double readRadarInches() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, PULSE_TIMEOUT_US);
  if (duration == 0) {
    return INVALID_READING;
  }

  // Speed of sound conversion for HC-SR04:
  // distance(cm) = duration(us) * 0.0343 / 2
  // then convert to inches by dividing by 2.54
  double distanceCm = (duration * 0.0343) / 2.0;
  double distanceIn = distanceCm / 2.54;

  // Basic sanity bounds for this deployment
  if (distanceIn < 0.5 || distanceIn > SENSOR_HEIGHT_IN + 10.0) {
    return INVALID_READING;
  }

  return distanceIn;
}

double clampDouble(double value, double lower, double upper) {
  if (value < lower) return lower;
  if (value > upper) return upper;
  return value;
}

double computeWaterHeight(double radarIn) {
  // water height from tank base = sensor height - radar distance
  double waterHeightIn = SENSOR_HEIGHT_IN - radarIn;
  if (waterHeightIn < 0.0) {
    waterHeightIn = 0.0;
  }
  return waterHeightIn;
}

double computeReserveGallons(double waterHeightIn) {
  double radiusIn = TANK_DIAMETER_IN / 2.0;
  double volumeIn3 = M_PI * radiusIn * radiusIn * waterHeightIn;
  return volumeIn3 / GALLON_CUBIC_IN;
}

double computePercentage(double waterHeightIn) {
  double raw = (waterHeightIn / MAX_WATER_HEIGHT_IN) * 100.0;
  return clampDouble(raw, 0.0, 100.0);
}


