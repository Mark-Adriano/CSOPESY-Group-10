#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

using namespace std;

string marquee_text = "Default Text";
int refresh_speed_ms = 200;
atomic<bool> is_running(false);
mutex text_mutex;
thread marquee_thread;

void marquee() {
    int position = 0;
    int direction = 1;
    const int console_width = 50;

    while (is_running) {
        string current_text;
        int current_delay;

        {
            lock_guard<mutex> lock(text_mutex);
            current_text = marquee_text;
            current_delay = refresh_speed_ms;
        }

        if (current_text.empty()) {
            current_text = "Default Marquee Text";
        }

        int max_pos = console_width - static_cast<int>(current_text.length());
        if (max_pos < 1) max_pos = 1;

        string spaces(position, ' ');
        cout << "\r" << spaces << current_text << "    " << flush;

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
        cout << "Marquee is not currently running.\n\n";
        return;
    }
    is_running = false;
    if (marquee_thread.joinable()) {
        marquee_thread.join();
    }
    cout << "\nMarquee animation stopped.\n\n";
}

int main() {
    cout << "Welcome to CSOPESY \n \n";
    cout << "Group developer: \n";
    cout << "Adriano, Mark Luis\nBuenavente, Djuvalle Antoiney \nLee, Jason Benedict\nPerez, Jose Bryan\n\n";
    cout << "Version date: 2026-09-20\n\n";

    string command;

    while (true) {
        cout << "Command> ";
        if (!getline(cin, command)) break;

        if (command == "help") {
            cout << "help - displays the commands and its description\n";
            cout << "start_marquee - starts the marquee \"animation\" \n";
            cout << "stop_marquee - stops the marquee \"animation\" \n";
            cout << "set_text - accepts a text input and displays it as a marquee \n";
            cout << "set_speed - sets the marquee animation refresh in milliseconds \n";
            cout << "exit - terminates the console \n\n";
        }
        else if (command == "start_marquee") {
            start_marquee();
        }
        else if (command == "stop_marquee") {
            stop_marquee();
        }
        else if (command.rfind("set_text", 0) == 0) {
            string new_text = "";
            if (command.length() > 8) {
                size_t start = (command[8] == ' ') ? 9 : 8;
                new_text = command.substr(start);
            }
            {
                lock_guard<mutex> lock(text_mutex);
                marquee_text = new_text;
            }
            cout << "Text saved for marquee: " << new_text << "\n\n";
        }
        else if (command.rfind("set_speed", 0) == 0) {
            string speed_str = "";
            if (command.length() > 9) {
                size_t start = (command[9] == ' ') ? 10 : 9;
                speed_str = command.substr(start);
            }
            try {
                int new_speed = stoi(speed_str);
                if (new_speed > 0) {
                    lock_guard<mutex> lock(text_mutex);
                    refresh_speed_ms = new_speed;
                    cout << "Speed saved for marquee: " << new_speed << " ms\n\n";
                } else {
                    cout << "Error: Speed must be greater than 0.\n\n";
                }
            } catch (...) {
                cout << "Error: Invalid speed value.\n\n";
            }
        }
        else if (command == "exit") {
            if (is_running) {
                stop_marquee();
            }
            cout << "Terminating console...\n";
            break;
        }
        else if (command.empty()) {
            continue;
        }
        else {
            cout << "Error: Unrecognized command.\n\n";
        }
    }

    return 0;
}