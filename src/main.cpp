#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>


using namespace std;

// Estructuras de usuario y contactos
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

// Función para registrar nuevo usuario
void registrarse(string nombre, string apellido, string correo, string contrasena, int client_fd){

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

// Funcion para iniciar sesion de un usuario
void iniciarSesion(string correo, string contrasena, int client_fd) {

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
                    
                    // Aqui hayq eu llamar a la interfaz de inicio...
                    cout << "Proceso de Login exitoso..." << endl;
                    //interfazAutenticado(client_fd);
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
    cout << "Mensaje añadido al chat de " << correoContacto << ": " << mensaje << endl;
}

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
class ChatWindow : public Gtk::ApplicationWindow {
    public:
        ChatWindow(int client_fd) : client_fd(client_fd) {
            set_title("SwiftTalk");
            set_default_size(800, 600);
            set_position(Gtk::WIN_POS_CENTER);
    
            // Layout principal: horizontal
            main_box.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
            add(main_box);
    
            // Lista de contactos (columna izquierda)
            contact_list.set_size_request(250);
            contact_list.set_border_width(5);
    
            // Cargar contactos desde archivo
            cargarContactos();
    
            // Llenar listbox con contactos dinámicamente
            for (const auto& contacto : lista_contactos) {
                std::string nombre_completo = contacto.nombre + " " + contacto.apellido;
                auto label = Gtk::make_managed<Gtk::Label>(nombre_completo);
                listbox_contacts.append(*label);
            }
    
            contact_list.pack_start(listbox_contacts);
            main_box.pack_start(contact_list, Gtk::PACK_SHRINK);
    
            // Área de chat (columna derecha)
            chat_area.set_border_width(5);
    
            // Botones de control
            control_buttons_box.set_orientation(Gtk::ORIENTATION_VERTICAL);
            control_buttons_box.set_spacing(10);
            control_buttons_box.set_border_width(5);
    
            btn_add_contact.set_label("Añadir contacto");
            btn_logout.set_label("Cerrar sesión");
    
            // Acciones de los botones
            btn_add_contact.signal_clicked().connect(sigc::mem_fun(*this, &ChatWindow::mostrarFormularioAgregarContacto));
            btn_logout.signal_clicked().connect(sigc::mem_fun(*this, &ChatWindow::cerrarSesion));
    
            control_buttons_box.pack_start(btn_add_contact, Gtk::PACK_SHRINK);
            control_buttons_box.pack_start(btn_logout, Gtk::PACK_SHRINK);
    
            chat_area.pack_start(control_buttons_box, Gtk::PACK_SHRINK);
            chat_text_view.set_editable(false);
            chat_text_view.set_wrap_mode(Gtk::WrapMode::WRAP_WORD);
    
            chat_area.pack_start(chat_text_view);
            chat_area.pack_start(chat_entry, Gtk::PACK_SHRINK);
            chat_area.pack_start(send_button, Gtk::PACK_SHRINK);
    
            send_button.set_label("Enviar");
    
            // Conectar el botón Enviar a la función de envío
            send_button.signal_clicked().connect(sigc::mem_fun(*this, &ChatWindow::enviarMensaje));
    
            // Conectar la selección de un contacto en la lista
            listbox_contacts.signal_row_selected().connect(sigc::mem_fun(*this, &ChatWindow::onContactoSeleccionado));
    
            main_box.pack_start(chat_area);

            // Revisar mensajes cada segundo
            Glib::signal_timeout().connect(sigc::mem_fun(*this, &ChatWindow::checkMessages), 1000);

            show_all_children();
        }
    
    private:
        int client_fd;
        std::string correo_contacto_seleccionado; // Almacenar el correo del contacto seleccionado
    
        Gtk::Box main_box{Gtk::ORIENTATION_HORIZONTAL};
        Gtk::Box contact_list{Gtk::ORIENTATION_VERTICAL};
        Gtk::ListBox listbox_contacts;
    
        Gtk::Box chat_area{Gtk::ORIENTATION_VERTICAL};
        Gtk::TextView chat_text_view;
        Gtk::Entry chat_entry;
        Gtk::Button send_button;
    
        Gtk::Box control_buttons_box;
        Gtk::Button btn_add_contact;
        Gtk::Button btn_logout;

        std::mutex mtx; // Mutex para proteger el acceso a los datos compartidos
    
        void cargarContactos() {
            std::string nombreArchivo = usuario_autenticado.correo + "-contactos.txt";
            std::ifstream archivo(nombreArchivo);
    
            if (!archivo.is_open()) {
                std::cout << "No hay contactos guardados aún para este usuario." << std::endl;
                return;
            }
    
            lista_contactos.clear();
            std::string linea;
            while (getline(archivo, linea)) {
                std::stringstream ss(linea);
                std::string nombre, apellido, correo;
                if (getline(ss, nombre, ',') && getline(ss, apellido, ',') && getline(ss, correo)) {
                    lista_contactos.push_back({nombre, apellido, correo});
                }
            }
    
            archivo.close();
            std::cout << "Contactos cargados correctamente desde " << nombreArchivo << std::endl;
        }

        void cargarChats(const std::string& correo_usuario) {
            std::string nombre_archivo = correo_usuario + "-chats.txt";
            std::ifstream archivo(nombre_archivo);

            if (!archivo.is_open()) {
                std::cerr << "No se pudo abrir el archivo de chats: " << nombre_archivo << std::endl;
                return;
            }

            std::string linea;
            std::string correo_contacto;
            std::string mensaje;
            bool es_mensaje_recibido = false;

            // Limpiar el área de chat antes de cargar nuevos mensajes
            chat_text_view.get_buffer()->set_text("");

            // Leer cada línea del archivo
            while (std::getline(archivo, linea)) {
                // Si la línea contiene el contacto, guardamos el correo del contacto
                if (linea.find("Contacto: ") == 0) {
                    correo_contacto = linea.substr(10);  // Obtener el correo del contacto
                    std::cout << "Cargando chat con: " << correo_contacto << std::endl;
                    continue;  // Continuar con la siguiente línea
                }

                // Si encontramos una línea de fin de chat, la procesamos
                if (linea.find("---- Fin de chat con ") == 0) {
                    std::cout << "Fin de chat con: " << correo_contacto << std::endl;
                    continue;  // Continuar con la siguiente línea
                }

                // Si la línea contiene un mensaje, procesamos el mensaje
                size_t pos_separador = linea.find(";");
                if (pos_separador != std::string::npos) {
                    es_mensaje_recibido = (linea[0] == '1');  // 1 indica mensaje recibido
                    mensaje = linea.substr(pos_separador + 2);  // El mensaje está después del "; "

                    // Mostrar el mensaje en el chat
                    agregarMensajeAlChat(correo_contacto, mensaje, es_mensaje_recibido ? 1 : 0);  // 1 para mensaje recibido, 0 para mensaje enviado
                }
            }

            archivo.close();
        }
    
        void mostrarFormularioAgregarContacto() {
            Gtk::Dialog dialogo("Añadir nuevo contacto", *this);
            dialogo.set_modal(true);
            dialogo.set_transient_for(*this);
    
            Gtk::Box* contenido = dialogo.get_content_area();
            Gtk::Entry entry_correo;
            entry_correo.set_placeholder_text("Correo del nuevo contacto");
    
            contenido->pack_start(entry_correo, Gtk::PACK_SHRINK);
            dialogo.add_button("Cancelar", Gtk::RESPONSE_CANCEL);
            dialogo.add_button("Guardar", Gtk::RESPONSE_OK);
    
            dialogo.show_all_children();
            int resultado = dialogo.run();
    
            if (resultado == Gtk::RESPONSE_OK) {
                std::string correo = entry_correo.get_text();
    
                // Enviar comando al servidor para obtener datos del contacto
                std::string comando = "GETUSER " + correo;
                send(client_fd, comando.c_str(), comando.length(), 0);
    
                char buffer[1024] = {0};
                int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    
                if (bytes_received > 0) {
                    std::string respuesta(buffer);
    
                    if (respuesta.find("ERROR") != std::string::npos) {
                        Gtk::MessageDialog dialog_error(*this, "Usuario no encontrado.", false, Gtk::MESSAGE_ERROR);
                        dialog_error.run();
                    } else {
                        std::istringstream ss(respuesta);
                        std::string temp, nombre, apellido, correo_response;
                        ss >> temp >> nombre;
                        ss >> temp >> apellido;
                        ss >> temp >> correo_response;
    
                        Contacto nuevo_contacto = {nombre, apellido, correo_response};
    
                        // Validar si ya existe
                        for (const auto &c : lista_contactos) {
                            if (c.correo == nuevo_contacto.correo) {
                                Gtk::MessageDialog ya_existe(*this, "El contacto ya está en la lista.", false, Gtk::MESSAGE_WARNING);
                                ya_existe.run();
                                return;
                            }
                        }
    
                        // Agregar a lista en memoria
                        lista_contactos.push_back(nuevo_contacto);
    
                        // Agregar a archivo
                        std::string archivoNombre = usuario_autenticado.correo + "-contactos.txt";
                        std::ofstream archivo(archivoNombre, std::ios::app);
                        if (archivo.is_open()) {
                            archivo << nombre << "," << apellido << "," << correo_response << std::endl;
                            archivo.close();
                        }
    
                        // Agregar a la interfaz gráfica
                        std::string etiqueta = nombre + " " + apellido;
                        listbox_contacts.append(*Gtk::make_managed<Gtk::Label>(etiqueta));
                        listbox_contacts.show_all_children();
                    }
                } else {
                    Gtk::MessageDialog dialog_error(*this, "Error al recibir respuesta del servidor.", false, Gtk::MESSAGE_ERROR);
                    dialog_error.run();
                }
            }
        }
    
    void cerrarSesion() {
        Gtk::MessageDialog confirmacion(*this, "¿Estás seguro de que quieres cerrar sesión?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
        int respuesta = confirmacion.run();

        if (respuesta == Gtk::RESPONSE_OK) {
            guardarContactos();         // Guardar los contactos del usuario autenticado
            disconnect(client_fd);      // Enviar DISCONNECT al servidor y cerrar socket
        }
    }

        void onContactoSeleccionado(Gtk::ListBoxRow* row) {
            if (row) {
                auto label = dynamic_cast<Gtk::Label*>(row->get_child());
                if (label) {
                    std::string nombre_completo = label->get_text();
                    for (const auto& contacto : lista_contactos) {
                        if (contacto.nombre + " " + contacto.apellido == nombre_completo) {
                            correo_contacto_seleccionado = contacto.correo;
                            chat_text_view.get_buffer()->set_text(""); // limpiar chat
                            cargarChats(usuario_autenticado.correo); // cargar chats del usuario autenticado
                            break;
                        }
                    }
                }
            }
        }
    
        void enviarMensaje() {
            std::string mensaje = chat_entry.get_text();
            if (!mensaje.empty() && !correo_contacto_seleccionado.empty()) {
                // Llamar a la función que envía el mensaje
                ::enviarMensaje(client_fd, correo_contacto_seleccionado, mensaje);
    
                // Limpiar el campo de entrada de texto
                chat_entry.set_text("");
    
                // Actualizar la ventana de chat (agregar el mensaje a la interfaz)
                agregarMensajeAlChat(correo_contacto_seleccionado, mensaje, 0);  // 0 indica mensaje enviado por el usuario
            } else {
                Gtk::MessageDialog dialog(*this, "Por favor, ingrese un mensaje y seleccione un contacto.");
                dialog.run();
            }
        }
    
        void agregarMensajeAlChat(const std::string& correo, const std::string& mensaje, int tipo) {
            // Tipo 0 para mensajes enviados por el usuario
            // Tipo 1 para mensajes recibidos del contacto
            std::string mensaje_con_id = (tipo == 0 ? "Yo: " : correo) + mensaje;
            Gtk::TextBuffer::iterator iter = chat_text_view.get_buffer()->get_iter_at_offset(-1);
            chat_text_view.get_buffer()->insert(iter, mensaje_con_id + "\n");
        }

        bool checkMessages() {
            // Bloquear el mutex mientras revisamos los mensajes
            std::lock_guard<std::mutex> lock(mtx);

            // Enviar el comando "CHECKMSG" al servidor
            std::string comando = "CHECKMSG";
            if (send(client_fd, comando.c_str(), comando.length(), 0) == -1) {
                std::cerr << "Error al enviar el comando al servidor." << std::endl;
                return true;  // Continuar escuchando en el siguiente ciclo
            }

            // Recibir respuesta del servidor
            char buffer[1024] = {0};
            int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    std::cerr << "El servidor cerró la conexión." << std::endl;
                } else {
                    std::cerr << "Error al recibir la respuesta del servidor. Código de error: " << errno << std::endl;
                }
                return true;  // Continuar escuchando en el siguiente ciclo
            }

            std::string respuesta(buffer, bytes_received);
            if (respuesta.find("ERROR") != std::string::npos) {
                std::cerr << "Error al revisar mensajes: " << respuesta << std::endl;
            } else {
                size_t posDe = respuesta.find("De: ");
                size_t posMensaje = respuesta.find("\nMensaje: ");
                if (posDe != std::string::npos && posMensaje != std::string::npos) {
                    std::string correoEmisor = respuesta.substr(posDe + 4, posMensaje - posDe - 4);
                    std::string mensajeContenido = respuesta.substr(posMensaje + 9);
                    std::cout << "Nuevo mensaje de " << correoEmisor << ": " << mensajeContenido << std::endl;

                    // Si el mensaje fue recibido correctamente, agregarlo al chat
                    agregarMensajeAlChat(correoEmisor, mensajeContenido, 1);  // 1 indica que es un mensaje recibido del contacto
                } else {
                    std::cerr << "Formato de mensaje recibido incorrecto: " << respuesta << std::endl;
                }
            }

            return true;  // Continuar escuchando en el siguiente ciclo
        }

        // Destructor
        ~ChatWindow() {
            guardarContactos();  // Guardar contactos al cerrar la ventana
            disconnect(client_fd);  // Desconectar del servidor
        }

        
    };
    

