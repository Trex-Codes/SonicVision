import socket
import requests
from bs4 import BeautifulSoup
import pyttsx3  # Importa pyttsx3 para la síntesis de voz en Windows
import time
import math

# Configuración del socket UDP
UDP_IP = "0.0.0.0"  # Escuchar en todas las interfaces
UDP_PORT_GPS = 4210
API_KEY = "Key API"  # Reemplaza con tu clave de API de Google

# Crear el socket UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT_GPS))

print("Esperando mensajes UDP en el puerto {}".format(UDP_PORT_GPS))

# Coordenadas del punto B (reemplaza con las coordenadas deseadas)
point_B = "4.625658520533938,-74.06870241224155"

# Variables
directions_called = False
route_steps = []  # Lista para almacenar los pasos de la ruta
coordinates_steps = []  # Lista para almacenar las coordenadas de cada paso
current_step_index = 0  # Índice del paso actual
last_position = None  # Posición anterior del usuario
threshold_distance = 100  # Distancia umbral en metros para avanzar al siguiente paso

# Inicializar el motor de síntesis de voz
engine = pyttsx3.init()
engine.setProperty('voice', 'spanish')  # Configura la voz en español
engine.setProperty('rate', 150)  # Ajusta la velocidad de la voz

# Función para calcular la distancia entre dos puntos (usando la fórmula de Haversine)
def haversine(lat1, lon1, lat2, lon2):
    R = 6371  # Radio de la Tierra en kilómetros
    lat1_rad = math.radians(lat1)
    lon1_rad = math.radians(lon1)
    lat2_rad = math.radians(lat2)
    lon2_rad = math.radians(lon2)

    dlat = lat2_rad - lat1_rad
    dlon = lon2_rad - lon1_rad

    a = math.sin(dlat / 2) ** 2 + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(dlon / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    distance = R * c * 1000  # Convertir a metros
    return distance

while True:
    data, addr = sock.recvfrom(1024)  # Buffer de 1024 bytes
    message = data.decode()  # Decodificar el mensaje recibido en formato string

    # Procesar el mensaje para extraer la latitud y longitud
    if "Latitud" in message and "Longitud" in message and not directions_called:
        try:
            # Extraer las coordenadas correctamente
            lat_str = message.split("Latitud: ")[1].split("\n")[0].strip()
            lng_str = message.split("Longitud: ")[1].split(",")[0].strip()  # Solo tomamos la primera longitud
            print(f"Latitud: {lat_str}, Longitud: {lng_str}")

            # Verificar si las coordenadas están vacías
            if not lat_str or not lng_str:
                print("Error: Coordenadas vacías.")
                continue

            # Llamar a la API de Directions
            point_A = f"{lat_str},{lng_str}"  # Punto A desde el ESP32
            url = f"https://maps.googleapis.com/maps/api/directions/json?origin={point_A}&destination={point_B}&mode=walking&language=es&key={API_KEY}"

            # Imprimir la URL de solicitud para depuración
            print('\n')
            print(f"URL de solicitud: {url}")

            response = requests.get(url)
            directions = response.json()

            # Procesar y mostrar la ruta en forma de lista
            if directions['status'] == 'OK':
                route_steps.clear()  # Limpiar la lista de pasos
                coordinates_steps.clear()  # Limpiar la lista de coordenadas

                for step in directions['routes'][0]['legs'][0]['steps']:
                    instruction = step['html_instructions']
                    
                    # Limpiar las instrucciones de HTML
                    clean_instruction = BeautifulSoup(instruction, "html.parser").get_text()
                    
                    # Obtener la distancia del paso y combinar con la instrucción
                    distance_text = step['distance']['text']
                    combined_instruction = f"{clean_instruction}. Distancia de este paso: {distance_text}."
                    
                    # Obtener coordenadas (lat, lon) de cada paso
                    step_lat = step['end_location']['lat']
                    step_lon = step['end_location']['lng']
                    
                    route_steps.append(combined_instruction)  # Agregar instrucción con distancia
                    coordinates_steps.append((step_lat, step_lon))  # Agregar coordenadas

                # Mostrar los pasos de la ruta como una lista
                print("Ruta encontrada en forma de lista:")
                for i, step in enumerate(route_steps, start=1):
                    print(f"{i}. {step}")
                print('\n')

            else:
                print("No se encontró ruta:", directions['status'])
                if 'error_message' in directions:
                    print(f"Mensaje de error: {directions['error_message']}")

            # Marcar que se ha llamado a la API de Directions
            directions_called = True

        except IndexError:
            print("Error: no se pudieron extraer las coordenadas del mensaje.")

    # Si ya se han obtenido direcciones y hay pasos disponibles
    if directions_called and route_steps:
        # Comprobar si el índice del paso actual es menor que la cantidad de pasos
        if current_step_index < len(route_steps):
            # Verificar si la posición actual está dentro del umbral de distancia al siguiente paso
            current_lat = float(lat_str)
            current_lon = float(lng_str)

            # Obtener las coordenadas del siguiente paso desde la lista
            next_lat, next_lon = coordinates_steps[current_step_index]

            # Calcular la distancia entre la ubicación actual y el siguiente paso
            distance_to_next_step = haversine(current_lat, current_lon, next_lat, next_lon)

            # Comprobar si hemos llegado al siguiente paso
            if distance_to_next_step <= threshold_distance:
                # Leer la instrucción en voz alta
                engine.say(route_steps[current_step_index])
                engine.runAndWait()
                print(f"Leído: {route_steps[current_step_index]}")
                current_step_index += 1  # Avanzar al siguiente paso
                time.sleep(2)  # Esperar un tiempo antes de dar la siguiente instrucción
            else:
                # Leer la distancia restante en voz alta
                distance_message = f"Aún no has alcanzado el siguiente paso. Distancia restante: {distance_to_next_step:.2f} metros."
                engine.say(distance_message)
                engine.runAndWait()
                print(distance_message)
                time.sleep(2)  # Esperar antes de volver a verificar


        else:
            engine.say("Todos los pasos han sido leidos")
            engine.runAndWait()
            print("Todos los pasos han sido leídos.")
    else:
        print("El mensaje no contiene latitud y longitud o no se han recibido direcciones.")
