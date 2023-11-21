#pragma once
#include <cstdint>
#include <iterator>

class Container {
    char* addr_;
    char* ptr_;
    uint64_t size_;

public:
    Container(char* ptr, uint64_t n) : addr_{ptr}, size_{n} {
        ptr_ = addr_;
    }

    using iterator_category = std::random_access_iterator_tag;
    using iterator = char*;
    using const_iterator = char const*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;



    iterator begin()  { return addr_; }
    iterator end() { return addr_ + size_; }
    const_iterator cbegin() const { return addr_; }
    const_iterator cend() const { return addr_ + size_; }
    reverse_iterator rbegin()  { return reverse_iterator(addr_ + size_); }
    reverse_iterator rend() { return reverse_iterator(addr_); }
    const_reverse_iterator crbegin() const { return reverse_iterator(addr_ + size_);}
    const_reverse_iterator crend() const { return reverse_iterator(addr_); }



};
