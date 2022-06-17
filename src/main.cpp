// Example testing sketch for various DHT humidity/temperature sensors written by ladyada
// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include "DHT.h"
#include <PubSubClient.h>
#include <WiFi.h>

#define DHTPIN 15 // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

#define TOPIC_PUBLISH_TEMPERATURE "topic_sensor_temperature"
#define TOPIC_PUBLISH_HUMIDITY "topic_sensor_humidity"

#define PUBLISH_DELAY 2000

#define ID_MQTT "esp32_mqtt"

// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

const char *SSID = "Betancur";         // SSID / nome da rede WI-FI que deseja se conectar
const char *PASSWORD = "betancur1088"; // Senha da rede WI-FI que deseja se conectar

// URL do broker MQTT que se deseja utilizar
const char *BROKER_MQTT = "rf09deee.us-east-1.emqx.cloud";
const char *mqtt_username = "jbetancur12";
const char *mqtt_password = "jorge900";

int BROKER_PORT = 15155; // Porta do Broker MQTT

unsigned long publishUpdate;

static char strTemperature[10] = {0};
static char strHumidity[10] = {0};

// Variáveis e objetos globais
WiFiClient espClient;         // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

/* Prototypes */
float getTemperature(void);
float getHumidity(void);

void initWiFi(void);
void initMQTT(void);
void callbackMQTT(char *topic, byte *payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void checkWiFIAndMQTT(void);

/* Takes temperature reading (DHT22 sensor) Return: temperature (degrees Celsius) */
float getTemperature(void)
{
  float data = dht.readTemperature();

  if (!(isnan(data)))
    return data;
  else
    return -99.99;
}

/* Reads relative humidity (DHT22 sensor) Return: humidity (0 - 100%) */
float getHumidity(void)
{
  float data = dht.readHumidity();

  if (!(isnan(data)))
    return data;
  else
    return -99.99;
}

/* Initializes and connects to the desired WI-FI network */
void initWiFi(void)
{
  delay(10);
  Serial.println("------Connection WI-FI------");
  Serial.print("Connecting to the Network: ");
  Serial.println(SSID);
  Serial.println("Please wait...");

  reconnectWiFi();
}

/* Initialize the MQTT connection parameters (broker address, port, and arrow callback function) */
void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Informa qual broker e porta deve ser conectado
}

/* Reconnects to the MQTT broker (if not already connected or in case of connection drop) in case of successful connection or reconnection, the thread subscription is remade. */
void reconnectMQTT(void)
{
  while (!MQTT.connected())
  {
    Serial.print("* Trying to connect to the MQTT Broker: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT, mqtt_username, mqtt_password))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
    }
    else
    {
      Serial.println("Successfully connected to the MQTT broker!");
      Serial.println("New connection attempt in 2 seconds.");
      delay(2000);
    }
  }
}

/* Checks the status of the WiFI connections and the MQTT broker.  In case of a disconnection (either one), the connection is remade. */
void checkWiFIAndMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); // if there is no connection to the Broker, the connection is remade
  reconnectWiFi();   // if there is no connection to WiFI, the connection is remade
}

void reconnectWiFi(void)
{
  // If you are already connected to the WI-FI network, nothing is done.
  // Otherwise, connection attempts are made
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Connects to the WI-FI network

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Successfully connected to the network ");
  Serial.print(SSID);
  Serial.println("IP obtained: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));

  dht.begin();

  // Initializes Wi-Fi connection
  initWiFi();

  // Initialize connection to MQTT broker
  initMQTT();
}

void loop()
{

  if ((millis() - publishUpdate) >= PUBLISH_DELAY)
  {
    publishUpdate = millis();
    // Verifica o funcionamento das conexões WiFi e ao broker MQTT
    checkWiFIAndMQTT();

    // Formata as strings a serem enviadas para o dashboard (campos texto)
    sprintf(strTemperature, "%.2fC", getTemperature());
    sprintf(strHumidity, "%.2f", getHumidity());

    // Envia as strings ao dashboard MQTT
    MQTT.publish(TOPIC_PUBLISH_TEMPERATURE, strTemperature);
    MQTT.publish(TOPIC_PUBLISH_HUMIDITY, strHumidity);
    Serial.println(strTemperature);
    // Keep-alive da comunicação com broker MQTT
    MQTT.loop();
  }
}