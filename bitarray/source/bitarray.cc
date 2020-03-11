#include "bitarray.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

void bitarray::swap(bitarray &lhs, bitarray &rhs) {
    std::swap(lhs.arr_, rhs.arr_);
    std::swap(lhs.size_, rhs.size_);
    std::swap(lhs.chrsize_, rhs.chrsize_);
    std::swap(lhs.capacity_, rhs.capacity_);
}

bitarray::bitarray(int size, bool filling)
        :
        arr_(nullptr),
        size_(size),
        chrsize_(0),
        capacity_(0)
{
    if(size != 0) {
        if(size < 0)
            throw std::out_of_range("bad size in ctor");

        arr_        = new uint64_t[(static_cast<int>((size + 63)/ 64))]{};
        chrsize_    = static_cast<int>(size / 64) + 1;
        capacity_   = static_cast<int>(size / 64) + 1;
    }

    if(filling)
        for(int i = 0; i < capacity_; ++i)
            arr_[i] = 0xFFFFFFFFFFFFFFFF;
}

bitarray::~bitarray() noexcept {
    delete [] arr_;
}

bitarray::bitarray(const bitarray &other)
        :
        bitarray(other.size_)
{
    for(int i = 0; i < std::min(capacity_, other.capacity_); ++i)
        arr_[i] = other.arr_[i];

    size_       = other.size_;
    chrsize_    = other.chrsize_;
}

bitarray::bitarray(bitarray &&other) noexcept {
    arr_        = other.arr_;
    size_       = other.size_;
    capacity_   = other.capacity_;

    other.arr_ = nullptr;
}

bitarray &bitarray::operator=(const bitarray &other) {
    if(this != &other) {
        bitarray tmp(other);
        swap(*this, tmp);
    }
    return *this;
}

bitarray &bitarray::operator=(bitarray &&other) noexcept {
    swap(*this, other);
    return *this;
}

void bitarray::push_back(bool expr) {

    if(size_ == capacity_* 64) {
        if(size_ == 0) {
            bitarray tmp(16);

            tmp.resize(0);
            tmp.push_back(expr);

            swap(*this, tmp);
            return;
        }

        bitarray tmp(capacity_ * 2 * 64);
        for(int i = 0; i < capacity_; ++i)
            tmp.arr_[i] = arr_[i];

        tmp.size_       = size_;
        tmp.chrsize_    = chrsize_;
        tmp.push_back(expr);

        swap(*this, tmp);
        return;
    } else {
        ++size_;
        if (static_cast<float>(size_) / 64 > static_cast<float>(chrsize_))
            ++chrsize_;
    }

    uint32_t bitplace = 0;

    if (size_ > 64)
        bitplace = size_ - 64 * (chrsize_ - 1);
    else
        bitplace = size_;

    uint64_t mask = static_cast<uint64_t>(expr) << (64 - bitplace);

    if(expr)
        arr_[chrsize_ - 1] = arr_[chrsize_ - 1] | mask;
    else
        arr_[chrsize_ - 1] = arr_[chrsize_ - 1] & (~mask);
}

bool bitarray::operator[](int pos) const {
    if(pos < 0)
        throw std::out_of_range("index can't be negative");

    uint32_t bitplace = static_cast<uint32_t>(pos % 64);
    uint64_t mask = arr_[static_cast<int>(pos / 64)] << bitplace;

    mask = mask >> 63U;
    return mask != 0;
}

bitarray::proxy bitarray::operator[] (int pos) {
    if(pos < 0)
        throw std::out_of_range("index can't be negative");

    return {arr_[static_cast<int>(pos / 64)],
            static_cast<uint32_t>(pos % 64)};
}

void bitarray::resize(int size) {
    if(size < 0)
        throw std::runtime_error("bad size");

    if(size > capacity_ * 64) {
        bitarray tmp(size);
        for(int i = 0; i < capacity_; ++i)
            tmp.arr_[i] = arr_[i];
        swap(*this, tmp);
        return;
    }

    if (size_ == 0)
        chrsize_ = 0;
    else
        chrsize_ = static_cast<int>(size / 64) + 1;

    size_ = size;
}

bitarray::iterator bitarray::at(int pos) {
    if(pos >= size_)
        throw std::out_of_range("index greater than size");
    return {*this, pos};
}

bitarray::const_iterator bitarray::at(int pos) const {
    if(pos >= size_)
        throw std::out_of_range("index greater than size");
    return {*this, pos};
}

int bitarray::size() const noexcept { return size_; }

bitarray::iterator bitarray::begin() { return {*this, 0}; }

bitarray::iterator bitarray::end() { return {*this, size_}; }

bitarray::const_iterator bitarray::begin() const { return {*this, 0}; }

bitarray::const_iterator bitarray::end() const { return {*this, size_}; }

int bitarray::find_bit(int first_bit, int last_bit, uint64_t var, bool elem) {
    if(elem) {
        for(int i = first_bit; i < last_bit; i++)
            if (var & (0x8000000000000000 >> i))
                return i;
    } else {
        for(int i = first_bit; i < last_bit; ++i)
            if ((var & (0x8000000000000000 >> i)) == 0)
                return i;
    }

    return -1;
}

