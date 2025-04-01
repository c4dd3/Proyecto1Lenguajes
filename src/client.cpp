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
#include <sys/types.h>
#include <sys/time.h>
#include <limits>


using namespace std;

struct Usuario {
    string nombre;
    string apellido;
    string correo;
    string contrasena;
};
struct Usuario usuario_autenticado;

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

// Función para agregar un contacto desde la interfaz
void agregar_contacto_func(int client_fd) {
    string correo;
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

            // Si el contacto es el propio usuario, lo guardamos en la variable usuario_autenticado
            if (correo == usuario_autenticado.correo) {
                usuario_autenticado.nombre = nombre;
                usuario_autenticado.apellido = apellido;
                usuario_autenticado.correo = correo;
                cout << "Datos de usuario actualizados." << endl;
            } else {
                agregar_contacto(nuevo_contacto);
            }
        }
    } else {
        cerr << "Error al recibir la información del usuario." << endl;
    }
}

// Función para Desonectar al usuario
void disconnect(int client_fd){
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

// Función para Recibir mensajes desde el servidor
void recibirMensajes(int client_fd) {
    char buffer[1024];
    int bytes_recibidos = recv(client_fd, buffer, sizeof(buffer), 0);
    
    // Verificar si se recibió algún dato
    if (bytes_recibidos <= 0) {
        if (bytes_recibidos == 0) {
            cout << "El servidor cerró la conexión." << endl;
        } else {
            cerr << "Error al recibir mensaje. Código de error: " << errno << endl;
        }
        exit(-1); // Termina la conexión si no se recibe respuesta
    }

    buffer[bytes_recibidos] = '\0'; // Asegura que la cadena esté bien terminada
    cout << "Mensaje recibido: " << buffer << endl;
}


// Función para enviar mensajes al servidor
void enviarMensaje(int client_fd, const string& correo_destino, const string& mensaje) {
    string comando = "MSG " + correo_destino + " " + mensaje;
    
    // Enviar el comando al servidor
    if (send(client_fd, comando.c_str(), comando.length(), 0) == -1) {
        cerr << "Error al enviar el mensaje." << endl;
        return;
    } 

    cout << "Mensaje enviado a " << correo_destino << " correctamente." << endl;

    // Recibir respuesta del servidor
    char buffer[1024] = {0};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Asegurar que sea una cadena válida
        cout << "Respuesta del servidor: " << buffer << endl;
    } else {
        cerr << "No se recibió confirmación del servidor." << endl;
    }
}

// Interfaz pos-ingreso
void interfazAutenticado(int client_fd) {
    int opcion;
    fd_set read_fds;
    int max_fd = client_fd; // Definir el descriptor máximo

    cout << "\nHola, " << usuario_autenticado.nombre << "!" << endl;

    cout << "\nElija una opción: \n";
    cout << "1. Agregar Contacto\n";
    cout << "2. Mostrar Contactos\n";
    cout << "3. Enviar Mensaje\n";
    cout << "4. Desconectar\n";
    cout << "Opción: ";
    cout.flush(); // Asegura que se imprima inmediatamente
    
    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds); // Monitorear el socket
        FD_SET(STDIN_FILENO, &read_fds); // Monitorear entrada del usuario
        
        select(max_fd + 1, &read_fds, NULL, NULL, NULL); // Esperar eventos
    
        if (FD_ISSET(client_fd, &read_fds)) {
            // Recibir mensaje del servidor si hay mensajes nuevos
            recibirMensajes(client_fd);
        }
    
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            string input;
            getline(cin, input); // Leer la línea completa del usuario

            if (input.empty()) {
                continue; // Evita procesar entradas vacías
            }
    
            stringstream ss(input);
            int opcion;
            ss >> opcion;
    
            if (ss.fail()) {  // Si la conversión falló, limpiar cin
                cout << "Entrada no válida. Intente nuevamente." << endl;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
                continue;
            }
    
            if (opcion == 1) {
                agregar_contacto_func(client_fd);
            } else if (opcion == 2) {
                mostrar_contactos();
            } else if (opcion == 3) { // Enviar Mensaje
                string correo, mensaje;
                cout << "Ingrese el correo del destinatario: ";
                cin >> correo;
                cin.ignore(); // Limpiar buffer de salto de línea
                cout << "Escriba su mensaje: ";
                getline(cin, mensaje); // Leer mensaje completo
                enviarMensaje(client_fd, correo, mensaje);
            } else if (opcion == 4) { // Desconectar
                disconnect(client_fd);
                break; // Salir del ciclo si se desconecta
            } else {
                cout << "Opción no válida. Intente nuevamente." << endl;
            }

            // Limpiar la consola para la siguiente interacción
            cout << "\nElija una opción: \n";
            cout << "1. Agregar Contacto\n";
            cout << "2. Mostrar Contactos\n";
            cout << "3. Enviar Mensaje\n";
            cout << "4. Desconectar\n";
            cout << "Opción: ";
            cout.flush();
        }
        usleep(100000);  // 100 ms
    }
}


