# pn532.py - Biblioteca para PN532 NFC
from machine import Pin
import time

PN532_I2C_ADDRESS = 0x24

# Comandos PN532
COMMAND_GET_FIRMWARE_VERSION = 0x02
COMMAND_SAMCONFIGURATION = 0x14
COMMAND_INLISTPASSIVETARGET = 0x4A

class PN532_I2C:
    def __init__(self, i2c, address=PN532_I2C_ADDRESS):
        self.i2c = i2c
        self.address = address
        self._gpio_init()
        self.SAMconfigure()
    
    def _gpio_init(self):
        """Inicializa pines GPIO necesarios"""
        time.sleep(0.1)
    
    def _write_data(self, data):
        """Escribe datos al PN532"""
        try:
            self.i2c.writeto(self.address, bytes(data))
        except Exception as e:
            print("Error escribiendo al PN532:", e)
    
    def _read_data(self, count):
        """Lee datos del PN532"""
        try:
            return self.i2c.readfrom(self.address, count)
        except Exception as e:
            print("Error leyendo del PN532:", e)
            return None
    
    def _write_frame(self, data):
        """Escribe un frame completo al PN532"""
        length = len(data)
        frame = bytearray(length + 7)
        frame[0] = 0x00  # Preamble
        frame[1] = 0x00  # Start code 1
        frame[2] = 0xFF  # Start code 2
        frame[3] = length + 1  # Length of data + 1
        frame[4] = 0x100 - (length + 1)  # Length checksum
        frame[5:5+length] = data  # Data
        checksum = sum(frame[5:5+length]) & 0xFF
        frame[5+length] = 0x100 - checksum  # Data checksum
        frame[6+length] = 0x00  # Postamble
        
        self._write_data(frame)
    
    def _read_frame(self):
        """Lee un frame completo del PN532"""
        # Leer hasta encontrar preamble
        while True:
            response = self._read_data(1)
            if response and response[0] == 0x01:
                break
        
        response = self._read_data(6)
        length = response[3]
        data = self._read_data(length + 2)  # +2 para checksum y postamble
        
        return data[:-2]  # Retorna datos sin checksum ni postamble
    
    @property
    def firmware_version(self):
        """Lee la versión del firmware"""
        self._write_frame([COMMAND_GET_FIRMWARE_VERSION])
        response = self._read_frame()
        if response and len(response) >= 4:
            return (response[0], response[1], response[2])
        return None
    
    def SAMconfigure(self):
        """Configura el Secure Access Module"""
        self._write_frame([COMMAND_SAMCONFIGURATION, 0x01, 0x14, 0x01])
        response = self._read_frame()
        return response and response[0] == 0x15
    
    def read_passive_target(self, timeout=1):
        """Lee una tarjeta pasiva"""
        self._write_frame([
            COMMAND_INLISTPASSIVETARGET,
            0x01,  # Max targets
            0x00   # Baud rate (106 kbps type A)
        ])
        
        start = time.time()
        while (time.time() - start) < timeout:
            response = self._read_frame()
            if response and response[0] == 0x4B:
                target_count = response[1]
                if target_count > 0:
                    uid_length = response[5]
                    return response[6:6+uid_length]
            time.sleep(0.1)
        
        return None