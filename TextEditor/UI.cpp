// Cai dat giao dien UI console dung bo dem khung hinh va ghi de tai cho de chong giat
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

#include "UI.h"
#include <iostream>
#include <cstdlib>

using namespace std;

UI::UI()
    : rawActive_(false), vtSupported_(true),
    statusMsg_("San sang. Nhan phim de nhap hoac dung phim tat ben duoi.") {
}

UI::~UI() {
    disableRaw();
}

void UI::enableRaw() {
    if (rawActive_) return;
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hIn, &origInMode_);
    GetConsoleMode(hOut, &origOutMode_);
    DWORD inMode = origInMode_;
    inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(hIn, inMode);
    vtSupported_ = (SetConsoleMode(hOut, origOutMode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0);
#else
    if (isatty(STDIN_FILENO)) {
        tcgetattr(STDIN_FILENO, &origTerm_);
        termios raw = origTerm_;
        raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
    vtSupported_ = true;
#endif
    rawActive_ = true;
    resetScreen();
}

void UI::disableRaw() {
    if (!rawActive_) return;
    if (vtSupported_) {
        cout << "\033[?25h\033[?1049l";
        cout.flush();
    }
#ifdef _WIN32
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), origInMode_);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), origOutMode_);
#else
    if (isatty(STDIN_FILENO)) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTerm_);
    }
#endif
    rawActive_ = false;
}

void UI::resetScreen() {
    if (vtSupported_) {
        cout << "\033[?1049h\033[2J\033[H";
        cout.flush();
    }
    else {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
            DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
            DWORD written = 0;
            COORD home = { 0, 0 };
            FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
            SetConsoleCursorPosition(hOut, home);
        }
#endif
    }
}

KeyEvent UI::readKey() {
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 224) {
        int ext = _getch();
        if (ext == 72) return KeyEvent{ KEY_UP, 0 };
        if (ext == 80) return KeyEvent{ KEY_DOWN, 0 };
        if (ext == 75) return KeyEvent{ KEY_LEFT, 0 };
        if (ext == 77) return KeyEvent{ KEY_RIGHT, 0 };
        if (ext == 71) return KeyEvent{ KEY_HOME, 0 };
        if (ext == 79) return KeyEvent{ KEY_END, 0 };
        if (ext == 83) return KeyEvent{ KEY_DELETE, 0 };
        return KeyEvent{ KEY_NONE, 0 };
    }
    if (c == 13) return KeyEvent{ KEY_ENTER, 0 };
    if (c == 8 || c == 127) return KeyEvent{ KEY_BACKSPACE, 0 };
    if (c == 9) return KeyEvent{ KEY_TAB, 0 };
    if (c == 27) return KeyEvent{ KEY_ESC, 0 };
    if (c >= 1 && c <= 26) return KeyEvent{ KEY_CTRL, (char)('A' + c - 1) };
    if (c >= 32) return KeyEvent{ KEY_CHAR, (char)c };
    return KeyEvent{ KEY_NONE, 0 };
