#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

// Function to handle receiving messages from the server
void receive_messages(int socket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Server disconnected.\n";
            close(socket);
            exit(0);
        }
        std::cout << std::string(buffer, bytes_received) << std::endl;
    }
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Failed to create socket.\n";
        return -1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Failed to connect to server.\n";
        return -1;
    }

    std::cout << "Connected to the server.\n";
    std::thread(receive_messages, client_socket).detach();

    // Handle sending messages
    while (true) {
        std::string message;
        std::getline(std::cin, message);
        if (!message.empty()) {
            send(client_socket, message.c_str(), message.size(), 0);
        }
    }

    close(client_socket);
    return 0;
}
