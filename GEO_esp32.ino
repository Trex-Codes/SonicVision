#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "BocaEfa";     // Reemplaza con el nombre de tu red Wi-Fi
const char* password = "amigo123"; // Reemplaza con la contraseña de tu red Wi-Fi
//const char* udpAddress = "192.168.58.185";  // IP del Raspberry Pi
const char* udpAddress = "192.168.58.189";  // IP del pc laptopTrexCodes
const int udpPort_GPS = 4210;  // Puerto UDP

// Reemplaza por tu clave de API de Google
const char* googleMapsApiKey = "Key API";

WiFiUDP udp; // Inicializa el objeto UDP
float lat = 0.0; // Inicializa latitud
float lng = 0.0; // Inicializa longitud
bool ubicacionEnviada = false; // Bandera para verificar si se ha enviado la ubicación
unsigned long previousMillis = 0; // Guardar el tiempo anterior
const long interval = 10000; // Intervalo de 10 segundos

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Conectado a Wi-Fi.");

  // Obtener la ubicación una sola vez
  obtenerUbicacion();
}

void loop() {
  // Obtener el tiempo actual
  unsigned long currentMillis = millis();

  // Verificar si han pasado 10 segundos
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
      String url = "https://www.googleapis.com/geolocation/v1/geolocate?key=" + String(googleMapsApiKey);
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
