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

#define MAX_CONNECTIONS 100
#define DEFAULT_PORT 8080
#define SHM_KEY 1234

using namespace std;

struct Usuario {
    char nombre[50];
    char apellido[50];
    char correo[50];
    char contrasena[50];
    int socket_cliente;                 // Nuevo campo para almacenar el socket del cliente
    sem_t *sem_socket;                  // Semáforo para el acceso al socket del usuario
    char ip_cliente[INET_ADDRSTRLEN];   // Almacena la IP del cliente
    bool conectado;                     // Estado de conexión del usuario (true = conectado, false = desconectado)
};


struct SharedData {
    Usuario lista_usuarios[100];
    int user_count;
};

SharedData *shared_data;
sem_t *sem;                 // Semáforo general para acceder a la lista de usuarios


int server_fd;

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

// Función para guardar la lista de usuarios en un archivo
void guardar_usuarios() {
    ofstream user_file("users.txt");

    if (!user_file.is_open()) {
        cerr << "Error al abrir el archivo de usuarios" << endl;
        return;
    }

    // Recorrer la lista de usuarios y guardarlos
    for (int i = 0; i < shared_data->user_count; ++i) {
        Usuario& usuario = shared_data->lista_usuarios[i];
        // Guardamos el usuario, incluyendo la IP y el estado de conexión, excepto el socket_cliente, asignando un valor nulo (-1)
        user_file << usuario.nombre << " "
                  << usuario.apellido << " "
                  << usuario.correo << " "
                  << usuario.contrasena << " "
                  << usuario.ip_cliente << " "
                  << usuario.conectado << " "
                  << -1 << endl;  // -1 como valor nulo para el socket_cliente
    }

    user_file.close();
    cout << "Usuarios guardados correctamente en users.txt" << endl;
}

// Variable de control para salir del ciclo principal
bool server_running = true;
// Función de manejo de señales (para cerrar el servidor de forma controlada)
void shutdown_server(int signum) {
    cout << "Se recibió la señal " << signum << ". Cerrando el servidor..." << endl;
    // Llamar a la función para guardar los usuarios antes de cerrar
    guardar_usuarios();  // Descomentarlo si deseas guardar antes de cerrar
    server_running = false;
    close(server_fd);  
}

// Función para cargar usuarios desde el archivo
void cargar_usuarios() {
    ifstream user_file("users.txt");

    // Si no se puede abrir el archivo, lo creamos (vacío)
    if (!user_file.is_open()) {
        cout << "El archivo de usuarios no existe, creando archivo vacío..." << endl;
        ofstream new_file("users.txt");
        if (!new_file.is_open()) {
            cerr << "Error al crear el archivo de usuarios" << endl;
            return;
        }
        new_file.close(); // Cerrar el archivo recién creado
        cout << "Archivo de usuarios creado exitosamente." << endl;
        return;
    }

    // Variables para almacenar los datos temporales
    char nombre[50], apellido[50], correo[50], contrasena[50], ip_cliente[INET_ADDRSTRLEN];
    bool conectado;
    int socket_cliente;

    // Leer cada línea del archivo y cargar los datos de los usuarios
    while (user_file >> nombre >> apellido >> correo >> contrasena >> ip_cliente >> conectado >> socket_cliente) {
        // Verificamos que los campos leídos sean válidos
        if (user_file.fail()) {
            cerr << "Error al leer datos de usuario del archivo." << endl;
            break;
        }

        // Asignar los datos a la lista de usuarios
        strcpy(shared_data->lista_usuarios[shared_data->user_count].nombre, nombre);
        strcpy(shared_data->lista_usuarios[shared_data->user_count].apellido, apellido);
        strcpy(shared_data->lista_usuarios[shared_data->user_count].correo, correo);
        strcpy(shared_data->lista_usuarios[shared_data->user_count].contrasena, contrasena);

        // Verificamos que la IP sea válida. Si no lo es, asignamos una IP por defecto (por ejemplo, "0.0.0.0")
        if (strlen(ip_cliente) == 0) {
            strcpy(shared_data->lista_usuarios[shared_data->user_count].ip_cliente, "0.0.0.0");
        } else {
            strcpy(shared_data->lista_usuarios[shared_data->user_count].ip_cliente, ip_cliente);
        }

        // Si no se ha especificado el estado de conexión, lo dejamos como falso (no conectado)
        shared_data->lista_usuarios[shared_data->user_count].conectado = conectado ? true : false;

        // Asignar un valor nulo al socket_cliente (ya que el socket no es válido en este momento)
        shared_data->lista_usuarios[shared_data->user_count].socket_cliente = -1;  // -1 indica un socket no válido

        shared_data->user_count++;
    }

    // Cerrar el archivo
    user_file.close();
}