#else
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) return KeyEvent{ KEY_ESC, 0 };
    if (c == 27) {
        if (!isatty(STDIN_FILENO)) return KeyEvent{ KEY_ESC, 0 };
        termios tmp = origTerm_;
        tmp.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        tmp.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        tmp.c_cc[VMIN] = 0;
        tmp.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &tmp);

        unsigned char seq[3] = { 0, 0, 0 };
        int n1 = (int)read(STDIN_FILENO, &seq[0], 1);
        int n2 = (n1 == 1) ? (int)read(STDIN_FILENO, &seq[1], 1) : 0;

        tmp.c_cc[VMIN] = 1;
        tmp.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &tmp);

        if (n1 == 0) return KeyEvent{ KEY_ESC, 0 };
        if (seq[0] == '[' && n2 == 1) {
            if (seq[1] == 'A') return KeyEvent{ KEY_UP, 0 };
            if (seq[1] == 'B') return KeyEvent{ KEY_DOWN, 0 };
            if (seq[1] == 'C') return KeyEvent{ KEY_RIGHT, 0 };
            if (seq[1] == 'D') return KeyEvent{ KEY_LEFT, 0 };
            if (seq[1] == 'H') return KeyEvent{ KEY_HOME, 0 };
            if (seq[1] == 'F') return KeyEvent{ KEY_END, 0 };
            if (seq[1] >= '1' && seq[1] <= '8') {
                unsigned char tilde = 0;
                if (read(STDIN_FILENO, &tilde, 1) == 1 && tilde == '~') {
                    if (seq[1] == '1' || seq[1] == '7') return KeyEvent{ KEY_HOME, 0 };
                    if (seq[1] == '3') return KeyEvent{ KEY_DELETE, 0 };
                    if (seq[1] == '4' || seq[1] == '8') return KeyEvent{ KEY_END, 0 };
                }
            }
        }
        else if (seq[0] == 'O' && n2 == 1) {
            if (seq[1] == 'H') return KeyEvent{ KEY_HOME, 0 };
            if (seq[1] == 'F') return KeyEvent{ KEY_END, 0 };
        }
        return KeyEvent{ KEY_NONE, 0 };
    }
    if (c == 13 || c == 10) return KeyEvent{ KEY_ENTER, 0 };
    if (c == 127 || c == 8) return KeyEvent{ KEY_BACKSPACE, 0 };
    if (c == 9) return KeyEvent{ KEY_TAB, 0 };
    if (c >= 1 && c <= 26) return KeyEvent{ KEY_CTRL, (char)('A' + c - 1) };
    if (c >= 32) return KeyEvent{ KEY_CHAR, (char)c };
    return KeyEvent{ KEY_NONE, 0 };
#endif
}

string UI::promptLine(const string& prompt) {
    if (vtSupported_) {
        cout << "\033[?25h\n" << prompt;
        cout.flush();
    }
    else {
        cout << "\n" << prompt;
    }
    disableRaw();
    string s;
    getline(cin, s);
    enableRaw();
    return s;
}

bool UI::promptInt(const string& prompt, int& out) {
    string s = promptLine(prompt);
    if (s.empty()) return false;
    char* endPtr = nullptr;
    long val = strtol(s.c_str(), &endPtr, 10);
    if (*endPtr != '\0') return false;
    out = (int)val;
    return true;
}

void UI::render(const Editor& ed) {
    const Document& doc = ed.doc();
    const Cursor& cur = ed.cursor();
    int total = doc.lineCount();

    string eol = vtSupported_ ? "\033[K\n" : "\n";
    string frame;
    frame.reserve(4096);

    if (vtSupported_) {
        frame += "\033[?25l\033[H";
    }
    else {
#ifdef _WIN32
        COORD home = { 0, 0 };
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), home);
#endif
    }

    frame += "================================================================================" + eol;
    frame += " TEP: " + (ed.fileName().empty() ? string("(chua dat ten)") : ed.fileName())
        + (ed.modified() ? " [*]" : " [Da luu]")
        + " | Dong " + to_string(cur.row + 1) + "/" + to_string(total) + ", Cot " + to_string(cur.col + 1)
        + " | Undo: " + to_string(ed.undoCount())
        + (ed.maxHistory() > 0 ? "/" + to_string(ed.maxHistory()) : "")
        + " | Redo: " + to_string(ed.redoCount())
        + " | Clip: " + to_string(ed.clipboardSize()) + eol;
    frame += "--------------------------------------------------------------------------------" + eol;

    const int viewRows = 20;
    int start = cur.row - (viewRows / 2);
    if (start < 0) start = 0;
    int end = start + viewRows;
    if (end > total) {
        end = total;
        start = end - viewRows;
        if (start < 0) start = 0;
    }

    for (int i = 0; i < viewRows; ++i) {
        int r = start + i;
        if (r < end) {
            string num = to_string(r + 1);
            if (num.size() < 4) num.insert(0, 4 - num.size(), ' ');
            string line = doc.text(r);
            if (r == cur.row) line.insert(cur.col, "|");
            if (!vtSupported_ && line.size() < 72) line.append(72 - line.size(), ' ');
            frame += num + " | " + line + eol;
        }
        else {
            string emptyRow = "   ~ |";
            if (!vtSupported_) emptyRow.append(73, ' ');
            frame += emptyRow + eol;
        }
    }

    frame += "--------------------------------------------------------------------------------" + eol;
    string info = " [Thong bao] " + statusMsg_;
    if (!vtSupported_ && info.size() < 79) info.append(79 - info.size(), ' ');
    frame += info + eol;
    frame += "--------------------------------------------------------------------------------" + eol;
    frame += " Go truc tiep | Enter/Bksp/Del | Mui ten/Home/End | Ctrl+D: Xoa dong" + eol;
    frame += " Ctrl+Z: Undo | Ctrl+Y: Redo   | Ctrl+K: Gioi han k | Ctrl+C/X/V: Copy/Cut/Paste" + eol;
    frame += " Ctrl+F: Tim  | Ctrl+R: Thay   | Ctrl+B: Kiem ngoac | Ctrl+T: Thong ke" + eol;
    frame += " Ctrl+O: Mo   | Ctrl+S: Luu    | Ctrl+G: Do 10^4+   | Esc: Thoat" + eol;
    frame += "================================================================================";
    if (vtSupported_) {
        frame += "\033[K\033[J\033[?25h";
    }

    cout << frame;
    cout.flush();
}

