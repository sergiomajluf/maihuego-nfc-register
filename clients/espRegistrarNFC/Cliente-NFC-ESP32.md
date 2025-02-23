# Cliente NFC con ESP32

Este cliente utiliza un ESP32 con un lector RFID-RC522 para registrar etiquetas NFC en el sistema.

## Requisitos de Hardware

- ESP32 DevKit
- Módulo RFID-RC522
- LED Verde
- LED Rojo
- Resistencias 220Ω (2x)
- Cables Dupont

## Conexiones

### RFID-RC522 a ESP32:
```
RC522 -> ESP32
3.3V  -> 3.3V
RST   -> GPIO22
GND   -> GND
MISO  -> GPIO19
MOSI  -> GPIO23
SCK   -> GPIO18
SDA   -> GPIO21
```

### LEDs a ESP32:
```
LED Verde -> GPIO25 (con resistencia 220Ω)
LED Rojo  -> GPIO26 (con resistencia 220Ω)
```

## Configuración del Software

1. Instalar Arduino IDE
2. Agregar soporte para ESP32:
   - Abrir Arduino IDE
   - Ir a Archivo -> Preferencias
   - En "URLs adicionales de Gestor de Placas" agregar:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Ir a Herramientas -> Placa -> Gestor de Placas
   - Buscar "esp32" e instalar el paquete

3. Instalar bibliotecas requeridas:
   - Sketch -> Incluir Biblioteca -> Gestor de bibliotecas
   - Instalar:
     - MFRC522
     - ArduinoJson
     - WiFi

4. Configurar el código:
   - Abrir el archivo del cliente ESP32
   - Modificar las siguientes variables:
     ```cpp
     const char* ssid = "TU_SSID";              // Nombre de tu red WiFi
     const char* password = "TU_PASSWORD";       // Contraseña de tu red WiFi
     const char* serverUrl = "http://192.168.1.XXX:5000/api/registrar_nfc";  // IP de tu servidor
     const char* USUARIO = "ESP32_1";           // Nombre único para este dispositivo
     const char* COLOR = "Rojo";                // Color asignado a este dispositivo
     ```

## Carga del Programa

1. Conectar el ESP32 vía USB
2. En Arduino IDE:
   - Seleccionar la placa correcta (ESP32 Dev Module)
   - Seleccionar el puerto COM correcto
   - Velocidad de subida: 115200
3. Compilar y cargar el programa

## Verificación

1. Abrir el Monitor Serial (115200 baud)
2. Verificar:
   - Conexión WiFi exitosa
   - IP asignada
   - Inicialización correcta del RC522

## Uso

1. Acercar una tarjeta NFC al lector
2. Observar los LEDs:
   - Verde (2s): registro exitoso
   - Rojo (8s): error o tag duplicado
3. Verificar en el Monitor Serial los mensajes de estado

## Solución de Problemas

- **No se conecta al WiFi**: Verificar credenciales y señal
- **No lee tarjetas**: Revisar conexiones del RC522
- **Error de servidor**: Verificar IP y que el servidor esté activo
- **LEDs no funcionan**: Revisar conexiones y resistencias

## Mantenimiento

- Mantener limpio el lector RFID
- Verificar periódicamente las conexiones
- Monitorear el funcionamiento mediante el Monitor Serial