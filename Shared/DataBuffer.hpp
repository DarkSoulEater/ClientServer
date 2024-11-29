#pragma once
#include <cstdlib>
#include <cstring>
#include <cassert>

class DataBuffer {
private:
    char* data_ = nullptr;
    size_t size_ = 0;
public:
    DataBuffer() :  data_(nullptr), size_(0) {}
    DataBuffer(size_t size) : data_(new char[size]), size_(data_ == nullptr ? 0 : size) { assert(data_ != nullptr && "Buffer not allocate"); }
    DataBuffer(const std::string& str) : data_(new char[str.size() + 1]), size_(str.size() + 1) {
        memcpy(data_, str.c_str(), str.size());
    }
    ~DataBuffer() {
        if (data_ != nullptr) {
            delete[] data_;
        }
    }

    size_t Size() { return size_; }
    char* Buffer() { return data_; }
    const char* Buffer() const { return data_; }
    void Clear() { 
        if (data_ != nullptr) {
            delete[] data_;
        }
        data_ = nullptr;
        size_ = 0;
    }
    void Resize(size_t new_size) {
        Clear();
        data_ = new char[size_ = new_size];
    }
};