#include <gtest/gtest.h>
#include "../source/bitarray.hpp"

TEST(bitarrayGoodAlloc, Constructors) {

    ASSERT_THROW(bitarray bad_vec(-1), std::out_of_range);

    bitarray v(100, true);
    ASSERT_EQ(v.size(), 100);

    bitarray v1(v);
    ASSERT_EQ(v1.size(), 100);

    for(int i = 0 ; i < v.size(); i++)
        ASSERT_EQ(v1[i], v[i]);

    bitarray v_created_by_rvalue = std::move(bitarray{});
    ASSERT_EQ(v_created_by_rvalue.size(), 0);
}

TEST(bitarrayGoodAlloc, Assignments) {

    bitarray v(0, false);
    ASSERT_EQ(v.size(), 0);

    bitarray other(13, true);
    ASSERT_EQ(other.size(), 13);
    v = other;
    ASSERT_EQ(v.size(), other.size());

    for(int i = 0; i < other.size(); i++)
        ASSERT_EQ(v[i], other[i]);

    bitarray v1;
    ASSERT_EQ(v1.size(), 0);
    v1 = v = other;
    ASSERT_EQ(v1.size(), other.size());

    for(int i = 0; i < other.size(); i++)
        ASSERT_EQ(v1[i], other[i]);

    v1 = std::move(bitarray{41, false});
    ASSERT_EQ(v1.size(), 41);

    for(int i = 0; i < v1.size(); i++)
        ASSERT_EQ(v1[i], false);
}

TEST(bitarrayGoodAlloc, PushBack) {

    bitarray v;
    int n = 100;
    
    std::vector<bool> varr(n, false);

    for(unsigned int i = 0; i < varr.size() - 1; i += 3)
        varr[i] = true;

    for(int i = 0 ; i < n; i++)
        v.push_back(varr[i]);

    ASSERT_EQ(v.size(), n);

    for(int i = 0; i < n; i++)
        ASSERT_EQ(v[i], varr[i]);

    // operator[] for const object:
    const bitarray v_const = v;

    for(int i = 0 ; i < v_const.size(); i++)
        ASSERT_EQ(v_const[i], v[i]);
}

TEST(bitarrayGoodAlloc, BadRanges) {

    bitarray v;
    ASSERT_THROW(v[-1] , std::out_of_range);

    v.push_back(true);
    ASSERT_THROW(v.at(2), std::out_of_range);

    const bitarray v1(2, true);
    ASSERT_THROW(v1[-1], std::out_of_range);

    ASSERT_THROW(v1.at(3), std::out_of_range);
}

TEST(bitarrayGoodAlloc, Resize) {

    bitarray v;
    v.resize(123);

    ASSERT_EQ(v.size(), 123);

    v.resize(140);
    ASSERT_EQ(v.size(), 140);

    std::vector<bool> values(140);
    values.resize(0);

    for(int i = 0; i < v.size(); i++) {
        if(i % 2)
            values.push_back(v[i] = true);
        else
            values.push_back(v[i] = false);
    }

    v.resize(10);
    ASSERT_EQ(v.size(), 10);

    for(int i = 0; i < v.size(); i++)
        ASSERT_EQ(v[i], values[i]);

    ASSERT_THROW(v.resize(-1), std::runtime_error);

    v.resize(0);
    ASSERT_EQ(v.size(), 0);
}

TEST(bitarrayGoodAlloc, iterators) {

    bitarray v(100, true);
    ASSERT_EQ(v.size(), 100);

    for(auto elem : v)
        ASSERT_EQ(elem, true);

    *(v.at(41)) = false;

    ASSERT_EQ(v[41], false);
    ASSERT_EQ(*v.at(41), false);

    ASSERT_EQ(*v.begin(), 1);

    auto it = v.at(41);

    auto save_it = it++;
    ASSERT_EQ(*save_it, false);
    ASSERT_EQ(*it, true);

    auto save_it1 = it--;
    ASSERT_EQ(*save_it1, true);
    ASSERT_EQ(*it, false);

    it -= 41;
    ASSERT_TRUE(it == v.begin());

    it += 41;
    ASSERT_TRUE(it == save_it);

    ASSERT_TRUE((v.begin() + v.size()) == v.end());
    ASSERT_TRUE((v.end() - v.size()) == v.begin());
}

