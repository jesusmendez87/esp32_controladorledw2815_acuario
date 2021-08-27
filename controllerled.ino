#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <HTTPClient.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include "esp_system.h"


int led = 0;   // variable que guarda posición del led de la tira que encedemos/apagamos usado para control por http
int ledoff = 0;  // variable que guarda posición del led de la tira que encedemos/apagamos usado para control por http
int lednoche  = 0 ;  // variable que guarda posición del led de la tira que encedemos/apagamos usado para control por http

String estadofotoperiodo; // visualización fotoperiodo actual http

unsigned long instanteAnterior = 0;  // contador millis()
unsigned long instanteActual = millis();


void writeString(char add, String data);  // cadena para escribir eeprom
String read_String(char add);// cadena leer eeprom

#define PIN1 13 //PINES en el que conectaremos el bus de datos de las tiras
#define PIN2 12 //PINES en el que conectaremos el bus de datos de las tiras
#define FAN 27
#define oneWireBus 23  //PIN de sonda temperatura

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//Declaramos la tira de LEDS
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(144, PIN1, NEO_GRB + NEO_KHZ800);  //tira central 
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(288, PIN2, NEO_GRB + NEO_KHZ800); //tiras laterales unidas 144x2

// Datos conexión wifi. usa los tuyos!!!
const char* ssid = "xxxxxxx";
const char* password = "xxxxxxxxxxxxxxxxx";
const char* host = "esp32";

//  web server port number to 80
WebServer server(80); //WiFiServer server(80)inicia servidor con temperatura / hora / programación

// Variable to store the HTTP request
String header;

// WTD
const int loopTimeCtl = 0;
hw_timer_t *timer = NULL;
void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

//   Login page programacion 
const char* loginIndex =
  "<form name='loginForm'>"
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=4><b>ESP32 Login Page</b></font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<td>Username:</td>"
  "<td><input type='text' size=25 name='userid'><br></td>"
  "</tr>"
  "<br>"
  "<br>"
  "<tr>"
  "<td>Password:</td>"
  "<td><input type='Password' size=25 name='pwd'><br></td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
  "</tr>"
  "</table>"
  "</form>"
  "<script>"
  "function check(form)"
  "{"
  "if(form.userid.value=='root1234' && form.pwd.value=='admin1234')"
  "{"
  "window.open('/serverIndex')"
  "}"
  "else"
  "{"
  " alert('Error Password or Username')          "
  "}"
  "}"
  "</script>";


// Server Index Page
const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";






// NTP Servers:

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
byte H  ;
byte Min;
byte dia;
byte Sec;

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  H = timeinfo.tm_hour;
  Min = timeinfo.tm_min;
  Sec = timeinfo.tm_sec;
  dia = timeinfo.tm_mday;
}


String getdatos() {   //String con hora y temperatura para mostrar en servidor web
 // se añade leddia, lednoche, ledoff para comprobar el recorrido de los led
  String separador1 = "    -->";
  String separador = ":";
  String grados = "GC.";
  sensors.requestTemperatures();   //Se envía el comando para leer la temperatura
  float temp = sensors.getTempCByIndex(0); //Se obtiene la temperatura en ºC

  return String(estadofotoperiodo + separador1 + H + separador + Min + separador + Sec + separador1 + temp + grados + "leddia" + led + "lednoche" + lednoche + "ledoff" + ledoff);
}


void amanecer() {

  ledoff = 0; // usamos para poner a cero el contador de leds apagados

  estadofotoperiodo = "Sol en el Horizonte";
  unsigned long instanteActual = millis();
  if (led <= 287) {
    if ( (instanteActual - instanteAnterior) >  500 ) {
      instanteAnterior = instanteActual;

      led = led + 1;

      strip1.setPixelColor(led, strip1.Color(255, 255, 125));
      strip2.setPixelColor(led, strip2.Color(250, 250, 250));
      strip2.show();
      strip1.show();

    }
  }

}


void noche() {
  led = 0; //ponemos a cero el contador de leds del fotoperiodo anterior para usasrlo al día siguiente
  estadofotoperiodo = "Luz de luna";

  unsigned long instanteActual = millis();
  if (lednoche <= 287) {
    if ( (instanteActual - instanteAnterior) >  1000 ) {
      instanteAnterior = instanteActual;

      lednoche++;

      strip1.setPixelColor(lednoche, strip1.Color(05, 10, 10));
      strip2.setPixelColor(lednoche, strip2.Color( 05, 10, 10));
      strip2.show();
      strip1.show();

    }
  }
}


