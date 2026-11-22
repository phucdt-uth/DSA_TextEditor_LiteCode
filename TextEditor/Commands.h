// Khai bao Command Pattern, hang doi hai dau CommandDeque va lop lich su History
#pragma once
#include <string>
#include "Document.h"
#include "DynArray.h"

using namespace std;

struct LineChange {
    int row;
    string oldText;
    string newText;
};

class EditCommand {
public:
    virtual ~EditCommand() {}
    virtual void execute(Document& doc, Cursor& cur) = 0;
    virtual void undo(Document& doc, Cursor& cur) = 0;
};

class InsertTextCmd : public EditCommand {
public:
    InsertTextCmd(int row, int col, const string& text);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    int row_, col_;
    string text_;
};

class DeleteTextCmd : public EditCommand {
public:
    DeleteTextCmd(int row, int col, int len, int cursorColBefore);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    int row_, col_, len_, cursorColBefore_;
    string removed_;
};

class SplitLineCmd : public EditCommand {
public:
    SplitLineCmd(int row, int col);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    int row_, col_;
};

class MergeLinesCmd : public EditCommand {
public:
    MergeLinesCmd(int row, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    int row_, beforeRow_, beforeCol_, joinCol_;
};

class DeleteLinesCmd : public EditCommand {
public:
    DeleteLinesCmd(int row, int count, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    int row_, count_, beforeRow_, beforeCol_;
    bool allDeleted_;
    DynArray<string> texts_;
};

class PasteLinesCmd : public EditCommand {
public:
    PasteLinesCmd(int afterRow, const DynArray<string>& lines, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    int afterRow_, beforeRow_, beforeCol_;
    DynArray<string> lines_;
};

class ReplaceAllCmd : public EditCommand {
public:
    ReplaceAllCmd(const DynArray<LineChange>& changes, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur) override;
    void undo(Document& doc, Cursor& cur) override;
private:
    DynArray<LineChange> changes_;
    int beforeRow_, beforeCol_;
};

struct CmdNode {
    EditCommand* cmd;
    CmdNode* prev;
    CmdNode* next;
    CmdNode(EditCommand* c) : cmd(c), prev(nullptr), next(nullptr) {}
};

class CommandDeque {
public:
    CommandDeque() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~CommandDeque() { clear(); }
    CommandDeque(const CommandDeque&) = delete;
    CommandDeque& operator=(const CommandDeque&) = delete;

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void pushBack(EditCommand* cmd);
    EditCommand* popBack();
    void popFrontAndDelete();
    void clear();

private:
    CmdNode* head_;
    CmdNode* tail_;
    size_t size_;
};

class History {
public:
    History() : maxHistory_(0) {}
    void execute(EditCommand* cmd, Document& doc, Cursor& cur);
    bool undo(Document& doc, Cursor& cur);
    bool redo(Document& doc, Cursor& cur);
    void clear();
    void setMaxHistory(size_t k);
    size_t maxHistory() const { return maxHistory_; }
    size_t undoSize() const { return undo_.size(); }
    size_t redoSize() const { return redo_.size(); }

private:
    CommandDeque undo_;
    CommandDeque redo_;
    size_t maxHistory_;
};