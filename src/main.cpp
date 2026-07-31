#include "crow.h"
#include "services/ai_service.hpp"

int main() {
    crow::SimpleApp app;
    AIService aiService;

    // Endpoint POST
    CROW_ROUTE(app, "/api/generate-workout").methods(crow::HTTPMethod::POST)
    ([&aiService](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if (!json) return crow::response(400, "JSON inválido");

        std::string goal = json["goal"].s();
        std::string experience = json["experience"].s();

        std::string result = aiService.generateWorkout(goal, experience);

        crow::response res(200, result);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    app.port(18080).multithreaded().run();
}
