#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>  // Para gethostbyname()
#include <fstream>   // Para leer el archivo de configuración

using namespace std;

// Función para leer el archivo de configuración y obtener el puerto
void read_config(string &server_ip, int &server_port) {
    ifstream config_file("config.txt");

    if (config_file.is_open()) {
        string line;
        while (getline(config_file, line)) {
            if (line.find("ip=") == 0) {
                server_ip = line.substr(3);  // Extraer la IP después de "ip="
            }
            if (line.find("port=") == 0) {
                server_port = stoi(line.substr(5));  // Extraer el puerto después de "port="
            }
        }
        config_file.close();
    } else {
        cerr << "Error al leer el archivo de configuración." << endl;
    }
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr;

    // Variables para la IP y el puerto del servidor
    string server_ip = "127.0.0.1";  // Valor por defecto
    int server_port = 8080;          // Valor por defecto

    // Leer el archivo de configuración para obtener la IP y el puerto
    read_config(server_ip, server_port);

    // Mostrar la IP y puerto que se utilizarán para la conexión
    cout << "Conectando al servidor en IP: " << server_ip << " y puerto: " << server_port << endl;

    // Crear el socket del cliente
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear el socket del cliente" << endl;
        return -1;
    }

    // Configuración de la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);  // Puerto del servidor

    // Convertir la dirección IP del servidor a formato binario
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "Error en la conversión de la dirección IP" << endl;
        return -1;
    }

    // Conectar al servidor
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error al conectar con el servidor" << endl;
        return -1;
    }

    cout << "Conectado al servidor!" << endl;

    // Enviar un mensaje al servidor
    const char *message = "Hola, servidor!";
    send(client_fd, message, strlen(message), 0);

    // Recibir la respuesta del servidor
    char buffer[1024] = {0};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        cout << "Mensaje recibido del servidor: " << buffer << endl;
    } else {
        cerr << "Error al recibir el mensaje del servidor" << endl;
    }

    // Cerrar la conexión con el servidor
    close(client_fd);

    return 0;
}
