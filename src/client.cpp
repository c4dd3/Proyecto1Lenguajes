#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <map>
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

struct MensajeChat {
    string mensaje;  // Contenido del mensaje
    int tipo;        // 0 para enviado por el usuario, 1 para recibido del contacto

    // Constructor para inicializar el mensaje y el tipo
    MensajeChat(const string& msg, int t) : mensaje(msg), tipo(t) {}
};
map<string, vector<MensajeChat>> chatsPorContacto;

// Función para añadir un mensaje al chat según contacto
void agregarMensajeAlChat(const string& correoContacto, const string& mensaje, int tipo) {
    // Crear un nuevo mensaje de chat
    MensajeChat nuevoMensaje(mensaje, tipo);

    // Verificar si el contacto ya existe en el mapa
    if (chatsPorContacto.find(correoContacto) != chatsPorContacto.end()) {
        // Si existe, añadir el nuevo mensaje al vector de mensajes del contacto
        chatsPorContacto[correoContacto].push_back(nuevoMensaje);
    } else {
        // Si no existe, crear una nueva entrada en el mapa con ese correo
        vector<MensajeChat> nuevoChat = { nuevoMensaje };
        chatsPorContacto[correoContacto] = nuevoChat;
    }

    // Opcional: Mostrar el mensaje agregado
    cout << "Mensaje añadido al chat de " << correoContacto << ": " << mensaje << endl;
}


// Función para guardar los chats del usuario en un archivo txt único
void guardarChatsEnArchivo() {
    // Nombre del archivo donde guardamos todos los chats
    string nombreArchivo = usuario_autenticado.correo + "-chats.txt";
    
    // Abrir el archivo en modo de escritura
    ofstream archivo(nombreArchivo, ios::out);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo para guardar los chats." << endl;
        return;
    }

    // Recorrer todos los contactos y guardar sus mensajes
    for (const auto& chat : chatsPorContacto) {
        const string& correoContacto = chat.first; // Correo del contacto
        const vector<MensajeChat>& mensajes = chat.second; // Vector de mensajes

        // Escribir el nombre del contacto como título
        archivo << "Contacto: " << correoContacto << endl;

        // Guardar los mensajes en el archivo
        for (const auto& mensaje : mensajes) {
            archivo << mensaje.tipo << ";" << mensaje.mensaje << endl; // Guardar tipo y mensaje
        }

        archivo << "---- Fin de chat con " << correoContacto << " ----" << endl;
    }

    // Cerrar el archivo
    archivo.close();
    cout << "Todos los chats han sido guardados en: " << nombreArchivo << endl;
}

// Función para cargar los chats del usuario desde un archivo txt único
void cargarChatsDesdeArchivo() {
    // Nombre del archivo donde guardamos todos los chats
    string nombreArchivo = usuario_autenticado.correo + "-chats.txt";

    // Abrir el archivo en modo de lectura
    ifstream archivo(nombreArchivo, ios::in);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo de chats." << endl;
        return;
    }

    string linea;
    string correoContacto;
    vector<MensajeChat> mensajes;

    // Leer cada línea del archivo
    while (getline(archivo, linea)) {
        // Verificar si la línea indica el inicio de un chat con un contacto
        if (linea.find("Contacto: ") != string::npos) {
            if (!correoContacto.empty()) {
                // Si ya teníamos mensajes previos, guardarlos en el mapa
                chatsPorContacto[correoContacto] = mensajes;
            }

            // Extraer el correo del contacto
            correoContacto = linea.substr(10);  // "Contacto: " tiene 10 caracteres
            mensajes.clear();  // Limpiar los mensajes anteriores
        }
        else if (linea.find("---- Fin de chat con") != string::npos) {
            // Fin del chat con un contacto, guardar los mensajes
            chatsPorContacto[correoContacto] = mensajes;
        } 
        else {
            // Parsear el tipo y mensaje
            string tipoStr, mensaje;
            stringstream ss(linea);
            getline(ss, tipoStr, ';'); // Leer tipo (0 o 1)
            getline(ss, mensaje);     // Leer mensaje

            int tipo = stoi(tipoStr);  // Convertir tipo a int
            mensajes.push_back(MensajeChat(mensaje, tipo));  // Agregar el mensaje
        }
    }

    // Asegurarse de guardar los últimos mensajes leídos
    if (!correoContacto.empty()) {
        chatsPorContacto[correoContacto] = mensajes;
    }

    // Cerrar el archivo
    archivo.close();
    cout << "Chats cargados desde el archivo: " << nombreArchivo << endl;
}