// Función para registrar un nuevo usuario
void register_user(int client_socket, const string& comando) {
    // Extraer los datos del usuario desde el comando
    stringstream ss(comando.substr(8));  // Extraer todo después de "REGISTER "
    string nombre, apellido, correo, contrasena;
    ss >> nombre >> apellido >> correo >> contrasena;

    // Comprobamos si el correo ya está registrado
    sem_wait(sem);  // Bloquear semáforo para proteger la lista de usuarios
    bool usuario_existe = false;
    for (int i = 0; i < shared_data->user_count; ++i) {
        if (shared_data->lista_usuarios[i].correo == correo) {  // Comprobación por correo
            usuario_existe = true;
            break;
        }
    }

    if (usuario_existe) {
        const char* error_msg = "El correo ya está registrado.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
    } else {
        // Registrar el nuevo usuario
        Usuario nuevo_usuario;
        strcpy(nuevo_usuario.nombre, nombre.c_str());
        strcpy(nuevo_usuario.apellido, apellido.c_str());
        strcpy(nuevo_usuario.correo, correo.c_str());
        strcpy(nuevo_usuario.contrasena, contrasena.c_str());

        // Obtener la IP del cliente
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        getpeername(client_socket, (struct sockaddr*)&addr, &addr_size);
        string ip_cliente = inet_ntoa(addr.sin_addr);

        // Guardar la IP en la estructura del usuario
        strncpy(nuevo_usuario.ip_cliente, ip_cliente.c_str(), sizeof(nuevo_usuario.ip_cliente) - 1);
        nuevo_usuario.ip_cliente[sizeof(nuevo_usuario.ip_cliente) - 1] = '\0';

        // Estado de conexión
        nuevo_usuario.conectado = true;
        nuevo_usuario.socket_cliente = client_socket;

        // Añadir el nuevo usuario a la lista
        shared_data->lista_usuarios[shared_data->user_count] = nuevo_usuario;
        shared_data->user_count++;

        // Guardar los usuarios en el archivo
        guardar_usuarios();

        const char* success_msg = "Registro exitoso y conexión establecida.\n";
        send(client_socket, success_msg, strlen(success_msg), 0);
    }
    sem_post(sem);  // Liberar semáforo
}

// Función para manejar el login de un usuario
void login_user(int client_socket, const string& comando) {
    // Extraer los datos de login
    stringstream ss(comando.substr(6));  // Extraer todo después de "LOGIN "
    string correo, contrasena;
    ss >> correo >> contrasena;

    sem_wait(sem);  // Bloquear semáforo para proteger la lista de usuarios
    bool usuario_valido = false;

    for (int i = 0; i < shared_data->user_count; ++i) {
        if (shared_data->lista_usuarios[i].correo == correo && shared_data->lista_usuarios[i].contrasena == contrasena) {
            usuario_valido = true;

            // Cambiar estado a "conectado"
            shared_data->lista_usuarios[i].conectado = true;
            shared_data->lista_usuarios[i].socket_cliente = client_socket;
            // Obtener la IP del cliente
            struct sockaddr_in addr;
            socklen_t addr_size = sizeof(struct sockaddr_in);
            getpeername(client_socket, (struct sockaddr*)&addr, &addr_size);
            string ip_cliente = inet_ntoa(addr.sin_addr);

            // Almacenar la IP
            strncpy(shared_data->lista_usuarios[i].ip_cliente, ip_cliente.c_str(), sizeof(shared_data->lista_usuarios[i].ip_cliente) - 1);
            shared_data->lista_usuarios[i].ip_cliente[sizeof(shared_data->lista_usuarios[i].ip_cliente) - 1] = '\0';

            break;
        }
    }

    if (usuario_valido) {
        const char* success_msg = "Login exitoso.\n";
        send(client_socket, success_msg, strlen(success_msg), 0);
    } else {
        const char* error_msg = "Credenciales incorrectas.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
    }

    sem_post(sem);  // Liberar semáforo
}