// Ventana de Registro

class RegisterWindow : public Gtk::Window {
    public:
        RegisterWindow(int client_fd) : client_fd(client_fd) {  // Constructor que recibe client_fd
            set_title("Registro de Usuario");
            set_default_size(350, 250);
            set_position(Gtk::WIN_POS_CENTER);
    
            // Configuración del grid y otros componentes
            grid.set_row_spacing(5);
            grid.set_column_spacing(10);
    
            lbl_name.set_text("Nombre:");
            lbl_lastname.set_text("Apellido:");
            lbl_email.set_text("Correo:");
            lbl_password.set_text("Contraseña:");
            lbl_confirm_password.set_text("Confirmar Contraseña:");
    
            entry_password.set_visibility(false);
            entry_confirm_password.set_visibility(false);
    
            entry_name.set_placeholder_text("Ingrese su nombre");
            entry_lastname.set_placeholder_text("Ingrese su apellido");
            entry_email.set_placeholder_text("Ingrese su correo");
            entry_password.set_placeholder_text("Ingrese su contraseña");
            entry_confirm_password.set_placeholder_text("Confirme su contraseña");
    
            btn_register.set_label("Registrarse");
            btn_register.signal_clicked().connect(sigc::mem_fun(*this, &RegisterWindow::on_register_clicked));
    
            grid.attach(lbl_name, 0, 0, 1, 1);
            grid.attach(entry_name, 1, 0, 2, 1);
            grid.attach(lbl_lastname, 0, 1, 1, 1);
            grid.attach(entry_lastname, 1, 1, 2, 1);
            grid.attach(lbl_email, 0, 2, 1, 1);
            grid.attach(entry_email, 1, 2, 2, 1);
            grid.attach(lbl_password, 0, 3, 1, 1);
            grid.attach(entry_password, 1, 3, 2, 1);
            grid.attach(lbl_confirm_password, 0, 4, 1, 1);
            grid.attach(entry_confirm_password, 1, 4, 2, 1);
            grid.attach(btn_register, 1, 5, 1, 1);
    
            add(grid);
            show_all_children();
        }
    
