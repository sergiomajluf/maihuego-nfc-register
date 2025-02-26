# Control Remoto de Chromium en modo Kiosko para Raspberry Pi

Este documento describe cómo iniciar remotamente Chromium en modo kiosko (pantalla completa) en un Raspberry Pi a través de SSH. Ideal para aplicaciones de señalización digital, quioscos interactivos o paneles de control.

## Requisitos previos

- Raspberry Pi con sistema operativo basado en Debian (Raspberry Pi OS / Raspbian)
- Entorno gráfico instalado (X11)
- Chromium Browser instalado
- SSH habilitado en el Raspberry Pi
- Una aplicación web ejecutándose (en nuestro caso en http://127.0.0.1:5000)

## Comando básico

Para abrir Chromium en modo kiosko desde SSH:

```bash
DISPLAY=:0 chromium-browser --kiosk --incognito --noerrdialogs --disable-infobars http://127.0.0.1:5000 &
```

## Instrucciones detalladas

1. Conéctate a tu Raspberry Pi mediante SSH:
   ```bash
   ssh pi@direccion_ip_raspberry
   ```

2. Si tu aplicación web no está en ejecución, iníciala:
   ```bash
   # Ejemplo: Activar un entorno virtual y ejecutar una aplicación Flask
   cd /ruta/a/tu/aplicacion
   source venv/bin/activate
   python app.py &
   ```

3. Una vez que tu aplicación esté en ejecución, lanza Chromium en modo kiosko:
   ```bash
   DISPLAY=:0 chromium-browser --kiosk --incognito --noerrdialogs --disable-infobars http://127.0.0.1:5000 &
   ```

4. Para cerrar Chromium cuando lo necesites:
   ```bash
   pkill chromium
   ```

## Explicación de los parámetros

- `DISPLAY=:0`: Especifica que Chromium debe mostrarse en la pantalla principal conectada al Raspberry Pi
- `--kiosk`: Activa el modo pantalla completa sin bordes, barras de herramientas ni controles
- `--incognito`: Inicia en modo incógnito para evitar diálogos de restauración de sesiones anteriores
- `--noerrdialogs`: Suprime los diálogos de error que podrían interrumpir la experiencia
- `--disable-infobars`: Evita que aparezcan barras de información en la parte superior
- `&`: Ejecuta el proceso en segundo plano para que puedas seguir usando la terminal SSH

## Opciones adicionales útiles

Para una experiencia de kiosko aún más robusta, considera estos parámetros adicionales:

```bash
DISPLAY=:0 chromium-browser --kiosk --incognito --noerrdialogs --disable-infobars --disable-session-crashed-bubble --disable-translate --no-first-run --no-default-browser-check http://127.0.0.1:5000 &
```

Parámetros adicionales:
- `--disable-session-crashed-bubble`: Evita mensajes de recuperación de sesión
- `--disable-translate`: Desactiva sugerencias de traducción
- `--no-first-run`: Omite la configuración inicial de primer uso
- `--no-default-browser-check`: Evita la pregunta sobre navegador predeterminado

## Solución de problemas

### Error: "Could not open display"

Si obtienes este error, puede ser un problema de permisos de X11. Prueba estas soluciones:

1. En el Raspberry Pi directamente (no por SSH), ejecuta:
   ```bash
   xhost +
   ```
   Esto permite temporalmente que cualquier cliente se conecte al servidor X.

2. Alternativamente, para una solución más segura:
   ```bash
   xhost +localhost
   ```
   Esto permite conexiones solo desde localhost.

### Chromium se abre pero no en pantalla completa

Asegúrate de que estás usando el parámetro `--kiosk` y no `--start-fullscreen` o `--start-maximized`.

### La aplicación web no carga

Verifica que tu aplicación web está funcionando correctamente:
```bash
curl http://127.0.0.1:5000
```

## Configuración de inicio automático

Para configurar que Chromium se inicie automáticamente en modo kiosko al arrancar:

1. Crea un archivo de servicio systemd para tu aplicación web.
2. Configura un archivo autostart para Chromium.

Ejemplo de archivo autostart (`~/.config/autostart/kiosk.desktop`):
```
[Desktop Entry]
Type=Application
Name=Kiosk Mode
Exec=chromium-browser --kiosk --incognito --noerrdialogs --disable-infobars http://127.0.0.1:5000
X-GNOME-Autostart-enabled=true
```

## Evitar que la pantalla se apague

Para prevenir que el protector de pantalla se active o que la pantalla se apague:

```bash
# Crear archivo autostart para deshabilitar el protector de pantalla
cat > ~/.config/autostart/disable-screensaver.desktop << EOF
[Desktop Entry]
Type=Application
Name=Disable Screensaver
Exec=xset s off -dpms
X-GNOME-Autostart-enabled=true
EOF
chmod +x ~/.config/autostart/disable-screensaver.desktop
```

---

Esta solución te permite controlar remotamente la pantalla de tu Raspberry Pi, convirtiéndolo en un sistema de visualización profesional para aplicaciones web.