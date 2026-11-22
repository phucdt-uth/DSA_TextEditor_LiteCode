// Khai bao lop giao dien UI console chong giat man hinh va ho tro phim tat
#pragma once
#include <string>
#include "Editor.h"

#ifndef _WIN32
#include <termios.h>
#endif

using namespace std;

enum KeyCode {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_TAB,
    KEY_ESC,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_CTRL
};

struct KeyEvent {
    KeyCode code;
    char ch;
};

class UI {
public:
    UI();
    ~UI();
    void run(Editor& ed);

private:
    bool rawActive_;
    bool vtSupported_;
    string statusMsg_;
#ifdef _WIN32
    unsigned long origInMode_;
    unsigned long origOutMode_;
#else
    termios origTerm_;
#endif

    void enableRaw();
    void disableRaw();
    void resetScreen();
    KeyEvent readKey();
    string promptLine(const string& prompt);
    bool promptInt(const string& prompt, int& out);
    void render(const Editor& ed);
};