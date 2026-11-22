// Cai dat cac thao tac quan ly dong van ban va truy xuat toi uu bang con tro dem
#include "Document.h"
#include <cstdlib>

using namespace std;

Document::Document() : head_(nullptr), tail_(nullptr), count_(0), cacheNode_(nullptr), cacheRow_(0) {
    appendLine("");
}

Document::~Document() { freeAll(); }

void Document::freeAll() {
    Line* p = head_;
    while (p) {
        Line* nx = p->next;
        delete p;
        p = nx;
    }
    head_ = tail_ = nullptr;
    count_ = 0;
    resetCache();
}

void Document::clear() {
    freeAll();
    appendLine("");
}

void Document::setLines(const DynArray<string>& lines) {
    freeAll();
    for (size_t i = 0; i < lines.size(); ++i) appendLine(lines[i]);
    if (count_ == 0) appendLine("");
}

Line* Document::nodeAt(int row) const {
    if (row < 0 || row >= count_) return nullptr;
    Line* p = head_;
    int pos = 0;
    int best = row;

    int dTail = count_ - 1 - row;
    if (dTail < best) { p = tail_; pos = count_ - 1; best = dTail; }

    if (cacheNode_) {
        int dCache = abs(row - cacheRow_);
        if (dCache < best) { p = cacheNode_; pos = cacheRow_; best = dCache; }
    }

    while (pos < row) { p = p->next; ++pos; }
    while (pos > row) { p = p->prev; --pos; }

    cacheNode_ = p;
    cacheRow_ = row;
    return p;
}

Line* Document::nodeAtNoCache(int row) const {
    if (row < 0 || row >= count_) return nullptr;
    Line* p = head_;
    int pos = 0;
    if (count_ - 1 - row < row) { p = tail_; pos = count_ - 1; }
    while (pos < row) { p = p->next; ++pos; }
    while (pos > row) { p = p->prev; --pos; }
    return p;
}

string& Document::text(int row) { return nodeAt(row)->text; }
const string& Document::text(int row) const { return nodeAt(row)->text; }
const string& Document::textNoCache(int row) const { return nodeAtNoCache(row)->text; }

void Document::insertLine(int row, const string& s) {
    if (row < 0) row = 0;
    Line* n = new Line(s);
    if (row >= count_) {
        n->prev = tail_;
        if (tail_) tail_->next = n; else head_ = n;
        tail_ = n;
    }
    else {
        Line* at = nodeAt(row);
        n->next = at;
        n->prev = at->prev;
        if (at->prev) at->prev->next = n; else head_ = n;
        at->prev = n;
    }
    ++count_;
    if (cacheNode_ && row <= cacheRow_) ++cacheRow_;
}

void Document::appendLine(const string& s) { insertLine(count_, s); }

string Document::removeLine(int row) {
    Line* n = nodeAt(row);
    if (!n) return "";
    string t = n->text;
    if (count_ == 1) {
        n->text.clear();
        return t;
    }
    if (n->prev) n->prev->next = n->next; else head_ = n->next;
    if (n->next) n->next->prev = n->prev; else tail_ = n->prev;

    if (cacheNode_ == n) resetCache();
    else if (cacheNode_ && row < cacheRow_) --cacheRow_;

    delete n;
    --count_;
    return t;
}

void Document::splitLine(int row, int col) {
    string& s = nodeAt(row)->text;
    if (col < 0) col = 0;
    if (col > (int)s.size()) col = (int)s.size();
    string rest = s.substr(col);
    s.erase(col);
    insertLine(row + 1, rest);
}

void Document::mergeWithNext(int row) {
    Line* n = nodeAt(row);
    if (!n || !n->next) return;
    n->text += n->next->text;
    removeLine(row + 1);
}