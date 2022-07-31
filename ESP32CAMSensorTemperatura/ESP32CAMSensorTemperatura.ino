/*
 * Conexión básica por MQTT del NodeMCU
 * por: Hugo Escalpelo
 * Fecha: 28 de julio de 2021
 * 
 * Este programa envía datos  por Internet a través del protocolo MQTT. Para poder
 * comprobar el funcionamiento de este programa, es necesario conectarse a un broker
 * y usar NodeRed para visualzar que la información se está recibiendo correctamente.
 * Este programa no requiere componentes adicionales.
 * 
 * Componente     PinESP32CAM     Estados lógicos
 * ledStatus------GPIO 33---------On=>LOW, Off=>HIGH
 * ledFlash-------GPIO 4----------On=>HIGH, Off=>LOW
 */

//Bibliotecas
#include <WiFi.h>  // Biblioteca para el control de WiFi
#include <PubSubClient.h> //Biblioteca para conexion MQTT
#include "DHT.h"  //Sensor DHT11

//Datos de WiFi
const char* ssid = "INFINITUM3759";  // Aquí debes poner el nombre de tu red
const char* password = "HQ2JC2hvFM";  // Aquí debes poner la contraseña de tu red

//Datos del broker MQTT
const char* mqtt_server = "192.168.1.89"; // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica
IPAddress server(192,168,1,89);  

#define DHTPIN 12
#define DHTTYPE DHT11


// Objetos
WiFiClient espClient; // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient); // Este objeto maneja los datos de conexion al broker

DHT dht(DHTPIN, DHTTYPE);

// Variables
long timeNow, timeLast; // Variables de control de tiempo no bloqueante
int data = 0; // Contador/temperatura
int datatemp = 0; //guardar contador de data
int wait = 5000;  // Indica la espera cada 5 segundos para envío de mensajes MQTT


//Constantes
const int LEDAm =4; //Tem alta
const int LEDAz =2; //Humedad alta
const int LEDVe =14; //Conexcion WiFi
const int statusLedPin = 33; // Para ser controlado por MQTT

const int Temp_ALTA = 23; //Umbral de alta temperatura
const int Hum_ALTA = 73;  //Umbral de alta humedad


//Variables
float t;  //Variable para la temperatura
float h;  //Variable para la humedad


// Inicialización del programa
void setup() {
  // Iniciar comunicación serial
  
  Serial.begin (115200);
  Serial.println(F("DHTxx Text!"));
   
  pinMode (LEDAm, OUTPUT);
  pinMode (LEDAz, OUTPUT);
  pinMode (LEDVe, OUTPUT);
  pinMode (statusLedPin, OUTPUT);
  
  digitalWrite (LEDAm, LOW);
  digitalWrite (LEDAz, LOW);
  digitalWrite (LEDVe, LOW);
  digitalWrite (statusLedPin, HIGH);

  Serial.println();
  Serial.println();
  Serial.print("Conectar a ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi
 
  while (WiFi.status() != WL_CONNECTED) { // Este bucle espera a que se realice la conexión
    digitalWrite (LEDVe, LOW);
    delay(500); //dado que es de suma importancia esperar a la conexión, debe usarse espera bloqueante
    digitalWrite (LEDVe, HIGH);
    Serial.print(".");  // Indicador de progreso
    delay (500);
  }
  
  // Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());

  // Si se logro la conexión, encender led
  if (WiFi.status () > 0){
  digitalWrite (LEDVe, HIGH);
  }
  
  delay (1000); // Esta espera es solo una formalidad antes de iniciar la comunicación con el broker

  // Conexión con el broker MQTT
  delay(1000);
  client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
  //client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos
  delay(1500);  // Esta espera es preventiva, espera a la conexión para no perder información

  timeLast = millis (); // Inicia el control de tiempo

  Serial.println(F("! Prueba del sensor DHT11!"));
  dht.begin();
  
}// fin del void setup ()

// Cuerpo del programa, bucle principal
void loop() {
  //Verificar siempre que haya conexión al broker
  if (!client.connected()) {
    reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
  }// fin del if (!client.connected())
  client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker

  lecturasensorDHT11();  //funcion para leer el sensor DHT11

  logica();
 }// fin del void loop ()

void lecturasensorDHT11() {
  timeNow = millis();  //Control de tiempo para bloqueantes
  
  if (timeNow - timeLast > wait){
      h = dht.readHumidity();  //Lee la temperatura grados centigrados
      t = dht.readTemperature(); //Lee la humedad en porcentaje
      
      //verificar si falllo la lectura
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Fallo la lectura del sensor DHT11!"));
    return;
  }
  Serial.println(F("Humedad :"));
  Serial.print(h);
  Serial.print(F("% Temperatura: "));
  Serial.print(t);
  Serial.println(F("oC "));
//---------------------
  
  timeNow = millis(); // Control de tiempo para esperas no bloqueantes

    data++; // Incremento a la variable para ser enviado por MQTT
    char dataStringtemp[8]; // Define una arreglo de caracteres para enviarlos por MQTT, especifica la longitud del mensaje en 8 caracteres
    char dataStringhum[8];  //Define string humedad
    dtostrf(t, 1, 2, dataStringtemp);  // Esta es una función nativa de leguaje AVR que convierte un arreglo de caracteres en una variable String
    dtostrf(h, 1, 2, dataStringhum);
    Serial.print(" Temperatura convertida ");
    Serial.print(dataStringhum);
    Serial.print(" Humedad convertida ");
    Serial.print(dataStringhum);
    
     
    client.publish("codigoIoT/G6/temp", dataStringtemp); // Esta es la función que envía los datos por MQTT, especifica el tema y el valor
      client.publish("STHumedadG6", dataStringhum); // Esta es la función que envía los datos por MQTT, especifica el tema y el valor
    data=datatemp;
    timeLast = timeNow;
 
  }// fin del if (timeNow - timeLast > wait)


}

// Funciones de usuario    --logica

// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void logica() {
  // Led indicador de alta temperatura
  if (t > Temp_ALTA) {
    digitalWrite(LEDAm, HIGH);
  } else {
    digitalWrite(LEDAm, LOW);
  }

  // Led indicador de alta humedad
  if (h>Hum_ALTA){
    digitalWrite(LEDAz, HIGH);
  }else {
    digitalWrite(LEDAz, LOW);
  }
}

  
// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      client.subscribe("esp32/output"); // Esta función realiza la suscripción al tema
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      // Espera de 5 segundos bloqueante
      delay(5000);
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect(