//
void imprimirChat(const string& correoContacto) {
    // Verificar si el contacto tiene mensajes en el mapa
    if (chatsPorContacto.find(correoContacto) == chatsPorContacto.end()) {
        cout << "No se ha encontrado un chat con el contacto: " << correoContacto << endl;
        return;
    }

    // Obtener los mensajes del contacto
    vector<MensajeChat> mensajes = chatsPorContacto[correoContacto];

    cout << "\nChat con " << correoContacto << ":\n";
    
    // Recorrer y mostrar los mensajes
    for (const auto& mensaje : mensajes) {
        if (mensaje.tipo == 0) {
            cout << "Tú: " << mensaje.mensaje << endl; // Mensaje enviado por el usuario
        } else {
            cout << correoContacto << ": " << mensaje.mensaje << endl; // Mensaje recibido del contacto
        }
    }
}



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

// Función para guardar los contactos en un txt personal del usuario
void guardarContactos() {
    // Crear nombre del archivo con el correo del usuario autenticado
    string nombreArchivo = usuario_autenticado.correo + "-contactos.txt";

    // Abrir archivo en modo de escritura
    ofstream archivo(nombreArchivo);

    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo para guardar los contactos." << endl;
        return;
    }

    for (const auto& contacto : lista_contactos) {
        archivo << contacto.nombre << "," 
                << contacto.apellido << "," 
                << contacto.correo << "\n";
    }

    archivo.close();
    cout << "Contactos guardados correctamente en " << nombreArchivo << endl;
}

// Función para cargar los contactos del usuario del txt personalizado
void cargarContactos() {
    // Crear nombre del archivo con el correo del usuario autenticado
    string nombreArchivo = usuario_autenticado.correo + "-contactos.txt";

    // Abrir archivo en modo de lectura
    ifstream archivo(nombreArchivo);

    if (!archivo.is_open()) {
        cout << "No hay contactos guardados aún para este usuario." << endl;
        return;
    }

    lista_contactos.clear(); // Limpiar lista antes de cargar

    string linea;
    while (getline(archivo, linea)) {
        stringstream ss(linea);
        string nombre, apellido, correo;

        if (getline(ss, nombre, ',') && 
            getline(ss, apellido, ',') && 
            getline(ss, correo)) {
            lista_contactos.push_back({nombre, apellido, correo});
        }
    }

    archivo.close();
    cout << "Contactos cargados correctamente desde " << nombreArchivo << endl;
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

    // Separar el mensaje en correo y contenido
    string mensaje_completo(buffer);
    size_t primer_espacio = mensaje_completo.find(':');
    
    if (primer_espacio != string::npos) {
        string correo_emisor = mensaje_completo.substr(0, primer_espacio); // Correo del emisor
        string mensaje = mensaje_completo.substr(primer_espacio + 2); // Mensaje (se omite el espacio después del ':')
        
        // Imprimir el correo y el mensaje por separado
        cout << "Mensaje de " << correo_emisor << ": " << mensaje << endl;
    } else {
        cout << "Formato del mensaje recibido incorrecto." << endl;
    }
}

// Función para enviar mensajes al servidor
void enviarMensaje(int client_fd, const string& correo_destino, const string& mensaje) {
    string comando = "MSG " + correo_destino + " " + mensaje;

    // Enviar el comando al servidor
    if (send(client_fd, comando.c_str(), comando.length(), 0) == -1) {
        cerr << "Error al enviar el mensaje." << endl;
        return;
    }

    cout << "Intentando enviar mensaje a " << correo_destino << endl;

    // Recibir respuesta del servidor
    char buffer[1024] = {0};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Asegurar que sea una cadena válida
        cout << "Respuesta del servidor: " << buffer << endl;
        // Verificar si la respuesta del servidor es un éxito
        if (string(buffer) == "Mensaje enviado correctamente.\n") {
            cout << "El mensaje fue enviado correctamente al contacto." << endl;
            // Si el mensaje fue enviado correctamente, agregarlo al chat
            agregarMensajeAlChat(correo_destino, mensaje, 0);  // 0 indica que es un mensaje enviado por el usuario
        } else {
            cout << "Hubo un error al enviar el mensaje: " << buffer << endl;
        }
    } else if (bytes_received == 0) {
        cout << "El servidor cerró la conexión." << endl;
    } else {
        cerr << "Error al recibir respuesta del servidor. Código de error: " << errno << endl;
    }
}

