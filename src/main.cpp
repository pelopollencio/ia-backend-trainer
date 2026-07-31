#include "crow.h"
#include <cstdlib>
#include <string>
#include <iostream>
#include <algorithm>
#include <curl/curl.h>

std::string trimString(const std::string& str) {
    std::string clipped = str;
    while (!clipped.empty() && (clipped.back() == '\r' || clipped.back() == '\n' || clipped.back() == ' ' || clipped.back() == '"' || clipped.back() == '\'')) {
        clipped.pop_back();
    }
    while (!clipped.empty() && (clipped.front() == '\r' || clipped.front() == '\n' || clipped.front() == ' ' || clipped.front() == '"' || clipped.front() == '\'')) {
        clipped.erase(0, 1);
    }
    return clipped;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string callAPI(const std::string& url, const std::string& apiKey, const std::string& jsonPayload, const std::string& method = "POST", const std::string& preferHeader = "") {
    CURL* curl = curl_easy_init();
    std::string responseString;

    if (curl) {
        std::string cleanApiKey = trimString(apiKey);
        std::string cleanUrl = trimString(url);

        std::cout << "[DEBUG cURL] URL limpia a conectar: [" << cleanUrl << "]" << std::endl;
        std::cout << "[DEBUG cURL] Longitud de API Key limpia: " << cleanApiKey.length() << std::endl;

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        if (cleanUrl.find("supabase.co") != std::string::npos) {
            headers = curl_slist_append(headers, ("apikey: " + cleanApiKey).c_str());
        }
        headers = curl_slist_append(headers, ("Authorization: Bearer " + cleanApiKey).c_str());
        
        if (!preferHeader.empty()) {
            headers = curl_slist_append(headers, trimString(preferHeader).c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, cleanUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        if (method == "GET") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        } else {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[-] Error de cURL al conectar con [" << cleanUrl << "]: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    } else {
        std::cerr << "[-] Error: No se pudo inicializar cURL." << std::endl;
    }
    return responseString;
}

bool validateWorkoutRequest(const crow::json::rvalue& body, std::string& errorMessage) {
    if (!body.has("deporte") || body["deporte"].t() != crow::json::type::String) {
        errorMessage = "El campo 'deporte' es obligatorio y debe ser un texto.";
        return false;
    }
    if (!body.has("duracion") || body["duracion"].t() != crow::json::type::Number) {
        errorMessage = "El campo 'duracion' es obligatorio y debe ser un número entero.";
        return false;
    }
    if (!body.has("tipo de entrenamiento") || body["tipo de entrenamiento"].t() != crow::json::type::String) {
        errorMessage = "El campo 'tipo de entrenamiento' es obligatorio y debe ser un texto.";
        return false;
    }
    if (body.has("ultimos entrenamientos") && body["ultimos entrenamientos"].t() != crow::json::type::List) {
        errorMessage = "El campo 'ultimos entrenamientos' debe ser una lista (array).";
        return false;
    }
    if (body.has("preferencias") && body["preferencias"].t() != crow::json::type::List) {
        errorMessage = "El campo 'preferencias' debe ser una lista.";
        return false;
    }
    return true;
}

bool verifySupabaseToken(const std::string& authHeader, std::string& outUserId) {
    if (authHeader.rfind("Bearer ", 0) != 0) {
        return false; 
    }
    return true; 
}

int main() {
    crow::App<crow::CORSHandler> app;

    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .methods("POST"_method, "GET"_method, "OPTIONS"_method)
        .headers("Content-Type", "Authorization");

    CROW_ROUTE(app, "/")([]() {
        return crow::response(200, "OK - Backend C++ con Supabase y Groq activo");
    });

    // 1. ENDPOINT DE REGISTRO
    CROW_ROUTE(app, "/api/auth/register").methods("POST"_method)([](const crow::request& req) {
        try {
            const char* urlEnv = std::getenv("SUPABASE_URL");
            const char* keyEnv = std::getenv("SUPABASE_KEY");

            if (!urlEnv || !keyEnv) {
                return crow::response(500, "{\"error\": \"Variables de entorno de Supabase no configuradas.\"}");
            }

            std::string cleanUrl = trimString(urlEnv);
            std::string cleanKey = trimString(keyEnv);

            auto body = crow::json::load(req.body);
            if (!body || !body.has("username") || !body.has("email") || !body.has("password")) {
                return crow::response(400, "{\"error\": \"Faltan campos obligatorios (username, email, password).\"}");
            }

            std::string username = body["username"].s();
            std::string email = body["email"].s();
            std::string password = body["password"].s();

            std::string checkUrl = cleanUrl + "?or=(username.eq." + username + ",email.eq." + email + ")&select=id";
            std::string checkResponse = callAPI(checkUrl, cleanKey, "", "GET");

            auto checkJson = crow::json::load(checkResponse);
            if (checkJson && checkJson.size() > 0) {
                return crow::response(400, "{\"status\": \"error\", \"error\": \"El nombre de usuario o el correo ya están en uso.\"}");
            }

            crow::json::wvalue payload;
            payload["username"] = username;
            payload["email"] = email;
            payload["password"] = password;

            callAPI(cleanUrl, cleanKey, payload.dump(), "POST", "Prefer: return=representation");

            return crow::response(201, "{\"status\": \"success\", \"message\": \"Usuario registrado correctamente.\"}");

        } catch (const std::exception& e) {
            return crow::response(500, "{\"error\": \"Error interno del servidor.\"}");
        }
    });

    // 2. ENDPOINT DE LOGIN
    CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)([](const crow::request& req) {
        try {
            const char* urlEnv = std::getenv("SUPABASE_URL");
            const char* keyEnv = std::getenv("SUPABASE_KEY");

            if (!urlEnv || !keyEnv) {
                return crow::response(500, "{\"error\": \"Variables de entorno de Supabase no configuradas.\"}");
            }

            std::string cleanUrl = trimString(urlEnv);
            std::string cleanKey = trimString(keyEnv);

            auto body = crow::json::load(req.body);
            if (!body || !body.has("email") || !body.has("password")) {
                return crow::response(400, "{\"error\": \"Faltan campos obligatorios (email, password).\"}");
            }

            std::string email = body["email"].s();
            std::string password = body["password"].s();

            std::string queryUrl = cleanUrl + "?email=eq." + email + "&select=id,username,password";
            std::string responseStr = callAPI(queryUrl, cleanKey, "", "GET");

            auto jsonRes = crow::json::load(responseStr);
            if (!jsonRes || jsonRes.size() == 0) {
                return crow::response(404, "{\"status\": \"error\", \"error\": \"El usuario no existe.\"}");
            }

            int userId = jsonRes[0]["id"].i();
            std::string storedPassword = jsonRes[0]["password"].s();

            if (storedPassword == password) {
                return crow::response(200, "{\"status\": \"success\", \"userId\": " + std::to_string(userId) + "}");
            } else {
                return crow::response(401, "{\"status\": \"error\", \"error\": \"Contraseña incorrecta.\"}");
            }

        } catch (const std::exception& e) {
            return crow::response(500, "{\"error\": \"Error interno del servidor.\"}");
        }
    });

    // 3. ENDPOINT DE GENERACIÓN DE ENTRENAMIENTO
    CROW_ROUTE(app, "/api/generate-workout").methods("POST"_method)([](const crow::request& req) {
        try {
            std::cout << "\n==================================================\n";
            std::cout << "[LOG - GENERATE-WORKOUT] JSON de Request Recibido:\n";
            std::cout << req.body << std::endl;
            std::cout << "==================================================\n";

            std::string authHeader = req.get_header_value("Authorization");
            std::string userId = "";
            
            if (!verifySupabaseToken(authHeader, userId)) {
                return crow::response(401, "{\"error\": \"No autorizado. Token inválido, expirado o ausente.\"}");
            }

            const char* apiKeyEnv = std::getenv("GROQ_API_KEY");
            if (!apiKeyEnv) {
                return crow::response(500, "{\"error\": \"La variable GROQ_API_KEY no está configurada.\"}");
            }

            std::string cleanGroqKey = trimString(apiKeyEnv);

            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, "{\"error\": \"JSON malformado o vacío.\"}");
            }

            std::string validationError = "";
            if (!validateWorkoutRequest(body, validationError)) {
                return crow::response(400, "{\"status\": \"error\", \"error\": \"" + validationError + "\"}");
            }

            std::string sport = body["deporte"].s();
            int duration = body["duracion"].i();
            std::string workoutType = body["tipo de entrenamiento"].s();

            std::string lastWorkoutsText = "Ninguno";
            if (body.has("ultimos entrenamientos") && body["ultimos entrenamientos"].size() > 0) {
                lastWorkoutsText = "";
                for (const auto& ent : body["ultimos entrenamientos"]) {
                    std::string sSport = ent.has("deporte") ? std::string(ent["deporte"].s()) : "Desconocido";
                    int sDur = ent.has("duracion") ? ent["duracion"].i() : 0;
                    std::string sType = ent.has("tipo de entrenamiento") ? std::string(ent["tipo de entrenamiento"].s()) : "General";
                    
                    lastWorkoutsText += "- Deporte: " + sSport + 
                                       ", Duración: " + std::to_string(sDur) + " min" +
                                       ", Tipo: " + sType + "\n";
                }
            }

            std::string prompt = "Actúa como un entrenador personal experto de élite. Diseña un nuevo entrenamiento estructurado basado en los siguientes parámetros:\n"
                                 "- Deporte: " + sport + "\n"
                                 "- Duración total objetivo: " + std::to_string(duration) + " minutos\n"
                                 "- Tipo de entrenamiento: " + workoutType + "\n"
                                 "- Últimos entrenamientos previos:\n" + lastWorkoutsText + "\n\n"
                                 "REGLA CRÍTICA DE SALIDA: Debes responder **EXCLUSIVAMENTE** con un objeto JSON válido, sin bloques markdown adicionales (como ```json), que cumpla milimétricamente con esta estructura:\n"
                                 "{\n"
                                 "    \"deporte\": \"" + sport + "\",\n"
                                 "    \"duracion\": " + std::to_string(duration) + ",\n"
                                 "    \"tipo de entrenamiento\": \"" + workoutType + "\",\n"
                                 "    \"calentamiento\": \"texto descriptivo del calentamiento\",\n"
                                 "    \"calentamiento_tiempo\": 10,\n"
                                 "    \"bloques de entrenamiento\": [\n"
                                 "        {\n"
                                 "            \"bloque\": 1,\n"
                                 "            \"tiempo\": 5,\n"
                                 "            \"ritmo\": \"2:00 min/km\",\n"
                                 "            \"descanso\": 2\n"
                                 "        }\n"
                                 "    ],\n"
                                 "    \"enfriamiento\": \"texto descriptivo del enfriamiento\",\n"
                                 "    \"enfriamiento_tiempo\": 5\n"
                                 "}";

            crow::json::wvalue payload;
            payload["model"] = "llama-3.3-70b-versatile";
            payload["messages"][0]["role"] = "user";
            payload["messages"][0]["content"] = prompt;

            std::string groqRawResponse = callAPI("https://api.groq.com/openai/v1/chat/completions", cleanGroqKey, payload.dump(), "POST");            
            std::cout << "Respuesta cruda de Groq:\n" << groqRawResponse << std::endl;

            auto groqJson = crow::json::load(groqRawResponse);
            
            std::string workoutText = "";
            if (groqJson && groqJson.has("choices") && groqJson["choices"].size() > 0) {
                if (groqJson["choices"][0].has("message") && 
                    groqJson["choices"][0]["message"].has("content")) {
                    workoutText = std::string(groqJson["choices"][0]["message"]["content"].s());
                }
            }

            crow::json::wvalue responseJson;
            if (workoutText.empty()) {
                responseJson["status"] = "error_groq";
                responseJson["error"] = "No se pudo extraer la respuesta de la IA.";
            } else {
                responseJson["status"] = "success";
                
                std::string cleanedText = workoutText;
                size_t startPos = cleanedText.find("{");
                size_t endPos = cleanedText.rfind("}");
                if (startPos != std::string::npos && endPos != std::string::npos && endPos > startPos) {
                    cleanedText = cleanedText.substr(startPos, (endPos - startPos) + 1);
                }

                auto parsedWorkout = crow::json::load(cleanedText);
                if (parsedWorkout) {
                    responseJson["workout"] = parsedWorkout;
                } else {
                    responseJson["workout"] = workoutText; 
                }
            }

            std::string finalResponseString = responseJson.dump();

            std::cout << "\n==================================================\n";
            std::cout << "[LOG - GENERATE-WORKOUT] JSON de Response Enviado:\n";
            std::cout << finalResponseString << std::endl;
            std::cout << "==================================================\n\n";

            crow::response res;
            res.code = 200;
            res.set_header("Content-Type", "application/json");
            res.write(finalResponseString);
            return res;

        } catch (const std::exception& e) {
            std::cerr << "Excepción: " << e.what() << std::endl;
            return crow::response(500, "{\"error\": \"Error interno del servidor.\"}");
        }
    });

    const char* portEnv = std::getenv("PORT");
    int port = portEnv ? std::stoi(portEnv) : 18080;

    app.port(port).multithreaded().run();
}