int bitarray::find(int first, int last, bool elem) const {
    if ((first >= size_ || first < 0) || (last > size_ || last < 0) || last < first)
        throw std::out_of_range("out of range");
    int nemo = 0;

    // all intervals [first, last)

    uint64_t* first_ptr  = arr_ + first / 64;
    uint64_t* last_ptr   = arr_ + last / 64 + static_cast<int>(last % 64 != 0);

    int first_bit = first % 64;
    int last_bit  = last % 64;

    if(first_bit != 0) {
        if(first_ptr != last_ptr - 1)
            last_bit = 64;
        if ((nemo = find_bit(first_bit, last_bit, *first_ptr, elem)) != -1)
            return static_cast<int>(first_ptr - arr_) * 64 + nemo;
        else
            ++first_ptr;
    }

    std::function<bool(uint64_t)> predicat;

    if(elem)
        predicat = [](uint64_t elem) -> bool {return elem != 0;};
    else
        predicat = [](uint64_t elem) -> bool {return elem != UINT64_MAX;};

    auto nemo_bay = std::find_if(first_ptr, last_ptr, predicat);

    if(nemo_bay == last_ptr)
        return -1;

    if(nemo_bay != last_ptr - 1)
        last_bit = 64;
    else
        last_bit = last % 64;

    if(nemo_bay != first_ptr)
        first_bit = 0;

    nemo = find_bit(first_bit, last_bit, *nemo_bay, elem);

    if (nemo == -1)
        return -1;
    else
        return static_cast<int>(first_ptr - arr_) * 64 + nemo;
}

bitarray::proxy::proxy(uint64_t &element, uint32_t bitplace)
        :
        element_(element),
        bitplace_(bitplace)
{}

bitarray::proxy::operator bool() const {
    uint64_t mask = element_ << bitplace_;
    mask = mask >> 63U;
    return mask != 0;
}

bitarray::proxy &bitarray::proxy::operator=(bool expr) {
    uint64_t mask = static_cast<uint64_t>(true) << (63 - bitplace_);

    if(expr)
        element_ = element_ | mask;
    else
        element_ = element_ & (~mask);


    return *this;
}

bitarray::iterator::iterator(bitarray &bitarr, int cur_pos)
        :
        bitarr_(bitarr),
        cur_pos_(cur_pos)
{}

bitarray::iterator &bitarray::iterator::operator++() {
    ++cur_pos_;
    return *this;
}

bitarray::iterator bitarray::iterator::operator++(int) {
    iterator tmp = *this;
    ++(*this);
    return tmp;
}

bitarray::iterator &bitarray::iterator::operator--() {
    --cur_pos_;
    return *this;
}

bitarray::iterator bitarray::iterator::operator--(int) {
    iterator tmp = *this;
    --(*this);
    return tmp;
}

bitarray::iterator &bitarray::iterator::operator+=(int shift) {
    cur_pos_ += shift;
    return *this;
}

bitarray::iterator &bitarray::iterator::operator-=(int shift) {
    cur_pos_ -= shift;
    return *this;
}

bitarray::proxy bitarray::iterator::operator*() { return bitarr_[cur_pos_]; }

bool bitarray::iterator::operator==(const bitarray::iterator &rhs) {
    return (&bitarr_ == &rhs.bitarr_) && (cur_pos_ == rhs.cur_pos_);
}

bool bitarray::iterator::operator!=(const bitarray::iterator &rhs) { return !(*this == rhs); }

bitarray::const_iterator::const_iterator(const bitarray &bitarr, int cur_pos)
        :
        bitarr_(bitarr),
        cur_pos_(cur_pos)
{}

bitarray::const_iterator &bitarray::const_iterator::operator++() {
    ++cur_pos_;
    return *this;
}

bitarray::const_iterator bitarray::const_iterator::operator++(int) {
    const_iterator tmp = *this;
    ++(*this);
    return tmp;
}

bitarray::const_iterator &bitarray::const_iterator::operator--() {
    --cur_pos_;
    return *this;
}

bitarray::const_iterator bitarray::const_iterator::operator--(int) {
    const_iterator tmp = *this;
    --(*this);
    return tmp;
}

bitarray::const_iterator &bitarray::const_iterator::operator+=(int shift) {
    cur_pos_ += shift;
    return *this;
}

bitarray::const_iterator &bitarray::const_iterator::operator-=(int shift) {
    cur_pos_ -= shift;
    return *this;
}

bool bitarray::const_iterator::operator==(const bitarray::const_iterator &rhs) {
    return (&bitarr_ == &rhs.bitarr_) && (cur_pos_ == rhs.cur_pos_);
}

bool bitarray::const_iterator::operator!=(const bitarray::const_iterator &rhs) { return !(*this == rhs); }

bool bitarray::const_iterator::operator*() { return bitarr_[cur_pos_]; }

bitarray::iterator operator+(int shift, const bitarray::iterator &rhs) {
    auto tmp = rhs;
    return tmp += shift;
}

bitarray::iterator operator+(const bitarray::iterator &lhs, int shift) {
    return shift + lhs;
}

bitarray::iterator operator-(const bitarray::iterator &lhs, int shift) {
    auto tmp = lhs;
    return tmp -= shift;
}

bitarray::const_iterator operator+(int shift, const bitarray::const_iterator &rhs) {
    auto tmp = rhs;
    return tmp += shift;
}

bitarray::const_iterator operator+(const bitarray::const_iterator &lhs, int shift) {
    return shift + lhs;
}

bitarray::const_iterator operator-(const bitarray::const_iterator &lhs, int shift) {
    auto tmp = lhs;
    return tmp -= shift;
}