    private:
        Gtk::Grid grid;
        Gtk::Label lbl_name, lbl_lastname, lbl_email, lbl_password, lbl_confirm_password;
        Gtk::Entry entry_name, entry_lastname, entry_email, entry_password, entry_confirm_password;
        Gtk::Button btn_register;
        int client_fd;  // Variable para almacenar el descriptor del socket
    
        void on_register_clicked() {
            Glib::ustring name = entry_name.get_text();
            Glib::ustring lastname = entry_lastname.get_text();
            Glib::ustring email = entry_email.get_text();
            Glib::ustring password = entry_password.get_text();
            Glib::ustring confirm_password = entry_confirm_password.get_text();
    
            if (password == confirm_password) {
                string command = "REGISTER " + string(name) + " " + string(lastname) + " " + string(email) + " " + string(password);
                
                // Aquí usas client_fd para enviar el comando
                send(client_fd, command.c_str(), command.length(), 0);
    
                // Recibir respuesta del servidor
                char buffer[1024] = {0};
                int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
                if (bytes_received > 0) {
                    string response(buffer);
                    Gtk::MessageDialog dialog(*this, response, false, Gtk::MESSAGE_INFO);
                    dialog.run();
                } else {
                    Gtk::MessageDialog dialog(*this, "Error en la conexión al servidor", false, Gtk::MESSAGE_ERROR);
                    dialog.run();
                }
            } else {
                Gtk::MessageDialog dialog(*this, "Las contraseñas no coinciden", false, Gtk::MESSAGE_ERROR);
                dialog.run();
            }
    
            // Limpiar los campos
            entry_name.set_text("");
            entry_lastname.set_text("");
            entry_email.set_text("");
            entry_password.set_text("");
            entry_confirm_password.set_text("");
        }
    };
    


