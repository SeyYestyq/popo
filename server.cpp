#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <fstream>

// Заменить SOCKET на int
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

// Заменить closesocket на close
// Remove the closesocket macro definition as it is not needed for Windows

struct Task {
    int id;
    std::string title;
    std::string description;
    std::string deadline;
    bool completed;
    
    Task(int i, std::string t, std::string desc, std::string dl, bool comp = false) 
        : id(i), title(t), description(desc), deadline(dl), completed(comp) {}
};

std::vector<Task> tasks;
int nextId = 1;

std::string getTasksJSON() {
    std::stringstream json;
    json << "[";
    for (size_t i = 0; i < tasks.size(); ++i) {
        json << "{";
        json << "\"id\":" << tasks[i].id << ",";
        json << "\"title\":\"" << tasks[i].title << "\",";
        json << "\"description\":\"" << tasks[i].description << "\",";
        json << "\"deadline\":\"" << tasks[i].deadline << "\",";
        json << "\"completed\":" << (tasks[i].completed ? "true" : "false");
        json << "}";
        if (i < tasks.size() - 1) json << ",";
    }
    json << "]";
    return json.str();
}

void handleRequest(SOCKET clientSocket, const std::string& request) {
    std::string response;
    
    // отладка вывода
    std::cout << "Получен запрос:\n" << request << std::endl;

    if (request.find("GET / ") != std::string::npos || 
        request.find("GET /index.html") != std::string::npos) {
        // Отдаем в браузер index.html
        std::ifstream file("../static/index.html");  // меняем путь 
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + buffer.str();
        } else {
            std::cerr << "Не удалось открыть index.html\n";
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
    } else if (request.find("GET /tasks") != std::string::npos) {
        response = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: application/json\r\n"
                  "Access-Control-Allow-Origin: *\r\n\r\n" + 
                  getTasksJSON();
    } else if (request.find("POST /tasks/add") != std::string::npos) {
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            std::string body = request.substr(bodyPos + 4);
            std::cout << "Тело запроса: " << body << std::endl;
            
            try {
                size_t titleStart = body.find("\"title\":\"") + 9;
                size_t titleEnd = body.find("\"", titleStart);
                std::string title = body.substr(titleStart, titleEnd - titleStart);

                size_t descStart = body.find("\"description\":\"") + 14;
                size_t descEnd = body.find("\"", descStart);
                std::string desc = body.substr(descStart, descEnd - descStart);

                size_t deadlineStart = body.find("\"deadline\":\"") + 11;
                size_t deadlineEnd = body.find("\"", deadlineStart);
                std::string deadline = body.substr(deadlineStart, deadlineEnd - deadlineStart);

                tasks.push_back(Task(nextId++, title, desc, deadline));
                
                response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Methods: POST, GET, DELETE, OPTIONS\r\n"
                          "Access-Control-Allow-Headers: Content-Type\r\n\r\n"
                          "{\"success\":true,\"id\":" + std::to_string(nextId-1) + "}";
            } catch (const std::exception& e) {
                std::cerr << "Ошибка при разборе JSON: " << e.what() << std::endl;
                response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            }
        }
    } else if (request.find("OPTIONS") != std::string::npos) {
        response = "HTTP/1.1 200 OK\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "Access-Control-Allow-Methods: POST, GET, DELETE, OPTIONS\r\n"
                  "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
    } else {
        std::cout << "Неизвестный запрос: " << request << std::endl;
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка инициализации Winsock\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета\n";
        return 1;
    }

    sockaddr_in serverAddr;
        closesocket(serverSocket);
        WSACleanup();
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Ошибка привязки сокета\n";
        closesocket(serverSocket);
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Ошибка прослушивания\n";
        closesocket(serverSocket);
        return 1;
    }

    std::cout << "Сервер запущен на http://localhost:8080\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Ошибка принятия соединения\n";
            continue;
        }

        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            std::string request(buffer, bytesReceived);
            handleRequest(clientSocket, request);
        }

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    return 0;
}