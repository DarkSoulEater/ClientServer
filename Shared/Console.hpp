#pragma once
#include <mutex>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <iostream>
#include <thread>
#include <vector>
#include <optional>
#include <sstream>
#include "Config.hpp"

struct Command {
    enum class Type {
        Exit,
        Send,
        ShowClients,
        Dialog,
        History
    } type;

    std::string str;
    size_t id_;

    Command(Command::Type type_, size_t id = -1) : type(type_), id_(id) {}
    Command(Command::Type type_, std::string str_, size_t id = -1) : type(type_), str(str_), id_(id) {}
};

class Commands {
    std::mutex mtx_;
    std::vector<Command> command_;

public:
    void push_back(const Command& cmd) {
        std::lock_guard<std::mutex> lock(mtx_);
        command_.push_back(cmd);
    }

    auto get() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (command_.empty())
            return std::optional<Command>();
        
        auto cmd = command_[command_.size() - 1];
        command_.pop_back();
        return std::optional<Command>(cmd);
    }
};

class Console {
private:
    std::mutex mtx_;
    std::string str_;

    int background_color_ = 40;

    int setEchoMode(bool enable) {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~ICANON;
        if (enable)
            newt.c_lflag |= ECHO;
        else
            newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }

    int getch() {
        return setEchoMode(false);
    }

    int getche() {
        return setEchoMode(true);
    }

    int wherexy(int &x, int &y) {
        printf("\033[6n");
        if (getch() != '\x1B')
            return 0;
        if (getch() != '\x5B')
            return 0;
        int in;
        int ly = 0;
        while ((in = getch()) != ';')
            ly = ly * 10 + in - '0';
        int lx = 0;
        while ((in = getch()) != 'R')
            lx = lx * 10 + in - '0';
        x = lx;
        y = ly;
        return 0;
    }

    int wherex() {
        int x = 0, y = 0;
        wherexy(x, y);
        return x;
    }

    int wherey() {
        int x = 0, y = 0;
        wherexy(x, y);
        return y;
    }

    void deleteLine() {
        printf("\033[1M");
        putch('\r');
    }

    void clearLine() {
        printf("\033[K");
    }

    void setCursorPosition(int x, int y) {
        printf("\033[%d;%df", y, x);
    }

    void clearScreen() {
        std::lock_guard<std::mutex> lock(mtx_);
        printf("\033[%dm\033[2J\033[1;1f", background_color_);
        for (size_t k = 0; k < 100; ++k) {
            deleteLine();
            putch('\n');
        }
    }

    void putch(const char c) {
        std::cout << c << std::flush;
    }

    void cputs(const char *str) {
        std::cout << str << std::flush;
        // printf("%s", str);
    }

    int putcmdbase() {
        std::cout << "> ";
        return 2;
    }

    void PrintServerHelp() {
        Print("---------------- Help ----------------", false);
        Print("Server commands:", false);
        Print("\texit - Close server", false);
        Print("\tsend [ID] [MSG] - Send message to ID client", false);
        Print("\thistory - Print list disconnect clients", false);
        deleteLine();
    }

    void PrintClientHelp() {
        Print("---------------- Help ----------------", false);
        Print("Client commands:", false);
        Print("\texit - Close client", false);
        Print("\tsend [MSG] - Send message to server", false);
        deleteLine();
    }

    int ParseServerCommand(std::string& cmd_, Commands& commands) {
        std::stringstream ss;
        ss << cmd_;

        std::string cmd;
        ss >> cmd;

        if (cmd == "exit") {
            commands.push_back(Command(Command::Type::Exit));
            return 1;
        } else if (cmd == "send") {
            size_t id;
            std::string sid;
            ss >> id;
            if (ss.fail()) {
                Log("For send need client ID");
                Log("Enter \"help\" to learn how to use send");
                deleteLine();
                return 0;
            }

            ss.get(); // Skip Space
            
            std::string msg;
            std::getline(ss, msg);
            
            if (msg.empty()) {
                Log("Entered empty msg. Skip");
                return 0;
            }
            
            commands.push_back(Command(Command::Type::Send, msg, id));
        } else if (cmd == "help") {
            PrintServerHelp();
        } else if (cmd == "clients") {
            commands.push_back(Command(Command::Type::ShowClients));
        } else if (cmd == "history") {
            commands.push_back(Command(Command::Type::History));
        } else {
            cputs("[Log]: Unknow command ");
            cputs(cmd.c_str());
            cputs("\n[Log]: Enter \"help\" for Help\n");
        }
        return 0;
    }

    int ParseClientCommand(std::string& cmd_, Commands& commands) {
        std::stringstream ss;
        ss << cmd_;

        std::string cmd;
        ss >> cmd;

        if (cmd == "exit") {
            commands.push_back(Command(Command::Type::Exit));
            return 1;
        } else if (cmd == "send") {
            ss.get(); // Skip Space
            
            std::string msg;
            std::getline(ss, msg);
            
            if (msg.empty()) {
                Log("Entered empty msg. Skip");
                return 0;
            }
            
            commands.push_back(Command(Command::Type::Send, msg));
        } else if (cmd == "help") {
            PrintClientHelp();
        } else {
            cputs("[Log]: Unknow command ");
            cputs(cmd.c_str());
            cputs("\n[Log]: Enter \"help\" for Help\n");
        }
        return 0;
    }
public:
    Console(/* args */) {
        clearScreen();
    }
    ~Console() {
        printf("\033[m");
    }

    template <typename T>
    void Print(const T& s, bool endl = false) {
        std::lock_guard<std::mutex> lock(mtx_);
        deleteLine();
        std::cout << s << std::endl;
        if (endl) {
            std::cout << "\n";
        }
        putcmdbase();
        cputs(str_.c_str());
    }

    template <typename T>
    void Log(const T& s, bool endl = false) {
        std::lock_guard<std::mutex> lock(mtx_);
        deleteLine();
        std::cout << "[LOG]: " << s << std::endl;
        if (endl) {
            std::cout << "\n";
        }
        putcmdbase();
        cputs(str_.c_str());
    }

    void Loop(Commands& commands, Device device = Device::Server) {
        putcmdbase();
        for (;;) {
            int c = getch();
            mtx_.lock();
            if (c == '\n') {
                putch('\n');
                mtx_.unlock();
                if (device == Device::Server) {
                    if (ParseServerCommand(str_, commands)) {
                        break;
                    }
                } else {
                    if (ParseClientCommand(str_, commands)) {
                        break;;
                    }
                }
                mtx_.lock();

                str_.clear();
                putcmdbase(); 
            } else if (c == 127) { // Backspace
                if (!str_.empty()) {
                    str_.pop_back();
                    deleteLine();
                    putcmdbase();
                    cputs(str_.c_str());
                }
            } else if (isalpha(c) || isspace(c) || isalnum(c)) {
                str_.push_back(c);
                putch(c);
            }
            
            // else {
            //     std::cerr << "Unknow: " << c << "\n";
            // }
            mtx_.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    }
};

#define _cprintf cprintf
#define _cscanf cscanf
#define _cputs cputs
#define _getche getche
#define _kbhit kbhit
#define _putch putch
#define _ungetch ungetch
