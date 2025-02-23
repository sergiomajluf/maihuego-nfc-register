#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <MFRC522.h>
#include <SPI.h>

// Configuración de WiFi
const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";

// Configuración del servidor
const char* serverUrl = "http://192.168.1.XXX:5000/api/registrar_nfc";  // Cambiar por IP del servidor

// Configuración del usuario
const char* USUARIO = "ESP32_1";  // Nombre predefinido del ESP32
const char* COLOR = "Rojo";       // Color predefinido

// Configuración de pines
#define LED_VERDE 25    // GPIO para LED verde
#define LED_ROJO 26     // GPIO para LED rojo
#define RST_PIN 22      // Pin RST del RC522
#define SS_PIN 21       // Pin SS del RC522

// Instancia MFRC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
    Serial.begin(115200);
    
    // Configurar LEDs
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_ROJO, OUTPUT);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, LOW);

    // Iniciar SPI
    SPI.begin();
    
    // Iniciar MFRC522
    mfrc522.PCD_Init();
    
    // Conectar a WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConectado a WiFi");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

String bytesToHex(byte* buffer, byte bufferSize) {
    String hex = "";
    for (byte i = 0; i < bufferSize; i++) {
        if (buffer[i] < 0x10) {
            hex += "0";
        }
        hex += String(buffer[i], HEX);
    }
    return hex;
}

void indicarEstado(bool esOK) {
    if (esOK) {
        digitalWrite(LED_VERDE, HIGH);
        delay(2000);
        digitalWrite(LED_VERDE, LOW);
    } else {
        digitalWrite(LED_ROJO, HIGH);
        delay(8000);
        digitalWrite(LED_ROJO, LOW);
    }
}

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
        
        String requestBody;
        serializeJson(doc, requestBody);
        
        // Enviar POST
        int httpResponseCode = http.POST(requestBody);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            
            // Parsear respuesta
            StaticJsonDocument<200> responseDoc;
            DeserializationError error = deserializeJson(responseDoc, response);
            
            if (!error) {
                const char* estado = responseDoc["estado"];
                indicarEstado(strcmp(estado, "nfc_OK") == 0);
            }
        } else {
            Serial.print("Error en HTTP Request: ");
            Serial.println(httpResponseCode);
            // Indicar error con LED rojo
            indicarEstado(false);
        }
        
        http.end();
    }
}

void loop() {
    // Verificar si hay una nueva tarjeta
    if (!mfrc522.PICC_IsNewCardPresent()) {
        delay(50);
        return;
    }
    
    // Leer la tarjeta
    if (!mfrc522.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }
    
    // Obtener ID de la tarjeta
    String nfcId = bytesToHex(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.print("NFC ID: ");
    Serial.println(nfcId);
    
    // Enviar al servidor
    enviarNFCId(nfcId);
    
    // Detener PICC y reiniciar RC522
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    // Esperar antes de la siguiente lectura
    delay(1000);
}