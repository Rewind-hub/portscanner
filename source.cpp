#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

const std::string VALID_KEY = "rewind.hub_on_top";
const std::string KEY_FILE = "key.txt";
std::mutex output_mutex;

enum ConsoleColor {
    RED = 12,
    GREEN = 10,
    YELLOW = 14,
    CYAN = 11,
    RESET = 15
};

void set_console_color(ConsoleColor color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void check_key() {
    std::ifstream key_file(KEY_FILE);
    if (!key_file) {
        std::ofstream new_key_file(KEY_FILE);
        new_key_file << "Enter your key here.";
        set_console_color(RED);
        std::cout << "[!] Key file '" << KEY_FILE << "' created. Please enter the valid key and restart the program.\n";
        set_console_color(RESET);
        std::cin.get();
        exit(0);
    }

    std::string key;
    std::getline(key_file, key);
    if (key != VALID_KEY) {
        set_console_color(RED);
        std::cout << "[!] Invalid key in '" << KEY_FILE << "'. Please update it with the correct key.\n";
        set_console_color(RESET);
        std::cin.get();
        exit(0);
    }
}

void scan_port(const std::string& host, int port, std::vector<int>& open_ports) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return;

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &server.sin_addr) <= 0) {
        std::lock_guard<std::mutex> lock(output_mutex);
        set_console_color(RED);
        std::cerr << "[!] Invalid IP address: " << host << "\n";
        set_console_color(RESET);
        closesocket(sock);
        return;
    }

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == 0) {
        std::lock_guard<std::mutex> lock(output_mutex);
        set_console_color(GREEN);
        std::cout << "[+] Port " << port << " is open.\n";
        set_console_color(RESET);
        open_ports.push_back(port);
    }

    closesocket(sock);
}

void scan_ports_range(const std::string& host, int start_port, int end_port, std::vector<int>& open_ports) {
    for (int port = start_port; port <= end_port; ++port) {
        scan_port(host, port, open_ports);
    }
}

void port_scanner() {
    std::string host;
    int start_port, end_port, num_threads;

    set_console_color(CYAN);
    std::cout << R"(
                    ____  _______        _____ _   _ ____   _   _ _   _ ____  
                   |  _ \| ____\ \      / /_ _| \ | |  _ \ | | | | | | | __ ) 
                   | |_) |  _|  \ \ /\ / / | ||  \| | | | || |_| | | | |  _ \ 
                   |  _ <| |___  \ V  V /  | || |\  | |_| ||  _  | |_| | |_) |
                   |_| \_\_____|  \_/\_/  |___|_| \_|____(_)_| |_|\___/|____/ 
                    
                      .:.:;.. Rewind.hub Secure Port Scanner V1.0 ..:.:;.
                       .:.:;.. © Rewind.hub. All Rights Reserved ..:.:;.

)" << std::endl;
    set_console_color(RESET);

    set_console_color(YELLOW);
    std::cout << "[>] Enter the target host (e.g., 127.0.0.1): ";
    set_console_color(RESET);
    std::cin >> host;
    set_console_color(YELLOW);
    std::cout << "[>] Enter the starting port (e.g., 1): ";
    set_console_color(RESET);
    std::cin >> start_port;
    set_console_color(YELLOW);
    std::cout << "[>] Enter the ending port (e.g., 65535): ";
    set_console_color(RESET);
    std::cin >> end_port;
    set_console_color(YELLOW);
    std::cout << "[>] Enter the number of threads to use: ";
    set_console_color(RESET);
    std::cin >> num_threads;

    std::vector<int> open_ports;
    set_console_color(CYAN);
    std::cout << "\n[*] Scanning " << host << " for open ports from " << start_port << " to " << end_port << " using " << num_threads << " threads...\n";
    set_console_color(RESET);

    int range_per_thread = (end_port - start_port + 1) / num_threads;
    int remainder = (end_port - start_port + 1) % num_threads;

    std::vector<std::thread> threads;

    int current_start_port = start_port;
    for (int i = 0; i < num_threads; ++i) {
        int current_end_port = current_start_port + range_per_thread - 1;
        if (remainder > 0) {
            current_end_port++;
            remainder--;
        }

        threads.push_back(std::thread(scan_ports_range, host, current_start_port, current_end_port, std::ref(open_ports)));

        current_start_port = current_end_port + 1;
    }

    for (auto& t : threads) {
        t.join();
    }

    set_console_color(CYAN);
    std::cout << "\n[*] Open ports on " << host << ": ";
    set_console_color(RESET);
    if (open_ports.empty()) {
        set_console_color(RED);
        std::cout << "None\n";
        set_console_color(RESET);
    }
    else {
        for (int port : open_ports) {
            set_console_color(GREEN);
            std::cout << port << " ";
        }
        set_console_color(RESET);
        std::cout << "\n";
    }

    set_console_color(YELLOW);
    std::cout << "[!] Press any key to restart...\n";
    set_console_color(RESET);
    std::cin.ignore();
    std::cin.get();
    port_scanner();
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        set_console_color(RED);
        std::cerr << "[!] Failed to initialize Winsock.\n";
        set_console_color(RESET);
        return 1;
    }

    try {
        check_key();
        port_scanner();
    }
    catch (const std::exception& e) {
        set_console_color(RED);
        std::cerr << "[!] An error occurred: " << e.what() << "\n";
        set_console_color(RESET);
    }

    WSACleanup();
    return 0;
}