// Ventana de Inicio de Sesión
class LoginWindow : public Gtk::Window {
    public:
        // Constructor modificado para recibir client_fd
        LoginWindow(int client_fd) : client_fd(client_fd) {
            set_title("Inicio de Sesión");
            set_default_size(300, 180);
            set_position(Gtk::WIN_POS_CENTER);
    
            // Configuración del grid
            grid.set_row_spacing(5);
            grid.set_column_spacing(10);
    
            // Configuración de etiquetas y campos de entrada
            lbl_user.set_text("Correo:");
            lbl_password.set_text("Contraseña:");
            entry_password.set_visibility(false);
    
            // Campos con texto de ayuda
            entry_user.set_placeholder_text("Ingrese su correo");
            entry_password.set_placeholder_text("Ingrese su contraseña");
    
            // Configurar los botones
            btn_login.set_label("Iniciar sesión");
            btn_register.set_label("Registrarse");
    
            // Conectar eventos de los botones
            btn_login.signal_clicked().connect(sigc::mem_fun(*this, &LoginWindow::on_login_clicked));
            btn_register.signal_clicked().connect(sigc::mem_fun(*this, &LoginWindow::on_register_clicked));
    
            // Agregar widgets al grid
            grid.attach(lbl_user, 0, 0, 1, 1);
            grid.attach(entry_user, 1, 0, 2, 1);
            grid.attach(lbl_password, 0, 1, 1, 1);
            grid.attach(entry_password, 1, 1, 2, 1);
            grid.attach(btn_login, 1, 2, 1, 1);
            grid.attach(btn_register, 2, 2, 1, 1); // Poner los botones en la misma fila
    
            add(grid);
            show_all_children();
        }
    
