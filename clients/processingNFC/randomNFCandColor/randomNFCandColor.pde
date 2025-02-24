import http.requests.*;
import java.util.Random;

String nfcID = "";
String serverURL = "http://192.168.1.101:5000/api/registrar_nfc";
String[] colores = {"Rojo", "Verde", "Azul", "Amarillo", "Blanco", "Negro"};
String[] names = {"Sergio", "Seba", "Daniel", "Damian", "Camilo"};
String[] origenes = {"Processing", "ESP32", "Web"};
Button sendRandomButton;
color backgroundColor = color(75);  // Color de fondo inicial (negro)
color defaultBackgroundColor = color(75);  // Color de fondo inicial (negro)
color successColor = color(0, 140, 0);  // Color de fondo exitoso
color errorColor = color(140, 0, 0);  // Color de fondo de error
String errorMessage = "";         // Variable para almacenar el mensaje de error
String sentData = "";             // Para almacenar los datos enviados
String receivedData = "";         // Para almacenar los datos recibidos
int timerStartTime = 0;           // Tiempo en que el temporizador inicia
boolean isTimerActive = false;    // Controla si el temporizador está activo

void setup() {
  size(400, 300);
  sendRandomButton = new Button(width / 2 - 50, height - 60, 100, 40, "Random");
}

void draw() {
  background(backgroundColor);  // Establecer el color de fondo según la respuesta
  fill(255);
  textSize(16);
  textAlign(CENTER, CENTER);
  text("Ingrese ID y presione Enter", width / 2, height / 4);
  textSize(20);
  text("ID actual:\n" + nfcID, width / 2, height / 2.8);
  
  sendRandomButton.display();
  
  // Mostrar los datos enviados y recibidos
  textSize(12);
  textAlign(LEFT, CENTER);
  text("Enviados: " + sentData, width/12, height / 1.65);
  text("Recibidos: " + receivedData, width/12, height / 1.45);
  
  // Si hay un error, mostrar el mensaje
  if (errorMessage.length() > 0) {
    fill(255);
    textSize(18);
    textAlign(CENTER, CENTER);
    text(errorMessage, width / 2, height / 1.85);
  }

  // Si el temporizador está activo, verificar el tiempo
  if (isTimerActive && millis() - timerStartTime > 3000) {
    resetScreen();  // Limpiar la pantalla después de 3 segundos
  }
}

void keyPressed() {
  if (key == ENTER || key == RETURN) {
    if (nfcID.length() > 0) {
      sendNFCData(nfcID, getRandomElement(names), getRandomElement(colores), getRandomElement(origenes));
      nfcID = ""; // Reiniciar ID después de enviarlo
    }
  } else if (key == BACKSPACE) {
    if (nfcID.length() > 0) {
      nfcID = nfcID.substring(0, nfcID.length() - 1);
    }
  } else {
    nfcID += key;
  }
}

void mousePressed() {
  if (sendRandomButton.isClicked(mouseX, mouseY)) {
    nfcID = generateRandomID(10);
    sendNFCData(nfcID, getRandomElement(names), getRandomElement(colores), getRandomElement(origenes));
  }
}

void sendNFCData(String nfcID, String userName, String miColor, String origen) {
  JSONObject data = new JSONObject();
  data.setString("nfc_id", nfcID);
  data.setString("usuario", userName);
  data.setString("color", miColor);
  data.setString("origen", origen);
  
  sentData = "ID: " + nfcID + ", Usuario: " + userName + ", Color: " + miColor + ", Origen: " + origen;

  PostRequest request = new PostRequest(serverURL);
  request.addHeader("Content-Type", "application/json");
  request.addData(data.toString());
  request.send();
  
  // Verificar la respuesta del servidor
  if (request.getContent() != null) {
    JSONObject response = parseJSONObject(request.getContent());  // Usar parseJSONObject en lugar de constructor
    if (response != null) {
      String estado = response.getString("estado");  // Obtener el valor de 'estado'
      String mensaje = response.getString("mensaje");  // Obtener el valor de 'mensaje'
      
      receivedData = "Estado: " + estado + ", Mensaje: " + mensaje;  // Mostrar los datos recibidos

      // Actualizar el fondo y el mensaje según el estado
      if (estado.equals("nfc_OK")) {
        backgroundColor = successColor;  // Fondo verde si el registro es exitoso
        println("Registro exitoso: " + mensaje);
      } else if (estado.equals("nfc_duplicado")) {
        backgroundColor = errorColor;  // Fondo rojo si es un NFC duplicado
        errorMessage = "Error: " + mensaje;   // Guardar el mensaje de error
        println("Error: " + mensaje);
      } else {
        backgroundColor = defaultBackgroundColor;  // Fondo por defecto si es otro tipo de error
        errorMessage = "Error desconocido: " + mensaje;
        println("Error desconocido: " + mensaje);
      }

      startTimer();  // Iniciar el temporizador después de la respuesta
    } else {
      backgroundColor = color(255, 0, 0);  // Error al analizar el JSON
      errorMessage = "Error al procesar la respuesta JSON";
      println("Error al procesar la respuesta JSON");
      startTimer();  // Iniciar el temporizador en caso de error
    }
  } else {
    backgroundColor = color(255, 0, 0);  // Error si no hay respuesta
    errorMessage = "Error en la petición";
    println("Error en la petición");
    startTimer();  // Iniciar el temporizador en caso de error
  }
}

void startTimer() {
  isTimerActive = true;  // Activar el temporizador
  timerStartTime = millis();  // Guardar el tiempo de inicio
}

void resetScreen() {
  backgroundColor = defaultBackgroundColor;  // Volver al color de fondo por defecto
  errorMessage = "";  // Limpiar el mensaje de error
  sentData = "";      // Limpiar los datos enviados
  receivedData = "";  // Limpiar los datos recibidos
  isTimerActive = false;  // Desactivar el temporizador
}

String generateRandomID(int length) {
  String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  StringBuilder sb = new StringBuilder();
  Random random = new Random();
  for (int i = 0; i < length; i++) {
    sb.append(chars.charAt(random.nextInt(chars.length())));
  }
  return sb.toString();
}

String getRandomElement(String[] array) {
  Random random = new Random();
  return array[random.nextInt(array.length)];
}

class Button {
  int x, y, w, h;
  String label;
  
  Button(int x, int y, int w, int h, String label) {
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
    this.label = label;
  }
  
  void display() {
    fill(100);
    rect(x, y, w, h);
    fill(255);
    textAlign(CENTER, CENTER);
    text(label, x + w / 2, y + h / 2);
  }
  
  boolean isClicked(int mx, int my) {
    return mx > x && mx < x + w && my > y && my < y + h;
  }
}
