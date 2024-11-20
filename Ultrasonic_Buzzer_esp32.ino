#include <WiFi.h>
#include <WiFiUdp.h>

// Variables para establecer conexion UDP
const char* ssid = "BocaEfa";
const char* password = "amigo123";
// const char* udpAddress = "192.168.58.185";  // IP del Raspberry Pi
const char* udpAddress = "192.168.58.189";  // IP del pc laptopTrexCodes
const int udpPort_Ultrasonic = 4211;  // Puerto UDP

const int Trigger = 13;  // Pin D13 para Trigger
const int Echo = 12;     // Pin D12 para Echo
const int buzzer = 25;   // Pin del buzzer (usando el pin digital 25)

WiFiUDP udp; // Crear objeto UDP

void setup() {
  Serial.begin(115200);           // Iniciar comunicación serial
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
}

void loop() {
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

  delay(500); // Esperar 500 ms antes de la próxima medición


  // ENVIAR DATOS A UDP RASPERRI
  udp.beginPacket(udpAddress, udpPort_Ultrasonic);
  udp.print(d);  // Enviar la distancia como texto
  udp.endPacket();
}
