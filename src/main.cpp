#include "crow.h"
#include <cstdlib>

int main() {
    // 1. Inicializamos la aplicación registrando el middleware CORS
    crow::App<crow::CORSHandler> app;

    // 2. Configuramos la política global de CORS
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
        .global()
        .origin("*") // Permite peticiones desde cualquier origen (en producción puedes usar "https://tu-frontend.vercel.app")
        .methods("POST"_method, "GET"_method, "OPTIONS"_method)
        .headers("Content-Type", "Authorization");

    // 3. Endpoint de comprobación de salud (Health Check)
    CROW_ROUTE(app, "/")([]() {
        return crow::response(200, "OK - Backend C++ activo");
    });

    // 4. Tu endpoint principal
    CROW_ROUTE(app, "/api/generate-workout").methods("POST"_method)([](const crow::request& req) {
        // Tu lógica actual de integración con la IA...
        return crow::response(200, "{\"message\": \"Rutina generada exitosamente\"}");
    });

    // 5. Lectura dinámica del puerto (compatible con Render y ejecuciones locales)
    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::atoi(port_env) : 18080;

    app.port(port).multithreaded().run();
}