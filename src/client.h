#ifndef CLIENTE_H
#define CLIENTE_H

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

// Estructuras
struct Usuario {
    string nombre;
    string apellido;
    string correo;
    string contrasena;
};

struct Contacto {
    string nombre;
    string apellido;
    string correo;
};

// Funciones
void read_config(string &server_ip, int &server_port);
void agregar_contacto(const Contacto &nuevo_contacto);
void mostrar_contactos();
void agregar_contacto_func(int client_fd);
void disconnect(int client_fd);
void interfazAutenticado(int client_fd);
void registrarse(string nombre, string apellido, string correo, string contrasena, int client_fd);
void iniciarSesion(string correo, string contrasena, int client_fd);
void interfazInicial(int client_fd);
int startConnection();

#endif // CLIENTE_H
