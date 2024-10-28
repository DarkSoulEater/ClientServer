#pragma once
#include <cstdlib>
#include <cassert>

class DataBuffer {
private:
    char* data_ = nullptr;
    size_t size_ = 0;
public:
    DataBuffer() :  data_(nullptr), size_(0) {}
    DataBuffer(size_t size) : data_(new char[size]), size_(data_ == nullptr ? 0 : size) { assert(data_ != nullptr && "Buffer not allocate"); }
    ~DataBuffer() {
        if (data_ != nullptr) {
            delete[] data_;
        }
    }

    size_t Size() { return size_; }
    char* Buffer() { return data_; }
};