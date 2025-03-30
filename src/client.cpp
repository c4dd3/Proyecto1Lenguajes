#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <sys/shm.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sstream>

using namespace std;

struct Contacto {
    string nombre;
    string apellido;
    string correo;
};
vector<Contacto> lista_contactos;

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

// Función para agregar un contacto
void agregar_contacto(const Contacto &nuevo_contacto) {
    for (const auto &c : lista_contactos) {
        if (c.correo == nuevo_contacto.correo) {
            cout << "El contacto ya está en la lista." << endl;
            return;
        }
    }
    lista_contactos.push_back(nuevo_contacto);
    cout << "Contacto agregado correctamente." << endl;
}

// Función para mostrar la lista de contactos
void mostrar_contactos() {
    cout << "\nLista de contactos:\n";
    if (lista_contactos.empty()) {
        cout << "No tienes contactos agregados." << endl;
    } else {
        for (const auto &c : lista_contactos) {
            cout << "- " << c.nombre << " " << c.apellido << " (" << c.correo << ")" << endl;
        }
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

    // Recibir la respuesta del servidor
    char buffer[1024] = {0};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        cout << "Respuesta del servidor: " << buffer << endl;
    } else {
        cerr << "Error al recibir la respuesta del servidor" << endl;
    }

    // Menú para el cliente
    int opcion;
    string nombre, apellido, correo, contrasena;

    while (true) {
        cout << "\nElija una opción: \n";
        cout << "1. Registrarse\n";
        cout << "2. Iniciar sesión\n";
        cout << "3. Desconectar\n";
        cout << "4. Salir\n";
        cout << "5. Agregar Contacto\n";
        cout << "6. Mostrar Contactos\n";
        cout << "Opción: ";
        cin >> opcion;

        if (opcion == 1) {
            // Registro de usuario
            cout << "Ingrese su nombre: ";
            cin >> nombre;
            cout << "Ingrese su apellido: ";
            cin >> apellido;
            cout << "Ingrese su correo: ";
            cin >> correo;
            cout << "Ingrese su contraseña: ";
            cin >> contrasena;

            string comando = "REGISTER " + nombre + " " + apellido + " " + correo + " " + contrasena;

            // Enviar el comando al servidor
            send(client_fd, comando.c_str(), comando.length(), 0);

            // Recibir la respuesta del servidor
            char buffer[1024] = {0};
            int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                cout << "Respuesta del servidor: " << buffer << endl;
            } else {
                cerr << "Error al recibir la respuesta del servidor" << endl;
            }
        } 
        else if (opcion == 2) {
            // Iniciar sesión
            cout << "Ingrese su correo: ";
            cin >> correo;
            cout << "Ingrese su contraseña: ";
            cin >> contrasena;

            string comando = "LOGIN " + correo + " " + contrasena;

            // Enviar el comando al servidor
            send(client_fd, comando.c_str(), comando.length(), 0);

            // Recibir la respuesta del servidor
            char buffer[1024] = {0};
            int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                cout << "Respuesta del servidor: " << buffer << endl;
            } else {
                cerr << "Error al recibir la respuesta del servidor" << endl;
            }
        } 
        else if (opcion == 3) {
            // Desconectar
            string comando = "DISCONNECT";
            send(client_fd, comando.c_str(), comando.length(), 0);
        
            // Recibir confirmación del servidor
            char buffer[1024] = {0};
            int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                cout << "Respuesta del servidor: " << buffer << endl;
            }
        
            cout << "Cerrando conexión..." << endl;
            close(client_fd);
            exit(0);
        }
        else if (opcion == 4) {
            // Salir
            cout << "Cerrando conexión..." << endl;
            break;
        }
        else if (opcion == 5) {  
            // Agregar Contacto
            cout << "Ingrese el correo del usuario a agregar: ";
            cin >> correo;
        
            string comando = "GETUSER " + correo;
            send(client_fd, comando.c_str(), comando.length(), 0);
        
            // Recibir la respuesta del servidor
            char buffer[1024] = {0};
            int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            
            if (bytes_received > 0) {
                string respuesta(buffer);
        
                if (respuesta.find("ERROR") != string::npos) {
                    cout << respuesta << endl;
                } else {
                    istringstream ss(respuesta);
                    string temp, nombre, apellido, correo;
        
                    ss >> temp >> nombre;
                    ss >> temp >> apellido;
                    ss >> temp >> correo;
        
                    Contacto nuevo_contacto = {nombre, apellido, correo};
                    agregar_contacto(nuevo_contacto);
                }
            } else {
                cerr << "Error al recibir la información del usuario." << endl;
            }
        }
        else if (opcion == 6) {  // Mostrar Contactos
            mostrar_contactos();
        }
        else {
            cout << "Opción no válida. Intente nuevamente." << endl;
        }
    }

    close(client_fd);
    return 0;
}