void UI::run(Editor& ed) {
    enableRaw();
    string err;

    while (true) {
        render(ed);
        KeyEvent ev = readKey();

        if (ev.code == KEY_ESC) {
            if (ed.modified()) {
                string ans = promptLine("Co thay doi chua luu, van muon thoat? (y/n): ");
                if (ans != "y" && ans != "Y") {
                    statusMsg_ = "Da huy thoat chuong trinh.";
                    continue;
                }
            }
            disableRaw();
            cout << "Da giai phong bo nho va thoat chuong trinh.\n";
            break;
        }

        if (ev.code == KEY_CHAR) {
            ed.insertText(string(1, ev.ch));
            statusMsg_ = "Da chen ky tu.";
        }
        else if (ev.code == KEY_TAB) {
            ed.insertText("    ");
            statusMsg_ = "Da chen Tab (4 khoang trang).";
        }
        else if (ev.code == KEY_ENTER) {
            ed.enter();
            statusMsg_ = "Da xuong dong.";
        }
        else if (ev.code == KEY_BACKSPACE) {
            ed.backspace();
            statusMsg_ = "Da xoa ky tu truoc con tro.";
        }
        else if (ev.code == KEY_DELETE) {
            ed.del();
            statusMsg_ = "Da xoa ky tu tai con tro.";
        }
        else if (ev.code == KEY_LEFT) {
            ed.moveLeft();
        }
        else if (ev.code == KEY_RIGHT) {
            ed.moveRight();
        }
        else if (ev.code == KEY_UP) {
            ed.moveUp();
        }
        else if (ev.code == KEY_DOWN) {
            ed.moveDown();
        }
        else if (ev.code == KEY_HOME) {
            ed.moveHome();
        }
        else if (ev.code == KEY_END) {
            ed.moveEnd();
        }
        else if (ev.code == KEY_CTRL) {
            switch (ev.ch) {
            case 'Z':
                statusMsg_ = ed.undo() ? "Da hoan tac (Undo)." : "Khong con thao tac de Undo.";
                break;
            case 'Y':
                statusMsg_ = ed.redo() ? "Da lam lai (Redo)." : "Khong con thao tac de Redo.";
                break;
            case 'D':
                ed.deleteLine();
                statusMsg_ = "Da xoa dong hien tai.";
                break;
            case 'K': {
                int k = 0;
                if (!promptInt("Nhap gioi han buoc Undo k (0 = vo han): ", k) || k < 0) {
                    statusMsg_ = "[Loi] Gia tri k phai la so nguyen >= 0.";
                }
                else {
                    ed.setMaxHistory((size_t)k);
                    statusMsg_ = "Da dat gioi han Undo k = " + to_string(k) + ".";
                }
                break;
            }
            case 'C':
            case 'X': {
                int from = 1, to = 1;
                if (!promptInt("Tu dong: ", from) || !promptInt("Den dong: ", to)) {
                    statusMsg_ = "[Loi] Chi so dong phai la so nguyen.";
                }
                else {
                    bool ok = (ev.ch == 'C') ? ed.copyLines(from, to) : ed.cutLines(from, to);
                    if (!ok) statusMsg_ = "[Loi] Khoang dong khong hop le (1.." + to_string(ed.doc().lineCount()) + ").";
                    else statusMsg_ = (ev.ch == 'C') ? "Da sao chep (Copy) vao clipboard." : "Da cat (Cut) vao clipboard.";
                }
                break;
            }
            case 'V':
                statusMsg_ = ed.paste() ? "Da dan (Paste) sau dong hien tai." : "Clipboard dang rong.";
                break;
            case 'F': {
                string kw = promptLine("Nhap tu khoa can tim: ");
                if (kw.empty()) {
                    statusMsg_ = "[Loi] Tu khoa tim kiem rong.";
                }
                else {
                    DynArray<Match> ms = ed.findAll(kw);
                    if (ms.empty()) {
                        statusMsg_ = "Khong tim thay '" + kw + "'.";
                    }
                    else {
                        ed.moveTo(ms[0].row, ms[0].col);
                        statusMsg_ = "Tim thay " + to_string(ms.size()) + " ket qua (dau tien tai dong " +
                            to_string(ms[0].row + 1) + ", cot " + to_string(ms[0].col + 1) + ").";
                    }
                }
                break;
            }
            case 'R': {
                string kw = promptLine("Nhap tu khoa can thay the: ");
                if (kw.empty()) {
                    statusMsg_ = "[Loi] Tu khoa rong.";
                }
                else {
                    string rep = promptLine("Nhap chuoi thay the: ");
                    int n = ed.replaceAll(kw, rep);
                    statusMsg_ = "Da thay the " + to_string(n) + " vi tri (co the Ctrl+Z).";
                }
                break;
            }
            case 'B': {
                BracketResult res = ed.checkBrackets();
                statusMsg_ = res.msg;
                if (!res.ok && res.row >= 0) ed.moveTo(res.row, res.col);
                break;
            }
            case 'T': {
                Stats st = ed.computeStats();
                statusMsg_ = "Thong ke: " + to_string(st.lines) + " dong | " +
                    to_string(st.words) + " tu | " + to_string(st.chars) + " ky tu.";
                break;
            }
            case 'O': {
                if (ed.modified()) {
                    string ans = promptLine("Co thay doi chua luu, van mo tep khac? (y/n): ");
                    if (ans != "y" && ans != "Y") {
                        statusMsg_ = "Da huy mo tep.";
                        break;
                    }
                }
                string path = promptLine("Nhap duong dan tep can mo: ");
                if (ed.openFile(path, err)) statusMsg_ = "Da mo tep: " + path;
                else statusMsg_ = "[Loi] " + err;
                break;
            }
            case 'S': {
                string path = ed.fileName();
                if (path.empty()) path = promptLine("Nhap ten tep can luu: ");
                if (ed.saveFile(path, err)) statusMsg_ = "Da luu tep: " + path;
                else statusMsg_ = "[Loi] " + err;
                break;
            }
            case 'G': {
                int n = 10000;
                if (!promptInt("Nhap so dong toi da de do thoi gian (>= 10000): ", n) || n < 10000) {
                    n = 10000;
                }
                disableRaw();
                ed.runBenchmark(n);
                cout << "Nhan phim bat ky de quay lai trinh soan thao...";
                cout.flush();
                enableRaw();
                readKey();
                resetScreen();
                statusMsg_ = "Da hoan tat do thoi gian voi N = " + to_string(n) + " dong.";
                break;
            }
            default:
                break;
            }
        }
    }
}