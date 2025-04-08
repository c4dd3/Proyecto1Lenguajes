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
    
            control_buttons_box.pack_start(btn_add_contact, Gtk::PACK_SHRINK);
            control_buttons_box.pack_start(btn_logout, Gtk::PACK_SHRINK);
    
            chat_area.pack_start(control_buttons_box, Gtk::PACK_SHRINK);
            chat_text_view.set_editable(false);
            chat_text_view.set_wrap_mode(Gtk::WrapMode::WRAP_WORD);
    
            chat_area.pack_start(chat_text_view);
            chat_area.pack_start(chat_entry, Gtk::PACK_SHRINK);
            chat_area.pack_start(send_button, Gtk::PACK_SHRINK);
    
            send_button.set_label("Enviar");
    
            main_box.pack_start(chat_area);
            show_all_children();
        }
    
    private:
        int client_fd;
    
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
