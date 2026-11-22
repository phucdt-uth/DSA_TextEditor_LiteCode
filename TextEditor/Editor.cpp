// Cai dat nghiep vu soan thao, clipboard, tim/thay the, kiem tra ngoac, tep va do thoi gian
#include "Editor.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <chrono>
#include <cstdio>

using namespace std;

static int minVal(int a, int b) { return a < b ? a : b; }
static int maxVal(int a, int b) { return a > b ? a : b; }

Editor::Editor() : modified_(false) {}

void Editor::run(EditCommand* cmd) {
    history_.execute(cmd, doc_, cur_);
    modified_ = true;
}

void Editor::moveLeft() {
    if (cur_.col > 0) cur_.col--;
    else if (cur_.row > 0) { cur_.row--; cur_.col = lineLen(cur_.row); }
}
void Editor::moveRight() {
    if (cur_.col < lineLen(cur_.row)) cur_.col++;
    else if (cur_.row + 1 < doc_.lineCount()) { cur_.row++; cur_.col = 0; }
}
void Editor::moveUp() {
    if (cur_.row > 0) { cur_.row--; cur_.col = minVal(cur_.col, lineLen(cur_.row)); }
}
void Editor::moveDown() {
    if (cur_.row + 1 < doc_.lineCount()) { cur_.row++; cur_.col = minVal(cur_.col, lineLen(cur_.row)); }
}
void Editor::moveHome() { cur_.col = 0; }
void Editor::moveEnd() { cur_.col = lineLen(cur_.row); }
void Editor::moveTo(int row, int col) {
    int r = maxVal(0, minVal(doc_.lineCount() - 1, row));
    int c = maxVal(0, minVal(lineLen(r), col));
    cur_.set(r, c);
}

void Editor::insertText(const string& text) {
    if (!text.empty()) run(new InsertTextCmd(cur_.row, cur_.col, text));
}
void Editor::enter() { run(new SplitLineCmd(cur_.row, cur_.col)); }

void Editor::backspace() {
    if (cur_.col > 0) run(new DeleteTextCmd(cur_.row, cur_.col - 1, 1, cur_.col));
    else if (cur_.row > 0) run(new MergeLinesCmd(cur_.row - 1, cur_.row, cur_.col));
}
void Editor::del() {
    if (cur_.col < lineLen(cur_.row)) run(new DeleteTextCmd(cur_.row, cur_.col, 1, cur_.col));
    else if (cur_.row + 1 < doc_.lineCount()) run(new MergeLinesCmd(cur_.row, cur_.row, cur_.col));
}
void Editor::deleteLine() { run(new DeleteLinesCmd(cur_.row, 1, cur_.row, cur_.col)); }

bool Editor::copyLines(int from, int to) {
    if (from < 1 || to < from || to > doc_.lineCount()) return false;
    clipboard_.clear();
    for (int r = from - 1; r <= to - 1; ++r) clipboard_.push_back(doc_.text(r));
    return true;
}
bool Editor::cutLines(int from, int to) {
    if (!copyLines(from, to)) return false;
    run(new DeleteLinesCmd(from - 1, to - from + 1, cur_.row, cur_.col));
    return true;
}
bool Editor::paste() {
    if (clipboard_.empty()) return false;
    run(new PasteLinesCmd(cur_.row, clipboard_, cur_.row, cur_.col));
    return true;
}

bool Editor::undo() {
    bool ok = history_.undo(doc_, cur_);
    if (ok) modified_ = true;
    return ok;
}
bool Editor::redo() {
    bool ok = history_.redo(doc_, cur_);
    if (ok) modified_ = true;
    return ok;
}
void Editor::setMaxHistory(size_t k) { history_.setMaxHistory(k); }

