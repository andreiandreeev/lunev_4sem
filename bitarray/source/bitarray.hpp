#pragma once

#include <cstdint>
#include <stdexcept>

class bitarray final {
    uint64_t* arr_                = nullptr;

    int size_                   = 0;
    int chrsize_                = 0;
    int capacity_               = 0;

public:

    class proxy final {
        uint64_t& element_;
        uint32_t bitplace_;
    public:
        proxy(uint64_t& element, uint32_t bitplace);

        operator bool() const;

        proxy& operator= (bool expr);
    };

    class iterator final {
        bitarray& bitarr_;
        int cur_pos_;
    public:
        iterator(bitarray& bitarr, int cur_pos);

        proxy operator* ();

        iterator& operator++();

        iterator operator++(int);

        iterator& operator--();

        iterator operator--(int);

        iterator& operator+=(int shift);

        iterator& operator-=(int shift);

        bool operator== (const iterator& rhs);

        bool operator!= (const iterator& rhs);
    };

    class const_iterator final {
        const bitarray& bitarr_;
        int cur_pos_;
    public:
        const_iterator(const bitarray& bitarr, int cur_pos);

        bool operator* ();

        const_iterator& operator++();

        const_iterator operator++(int);

        const_iterator& operator--();

        const_iterator operator--(int);

        const_iterator& operator+=(int shift);

        const_iterator& operator-=(int shift);

        bool operator== (const const_iterator& rhs);

        bool operator!= (const const_iterator& rhs);
    };

    static void swap(bitarray& lhs, bitarray& rhs);

    bitarray() = default;

    explicit bitarray(int size, bool filling = false);

    ~bitarray() noexcept;

    bitarray(const bitarray& other);

    bitarray(bitarray&& other) noexcept;

    bitarray& operator= (const bitarray& other);

    bitarray& operator= (bitarray&& other) noexcept;

    void push_back(bool expr);

    bool operator [] (int pos) const;

    proxy operator [] (int pos);

    int size() const noexcept;

    void resize(int size);

    iterator begin();

    iterator end();

    iterator at(int pos);

    const_iterator begin() const;

    const_iterator end() const;

    const_iterator at(int pos) const;

// finding elem in [first, last) range
    int find(int first, int last, bool elem) const;

    static int find_bit(int first_bit, int last_bit, uint64_t var, bool elem);
};

bitarray::iterator operator+ (int shift, const bitarray::iterator& rhs);

bitarray::iterator operator+ (const bitarray::iterator& rhs, int shift);

bitarray::iterator operator- (const bitarray::iterator& rhs, int shift);

bitarray::const_iterator operator+ (int shift, const bitarray::const_iterator& rhs);

bitarray::const_iterator operator+ (const bitarray::const_iterator& rhs, int shift);

bitarray::const_iterator operator- (const bitarray::const_iterator& rhs, int shift);