import socket
import requests
from bs4 import BeautifulSoup  # Importar BeautifulSoup
import subprocess
import time

# Configuración del socket UDP
UDP_IP = "0.0.0.0"  # Escuchar en todas las interfaces
UDP_PORT = 4210
API_KEY = "API KEY"  # Reemplaza con tu clave de API de Google

# Crear el socket UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print("Esperando mensajes UDP en el puerto {}".format(UDP_PORT))

# Coordenadas del punto B (reemplaza con las coordenadas deseadas)
point_B = "4.6590758260444325, -74.07158639131463"  # Por ejemplo, el centro de Bogotá

# Variable para indicar si ya se ha hecho la llamada a la API
directions_called = False
route_steps = []  # Lista para almacenar los pasos de la ruta
current_step_index = 0  # Índice del paso actual

while True:
    data, addr = sock.recvfrom(1024)  # Buffer de 1024 bytes
    message = data.decode()  # Decodificar el mensaje recibido en formato string
    print("Mensaje recibido:", message, "de", addr)

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
            print(f"URL de solicitud: {url}")

            response = requests.get(url)
            directions = response.json()

            # Procesar y mostrar la ruta en forma de lista
            if directions['status'] == 'OK':
                route_steps.clear()  # Limpiar la lista de pasos
                for step in directions['routes'][0]['legs'][0]['steps']:
                    instruction = step['html_instructions']
                    
                    # Limpiar las instrucciones de HTML
                    clean_instruction = BeautifulSoup(instruction, "html.parser").get_text()
                    route_steps.append(clean_instruction)  # Solo agregar la instrucción
                
                # Mostrar los pasos de la ruta como una lista
                print("Ruta encontrada en forma de lista:")
                for i, step in enumerate(route_steps, start=1):
                    print(f"{i}. {step}")

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
            # Leer la instrucción en voz alta
            instruction = route_steps[current_step_index]
            subprocess.run(["espeak-ng", "-v", "es-la", instruction])  # Especificar la voz de español latinoamericano
            print(f"Leído: {instruction}")
            current_step_index += 1  # Avanzar al siguiente paso
            time.sleep(2)  # Esperar un tiempo antes de dar la siguiente instrucción
        else:
            print("Todos los pasos han sido leídos.")
    else:
        print("El mensaje no contiene latitud y longitud o no se han recibido direcciones.")
