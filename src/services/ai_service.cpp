#include "services/ai_service.hpp"
#include <curl/curl.h>
#include <cstdlib>

std::string AIService::generateWorkout(const std::string& goal, const std::string& experience) {
    // 1. Obtener la API key de las variables de entorno del servidor
    const char* apiKey = std::getenv("GEMINI_API_KEY"); 

    // 2. Aquí construyes el prompt y haces la petición libcurl a la IA
    // 3. Devuelves el JSON resultante
    return R"({"workout": "Rutina generada por IA..."})";
}