    private:
        Gtk::Grid grid;
        Gtk::Label lbl_user, lbl_password;
        Gtk::Entry entry_user, entry_password;
        Gtk::Button btn_login, btn_register;
        int client_fd;  // Almacenar client_fd
        ChatWindow* chat_window = nullptr; 
    
        // Función que maneja el inicio de sesión
        void on_login_clicked() {
            string user = entry_user.get_text();
            string password = entry_password.get_text();

            iniciarSesion(user, password, client_fd);

            if (!usuario_autenticado.correo.empty()) {
                // Crear y mostrar la ventana de chat
                chat_window = new ChatWindow(client_fd);
                chat_window->set_application(get_application());
                chat_window->present();

                hide();  // Ocultamos la ventana de login, pero la app sigue viva gracias a chat_window
            }
            else {
                Gtk::MessageDialog dialog(*this, "Login fallido", false, Gtk::MESSAGE_ERROR);
                dialog.run();
            }
        }


    
        // Función que maneja el registro de un nuevo usuario
        void on_register_clicked() {
            // Crear la ventana de registro y pasar client_fd
            RegisterWindow* register_window = new RegisterWindow(client_fd);
            register_window->set_modal(true);  // Establecer la ventana de registro como modal
            register_window->show_all();  // Mostrar la ventana de registro
        }
    };
    
// Iniciar la conexion con el servidor
int startConnection(int argc, char* argv[]) {
    int client_fd;
    struct sockaddr_in server_addr;
    string server_ip = "127.0.0.1";
    int server_port = 8080;

    read_config(server_ip, server_port);
    cout << "Conectando al servidor en IP: " << server_ip << " y puerto: " << server_port << endl;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Error al crear el socket del cliente" << endl;
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "Error en la conversión de la dirección IP" << endl;
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error al conectar con el servidor" << endl;
        return -1;
    }

    cout << "Conectado al servidor!" << endl;

    char buffer[1024] = {0};
    int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        cout << "Respuesta del servidor: " << buffer << endl;
    } else {

        cerr << "Error al recibir la respuesta del servidor" << endl;
    }

    // Usar Gtk::Application
    auto app = Gtk::Application::create(argc, argv, "com.swifttalk.login");
    LoginWindow loginWindow(client_fd);  // Pasa client_fd a la ventana de login
    return app->run(loginWindow);  // Corre la ventana dentro del bucle de eventos de Gtk::Application
}

int main(int argc, char* argv[]) {
    // Llamar a startConnection, pasando los argumentos de argc y argv
    return startConnection(argc, argv);
}
