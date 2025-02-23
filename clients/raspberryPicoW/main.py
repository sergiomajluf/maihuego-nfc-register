# main.py para Raspberry Pico W
from machine import Pin, I2C
import network
import urequests
import json
import time
from pn532 import PN532_I2C

# Configuración de WiFi
SSID = "TU_SSID"
PASSWORD = "TU_PASSWORD"
SERVER_URL = "http://192.168.1.XXX:5000/api/registrar_nfc"

# Configuración del dispositivo
DEVICE_NAME = "PICO_1"
BOTTLE_COLOR = "Verde"

# Configuración de pines
LED_GREEN = Pin(15, Pin.OUT)  # GPIO15 para LED verde
LED_RED = Pin(14, Pin.OUT)    # GPIO14 para LED rojo

# Configuración I2C para PN532
i2c = I2C(1, scl=Pin(3), sda=Pin(2))  # Usa I2C1: GPIO3 (SCL) y GPIO2 (SDA)
nfc = PN532_I2C(i2c)

def connect_wifi():
    """Conecta a la red WiFi"""
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print('Conectando a WiFi...')
        wlan.connect(SSID, PASSWORD)
        while not wlan.isconnected():
            time.sleep(1)
    print('Conexión WiFi establecida')
    print('IP:', wlan.ifconfig()[0])

def indicate_status(is_ok):
    """Indica el estado mediante LEDs"""
    if is_ok:
        LED_GREEN.on()
        time.sleep(2)
        LED_GREEN.off()
    else:
        LED_RED.on()
        time.sleep(8)
        LED_RED.off()

def send_nfc_data(nfc_id):
    """Envía datos al servidor"""
    try:
        data = {
            "nfc_id": nfc_id,
            "usuario": DEVICE_NAME,
            "color": BOTTLE_COLOR
        }
        
        response = urequests.post(
            SERVER_URL,
            headers={'content-type': 'application/json'},
            json=data
        )
        
        if response.status_code == 200:
            result = response.json()
            indicate_status(result['estado'] == 'nfc_OK')
        else:
            print("Error en la petición:", response.status_code)
            indicate_status(False)
        
        response.close()
    
    except Exception as e:
        print("Error:", str(e))
        indicate_status(False)

def read_nfc():
    """Lee tarjeta NFC y retorna su ID en hexadecimal"""
    try:
        nfc.SAMconfigure()
        uid = nfc.read_passive_target()
        if uid is not None:
            return ''.join([hex(i)[2:].zfill(2) for i in uid]).upper()
        return None
    except Exception as e:
        print("Error leyendo NFC:", str(e))
        return None

def main():
    # Inicializar LEDs
    LED_GREEN.off()
    LED_RED.off()
    
    # Conectar WiFi
    connect_wifi()
    
    # Inicializar PN532
    print("Inicializando PN532...")
    version = nfc.firmware_version
    print("Found PN532 with firmware version:", version)
    
    print("Sistema listo para leer tarjetas...")
    
    while True:
        try:
            nfc_id = read_nfc()
            if nfc_id:
                print("Tarjeta detectada:", nfc_id)
                send_nfc_data(nfc_id)
                time.sleep(1)  # Esperar antes de la siguiente lectura
            time.sleep(0.1)  # Pequeña pausa para no saturar el bus I2C
            
        except Exception as e:
            print("Error en el loop principal:", str(e))
            time.sleep(1)

if __name__ == "__main__":
    main()