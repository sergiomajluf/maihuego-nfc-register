#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Preferences.h>  // Biblioteca para almacenamiento persistente

// Incluir archivo de configuración
#include "config.h"

// Variables globales
int succesCount = 0;

// Instancia del PN532
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// Instancia de Preferences para almacenamiento persistente
Preferences preferences;

// Variables para control de NFC
String lastReportedRfid = "";      // Último ID de RFID reportado
unsigned long lastReportTime = 0;  // Tiempo de la última lectura

// Secuencia especial para indicar guardado en memoria
void indicarGuardado() {
  // Patrón alternante de LEDs verde y rojo con pitidos
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);

    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, HIGH);
    delay(100);
  }
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
}

// Secuencia especial para indicar reinicio a valores de fábrica
void indicarReinicioFabrica() {
  // Tres secuencias completas de encendido de ambos LEDs
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }
}

// Función para guardar el estado actual en la memoria persistente
void guardarEstado() {
  preferences.begin("nfc-system", false);  // Abrir el espacio de nombres
  preferences.putInt("lote", lote);
  preferences.putInt("succesCount", succesCount);
  preferences.end();  // Cerrar el espacio de nombres

  Serial.println("---------------------------------------------");
  Serial.println("ESTADO GUARDADO EN MEMORIA PERSISTENTE");
  Serial.print("Lote: ");
  Serial.println(lote);
  Serial.print("Contador de botellas: ");
  Serial.println(succesCount);
  Serial.println("---------------------------------------------");

  indicarGuardado();
}

// Función para cargar el estado desde la memoria persistente
void cargarEstado() {
  preferences.begin("nfc-system", false);  // Abrir el espacio de nombres

  // Cargar valores con valores predeterminados si no existen
  lote = preferences.getInt("lote", 1);
  succesCount = preferences.getInt("succesCount", 0);

  preferences.end();  // Cerrar el espacio de nombres

  Serial.println("---------------------------------------------");
  Serial.println("ESTADO CARGADO DESDE MEMORIA PERSISTENTE");
  Serial.print("Lote: ");
  Serial.println(lote);
  Serial.print("Contador de botellas: ");
  Serial.println(succesCount);
  Serial.println("---------------------------------------------");
}

// Función para restablecer valores de fábrica
void restablecerValoresFabrica() {
  preferences.begin("nfc-system", false);
  preferences.clear();  // Borrar todos los valores almacenados
  preferences.end();

  // Restablecer valores a los predeterminados en config.h
  lote = 1;
  succesCount = 0;

  Serial.println("---------------------------------------------");
  Serial.println("VALORES RESTABLECIDOS A CONFIGURACIÓN DE FÁBRICA");
  Serial.println("Lote: 1");
  Serial.println("Contador de botellas: 0");
  Serial.println("---------------------------------------------");

  indicarReinicioFabrica();
}

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
    delay(240);
    digitalWrite(BUZZER_PIN, LOW);

    // Pausa entre pulsos
    if (j < 2) {
      delay(120);
    }
  }

  // Mantener el LED rojo encendido durante TIEMPO_LED_ERROR (que es 2x el tiempo de LED verde)
  // Restamos el tiempo ya transcurrido en la generación del sonido
  int tiempoRestante = TIEMPO_LED_ERROR - (3 * 240 + 2 * 120);
  if (tiempoRestante > 0) {
    delay(tiempoRestante);
  }

  digitalWrite(LED_ROJO, LOW);
}

// Función para enviar ID de NFC al servidor
void enviarNFCId(String nfcId) {
  // Verificar si es una tarjeta maestra
  if (nfcId.equalsIgnoreCase(MASTER_CARD_SAVE)) {
    Serial.println("¡TARJETA MAESTRA DETECTADA! Guardando estado y reiniciando...");
    guardarEstado();
    delay(1000);  // Esperar un segundo antes de reiniciar
    ESP.restart();
    return;
  }

  if (nfcId.equalsIgnoreCase(MASTER_CARD_RESET)) {
    Serial.println("¡TARJETA MAESTRA DE REINICIO DETECTADA! Restableciendo valores de fábrica...");
    restablecerValoresFabrica();
    delay(1000);  // Esperar un segundo antes de reiniciar
    ESP.restart();
    return;
  }

  // Si no es una tarjeta maestra, continuar con el proceso normal
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Configurar tiempos de espera más largos
    http.setConnectTimeout(10000);  // 10 segundos para conexión (por defecto 5s)
    http.setTimeout(15000);         // 15 segundos para recibir respuesta (por defecto 5s)

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

      // Imprimir el ID de la tarjeta escaneada
      Serial.print("Tarjeta escaneada - ID: ");
      Serial.println(result);

      // Enviar al servidor
      enviarNFCId(result);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n---------------------------------------------");
  Serial.println("Iniciando sistema NFC para etiquetado de botellas");
  Serial.println("---------------------------------------------");

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

  // Cargar el estado guardado desde la memoria persistente
  cargarEstado();

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
  Serial.println("Lector NFC inicializado correctamente");

  // Mostrar parámetros de configuración
  Serial.println("\nConfiguración cargada:");
  Serial.print("WiFi SSID: ");
  Serial.println(ssid);
  Serial.print("Servidor: ");
  Serial.println(serverUrl);
  Serial.print("Usuario: ");
  Serial.println(USUARIO);
  Serial.print("Color: ");
  Serial.println(COLOR);
  Serial.print("Origen: ");
  Serial.println(ORIGEN);
  Serial.print("Lote actual: ");
  Serial.println(lote);
  Serial.print("Contador de botellas: ");
  Serial.println(succesCount);
  Serial.print("Botellas por lote: ");
  Serial.println(botellasPorLote);
  Serial.println("---------------------------------------------");

  // Información sobre tarjetas maestras
  Serial.println("TARJETAS MAESTRAS CONFIGURADAS:");
  Serial.print("Guardar estado: ");
  Serial.println(MASTER_CARD_SAVE);
  Serial.print("Reiniciar a valores de fábrica: ");
  Serial.println(MASTER_CARD_RESET);
  Serial.println("---------------------------------------------");

  // Conectar a WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Parpadeo de LED para indicar intento de conexión
    digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));
  }

  digitalWrite(LED_ROJO, LOW);  // Apagar LED después de conectar

  Serial.println("\nConectado a WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Indicar conexión exitosa
  digitalWrite(LED_VERDE, HIGH);
  delay(500);
  digitalWrite(LED_VERDE, LOW);

  Serial.println("\nSistema listo para escanear tags NFC");
  Serial.println("---------------------------------------------");
}

void loop() {
  // Verificar conexión WiFi y reconectar si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi desconectado. Reconectando...");
    WiFi.begin(ssid, password);

    // Esperar a la reconexión con timeout
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
      delay(500);
      Serial.print(".");
      digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));  // Parpadear LED durante la reconexión
      timeout++;
    }

    digitalWrite(LED_ROJO, LOW);  // Apagar LED después del intento

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi reconectado");
    } else {
      Serial.println("\nFallo en la reconexión WiFi");
    }
  }

  // Leer tarjeta NFC
  leerTarjeta();

  // Pequeña pausa para ahorrar recursos
  delay(50);
}