DynArray<Match> Editor::findAll(const string& keyword) const {
    DynArray<Match> result;
    if (keyword.empty()) return result;
    for (int r = 0; r < doc_.lineCount(); ++r) {
        const string& s = doc_.text(r);
        size_t pos = 0;
        while ((pos = s.find(keyword, pos)) != string::npos) {
            Match m{ r, (int)pos };
            result.push_back(m);
            pos += keyword.size();
        }
    }
    return result;
}

int Editor::replaceAll(const string& keyword, const string& replacement) {
    if (keyword.empty()) return 0;
    DynArray<LineChange> changes;
    int count = 0;
    for (int r = 0; r < doc_.lineCount(); ++r) {
        string s = doc_.text(r);
        size_t pos = 0;
        bool changed = false;
        while ((pos = s.find(keyword, pos)) != string::npos) {
            s.replace(pos, keyword.size(), replacement);
            pos += replacement.size();
            ++count;
            changed = true;
        }
        if (changed) changes.push_back(LineChange{ r, doc_.text(r), s });
    }
    if (!changes.empty()) run(new ReplaceAllCmd(changes, cur_.row, cur_.col));
    return count;
}

Stats Editor::computeStats() const {
    Stats st{ doc_.lineCount(), 0, 0, 0 };
    for (int r = 0; r < doc_.lineCount(); ++r) {
        const string& s = doc_.text(r);
        st.chars += (int)s.size();
        bool inWord = false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (isspace((unsigned char)s[i])) inWord = false;
            else if (!inWord) { inWord = true; ++st.words; }
        }
    }
    st.charsWithNewline = st.chars + (st.lines - 1);
    return st;
}

struct BracketNode {
    char ch;
    int row;
    int col;
    BracketNode* next;
    BracketNode(char c, int r, int cl, BracketNode* nx) : ch(c), row(r), col(cl), next(nx) {}
};

static void freeBrackets(BracketNode* top) {
    while (top) {
        BracketNode* tmp = top;
        top = top->next;
        delete tmp;
    }
}

BracketResult Editor::checkBrackets() const {
    BracketNode* top = nullptr;
    for (int r = 0; r < doc_.lineCount(); ++r) {
        const string& s = doc_.text(r);
        for (int c = 0; c < (int)s.size(); ++c) {
            char ch = s[c];
            if (ch == '(' || ch == '[' || ch == '{') {
                top = new BracketNode(ch, r, c, top);
            }
            else if (ch == ')' || ch == ']' || ch == '}') {
                if (!top) {
                    return BracketResult{ false, r, c,
                        "Thua ngoac dong '" + string(1, ch) + "' tai dong " +
                        to_string(r + 1) + ", cot " + to_string(c + 1) };
                }
                char expected = (top->ch == '(') ? ')' : (top->ch == '[') ? ']' : '}';
                if (ch != expected) {
                    int openR = top->row, openC = top->col;
                    char openCh = top->ch;
                    freeBrackets(top);
                    return BracketResult{ false, r, c,
                        "Ngoac '" + string(1, ch) + "' tai dong " + to_string(r + 1) +
                        ", cot " + to_string(c + 1) + " khong khop '" + string(1, openCh) +
                        "' o dong " + to_string(openR + 1) + ", cot " + to_string(openC + 1) };
                }
                BracketNode* matched = top;
                top = top->next;
                delete matched;
            }
        }
    }
    if (top) {
        int openR = top->row, openC = top->col;
        char openCh = top->ch;
        freeBrackets(top);
        return BracketResult{ false, openR, openC,
            "Thieu ngoac dong cho '" + string(1, openCh) + "' tai dong " +
            to_string(openR + 1) + ", cot " + to_string(openC + 1) };
    }
    return BracketResult{ true, -1, -1, "Tat ca cap ngoac () [] {} deu can bang!" };
}

bool Editor::openFile(const string& path, string& err) {
    ifstream in(path.c_str(), ios::binary);
    if (!in.is_open()) { err = "Khong mo duoc tep: " + path; return false; }

    DynArray<string> lines;
    string line;
    bool first = true;
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (first && line.size() >= 3 && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
            line.erase(0, 3);
        }
        first = false;
        string out;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '\t') out += "    ";
            else out += line[i];
        }
        lines.push_back(out);
    }
    doc_.setLines(lines);
    cur_ = Cursor();
    history_.clear();
    fileName_ = path;
    modified_ = false;
    return true;
}

