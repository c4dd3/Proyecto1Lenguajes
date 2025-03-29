#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <fstream>

#define MAX_CONNECTIONS 100
#define DEFAULT_PORT 8080

// Función para leer el archivo de configuración y obtener el puerto
int read_config() {
    std::ifstream config_file("config.txt");
    int port = DEFAULT_PORT;

    if (config_file.is_open()) {
        std::string line;
        while (std::getline(config_file, line)) {
            if (line.find("port=") == 0) {
                port = std::stoi(line.substr(5));
            }
        }
        config_file.close();
    } else {
        std::cerr << "Error al leer el archivo de configuración." << std::endl;
    }

    return port;
}

// Función para manejar la conexión con un cliente
void handle_client(int client_socket) {
    //Codigo para manejar la conexion con el cliente
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Leer el puerto del archivo de configuración
    int port = read_config();

    // Crear el socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Error al crear el socket" << std::endl;
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
    server_addr.sin_port = htons(port);

    // Asociar el socket al puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error al hacer bind" << std::endl;
        return -1;
    }

    // Escuchar por conexiones
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        std::cerr << "Error al escuchar por conexiones" << std::endl;
        return -1;
    }

    std::cout << "Servidor escuchando en el puerto " << port << std::endl;

    // Bucle principal para aceptar conexiones
    while (true) {
        // Aceptar una nueva conexión
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            std::cerr << "Error al aceptar la conexión" << std::endl;
            continue;  // Volver a intentar aceptar nuevas conexiones
        }

        std::cout << "Conexión aceptada desde IP: " 
                  << inet_ntoa(client_addr.sin_addr) << std::endl;

        // Crear un nuevo proceso para manejar la conexión
        pid_t pid = fork();
        if (pid == 0) {  // Código del proceso hijo
            close(server_fd);  // El proceso hijo no necesita el socket principal
            handle_client(client_socket);  // Manejar la conexión con el cliente
            exit(0);  // Finalizar el proceso hijo después de manejar la conexión
        } else if (pid > 0) {  // Código del proceso padre
            close(client_socket);  // El proceso padre ya no necesita el socket del cliente
            // El padre vuelve a escuchar por nuevas conexiones
        } else {
            std::cerr << "Error al crear el proceso hijo" << std::endl;
        }
    }

    return 0;
}
