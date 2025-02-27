#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// Configuración de WiFi (hardcoded)
// const char* ssid = "bbrands";
// const char* password = "maihue2023.";
const char* ssid = "__";
const char* password = "majlufgarcia";

// Configuración del servidor
const char* serverUrl = "http://192.168.1.95:5000/api/registrar_nfc";
//const char* serverUrl = "http://10.0.1.226:5000/api/registrar_nfc";
//const char* serverUrl = "http://10.0.1.127:5000/api/registrar_nfc";

// Configuración del usuario
const char* USUARIO = "Felipe Majluf";
const char* COLOR = "Negro";
const char* ORIGEN = "esp32";

int lote = 1;
int succesCount = 0;
int botellasPorLote = 4; //48

// Configuración de pines
#define LED_VERDE 26   // GPIO para LED verde
#define LED_ROJO 32    // GPIO para LED rojo
#define BUZZER_PIN 27  // GPIO para Buzzer activo

// Configuración NFC por I2C
#define PN532_SDA 21   // Pin SDA para el PN532
#define PN532_SCL 22   // Pin SCL para el PN532
#define PN532_IRQ 2    // Pin IRQ para el PN532
#define PN532_RESET 3  // Pin RESET para el PN532 (No conectado en el Shield NFC)

// Parámetros de tiempo
#define TIEMPO_BASE_LED 2000    // Tiempo base para LEDs (ms)
#define TIEMPO_LED_OK TIEMPO_BASE_LED          // Tiempo encendido LED verde (ms)
#define TIEMPO_LED_ERROR (TIEMPO_BASE_LED*2)   // Tiempo encendido LED rojo (ms), el doble del verde
#define REPORT_COOLDOWN 1500   // Tiempo mínimo entre lecturas de la misma tarjeta (ms)

// Instancia del PN532
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// Variables para control de NFC
String lastReportedRfid = "";      // Último ID de RFID reportado
unsigned long lastReportTime = 0;  // Tiempo de la última lectura

// Función para mostrar estado OK (solo LED verde)
void indicarEstadoOK() {
  digitalWrite(LED_VERDE, HIGH);
  delay(TIEMPO_LED_OK);
  digitalWrite(LED_VERDE, LOW);
}

// Función para mostrar error (LED rojo + sonido de error)
void indicarEstadoError() {
  digitalWrite(LED_ROJO, HIGH);

  // Sonido grave - repetido 3 veces
  for (int j = 0; j < 3; j++) {
    // Pulso largo (grave)
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);

    // Pausa entre pulsos
    if (j < 2) {
      delay(100);
    }
  }

  // Mantener el LED rojo encendido durante TIEMPO_LED_ERROR (que es 2x el tiempo de LED verde)
  // Restamos el tiempo ya transcurrido en la generación del sonido
  int tiempoRestante = TIEMPO_LED_ERROR - (3 * 360 + 2 * 180);
  if (tiempoRestante > 0) {
    delay(tiempoRestante);
  }
  
  digitalWrite(LED_ROJO, LOW);
}