bool Editor::saveFile(const string& path, string& err) {
    ofstream out(path.c_str(), ios::binary | ios::trunc);
    if (!out.is_open()) { err = "Khong ghi duoc tep: " + path; return false; }
    for (int r = 0; r < doc_.lineCount(); ++r) {
        out << doc_.text(r);
        if (r + 1 < doc_.lineCount()) out << '\n';
    }
    out.flush();
    if (!out.good()) { err = "Loi khi ghi tep: " + path; return false; }
    fileName_ = path;
    modified_ = false;
    return true;
}

void Editor::runBenchmark(int maxLines) const {
    if (maxLines < 10000) maxLines = 10000;
    int sizes[4] = { 30, 1000, 5000, maxLines };

    cout << "\n=== BANG DO THOI GIAN THUC NGHIEM (Don vi: ms) ===\n";
    cout << left << setw(12) << "So dong N"
        << setw(16) << "Doc tep (ms)"
        << setw(22) << "1000 edit CO Cache"
        << setw(24) << "1000 edit KHONG Cache"
        << setw(18) << "Tim kiem (ms)"
        << setw(18) << "Kiem ngoac (ms)" << "\n";
    cout << string(110, '-') << "\n";

    for (int idx = 0; idx < 4; ++idx) {
        int n = sizes[idx];
        string tmpPath = "benchmark_temp.txt";
        {
            ofstream out(tmpPath.c_str(), ios::binary | ios::trunc);
            for (int i = 0; i < n; ++i) {
                out << "int value_" << i << " = func(arr[" << (i % 10) << "]);\n";
            }
        }

        Editor tempEd;
        string err;

        auto t1 = chrono::high_resolution_clock::now();
        tempEd.openFile(tmpPath, err);
        auto t2 = chrono::high_resolution_clock::now();
        double msLoad = chrono::duration<double, milli>(t2 - t1).count();

        int mid = n / 2;
        volatile size_t sink = 0;
        auto t3 = chrono::high_resolution_clock::now();
        for (int k = 0; k < 1000; ++k) {
            int r = mid + (k % 3) - 1;
            if (r >= 0 && r < tempEd.doc().lineCount()) sink += tempEd.doc().text(r).size();
        }
        auto t4 = chrono::high_resolution_clock::now();
        double msCache = chrono::duration<double, milli>(t4 - t3).count();

        auto t5 = chrono::high_resolution_clock::now();
        for (int k = 0; k < 1000; ++k) {
            int r = mid + (k % 3) - 1;
            if (r >= 0 && r < tempEd.doc().lineCount()) sink += tempEd.doc().textNoCache(r).size();
        }
        auto t6 = chrono::high_resolution_clock::now();
        double msNoCache = chrono::duration<double, milli>(t6 - t5).count();
        (void)sink;

        auto t7 = chrono::high_resolution_clock::now();
        DynArray<Match> ms = tempEd.findAll("func");
        auto t8 = chrono::high_resolution_clock::now();
        double msFind = chrono::duration<double, milli>(t8 - t7).count();
        (void)ms;

        auto t9 = chrono::high_resolution_clock::now();
        BracketResult br = tempEd.checkBrackets();
        auto t10 = chrono::high_resolution_clock::now();
        double msBracket = chrono::duration<double, milli>(t10 - t9).count();
        (void)br;

        cout << left << fixed << setprecision(3)
            << setw(12) << n
            << setw(16) << msLoad
            << setw(22) << msCache
            << setw(24) << msNoCache
            << setw(18) << msFind
            << setw(18) << msBracket << "\n";
    }
    remove("benchmark_temp.txt");
    cout << string(110, '-') << "\n";
}