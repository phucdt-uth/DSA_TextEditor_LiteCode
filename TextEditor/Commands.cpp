// Cai dat thuc thi, hoan tac cac lenh chinh sua va quan ly lich su Undo/Redo
#include "Commands.h"

using namespace std;

static int clampMin(int a, int b) { return a < b ? a : b; }

InsertTextCmd::InsertTextCmd(int row, int col, const string& text)
    : row_(row), col_(col), text_(text) {
}

void InsertTextCmd::execute(Document& doc, Cursor& cur) {
    doc.text(row_).insert(col_, text_);
    cur.set(row_, col_ + (int)text_.size());
}
void InsertTextCmd::undo(Document& doc, Cursor& cur) {
    doc.text(row_).erase(col_, text_.size());
    cur.set(row_, col_);
}

DeleteTextCmd::DeleteTextCmd(int row, int col, int len, int cursorColBefore)
    : row_(row), col_(col), len_(len), cursorColBefore_(cursorColBefore) {
}

void DeleteTextCmd::execute(Document& doc, Cursor& cur) {
    string& s = doc.text(row_);
    removed_ = s.substr(col_, len_);
    s.erase(col_, len_);
    cur.set(row_, col_);
}
void DeleteTextCmd::undo(Document& doc, Cursor& cur) {
    doc.text(row_).insert(col_, removed_);
    cur.set(row_, cursorColBefore_);
}

SplitLineCmd::SplitLineCmd(int row, int col) : row_(row), col_(col) {}

void SplitLineCmd::execute(Document& doc, Cursor& cur) {
    doc.splitLine(row_, col_);
    cur.set(row_ + 1, 0);
}
void SplitLineCmd::undo(Document& doc, Cursor& cur) {
    doc.mergeWithNext(row_);
    cur.set(row_, col_);
}

MergeLinesCmd::MergeLinesCmd(int row, int cursorRowBefore, int cursorColBefore)
    : row_(row), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore), joinCol_(0) {
}

void MergeLinesCmd::execute(Document& doc, Cursor& cur) {
    joinCol_ = (int)doc.text(row_).size();
    doc.mergeWithNext(row_);
    cur.set(row_, joinCol_);
}
void MergeLinesCmd::undo(Document& doc, Cursor& cur) {
    doc.splitLine(row_, joinCol_);
    cur.set(beforeRow_, beforeCol_);
}

DeleteLinesCmd::DeleteLinesCmd(int row, int count, int cursorRowBefore, int cursorColBefore)
    : row_(row), count_(count), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore), allDeleted_(false) {
}

void DeleteLinesCmd::execute(Document& doc, Cursor& cur) {
    texts_.clear();
    allDeleted_ = (count_ >= doc.lineCount());
    for (int i = 0; i < count_; ++i) texts_.push_back(doc.removeLine(row_));
    cur.set(clampMin(row_, doc.lineCount() - 1), 0);
}
void DeleteLinesCmd::undo(Document& doc, Cursor& cur) {
    if (allDeleted_) {
        doc.text(0) = texts_[0];
        for (size_t i = 1; i < texts_.size(); ++i) doc.insertLine((int)i, texts_[i]);
    }
    else {
        for (size_t i = 0; i < texts_.size(); ++i) doc.insertLine(row_ + (int)i, texts_[i]);
    }
    cur.set(beforeRow_, beforeCol_);
}

PasteLinesCmd::PasteLinesCmd(int afterRow, const DynArray<string>& lines, int cursorRowBefore, int cursorColBefore)
    : afterRow_(afterRow), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore), lines_(lines) {
}

void PasteLinesCmd::execute(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < lines_.size(); ++i) doc.insertLine(afterRow_ + 1 + (int)i, lines_[i]);
    int last = afterRow_ + (int)lines_.size();
    cur.set(last, (int)doc.text(last).size());
}
void PasteLinesCmd::undo(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < lines_.size(); ++i) doc.removeLine(afterRow_ + 1);
    cur.set(beforeRow_, beforeCol_);
}

ReplaceAllCmd::ReplaceAllCmd(const DynArray<LineChange>& changes, int cursorRowBefore, int cursorColBefore)
    : changes_(changes), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore) {
}

void ReplaceAllCmd::execute(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < changes_.size(); ++i) doc.text(changes_[i].row) = changes_[i].newText;
    cur.set(cur.row, clampMin(cur.col, (int)doc.text(cur.row).size()));
}
void ReplaceAllCmd::undo(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < changes_.size(); ++i) doc.text(changes_[i].row) = changes_[i].oldText;
    cur.set(beforeRow_, beforeCol_);
}

void CommandDeque::pushBack(EditCommand* cmd) {
    CmdNode* n = new CmdNode(cmd);
    n->prev = tail_;
    if (tail_) tail_->next = n; else head_ = n;
    tail_ = n;
    ++size_;
}

EditCommand* CommandDeque::popBack() {
    if (!tail_) return nullptr;
    CmdNode* n = tail_;
    EditCommand* c = n->cmd;
    tail_ = n->prev;
    if (tail_) tail_->next = nullptr; else head_ = nullptr;
    delete n;
    --size_;
    return c;
}

void CommandDeque::popFrontAndDelete() {
    if (!head_) return;
    CmdNode* n = head_;
    head_ = n->next;
    if (head_) head_->prev = nullptr; else tail_ = nullptr;
    delete n->cmd;
    delete n;
    --size_;
}

void CommandDeque::clear() {
    CmdNode* p = head_;
    while (p) {
        CmdNode* nx = p->next;
        delete p->cmd;
        delete p;
        p = nx;
    }
    head_ = tail_ = nullptr;
    size_ = 0;
}

void History::clear() {
    undo_.clear();
    redo_.clear();
}

void History::setMaxHistory(size_t k) {
    maxHistory_ = k;
    if (maxHistory_ > 0) {
        while (undo_.size() > maxHistory_) undo_.popFrontAndDelete();
    }
}

void History::execute(EditCommand* cmd, Document& doc, Cursor& cur) {
    cmd->execute(doc, cur);
    redo_.clear();
    undo_.pushBack(cmd);
    if (maxHistory_ > 0 && undo_.size() > maxHistory_) {
        undo_.popFrontAndDelete();
    }
}

bool History::undo(Document& doc, Cursor& cur) {
    if (undo_.empty()) return false;
    EditCommand* cmd = undo_.popBack();
    cmd->undo(doc, cur);
    redo_.pushBack(cmd);
    return true;
}

bool History::redo(Document& doc, Cursor& cur) {
    if (redo_.empty()) return false;
    EditCommand* cmd = redo_.popBack();
    cmd->execute(doc, cur);
    undo_.pushBack(cmd);
    if (maxHistory_ > 0 && undo_.size() > maxHistory_) {
        undo_.popFrontAndDelete();
    }
    return true;
}