void apagado()  {

  lednoche = 0;

  estadofotoperiodo = "Noche profunda";
  unsigned long instanteActual = millis();

  if (ledoff <= 287 ) {
    if ( (instanteActual - instanteAnterior) >  500 ) {
      instanteAnterior = instanteActual;

      ledoff++;


      strip1.setPixelColor(led, strip1.Color(0, 0, 0));
      strip2.setPixelColor(led, strip2.Color(0, 0, 0));
      strip2.show();
      strip1.show();
    }
  }
}

////////////////////////////////////////////////////////////////////// fin fotoperiodo ///////////////////////////////////////////////////////////////////


void setup() {

  pinMode(FAN, OUTPUT);
  digitalWrite(FAN, HIGH);
  String encendido;
  String apagado;
  String fotoperiodo;
  String horaenc;

#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif


  //INICIAMOS TIRAS LED
  strip1.begin();
  delay(500);
  strip2.begin();

  delay(500);
  EEPROM.begin(512);
  sensors.begin();   //Se inicia el sensor de temperatura

  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Iniciando...");

  delay(1000);


  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  delay(2000);

  if (WiFi.status() != WL_CONNECTED) {
    WiFiManager wifiManager;
    wifiManager.setTimeout(180);

    if (!wifiManager.startConfigPortal("Africanos Red")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(2000);
    }
  }

  Serial.println(WiFi.localIP());
  delay(500);

  // Initialize a NTPClient to get time
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();


  server.begin();
  

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    HTTPClient http;
    http.begin("http://hostinglecturadefotoperiodo.com/fotoperiodo.php"); //Specify the URL donde rescatar hora
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code
      String fotoperiodo = http.getString();
      String encendido = fotoperiodo.substring(0, 8);
      String apagado = fotoperiodo.substring(9, 20);
      String horaenc = fotoperiodo.substring(0, 2);
      String horaapa = fotoperiodo.substring(9, 11);
      Serial.println(fotoperiodo);
      Serial.println(encendido);
      Serial.println(apagado);

      //lecura eeprom

      String recivedData;
      recivedData = read_String(10);
      delay(1000);
      if (recivedData != fotoperiodo) {
        writeString(10, fotoperiodo);
        Serial.println("Nuevo fotoperiodo!!");
      }
      else {
        Serial.println("No es necesario actualizar fotoperiodo!!");
      }
      delay(3000);

    }
    else {
      Serial.println("Error on HTTP request");
    }

    http.end(); //Free the resources
  }


  /*use mdns for host name resolution*/

  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", getdatos());
  });

  server.on("/admin", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/

      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });


  delay(2000);
  Serial.println("Start program.... ");

  //wtd

  timer = timerBegin(0, 80, true); //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 80000000, false); //set time in us 80 segundos
  timerAlarmEnable(timer); //enable interrupt


}


void loop() {

  sensors.requestTemperatures();   //Se envía el comando para leer la temperatura
  float temp = sensors.getTempCByIndex(0); //Se obtiene la temperatura en ºC

  if (temp > 26) {
    digitalWrite(FAN, LOW);
  } else {
    digitalWrite(FAN, HIGH);
  }

  timerWrite(timer, 0); //reset timer 80 segundos máximo espera
  server.handleClient();
  printLocalTime();

 

  String recivedData;
  recivedData = read_String(10);
  String horainicio = recivedData.substring(0, 2);
  String horaapagado = recivedData.substring(9, 11);
  String minutoinicio = recivedData.substring(3, 5);
  String minutoapagado = recivedData.substring(12, 14);
  int horaintinicio;
  horaintinicio = horainicio.toInt();
  int horaintapagado;
  horaintapagado = horaapagado.toInt();
  int minutointinicio;
  minutointinicio = minutoinicio.toInt();
  int minutointapagado;
  minutointapagado = minutoapagado.toInt();


  if  (((H == horaintinicio) && (Min >= minutointinicio)) || ((H == horaintapagado) && (Min <= (minutointapagado)))) { //inicio fotoperiodo amanecer
    amanecer();
  }
  else  if  ((H >  horaintinicio) && (H <  horaintapagado)) { //inicio fotoperiodo amanecer
    amanecer();
  }

  else     if  ((H == horaintapagado) && (Min > (minutointapagado))) {
    noche();
  }
  else  if  ((H == (horaintapagado + 1)) || (H == (horaintapagado + 2))) {
    noche();
  }

  else  if  ((H == (horaintapagado + 3 ) && (Min == 03) && (Sec > 58)) {
  ESP.restart();
  }
  else {
    apagado();
  }

}



//end loop


void writeString(char add, String fotoperiodo)
{
  int _size = fotoperiodo .length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, fotoperiodo[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}


String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}
