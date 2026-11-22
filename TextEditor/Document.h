// Khai bao cau truc con tro, nut dong va lop Document (danh sach lien ket doi)
#pragma once
#include <string>
#include "DynArray.h"

using namespace std;

struct Cursor {
    int row;
    int col;
    Cursor() : row(0), col(0) {}
    void set(int r, int c) { row = r; col = c; }
};

struct Line {
    string text;
    Line* prev;
    Line* next;
    Line(const string& t = "") : text(t), prev(nullptr), next(nullptr) {}
};

class Document {
public:
    Document();
    ~Document();
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    int lineCount() const { return count_; }
    void clear();
    void setLines(const DynArray<string>& lines);

    string& text(int row);
    const string& text(int row) const;
    const string& textNoCache(int row) const;

    void insertLine(int row, const string& s);
    void appendLine(const string& s);
    string removeLine(int row);
    void splitLine(int row, int col);
    void mergeWithNext(int row);

private:
    Line* head_;
    Line* tail_;
    int count_;
    mutable Line* cacheNode_;
    mutable int cacheRow_;

    Line* nodeAt(int row) const;
    Line* nodeAtNoCache(int row) const;
    void resetCache() const { cacheNode_ = nullptr; cacheRow_ = 0; }
    void freeAll();
};