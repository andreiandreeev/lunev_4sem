#include "bitarray.hpp"

void bitarray::swap(bitarray &lhs, bitarray &rhs) {
    std::swap(lhs.arr_, rhs.arr_);
    std::swap(lhs.size_, rhs.size_);
    std::swap(lhs.chrsize_, rhs.chrsize_);
    std::swap(lhs.capacity_, rhs.capacity_);
}

bitarray::bitarray(int size, bool filling)
        :
        arr_((size == 0) ?
             nullptr : new u_char[(static_cast<int>(size / 8) + 1)]{}),
        size_(size),
        chrsize_((size == 0) ?
                 0 : static_cast<int>(size / 8) + 1),
        capacity_((size == 0) ?
                  0 : static_cast<int>(size / 8) + 1)
{
    if(filling)
        for(int i = 0; i < capacity_; ++i)
            arr_[i] = 0xFF;
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

    if(size_ == capacity_* 8) {
        if(size_ == 0) {
            bitarray tmp(16);

            tmp.resize(0);
            tmp.push_back(expr);

            swap(*this, tmp);
            return;
        }

        bitarray tmp(capacity_ * 2 * 8);
        for(int i = 0; i < capacity_; ++i)
            tmp.arr_[i] = arr_[i];

        tmp.size_       = size_;
        tmp.chrsize_    = chrsize_;
        tmp.push_back(expr);

        swap(*this, tmp);
        return;
    } else {
        ++size_;
        if (static_cast<float>(size_) / 8 > static_cast<float>(chrsize_))
            ++chrsize_;
    }

    uint32_t bitplace = 0;

    if (size_ > 8)
        bitplace = size_ - 8 * (chrsize_ - 1);
    else
        bitplace = size_;

    u_char mask = static_cast<u_char>(expr) << (8 - bitplace);

    if(expr)
        arr_[chrsize_ - 1] = arr_[chrsize_ - 1] | mask;
    else
        arr_[chrsize_ - 1] = arr_[chrsize_ - 1] & (~mask);
}

bool bitarray::operator[](int pos) const {
    if(pos < 0)
        throw std::out_of_range("index can't be negative");

    uint32_t bitplace = static_cast<uint32_t>(pos % 8);
    u_char mask = arr_[static_cast<int>(pos / 8)] << bitplace;

    mask = mask >> 7U;
    return mask != 0;
}

bitarray::proxy bitarray::operator[] (int pos) {
    if(pos < 0)
        throw std::out_of_range("index can't be negative");

    return {arr_[static_cast<int>(pos / 8)],
            static_cast<uint32_t>(pos % 8)};
}

void bitarray::resize(int size) {
    if(size < 0)
        throw std::runtime_error("bad size");

    if(size > capacity_ * 8) {
        bitarray tmp(size);
        for(int i = 0; i < capacity_; ++i)
            tmp.arr_[i] = arr_[i];
        swap(*this, tmp);
        return;
    }

    chrsize_    = (size == 0) ? 0 : static_cast<int>(size / 8) + 1;
    size_       = size;
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

bitarray::proxy::proxy(u_char &element, uint32_t bitplace)
        :
        element_(element),
        bitplace_(bitplace)
{}

bitarray::proxy::operator bool() const {
    u_char mask = element_ << bitplace_;
    mask = mask >> 7U;
    return mask != 0;
}

bitarray::proxy &bitarray::proxy::operator=(bool expr) {
    u_char mask = static_cast<u_char>(true) << (7 - bitplace_);

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