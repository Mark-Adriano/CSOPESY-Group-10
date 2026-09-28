#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// default values that will be used if config.txt is missing
struct Config {
    vector<string> developers = {
        "Adriano, Mark Luis",
        "Buenavente, Djuvalle Antoiney",
        "Lee, Jason Benedict",
        "Perez, Jose Bryan"
    };
    string version_date = "2026-09-28";
    string default_text = "Default Text";
    int default_speed_ms= 200;
    int console_width = 50;
};

Config g_config;

string marquee_text = "Default Text";
int refresh_speed_ms = 200;
atomic<bool> is_running(false);
mutex state_mutex; // guards marquee_text / refresh_speed_ms
mutex io_mutex; // guards all cout writes, shared by main + marquee thread
thread marquee_thread;

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

void load_config(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        lock_guard<mutex> lock(io_mutex);
        cout << "[Notice] " << path << " not found next to the executable — using built-in defaults.\n\n";
        return;
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t eq = line.find('=');
        if (eq == string::npos) continue;

        string key = trim(line.substr(0, eq));
        string value = trim(line.substr(eq + 1));

        if (key == "version_date") {
            g_config.version_date = value;
        } else if (key == "default_text") {
            g_config.default_text = value;
        } else if (key == "default_speed_ms") {
            try {
                int v = stoi(value);
                if (v > 0) g_config.default_speed_ms = v;
            } catch (...) { /* ignore malformed value, keep default */ }
        } else if (key == "console_width") {
            try {
                int v = stoi(value);
                if (v > 0) g_config.console_width = v;
            } catch (...) { /* ignore malformed value, keep default */ }
        } else if (key == "developers") {
            g_config.developers.clear();
            stringstream ss(value);
            string name;
            while (getline(ss, name, ';')) {
                name = trim(name);
                if (!name.empty()) g_config.developers.push_back(name);
            }
        }
    }
}

void goToXy(int x, int y)
{
    COORD coord;
    coord.X = x - 1;
    coord.Y = y - 1;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void marquee() {
    int position = 0;
    int direction = 1;

    while (is_running) {
        string current_text;
        int current_delay;
        int width;

        {
            lock_guard<mutex> lock(state_mutex);
            current_text = marquee_text;
            current_delay = refresh_speed_ms;
            width = g_config.console_width;
        }

        if (current_text.empty()) {
            current_text = "Default Marquee Text";
        }

        int max_pos = width - static_cast<int>(current_text.length());
        if (max_pos < 1) max_pos = 1;

        {
            lock_guard<mutex> lock(io_mutex);

            HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;

            GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
            COORD coord = csbi.dwCursorPosition;
            goToXy(1, coord.Y);

            string spaces(position, ' ');
            cout << "\r" << spaces << current_text << "    " << flush;
            goToXy(coord.X + 1, coord.Y + 1);
        }

        position += direction;
        if (position >= max_pos) {
            position = max_pos;
            direction = -1;
        } else if (position <= 0) {
            position = 0;
            direction = 1;
        }

        this_thread::sleep_for(chrono::milliseconds(current_delay));
    }
}

void start_marquee() {
    lock_guard<mutex> lock(io_mutex);
    if (is_running) {
        cout << "Marquee is already running.\n\n";
        return;
    }
    is_running = true;
    marquee_thread = thread(marquee);
    cout << "Marquee animation started.\n\n";
}

void stop_marquee() {
    if (!is_running) {
        lock_guard<mutex> lock(io_mutex);
        cout << "Marquee is not currently running.\n\n";
        return;
    }
    is_running = false;
    if (marquee_thread.joinable()) {
        marquee_thread.join();
    }
    lock_guard<mutex> lock(io_mutex);
    cout << "\nMarquee animation stopped.\n\n";
}

int main() {
#ifdef _WIN32
    // enables ANSI/VT sequences and in case the console UI is extended later
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif

    load_config("config.txt");

    marquee_text = g_config.default_text;
    refresh_speed_ms = g_config.default_speed_ms;

    cout << "Welcome to CSOPESY \n \n";
    cout << "Group developer: \n";
    for (const auto& name : g_config.developers) {
        cout << name << "\n";
    }
    cout << "\n";
    cout << "Version date: " << g_config.version_date << "\n\n";

    string command;

    while (true) {
        {
            lock_guard<mutex> lock(io_mutex);
            cout << "Command> ";
        }
        if (!getline(cin, command)) break;

        command = trim(command);

        // tokenize
        istringstream iss(command);
        string cmd, rest;
        iss >> cmd;
        getline(iss, rest);
        rest = trim(rest);

        if (cmd.empty()) {
            continue;
        }
        else if (cmd == "help") {
            lock_guard<mutex> lock(io_mutex);
            cout << "help - displays the commands and its description\n";
            cout << "start_marquee - starts the marquee \"animation\" \n";
            cout << "stop_marquee - stops the marquee \"animation\" \n";
            cout << "set_text - accepts a text input and displays it as a marquee \n";
            cout << "set_speed - sets the marquee animation refresh in milliseconds \n";
            cout << "exit - terminates the console \n\n";
        }
        else if (cmd == "start_marquee") {
            start_marquee(); // locks io_mutex internally
        }
        else if (cmd == "stop_marquee") {
            stop_marquee(); // locks io_mutex internally
        }
        else if (cmd == "set_text") {
            {
                lock_guard<mutex> lock2(state_mutex);
                marquee_text = rest;
            }
            lock_guard<mutex> lock(io_mutex);
            cout << "Text saved for marquee: " << rest << "\n\n";
        }
        else if (cmd == "set_speed") {
            try {
                int new_speed = stoi(rest);
                lock_guard<mutex> lock(io_mutex);
                if (new_speed > 0) {
                    {
                        lock_guard<mutex> lock2(state_mutex);
                        refresh_speed_ms = new_speed;
                    }
                    cout << "Speed saved for marquee: " << new_speed << " ms\n\n";
                } else {
                    cout << "Error: Speed must be greater than 0.\n\n";
                }
            } catch (...) {
                lock_guard<mutex> lock(io_mutex);
                cout << "Error: Invalid speed value.\n\n";
            }
        }
        else if (cmd == "exit") {
            lock_guard<mutex> lock(io_mutex);
            cout << "Terminating console...\n";
            break;
        }
        else {
            lock_guard<mutex> lock(io_mutex);
            cout << "Error: Unrecognized command.\n\n";
        }
    }

    if (is_running) {
        stop_marquee();
    }

    return 0;
}