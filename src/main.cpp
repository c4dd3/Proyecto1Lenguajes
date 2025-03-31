#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// Ventana de Registro
class RegisterWindow : public Gtk::Window {
public:
    RegisterWindow() {
        set_title("Registro de Usuario");
        set_default_size(350, 250);
        set_position(Gtk::WIN_POS_CENTER);

        // Configuración del grid
        grid.set_row_spacing(5);
        grid.set_column_spacing(10);

        // Configuración de etiquetas y campos de entrada
        lbl_name.set_text("Nombre:");
        lbl_lastname.set_text("Apellido:");
        lbl_email.set_text("Correo:");
        lbl_password.set_text("Contraseña:");
        lbl_confirm_password.set_text("Confirmar Contraseña:");

        entry_password.set_visibility(false);
        entry_confirm_password.set_visibility(false);

        // Campos con texto de ayuda
        entry_name.set_placeholder_text("Ingrese su nombre");
        entry_lastname.set_placeholder_text("Ingrese su apellido");
        entry_email.set_placeholder_text("Ingrese su correo");
        entry_password.set_placeholder_text("Ingrese su contraseña");
        entry_confirm_password.set_placeholder_text("Confirme su contraseña");

        // Configurar botón de registro
        btn_register.set_label("Registrarse");
        btn_register.signal_clicked().connect(sigc::mem_fun(*this, &RegisterWindow::on_register_clicked));

        // Agregar widgets al grid
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

    void on_register_clicked() {
        Glib::ustring name = entry_name.get_text();
        Glib::ustring lastname = entry_lastname.get_text();
        Glib::ustring email = entry_email.get_text();
        Glib::ustring password = entry_password.get_text();
        Glib::ustring confirm_password = entry_confirm_password.get_text();

        cout << "Nombre: " << name << endl;
        cout << "Apellido: " << lastname << endl;
        cout << "Correo: " << email << endl;
        cout << "Contraseña: " << password << endl;
        cout << "Confirmar Contraseña: " << confirm_password << endl;

        if (password == confirm_password) {
            ofstream archivo("users.txt", ios::app);
            if (!archivo) {
                cerr << "Error: No se pudo abrir el archivo users.txt" << endl;
                return;
            }

            archivo << name << " " << lastname << " " << email << " " << password << " 0.0.0.0 0 -1" << endl;
            
            Gtk::MessageDialog dialog(*this, "Usuario registrado con éxito", false, Gtk::MESSAGE_INFO);
            dialog.run();
            
            // Limpiar los campos
            entry_name.set_text("");
            entry_lastname.set_text("");
            entry_email.set_text("");
            entry_password.set_text("");
            entry_confirm_password.set_text("");
        } else {
            Gtk::MessageDialog dialog(*this, "Las contraseñas no coinciden", false, Gtk::MESSAGE_ERROR);
            dialog.run();
        }
    }
};

// Ventana de Inicio de Sesión
class LoginWindow : public Gtk::Window {
public:
    LoginWindow() {
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

    // Función para verificar credenciales en users.txt
    bool validar_credenciales(const string& correo, const string& contrasena) {
        ifstream archivo("users.txt");
        if (!archivo) {
            cerr << "Error: No se pudo abrir users.txt" << endl;
            return false;
        }

        string linea;
        while (getline(archivo, linea)) {
            istringstream ss(linea);
            vector<string> datos;
            string dato;
            while (ss >> dato) {
                datos.push_back(dato);
            }

            if (datos.size() >= 4) {  // Asegurar que haya suficientes campos
                if (datos[2] == correo && datos[3] == contrasena) {
                    return true;
                }
            }
        }
        return false;
    }

    // Función que maneja el inicio de sesión
    void on_login_clicked() {
        string user = entry_user.get_text();
        string password = entry_password.get_text();


        if (validar_credenciales(user, password)) {
            Gtk::MessageDialog dialog(*this, "Inicio de sesión exitoso", false, Gtk::MESSAGE_INFO);
            cout << "Inicio de sesión exitoso" << endl;
            cout << "Usuario: " << user << " | Contraseña: " << password << endl;
            dialog.run();
        } else {
            Gtk::MessageDialog dialog(*this, "Correo o contraseña incorrectos", false, Gtk::MESSAGE_ERROR);
            cout << "Correo o contraseña incorrectos" << endl;
            dialog.run();
        }
    }

    // Función que maneja el registro de un nuevo usuario
    void on_register_clicked() {
        RegisterWindow* register_window = new RegisterWindow();
        register_window->set_modal(true);  // Establecer la ventana de registro como modal
        register_window->show();  // Mostrar la ventana de registro
    }
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("com.example.login");
    LoginWindow window;
    return app->run(window);
}
