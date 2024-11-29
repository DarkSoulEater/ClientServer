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
    DataBuffer(size_t size) : data_((char*)calloc(sizeof(char), size+ 1)), size_(data_ == nullptr ? 0 : size) { assert(data_ != nullptr && "Buffer not allocate"); }
    DataBuffer(const std::string& str) : data_(new char[str.size() + 1]), size_(str.size() + 1) {
        memcpy(data_, str.c_str(), str.size());
    }
    ~DataBuffer() {
        Clear();
    }

    DataBuffer& operator=(DataBuffer&& other) {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() { return size_; }
    char* Buffer() { return data_; }
    const char* Buffer() const { return data_; }
    void Clear() { 
        if (data_ != nullptr) {
            free(data_);
        }
        data_ = nullptr;
        size_ = 0;
    }
    void Resize(size_t new_size) {
        Clear();
        data_ = (char*)calloc(sizeof(char), (size_ = new_size) + 1);
    }
};