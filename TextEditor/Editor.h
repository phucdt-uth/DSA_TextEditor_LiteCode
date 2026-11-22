// Khai bao lop dieu phoi Editor cung cac cau truc tim kiem, thong ke va kiem tra ngoac
#pragma once
#include <string>
#include "DynArray.h"
#include "Document.h"
#include "Commands.h"

using namespace std;

struct Match {
    int row;
    int col;
};

struct Stats {
    int lines;
    int words;
    int chars;
    int charsWithNewline;
};

struct BracketResult {
    bool ok;
    int row;
    int col;
    string msg;
};

class Editor {
public:
    Editor();

    const Document& doc() const { return doc_; }
    const Cursor& cursor() const { return cur_; }
    const string& fileName() const { return fileName_; }
    void setFileName(const string& n) { fileName_ = n; }
    bool modified() const { return modified_; }
    size_t undoCount() const { return history_.undoSize(); }
    size_t redoCount() const { return history_.redoSize(); }
    size_t maxHistory() const { return history_.maxHistory(); }
    size_t clipboardSize() const { return clipboard_.size(); }

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    void moveHome();
    void moveEnd();
    void moveTo(int row, int col);

    void insertText(const string& text);
    void enter();
    void backspace();
    void del();
    void deleteLine();

    bool copyLines(int from, int to);
    bool cutLines(int from, int to);
    bool paste();

    bool undo();
    bool redo();
    void setMaxHistory(size_t k);

    DynArray<Match> findAll(const string& keyword) const;
    int replaceAll(const string& keyword, const string& replacement);
    Stats computeStats() const;
    BracketResult checkBrackets() const;

    bool openFile(const string& path, string& err);
    bool saveFile(const string& path, string& err);
    void runBenchmark(int maxLines = 10000) const;

private:
    Document doc_;
    Cursor cur_;
    History history_;
    DynArray<string> clipboard_;
    string fileName_;
    bool modified_;

    void run(EditCommand* cmd);
    int lineLen(int row) const { return (int)doc_.text(row).size(); }
};