TEST(bitarrayGoodAlloc, const_iterators) {

    bitarray v1(100, true);
    ASSERT_EQ(v1.size(), 100);

    for(auto elem : v1)
        ASSERT_EQ(elem, true);

    *(v1.at(41)) = false;

    const bitarray v = v1;

    ASSERT_EQ(v[41], false);
    ASSERT_EQ(*v.at(41), false);

    ASSERT_EQ(*v.begin(), 1);

    auto it = v.at(41);

    auto save_it = it++;
    ASSERT_EQ(*save_it, false);
    ASSERT_EQ(*it, true);

    auto save_it1 = it--;
    ASSERT_EQ(*save_it1, true);
    ASSERT_EQ(*it, false);

    ASSERT_TRUE(save_it1 != it);

    it -= 41;
    ASSERT_TRUE(it == v.begin());

    it += 41;
    ASSERT_TRUE(it == save_it);

    ASSERT_TRUE((v.begin() + v.size()) == v.end());
    ASSERT_TRUE((v.end() - v.size()) == v.begin());
}

TEST(bitarrayGoodAlloc, find) {
    bitarray elems(100, true);

    for(int i = 0; i < elems.size(); i += 3)
        elems[i] = false;

    ASSERT_EQ(bitarray::find_bit(0, 64, 0x6db6db6db6db6db6, false), 0);

    ASSERT_EQ(elems.find(0, elems.size(), false), 0);
    ASSERT_EQ(elems.find(3, elems.size(), false), 3);
    ASSERT_EQ(elems.find(99, elems.size(), true), -1);
    ASSERT_EQ(elems.find(0, 3, false), 0);
    ASSERT_EQ(elems.find(0, 3, true), 1);

    ASSERT_THROW(elems.find(-1, 3, false), std::out_of_range);
}

// BAD ALLOC TESTS
bool BAD_NEW = false;
void* operator new(size_t size) {
    if(BAD_NEW)
        throw std::bad_alloc();

    void* ptr = malloc(size);

    if(!ptr)
        throw std::bad_alloc();

    return ptr;
}

TEST(bitarrayBadAlloc, constructors) {

    BAD_NEW = true;
    ASSERT_THROW(bitarray v_bad_constructor(12, true) , std::bad_alloc);
    BAD_NEW = false;

    // CREATING BITARRAY WITH NORMAL new TO TEST OTHER CONSTRUCTORS AND OTHER
    bitarray v(121, true);
    ASSERT_EQ(v.size(), 121);
    for(auto elem : v)
        ASSERT_EQ(elem, true);
    //

    BAD_NEW = true;
    ASSERT_THROW(bitarray v_bad_copyctor = v, std::bad_alloc);
    BAD_NEW = false;

    bitarray v_assign(11, true);
    ASSERT_EQ(v_assign.size(), 11);
    for(auto elem : v_assign)
        ASSERT_EQ(elem, true);

    BAD_NEW = true;
    ASSERT_THROW(v_assign = v, std::bad_alloc);
    BAD_NEW = false;

    ASSERT_EQ(v_assign.size(), 11);
    for(auto elem : v_assign)
        ASSERT_EQ(elem, true);

    BAD_NEW = true;
    ASSERT_THROW(v_assign.resize(100000), std::bad_alloc);
    BAD_NEW = false;

    ASSERT_EQ(v_assign.size(), 11);
    for(auto elem : v_assign)
        ASSERT_EQ(elem, true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}