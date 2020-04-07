
#include <sched.h>
#include <pthread.h>
#include <cassert>
#include <sys/sysinfo.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <deque>
#include <limits>
#include <climits>

#define ASSERT_TRUE(expr)   do {                                                            \
                                if (!(expr)) {                                              \
                                    std::stringstream ss;                                   \
                                    ss << "in " << __PRETTY_FUNCTION__ << " err: " << #expr;\
                                    throw std::runtime_error(ss.str());                     \
                                }                                                           \
                            } while (0);

template <typename T>
using vec_iter = typename std::vector<T>::iterator;

using dot_t = std::pair<int, int>;

struct ThreadRoutineArg final {
    const std::vector<dot_t>& all_dots;
    //range of points among which the search is conducted
    // [first, last)
    std::pair<vec_iter<dot_t>, vec_iter<dot_t>> range;
    long min_distance = std::numeric_limits<long>::max();
};

using routine_t = std::pair<pthread_t, ThreadRoutineArg>;

long distance_sqr(const dot_t&, const dot_t&);
void* BruteForceSearchRoutine(void*);
void InitRoutinesArgs(std::deque<routine_t>&, int, std::vector<dot_t>&);
int get_number(const char*);

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "too much args " << std::endl;
        std::cerr << "usage: " << argv[0] << " <threads number>" << std::endl;
        return 1;
    }

    int threads_num = get_number(argv[1]);
    if ((threads_num <= 0)) {
        std::cerr << "usage: " << argv[0] << " <threads number>" << std::endl;
        return 1;
    }
    threads_num = std::min(get_nprocs(), threads_num);

    int dots_number = 0;
    std::cin >> dots_number;
    assert(std::cin.good() || dots_number >= 0);

    if (dots_number < threads_num) {
        std::cerr << "number of dots must not be less than number of threads" << std::endl;
        return 1;
    }

    std::vector<dot_t> dots(dots_number);
    for (auto& [first, second]: dots) {
        std::cin >> first;
        assert(std::cin.good());
        std::cin >> second;
        assert(std::cin.good());
    }

    std::deque<routine_t> routines;
    InitRoutinesArgs(routines, threads_num, dots);

    pthread_attr_t pthread_attr;
    cpu_set_t cpu_set;
    for (int i = 0; i < threads_num; ++i) {
        auto& [pthread, Arg] = routines[i];
        ASSERT_TRUE(pthread_attr_init(&pthread_attr) == 0 );

        CPU_ZERO(&cpu_set);
        CPU_SET(i, &cpu_set);
        ASSERT_TRUE(pthread_attr_setaffinity_np(&pthread_attr, sizeof(cpu_set_t), &cpu_set) == 0);
        ASSERT_TRUE(pthread_create(&pthread, &pthread_attr,
                BruteForceSearchRoutine, reinterpret_cast<void*>(&Arg)) == 0);
    }

    for (auto& [pthread, Arg] : routines)
        ASSERT_TRUE(pthread_join(pthread, nullptr) == 0);

    long answ = routines.front().second.min_distance;
    for (auto& [pthread, Arg] : routines)
        answ = std::min(answ, Arg.min_distance);

    return 0;
}

void *BruteForceSearchRoutine(void * argument) {
    ThreadRoutineArg& arg = *reinterpret_cast<ThreadRoutineArg*>(argument);

    auto& [first, last] = arg.range;
    auto& answ = arg.min_distance;

    answ = std::numeric_limits<long>::max();

    for (auto it = first; it != last; ++it) {
        for (auto dot = arg.all_dots.begin(), end = arg.all_dots.end(); dot != end; ++dot) {
            if (dot != it)
                answ  = std::min(answ, distance_sqr(*it, *dot));
        }
    }

    return argument;
}

template <typename T>
static T sqr(T elem) { return elem * elem; }

long distance_sqr(const dot_t& lhs, const dot_t& rhs) {
    return sqr(static_cast<long>(rhs.first - lhs.first))
           + sqr(static_cast<long>(rhs.second - lhs.second));
}

void InitRoutinesArgs(std::deque<routine_t>& routines, int threads_num, std::vector<dot_t>& dots) {
    assert(threads_num > 0);

    auto first = dots.begin();
    auto last = dots.begin() + dots.size() / threads_num + 1;
    auto step = std::distance(first, last);

    for (int i = 0; i < threads_num; ++i) {
        ThreadRoutineArg arg{dots, {first, last}, std::numeric_limits<long>::max()};
        routines.push_back(std::make_pair(pthread_t{}, std::move(arg)));
        first = last;
        last = (threads_num - i == 1) ? dots.end() : last + step;
    }
}

int get_number(const char* str) {
    char* str_end;
    long number = strtol(str, &str_end, 10);

    if (errno == ERANGE && (number == LONG_MAX || number == LONG_MIN))
        return -1;
    if ((str_end == str) || (errno != 0 && number == 0))
        return -1;

    return number;
}