// Función que revisa si hay un nuevo mensaje o no
void checkMessages(int client_socket) {
    // Enviar el comando "CHECKMSG" al servidor
    string comando = "CHECKMSG";
    if (send(client_socket, comando.c_str(), comando.length(), 0) == -1) {
        cerr << "Error al enviar el comando al servidor." << endl;
        return;
    }

    // Buffer para recibir la respuesta del servidor
    char buffer[1024] = {0};
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            cerr << "El servidor cerró la conexión." << endl;
        } else {
            cerr << "Error al recibir la respuesta del servidor. Código de error: " << errno << endl;
        }
        return;
    }

    // Convertir la respuesta a un string
    string respuesta(buffer, bytes_received);

    // Verificar si hay un mensaje nuevo
    if (respuesta.find("ERROR") != string::npos) {
        cerr << "Error al revisar mensajes: " << respuesta << endl;
    } else {
        // Si no hay error, imprimir la respuesta del servidor
        cout << "Respuesta del servidor: " << respuesta << endl;

        // Procesar el mensaje recibido
        // Asumimos que la respuesta está en el formato:
        // "De: correoEmisor\nMensaje: contenidoMensaje"
        
        // Extraer correoEmisor
        size_t posDe = respuesta.find("De: ");
        size_t posMensaje = respuesta.find("\nMensaje: ");
        
        if (posDe != string::npos && posMensaje != string::npos) {
            string correoEmisor = respuesta.substr(posDe + 4, posMensaje - posDe - 4);
            string mensajeContenido = respuesta.substr(posMensaje + 9);  // El contenido después de "Mensaje: "

            // Mostrar el mensaje
            cout << "Nuevo mensaje de " << correoEmisor << ": " << mensajeContenido << endl;

            // Agregar el mensaje al chat del contacto
            agregarMensajeAlChat(correoEmisor, mensajeContenido, 1); // 1 indica que es un mensaje recibido del contacto
        } else {
            cerr << "Formato de mensaje recibido incorrecto: " << respuesta << endl;
        }
    }
}

// Interfaz post-ingreso (después de iniciar sesión)
void interfazAutenticado(int client_fd) {
    cout << "\nHola, " << usuario_autenticado.nombre << "!" << endl;
    int opcion;
    cargarContactos();
    cargarChatsDesdeArchivo();
    
    while (true) {
        // Mostrar menú
        cout << "\nElija una opción: \n";
        cout << "1. Agregar Contacto\n";
        cout << "2. Mostrar Contactos\n";
        cout << "3. Enviar Mensaje\n";
        cout << "4. Desconectar\n";
        cout << "5. Buscar Mensaje\n";
        cout << "6. Ver Chat con Contacto\n"; // Nueva opción
        cout << "Opción: ";
        cout.flush();

        cin >> opcion;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar buffer

        if (opcion == 1) {
            agregar_contacto_func(client_fd);
        } else if (opcion == 2) {
            mostrar_contactos();
        } else if (opcion == 3) {
            string correo, mensaje;
            cout << "Ingrese el correo del destinatario: ";
            getline(cin, correo);
            cout << "Escriba su mensaje: ";
            getline(cin, mensaje);
            enviarMensaje(client_fd, correo, mensaje);
        } else if (opcion == 4) {
            guardarContactos();
            guardarChatsEnArchivo();
            disconnect(client_fd);
            break;
        } else if (opcion == 5) {
            checkMessages(client_fd);
        } else if (opcion == 6) {
            string correoContacto;
            cout << "Ingrese el correo del contacto para ver el chat: ";
            getline(cin, correoContacto);
            imprimirChat(correoContacto); // Llamada a la función para imprimir el chat
        } else {
            cout << "Opción no válida. Intente nuevamente." << endl;
        }
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