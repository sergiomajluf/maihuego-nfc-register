# Cliente NFC con Raspberry Pico W

Este cliente utiliza un Raspberry Pico W con un módulo PN532 para registrar etiquetas NFC en el sistema.

## Requisitos de Hardware

- Raspberry Pico W
- Módulo NFC PN532
- LED Verde
- LED Rojo
- Resistencias 220Ω (2x)
- Cables Dupont
- Cable micro USB

## Conexiones

### PN532 a Pico W:
```
PN532 -> Pico W
VCC   -> 3.3V (Pin 36)
GND   -> GND (Pin 38)
SDA   -> GPIO2 (Pin 4)
SCL   -> GPIO3 (Pin 5)
```

### LEDs a Pico W:
```
LED Verde -> GPIO15 (con resistencia 220Ω)
LED Rojo  -> GPIO14 (con resistencia 220Ω)
```

## Configuración del Software

1. Instalar MicroPython en Pico W:
   - Descargar el firmware UF2 de MicroPython para Pico W desde:
     ```
     https://micropython.org/download/rp2-pico-w/
     ```
   - Mantener presionado el botón BOOTSEL mientras conectas la Pico
   - Copiar el archivo UF2 a la unidad RPI-RP2 que aparece

2. Instalar Thonny IDE:
   - Descargar e instalar desde: https://thonny.org/
   - Abrir Thonny
   - Ir a Herramientas -> Opciones -> Intérprete
   - Seleccionar "MicroPython (Raspberry Pi Pico)"

3. Cargar los archivos:
   - main.py
   - pn532.py

4. Configurar variables en main.py:
   ```python
   SSID = "TU_SSID"                    # Nombre de tu red WiFi
   PASSWORD = "TU_PASSWORD"             # Contraseña de tu red WiFi
   SERVER_URL = "http://192.168.1.XXX:5000/api/registrar_nfc"  # IP de tu servidor
   DEVICE_NAME = "PICO_1"              # Nombre único para este dispositivo
   BOTTLE_COLOR = "Verde"              # Color asignado a este dispositivo
   ```

## Verificación Inicial

1. Conectar la Pico W vía USB
2. En Thonny:
   - Abrir Shell
   - Verificar mensajes de inicio:
     - Conexión WiFi
     - Inicialización PN532
     - IP asignada

## Uso

1. El dispositivo iniciará automáticamente al energizarse
2. Acercar una tarjeta NFC al lector
3. Observar los LEDs:
   - Verde (2s): registro exitoso
   - Rojo (8s): error o tag duplicado
4. Monitorear en Thonny Shell para mensajes de estado

## Solución de Problemas

### Problemas comunes:

1. No se conecta al WiFi:
   - Verificar credenciales en main.py
   - Confirmar que la red esté disponible
   - Revisar que sea una Pico W (no una Pico normal)

2. No detecta el PN532:
   - Verificar conexiones I2C
   - Comprobar alimentación del módulo
   - Revisar que los pines en el código coincidan con las conexiones

3. LEDs no funcionan:
   - Verificar polaridad
   - Comprobar resistencias
   - Confirmar números de GPIO en el código

4. Errores de comunicación con servidor:
   - Verificar IP del servidor
   - Confirmar que el servidor esté activo
   - Revisar conectividad en la red local

## Mantenimiento

- Actualizar el firmware de MicroPython periódicamente
- Mantener limpio el lector NFC
- Verificar conexiones regularmente
- Monitorear el consumo de memoria (usando Thonny)

## Desarrollo y Depuración

Para desarrollo y pruebas:
1. Mantener la Pico conectada vía USB
2. Usar Thonny para monitoreo en tiempo real
3. Utilizar print() para debug
4. Verificar excepciones en el Shell

## Notas Importantes

- La Pico W debe estar correctamente alimentada (5V vía USB o VIN)
- El módulo PN532 debe usar 3.3V (¡no 5V!)
- Los LEDs deben tener sus resistencias limitadoras
- El código maneja reconexión WiFi automática
- Los archivos se ejecutan automáticamente al iniciar