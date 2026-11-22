// Mang dong template tu cai dat thay the std::vector
#pragma once
#include <cstddef>

using namespace std;

template <typename T>
class DynArray {
public:
    DynArray() : data_(nullptr), size_(0), cap_(0) {}

    DynArray(const DynArray& other) : data_(nullptr), size_(0), cap_(0) {
        if (other.size_ > 0) {
            reserve(other.size_);
            for (size_t i = 0; i < other.size_; ++i) data_[i] = other.data_[i];
            size_ = other.size_;
        }
    }

    DynArray& operator=(const DynArray& other) {
        if (this != &other) {
            clear();
            if (other.size_ > 0) {
                reserve(other.size_);
                for (size_t i = 0; i < other.size_; ++i) data_[i] = other.data_[i];
                size_ = other.size_;
            }
        }
        return *this;
    }

    ~DynArray() { delete[] data_; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { size_ = 0; }

    void push_back(const T& val) {
        if (size_ == cap_) reserve(cap_ == 0 ? 8 : cap_ * 2);
        data_[size_++] = val;
    }

    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }

private:
    T* data_;
    size_t size_;
    size_t cap_;

    void reserve(size_t newCap) {
        if (newCap <= cap_) return;
        T* newData = new T[newCap];
        for (size_t i = 0; i < size_; ++i) newData[i] = data_[i];
        delete[] data_;
        data_ = newData;
        cap_ = newCap;
    }
};