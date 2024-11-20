import socket
import threading
import tkinter as tk

# Dirección y puerto del ESP32
ESP32_IP = "192.168.255.145"  # IP de ESP32
UDP_PORT = 4211  # Cambiar el puerto a 4211
LOCAL_IP = "0.0.0.0"  # Para recibir datos de cualquier IP

# Función para recibir datos UDP del ESP32
def recibir_udp():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((LOCAL_IP, UDP_PORT))
    print(f"Esperando mensajes en {LOCAL_IP}:{UDP_PORT}...")

    while True:
        data, addr = sock.recvfrom(1024)
        mensaje = f"Distancia: {data.decode()} cm de {addr}"
        print(mensaje)
        # Actualizar la etiqueta de Tkinter con el mensaje recibido
        etiqueta_distancia.config(text=mensaje)

# Configuración de la interfaz Tkinter
ventana = tk.Tk()
ventana.title("Monitor de Distancia ESP32")
ventana.geometry("400x200")

# Crear y posicionar una etiqueta para mostrar los datos
etiqueta_distancia = tk.Label(ventana, text="Esperando datos...", font=("Arial", 16))
etiqueta_distancia.pack(pady=20)

# Crear el hilo para recibir datos UDP
hilo_recibir = threading.Thread(target=recibir_udp)
hilo_recibir.daemon = True  # Para que se cierre el hilo al cerrar la ventana
hilo_recibir.start()

# Ejecutar la interfaz de Tkinter
ventana.mainloop()
