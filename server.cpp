#include <iostream>
#include <thread>
#include <algorithm>
#include <vector>
#include <mutex>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

// Global vector to hold connected clients
std::vector<int> clients;
std::mutex clients_mutex;

// Function to broadcast a message to all clients
void broadcast(const std::string &message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int client : clients) {
        send(client, message.c_str(), message.size(), 0);
    }
}

// Function to handle individual client communication
void handle_client(int client_socket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        // If the client disconnects or sends an invalid message
        if (bytes_received <= 0) {
            std::cerr << "Client disconnected or idle.\n";
            // Optionally notify the client about disconnection
            send(client_socket, "Connection terminated due to inactivity.\n", 40, 0);

            // Remove the client from the list of active clients
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.erase(
                    std::remove(clients.begin(), clients.end(), client_socket),
                    clients.end()
                );
            }
            close(client_socket);
            break;
        }

        // Broadcast the received message to all clients
        std::string message = std::string(buffer, bytes_received);
        std::cout << "Client: " << message << std::endl;
        broadcast("Client: " + message);
    }
}

int main() {
    // Create the server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket.\n";
        return -1;
    }

    // Configure server address structure
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket to the specified port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Failed to bind socket.\n";
        close(server_socket);
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        std::cerr << "Failed to listen on socket.\n";
        close(server_socket);
        return -1;
    }

    std::cout << "Server listening on port 8080...\n";

    // Thread to handle server-originated messages
    std::thread([&]() {
        while (true) {
            std::string server_message;
            std::getline(std::cin, server_message);
            if (!server_message.empty()) {
                broadcast("Server: " + server_message);
            }
        }
    }).detach();

    // Main loop to accept and manage clients
    while (true) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);

        // Accept a new client
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
        if (client_socket == -1) {
            std::cerr << "Failed to accept client.\n";
            continue;
        }

        std::cout << "New client connected.\n";

        // Add the new client to the list of active clients
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_socket);
        }

        // Start a new thread to handle the client
        std::thread(handle_client, client_socket).detach();
    }

    // Close the server socket (unreachable in this code, but good practice)
    close(server_socket);
    return 0;
}