// Función para registrar nuevo usuario
void registrarse(string nombre, string apellido, string correo, string contrasena, int client_fd){
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
        string respuesta(buffer);
        cout << "Respuesta del servidor: " << respuesta << endl;
        if (respuesta.find("Registro exitoso") != string::npos) {
            // Si el registro es exitoso, buscar al usuario en el servidor
            // Obtener los datos del usuario
            string comando_getuser = "GETUSER " + correo;
            send(client_fd, comando_getuser.c_str(), comando_getuser.length(), 0);

            // Recibir la respuesta del servidor
            bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                string respuesta_usuario(buffer);
                if (respuesta_usuario.find("ERROR") == string::npos) {
                    // Parsear la respuesta del servidor
                    istringstream ss(respuesta_usuario);
                    string temp, nombre, apellido, correo;
                    ss >> temp >> nombre;     // Ignorar "User"
                    ss >> temp >> apellido;   // Ignorar "Apellido"
                    ss >> temp >> correo;     // Ignorar "Correo"
                    // Guardar la información en la variable global usuario_autenticado
                    usuario_autenticado.nombre = nombre;
                    usuario_autenticado.apellido = apellido;
                    usuario_autenticado.correo = correo;
                    usuario_autenticado.contrasena = contrasena;

                    cout << "Datos de usuario guardados correctamente." << endl;
                    // Ahora que ya tenemos la información, podemos ir a la interfaz autenticada
                    interfazAutenticado(client_fd);
                } else {
                    cout << "Error al obtener la información del usuario." << endl;
                }
            } else {
                cerr << "Error al recibir la información del usuario." << endl;
            }
        }
    } else {
        cerr << "Error al recibir la respuesta del servidor" << endl;
    }
}

// Función para iniciar sesión
void iniciarSesion(string correo, string contrasena, int client_fd) {
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
        string respuesta(buffer);
        cout << "Respuesta del servidor: " << respuesta << endl;
        
        // Si el inicio de sesión fue exitoso
        if (respuesta.find("Login exitoso") != string::npos) {
            // Ahora obtenemos los datos del usuario autenticado
            string comando_getuser = "GETUSER " + correo;
            send(client_fd, comando_getuser.c_str(), comando_getuser.length(), 0);

            // Recibir la respuesta del servidor
            bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received > 0) {
                string respuesta_usuario(buffer);
                if (respuesta_usuario.find("ERROR") == string::npos) {
                    // Parsear la respuesta del servidor
                    istringstream ss(respuesta_usuario);
                    string temp, nombre, apellido, correo;
                    ss >> temp >> nombre;     // Ignorar "User"
                    ss >> temp >> apellido;   // Ignorar "Apellido"
                    ss >> temp >> correo;     // Ignorar "Correo"
                    
                    // Guardar la información en la variable global usuario_autenticado
                    usuario_autenticado.nombre = nombre;
                    usuario_autenticado.apellido = apellido;
                    usuario_autenticado.correo = correo;
                    usuario_autenticado.contrasena = contrasena;

                    cout << "Datos de usuario guardados correctamente." << endl;
                    // Ahora que ya tenemos la información, podemos ir a la interfaz autenticada
                    interfazAutenticado(client_fd);
                } else {
                    cout << "Error al obtener la información del usuario." << endl;
                }
            } else {
                cerr << "Error al recibir la información del usuario." << endl;
            }
        }
    } else {
        cerr << "Error al recibir la respuesta del servidor" << endl;
    }
}

// Interfaz Inicial
void interfazInicial(int client_fd){
    int opcion;
    string nombre, apellido, correo, contrasena;
    while (true) {
        cout << "\nElija una opción: \n";
        cout << "1. Registrarse\n";
        cout << "2. Iniciar sesión\n";
        cout << "3. Desconectar\n";
        cout << "Opción: ";
        cin >> opcion;
        if (opcion == 1) {
            // Registro de usuario
            registrarse(nombre, apellido, correo, contrasena, client_fd);
        } 
        else if (opcion == 2) {
            // Iniciar sesión
            iniciarSesion(correo, contrasena, client_fd);
        } 
        else if (opcion == 3) {
            // Desconectar
            disconnect(client_fd);
        }
        else {
            cout << "Opción no válida. Intente nuevamente." << endl;
        }
    }
}

// Función que inicia la conxión con el servidor
int startConnection(){
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
    interfazInicial(client_fd);

    close(client_fd); 
    return 0;
}

int main() {
    startConnection();
    return 0;
}