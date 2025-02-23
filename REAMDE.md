# Sistema de Registro NFC

Sistema para registrar etiquetas NFC usando múltiples clientes (ESP32 o Raspberry Pico W) y un servidor central Flask.

## Estructura del Proyecto

- `server/`: Servidor Flask con interfaz web y API
- `clients/`: Clientes para diferentes dispositivos
  - `esp32/`: Cliente para ESP32 con RC522
  - `pico/`: Cliente para Raspberry Pico W con PN532

## Requisitos

### Servidor
- Python 3.7+
- Flask y dependencias (ver requirements.txt)
- Red local para comunicación con clientes

### Clientes
Ver README específico en cada carpeta de cliente para detalles de hardware y configuración.

## Instalación

1. Clonar el repositorio:
```bash
git clone https://github.com/tu-usuario/nfc-registration-system.git
cd nfc-registration-system
```

2. Configurar el servidor:
```bash
cd server
python -m venv venv
source venv/bin/activate  # En Windows: venv\Scripts\activate
pip install -r requirements.txt
```

3. Iniciar el servidor:
```bash
python app.py
```

El servidor estará disponible en `http://ip-local:5000`

## Configuración de Clientes

- Para ESP32, ver: [Instrucciones ESP32](clients/esp32/README.md)
- Para Raspberry Pico W, ver: [Instrucciones Pico](clients/pico/README.md)

## Uso

1. Iniciar el servidor Flask
2. Configurar y conectar los clientes NFC
3. Acceder a la interfaz web para:
   - Registrar usuarios
   - Ver registros en tiempo real
   - Consultar estadísticas
   - Descargar datos en CSV

## Licencia

MIT