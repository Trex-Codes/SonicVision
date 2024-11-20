#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Variables para conexión Wi-Fi
const char* ssid = "BocaEfa";
const char* password = "amigo123";

// Dirección IP y puertos UDP
const char* udpAddress = "192.168.255.189";  // IP del PC 
// const char* udpAddress = "192.168.58.189";  // IP del Raspberry PI 
const int udpPort_Ultrasonic = 4211;  // Puerto para el sensor ultrasónico
const int udpPort_GPS = 4210;  // Puerto para las coordenadas GPS

// Clave de API de Google Maps
const String googleMapsApiKey = "API KEY";  // Reemplaza con tu clave de API

// Pines para el sensor ultrasónico
const int Trigger = 13;  // Pin D13 para Trigger
const int Echo = 12;     // Pin D12 para Echo
const int buzzer = 25;   // Pin del buzzer (usando el pin digital 25)

// Variables para la ubicación GPS
float lat = 0.0; // Latitud
float lng = 0.0; // Longitud
bool ubicacionEnviada = false; // Bandera para verificar si se ha enviado la ubicación
unsigned long previousMillis = 0; // Guardar el tiempo anterior
const long interval = 40000; // Intervalo de 10 segundos para enviar ubicación

// Inicialización de objetos
WiFiUDP udp; // Objeto UDP

// Variables para el ultrasonido
unsigned long previousDistanceMillis = 0; // Temporizador para medición ultrasónica
const long distanceInterval = 500; // Intervalo de 500ms entre mediciones ultrasónicas

void setup() {
  Serial.begin(115200);  // Iniciar comunicación serial
  pinMode(Trigger, OUTPUT);     // Configurar D13 como salida
  pinMode(Echo, INPUT);         // Configurar D12 como entrada
  pinMode(buzzer, OUTPUT);      // Configurar el pin del buzzer como salida

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");

  // Obtener la ubicación una sola vez al inicio
  obtenerUbicacion();
}

void loop() {
  // Obtener la ubicación cada 10 segundos sin bloquear el código
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Actualizar el tiempo anterior
    if (!ubicacionEnviada) {
      enviarUbicacionPorUDP();
      ubicacionEnviada = true; // Cambiar la bandera a true para evitar el reenvío
      Serial.println("Ubicación enviada.");
    } else {
      // Actualizar la ubicación y enviarla nuevamente
      obtenerUbicacion(); // Actualizar la ubicación
      enviarUbicacionPorUDP(); // Enviar la nueva ubicación
      Serial.println("Nueva ubicación enviada.");
    }
  }

  // Medición del sensor ultrasónico
  if (currentMillis - previousDistanceMillis >= distanceInterval) {
    previousDistanceMillis = currentMillis; // Actualizar el tiempo de la medición
    medirDistancia();
  }

  delay(100); // Dejar algo de tiempo para otras tareas si es necesario
}

void medirDistancia() {
  long t;  // Tiempo que demora en llegar el eco
  long d;  // Distancia en centímetros

  // Generar pulso en Trigger
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);        // Pulso de 10 microsegundos
  digitalWrite(Trigger, LOW);

  // Medir duración del pulso recibido en Echo
  t = pulseIn(Echo, HIGH);      // Obtener el ancho del pulso
  d = t / 58;                   // Convertir el tiempo a distancia en cm

  // Mostrar la distancia en el monitor serial
  Serial.print("Distancia: ");
  Serial.print(d);             
  Serial.println(" cm");

  // Si la distancia es menor a 40 cm, activar el buzzer
  if (d < 40) {
    int times = max(1, (int)(5 - d / 8)); // Ajuste de frecuencia en función de la distancia
    for (int i = 0; i < times; i++) {
      tone(buzzer, 2000);  // Generar tono a 2000 Hz para mayor intensidad
      delay(100);          // Duración del tono (100 ms)
      noTone(buzzer);      // Detener el tono
      delay(50);           // Esperar menos tiempo entre tonos para sonar más rápido
    }
  } else {
    noTone(buzzer);            // Detener el tono si la distancia es mayor o igual a 40 cm
  }

  // ENVIAR DATOS A UDP (sensor ultrasónico)
  udp.beginPacket(udpAddress, udpPort_Ultrasonic);
  udp.print("Distancia: ");
  udp.println(d);  // Enviar la distancia como texto
  udp.endPacket();
}

void obtenerUbicacion() {
  // Escanear redes Wi-Fi
  Serial.println("Escaneando redes Wi-Fi...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No se encontraron redes Wi-Fi.");
    return;
  } else {
    Serial.println("Redes Wi-Fi encontradas:");
    // Crear el JSON para enviar a Google Maps Geolocation API
    String jsonPayload = "{ \"wifiAccessPoints\": [";

    for (int i = 0; i < n; ++i) {
      if (i > 0) {
        jsonPayload += ",";
      }
      jsonPayload += "{";
      jsonPayload += "\"macAddress\": \"" + WiFi.BSSIDstr(i) + "\",";
      jsonPayload += "\"signalStrength\": " + String(WiFi.RSSI(i));
      jsonPayload += "}";
    }

    jsonPayload += "] }";

    // Enviar el JSON a Google Maps Geolocation API
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + googleMapsApiKey;
      http.begin(url);
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(jsonPayload);
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respuesta de Google Maps API:");
        Serial.println(response);

        // Analizar la respuesta JSON
        StaticJsonDocument<500> doc;
        DeserializationError error = deserializeJson(doc, response);
        if (!error) {
          lat = doc["location"]["lat"];
          lng = doc["location"]["lng"];
          Serial.print("Latitud: ");
          Serial.println(lat, 6);
          Serial.print("Longitud: ");
          Serial.println(lng, 6);
        } else {
          Serial.print("Error al analizar JSON: ");
          Serial.println(error.c_str());
        }
      } else {
        Serial.print("Error en la solicitud HTTP: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }
  }
}

void enviarUbicacionPorUDP() {
  // Enviar la ubicación por UDP
  udp.beginPacket(udpAddress, udpPort_GPS);
  udp.print("Latitud: ");
  udp.println(lat, 6);
  udp.print("Longitud: ");
  udp.println(lng, 6);
  udp.endPacket();
}
