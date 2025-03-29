#include <gtkmm.h>

class LoginWindow : public Gtk::Window {
public:
    LoginWindow() {
        set_title("Inicio de Sesión");
        set_default_size(400, 250);

        // Center the window on the screen
        set_position(Gtk::WIN_POS_CENTER);

        // Set the overall padding and margin for the window
        set_margin_top(50);
        set_margin_bottom(50);
        set_margin_left(50);
        set_margin_right(50);

        // Configure the grid layout
        grid.set_row_spacing(20);
        grid.set_column_spacing(20);

        // Set up the labels and entries
        lbl_user.set_text("Usuario:");
        lbl_password.set_text("Contraseña:");
        entry_password.set_visibility(false);
        
        // Set placeholders for text entries
        entry_user.set_placeholder_text("Ingrese su nombre de usuario");
        entry_password.set_placeholder_text("Ingrese su contraseña");
        
        // Configure the login button
        btn_login.set_label("Iniciar sesión");
        btn_login.signal_clicked().connect(sigc::mem_fun(*this, &LoginWindow::on_login_clicked));

        // Add widgets to the grid
        grid.attach(lbl_user, 0, 0, 1, 1);
        grid.attach(entry_user, 1, 0, 2, 1);
        grid.attach(lbl_password, 0, 1, 1, 1);
        grid.attach(entry_password, 1, 1, 2, 1);
        grid.attach(btn_login, 1, 2, 1, 1);

        // Add the grid to the window
        add(grid);

        show_all_children();
    }

private:
    Gtk::Grid grid;
    Gtk::Label lbl_user, lbl_password;
    Gtk::Entry entry_user, entry_password;
    Gtk::Button btn_login;

    void on_login_clicked() {
        Glib::ustring user = entry_user.get_text();
        Glib::ustring password = entry_password.get_text();

        if (user == "admin" && password == "1234") {
            Gtk::MessageDialog dialog(*this, "Inicio de sesión exitoso", false, Gtk::MESSAGE_INFO);
            dialog.run();
        } else {
            Gtk::MessageDialog dialog(*this, "Usuario o contraseña incorrectos", false, Gtk::MESSAGE_ERROR);
            dialog.run();
        }
    }
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("com.example.login");
    LoginWindow window;
    return app->run(window);
}