// Función para manejar la desconexión del usuario
void disconnect_user(int client_socket) {
    sem_wait(sem);  // Bloquear semáforo para proteger la lista de usuarios

    bool usuario_encontrado = false;
    for (int i = 0; i < shared_data->user_count; ++i) {
        if (shared_data->lista_usuarios[i].socket_cliente == client_socket) {
            // Cambiar estado de conexión
            shared_data->lista_usuarios[i].conectado = false;
            shared_data->lista_usuarios[i].socket_cliente = -1;
            strcpy(shared_data->lista_usuarios[i].ip_cliente, "0.0.0.0");  // Resetear la IP

            usuario_encontrado = true;
            break;
        }
    }

    sem_post(sem);  // Liberar semáforo

    if (usuario_encontrado) {
        const char* disconnect_msg = "Desconexión exitosa. Adiós.\n";
        send(client_socket, disconnect_msg, strlen(disconnect_msg), 0);
    }

    close(client_socket);  // Cerrar el socket del cliente
    cout << "Cerrando proceso hijo" << endl;
    exit(0);  // Terminar el proceso hijo
}

// Función para obtener la información de un usuario
void get_user_info(int client_socket, const string &comando) {
    // Extraer el correo del usuario desde el comando
    istringstream ss(comando);
    string command, correo;
    ss >> command >> correo;

    if (correo.empty()) {
        const char *error_msg = "ERROR: Debe proporcionar un correo para obtener la información.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }

    sem_wait(sem);  // Bloquear semáforo para proteger la lista de usuarios

    bool found = false;
    string user_info;

    for (int i = 0; i < shared_data->user_count; ++i) {
        if (shared_data->lista_usuarios[i].correo == correo) {
            found = true;
            user_info = "Nombre: " + string(shared_data->lista_usuarios[i].nombre) +
                        "\nApellido: " + string(shared_data->lista_usuarios[i].apellido) +
                        "\nCorreo: " + string(shared_data->lista_usuarios[i].correo);
            break;
        }
    }

    sem_post(sem);  // Liberar semáforo

    if (found) {
        send(client_socket, user_info.c_str(), user_info.length(), 0);
    } else {
        const char *not_found_msg = "ERROR: Usuario no encontrado.\n";
        send(client_socket, not_found_msg, strlen(not_found_msg), 0);
    }
}

// Función para recibir y enviar mensajes // TODO
void procesarMensaje(int client_socket, const string& comando) {
    // El comando esperado es "MSG <correo> <mensaje>"
    size_t primer_espacio = comando.find(' ');
    size_t segundo_espacio = comando.find(' ', primer_espacio + 1);

    if (primer_espacio == string::npos || segundo_espacio == string::npos) {
        const char* error_msg = "Formato incorrecto. Use: MSG <correo> <mensaje>\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }

    string correo_destino = comando.substr(primer_espacio + 1, segundo_espacio - primer_espacio - 1);
    string mensaje = comando.substr(segundo_espacio + 1);

    cout << "Mensaje recibido para " << correo_destino << ": " << mensaje << endl;
    cout << "Intentando enviar mensaje..." << endl;

    // Buscar el correo del emisor
    string correo_emisor = ""; // Variable para el correo del emisor
    for (int i = 0; i < shared_data->user_count; ++i) {
        if (shared_data->lista_usuarios[i].socket_cliente == client_socket) {
            correo_emisor = shared_data->lista_usuarios[i].correo;
            break;
        }
    }

    // Si no se encuentra el emisor, terminamos el procesamiento
    if (correo_emisor.empty()) {
        const char* error_msg = "No se encontró el correo del emisor.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }

    // Buscar el usuario en la lista compartida
    int destinatario_socket = -1;
    sem_t* sem_destinatario = nullptr;
    bool encontrado = false;

    for (int i = 0; i < shared_data->user_count; ++i) {
        if (correo_destino == shared_data->lista_usuarios[i].correo) {
            encontrado = true;
            if (shared_data->lista_usuarios[i].conectado) {
                destinatario_socket = shared_data->lista_usuarios[i].socket_cliente;
                sem_destinatario = shared_data->lista_usuarios[i].sem_socket;
            }
            break;
        }
    }

    if (!encontrado) {
        cout << "Usuario no encontrado." << endl;
        const char* error_msg = "Usuario no encontrado.\n";
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }

    if (destinatario_socket == -1) {
        cout << "El usuario está desconectado." << endl;
        const char* offline_msg = "El usuario está desconectado.\n";
        send(client_socket, offline_msg, strlen(offline_msg), 0);
        return;
    }
    // Mensaje de confirmación
    const char* success_msg = "Mensaje enviado correctamente.\n";
    send(client_socket, success_msg, strlen(success_msg), 0);

    cout << "Socket Emisor:" << correo_emisor <<" | Socket Receptor:" << destinatario_socket << endl;
    string mensaje_final = "Mensaje de " + correo_emisor + ": " + mensaje + "\n";
    cout << "Enviando" << endl;
    // Enviar el mensaje al destinatario con control de concurrencia
    sem_wait(sem_destinatario); // Bloquear el semáforo del destinatario
    send(destinatario_socket, mensaje_final.c_str(), mensaje_final.length(), 0);
    sem_post(sem_destinatario); // Liberar el semáforo
}

// Función para manejar la conexión con un cliente
void handle_client(int client_socket) {
    // Enviar mensaje de confirmación de conexión al cliente
    const char* confirmation_msg = "Conexión establecida correctamente. Bienvenido al servidor de mensajería.\n";
    send(client_socket, confirmation_msg, strlen(confirmation_msg), 0);

    // Bucle para recibir múltiples comandos del cliente
    while (true) {
        // Recibir el comando del cliente
        char buffer[1024];
        int bytes_received;
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        // Manejo de errores de fallo de transmisión o desconexión inesperada del cliente
        if (bytes_received <= 0) {
            cerr << "Error al recibir el comando o cliente desconectado." << endl;
            break;
        }
        buffer[bytes_received] = '\0';
        string comando(buffer);
        //---------Comandos que procesa el servidor---------//
        // Si el comando es "REGISTER"
        if (comando.substr(0, 8) == "REGISTER") {
            register_user(client_socket, comando);
        }
        // Si el comando es "LOGIN"
        else if (comando.substr(0, 5) == "LOGIN") {
            login_user(client_socket, comando);
        }
        // Si el comando es "DISCONNECT"
        else if (comando.substr(0, 10) == "DISCONNECT") {
            disconnect_user(client_socket);
        }
        // Si es el comando "GETUSER"
        else if (comando.substr(0, 7) == "GETUSER"){
            get_user_info(client_socket, comando);
        } 
        else if (comando.substr(0, 3) == "MSG") {
            procesarMensaje(client_socket, comando);
        } else {
            const char* error_msg = "Comando no reconocido.\n";
            send(client_socket, error_msg, strlen(error_msg), 0);
        }
        //--------------------------------------------------//
    }

    // Cerrar la conexión después de procesar todos los comandos
    close(client_socket);
}

int main() {
    // Inicialización de variables y estructuras
    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Lectura de la configuración
    int port = read_config();

    // Creación de memoria compartida para los subprocesos
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    shared_data = (SharedData*)shmat(shmid, nullptr, 0);
    shared_data->user_count = 0;

    // Cargar los usuarios desde el archivo
    cargar_usuarios();

    // Creación de un semáforo para sincronización
    sem = sem_open("/user_semaphore", O_CREAT, 0644, 1);

    // Creación del socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Error al crear el socket" << endl;
        return -1;
    }

    // Configuración de la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Enlazar el socket del servidor con la dirección
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error al hacer bind" << endl;
        return -1;
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        cerr << "Error al escuchar por conexiones" << endl;
        return -1;
    }

    cout << "Servidor escuchando en el puerto " << port << endl;

    // Capturar la señal SIGINT (Ctrl+C) para cerrar el servidor
    signal(SIGINT, shutdown_server);

    // Ciclo principal para aceptar y manejar conexiones de clientes
    while (server_running) {
        // Aceptar la conexión de un cliente
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            cerr << "Error al aceptar la conexión" << endl;
            continue;
        }

        // Mostrar la IP del cliente
        cout << "Conexión aceptada desde IP: " << inet_ntoa(client_addr.sin_addr) << endl;

        // Crear un proceso hijo para manejar este cliente
        pid_t pid = fork();
        
        if (pid == 0) {
            // Proceso hijo
            close(server_fd);               // El hijo ya no necesita escuchar nuevas conexiones
            handle_client(client_socket);   // Manejar la comunicación con el cliente
            shmdt(shared_data);             // Desasociar la memoria compartida
            exit(0);                        // Terminar el proceso hijo después de manejar al cliente
        } else if (pid > 0) {
            // Proceso padre
            close(client_socket);           // El padre no necesita manejar este cliente, solo aceptar nuevas conexiones
        } else {
            cerr << "Error al crear el proceso hijo" << endl;
        }
    }

    // Liberar los recursos al finalizar
    cout << "Cerrando servidor..." << endl;
    shmdt(shared_data);                     // Desasociar la memoria compartida
    shmctl(shmid, IPC_RMID, nullptr);       // Liberar la memoria compartida
    sem_close(sem);                         // Cerrar el semáforo
    sem_unlink("/user_semaphore");          // Desvincular el semáforo

    exit(0);

    return 0;
}
