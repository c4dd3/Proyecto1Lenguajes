#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>

#define MAX_CONNECTIONS 100
#define DEFAULT_PORT 8080

using namespace std;

// Función para leer el archivo de configuración y obtener el puerto
int read_config() {
    ifstream config_file("config.txt");
    int port = DEFAULT_PORT;

    if (config_file.is_open()) {
        string line;
        while (getline(config_file, line)) {
            if (line.find("port=") == 0) {
                port = stoi(line.substr(5));
            }
        }
        config_file.close();
    } else {
        cerr << "Error al leer el archivo de configuración." << endl;
    }

    return port;
}

// Función para manejar la conexión con un cliente
void handle_client(int client_socket) {
    // ------------------------------------------------------------ //
    char buffer[1024];
    int bytes_received;

    // Recibir el mensaje del cliente
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        // Asegurarse de que los datos recibidos están bien formateados
        buffer[bytes_received] = '\0';  // Aseguramos que el buffer está null-terminated

        // Verificar los datos recibidos (imprimir en consola)
        cout << "Datos recibidos del cliente: " << buffer << endl;

        // Enviar una respuesta de vuelta al cliente
        const char *response = "Mensaje procesado";
        send(client_socket, response, strlen(response), 0);

        // Si deseas hacer una pausa para procesar múltiples mensajes, puedes descomentar lo siguiente
        // usleep(1000000); // Espera 1 segundo (opcional)
    }

    if (bytes_received == 0) {
        cout << "Cliente desconectado" << endl;
    } else if (bytes_received == -1) {
        cerr << "Error al recibir los datos del cliente" << endl;
    }
    // ------------------------------------------------------------ //

    // Cerrar el socket del cliente después de terminar la comunicación
    close(client_socket);
}


int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Leer el puerto del archivo de configuración
    int port = read_config();

    // Crear el socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Error al crear el socket" << endl;
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
    server_addr.sin_port = htons(port);

    // Asociar el socket al puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error al hacer bind" << endl;
        return -1;
    }

    // Escuchar por conexiones
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        cerr << "Error al escuchar por conexiones" << endl;
        return -1;
    }

    cout << "Servidor escuchando en el puerto " << port << endl;

    // Bucle principal para aceptar conexiones
    while (true) {
        // Aceptar una nueva conexión
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            cerr << "Error al aceptar la conexión" << endl;
            continue;  // Volver a intentar aceptar nuevas conexiones
        }

        cout << "Conexión aceptada desde IP: " 
             << inet_ntoa(client_addr.sin_addr) << endl;

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
            cerr << "Error al crear el proceso hijo" << endl;
        }
    }

    return 0;
}