// Función para enviar ID de NFC al servidor
void enviarNFCId(String nfcId) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Crear JSON
    StaticJsonDocument<200> doc;
    doc["nfc_id"] = nfcId;
    doc["usuario"] = USUARIO;
    doc["color"] = COLOR;
    doc["origen"] = ORIGEN;
    doc["lote"] = lote;

    String requestBody;
    serializeJson(doc, requestBody);

    // Mostrar el JSON completo que se enviará
    Serial.println("---------------------------------------");
    Serial.println("Enviando JSON al servidor:");
    Serial.println(requestBody);
    Serial.println("---------------------------------------");

    // Enviar POST
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode == 200) {
      String response = http.getString();

      // Mostrar la respuesta completa del servidor
      Serial.println("Respuesta del servidor:");
      Serial.println(response);
      Serial.println("---------------------------------------");

      // Intentar parsear la respuesta JSON
      StaticJsonDocument<200> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, response);

      if (!error) {
        // Verificar el estado
        const char* estado = responseDoc["estado"];

        if (strcmp(estado, "nfc_OK") == 0) {
          succesCount++;
          Serial.print("Botella registrada. Lote: ");
          Serial.print(lote);
          Serial.print(", Botella: ");
          Serial.print(succesCount);
          Serial.print("/");
          Serial.println(botellasPorLote);
          
          if (succesCount >= botellasPorLote) {
            lote++;
            succesCount = 0;
            Serial.println("¡LOTE COMPLETADO! Avanzando al siguiente lote.");
          }
          
          indicarEstadoOK();  // Solo LED verde por tiempo base
        } else {
          Serial.println("Respuesta: Registro Duplicado");
          indicarEstadoError();  // LED rojo + sonido de error, tiempo doble
        }
      } else {
        Serial.println("Respuesta: Error de formato");
        indicarEstadoError();
      }
    } else {
      Serial.print("Error en HTTP Request: ");
      Serial.println(httpResponseCode);
      indicarEstadoError();
    }

    http.end();
  } else {
    Serial.println("Error: WiFi desconectado");
    // Parpadear LED rojo para indicar error de conexión
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_ROJO, HIGH);
      delay(200);
      digitalWrite(LED_ROJO, LOW);
      delay(200);
    }
  }
}

// Función para leer tarjeta NFC
void leerTarjeta() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer para almacenar el UID
  uint8_t uidLength;                        // Longitud del UID (4 o 7 bytes según tipo de tarjeta ISO14443A)

  // Espera por tarjetas tipo ISO14443A (Mifare, etc.)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  if (success) {    
    String result = "";
    for (int szPos = 0; szPos < uidLength; szPos++) {
      if (uid[szPos] <= 0xF) {
        result += "0";
      }
      result += String(uid[szPos] & 0xff, HEX);
    }

    // Sonido de lectura de tarjeta - un brrr corto
    for (int i = 0; i < 2; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(40);
      digitalWrite(BUZZER_PIN, LOW);
      delay(20);
    }

    unsigned long currentTime = millis();
    // Verifica si es una tarjeta nueva o si ha pasado suficiente tiempo desde el último reporte
    if (result != lastReportedRfid || (currentTime - lastReportTime >= REPORT_COOLDOWN)) {
      lastReportedRfid = result;
      lastReportTime = currentTime;

      // Enviar al servidor
      enviarNFCId(result);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configurar pines
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Estado inicial
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);

  // Test de LEDs al inicio
  digitalWrite(LED_VERDE, HIGH);
  delay(300);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, HIGH);
  delay(300);
  digitalWrite(LED_ROJO, LOW);

  // Test inicial de buzzer - un solo beep corto
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50);
  digitalWrite(BUZZER_PIN, LOW);

  // Iniciar comunicación I2C
  Wire.begin(PN532_SDA, PN532_SCL);

  // Iniciar PN532
  nfc.begin();

  // Verificar conexión con el PN532
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Error: Hardware NFC no encontrado");
    digitalWrite(LED_ROJO, HIGH);  // Mantener LED rojo encendido para indicar error
    while (1)
      ;  // Detener ejecución
  }

  // Configurar el PN532 para leer tarjetas RFID
  nfc.SAMConfig();

  // Conectar a WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    // Parpadeo de LED para indicar intento de conexión
    digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));
  }

  digitalWrite(LED_ROJO, LOW);  // Apagar LED después de conectar

  // Indicar conexión exitosa
  digitalWrite(LED_VERDE, HIGH);
  delay(500);
  digitalWrite(LED_VERDE, LOW);

  Serial.println("Iniciándose, hardware OK");
  Serial.print("Lote actual: ");
  Serial.println(lote);
  Serial.print("Botellas por lote: ");
  Serial.println(botellasPorLote);
}

void loop() {
  // Verificar conexión WiFi y reconectar si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);

    // Esperar a la reconexión con timeout
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
      delay(500);
      digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));  // Parpadear LED durante la reconexión
      timeout++;
    }

    digitalWrite(LED_ROJO, LOW);  // Apagar LED después del intento
  }

  // Leer tarjeta NFC
  leerTarjeta();

  // Pequeña pausa para ahorrar recursos
  delay(50);
}