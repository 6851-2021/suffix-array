#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct SuffixArray {
    std::vector<unsigned> ind;
    std::vector<unsigned> lcp;
};

//
// Naive algorithm
//
unsigned naive_lcp(const std::vector<unsigned> &s, unsigned i, unsigned j) {
    unsigned l=0;
    while (s[i + l] == s[j + l]) {
        l++;
    }
    return l;
}

struct SuffixComparator {
    const std::vector<unsigned> &s;

    SuffixComparator(const std::vector<unsigned> &s): s(s) {}

    bool operator() (unsigned i, unsigned j) {
        unsigned l = naive_lcp(s, i, j);
        return s[i + l] < s[j + l];
    }
};

SuffixArray compute_naive(const std::vector<unsigned> &s) {
    SuffixArray res;

    res.ind.resize(s.size());
    for (unsigned i = 0;i < s.size();i++) {
        res.ind[i] = i;
    }
    SuffixComparator cmp(s);
    std::sort(res.ind.begin(), res.ind.end(), cmp);

    res.lcp.resize(s.size() - 1);
    for (unsigned i = 0;i + 1 < s.size();i++) {
        res.lcp[i] = naive_lcp(s, res.ind[i], res.ind[i + 1]);
    }
    return res;
}

//
// Shared Utilities
//

template <typename T>
std::ostream& operator<<(std::ostream &s, const std::vector<T> &v) {
    for (const auto &x : v) {
        s << x << " ";
    }
    return s;
}

std::vector<unsigned> sort_chars(const std::vector<unsigned> &s) {
    std::map<unsigned, unsigned> charPtr;
    for (unsigned c : s) {
        charPtr[c]++;
    }

    unsigned prefixSum = 0;
    for (auto &kv : charPtr) {
        unsigned newSum = prefixSum + kv.second;
        kv.second = prefixSum;
        prefixSum = newSum;
    }

    std::vector<unsigned> res(s.size());
    for (unsigned i = 0;i < s.size();i++) {
        auto it = charPtr.find(s[i]);
        res[(it->second)++] = i;
    }
    return res;
}

// Return an vector of the indices of the elements in sorted order
template <size_t K>
void radix_sort(
    const std::vector<std::array<unsigned, K>> &v,
    std::vector<unsigned> &ord
) {
    // (hopefully) reuse memory across function calls
    static std::vector<unsigned> bPtr;
    static std::vector<unsigned> newOrd;

    ord.resize(v.size());
    for (unsigned i = 0;i < v.size();i++) {
        ord[i] = i;
    }

    for (unsigned i = K - 1;i < K;i--) {
        // TODO see if it makes sense to just skip the first iteration
        bPtr.resize(v.size());
        std::fill(bPtr.begin(), bPtr.end(), 0);
        for (const auto &x : v) {
            bPtr[x[i]]++;
        }

        unsigned prefSum = 0;
        for (unsigned j = 0;j < v.size();j++) {
            unsigned newSum = prefSum + bPtr[j];
            bPtr[j] = prefSum;
            prefSum = newSum;
        }

        newOrd.resize(v.size());
        for (unsigned j = 0;j < ord.size();j++) {
            unsigned bInd = v[ord[j]][i];
            //std::cerr << "        " << j << " -> b=" << bInd << "; ind="  << bPtr[bInd] << "\n";
            newOrd[bPtr[bInd]++] = ord[j];
        }

        // O(1) because of C++ being smart
        std::swap(ord, newOrd);
        //std::cerr << "    ord at i=" << i << ": " << ord << "\n";
    }
}

template <typename T>
void get_rank(
    const std::vector<T> &v,
    const std::vector<unsigned> &ord,
    std::vector<unsigned> &rank
) {
    rank.resize(ord.size());
    if (v.empty()) {
        return;
    }
    rank[0] = 0;

    unsigned r = 0;
    for (unsigned i = 1;i < ord.size();i++) {
        if (v[ord[i]] != v[ord[i - 1]]) {
            r = i;
        }
        rank[ord[i]] = r;
    }
}

//
// NlogN
//

SuffixArray compute_nlogn(const std::vector<unsigned> &input) {
    static std::vector<std::array<unsigned, 2>> sortArr;
    static std::vector<unsigned> rank;

    SuffixArray sa;
    sa.ind = sort_chars(input);
    get_rank(input, sa.ind, rank);

    for (unsigned stride = 1;stride < input.size();stride *= 2) {
        //std::cerr << "before stride=" << stride << "\n";
        //std::cerr << "ind: " << sa.ind << "\n";
        //std::cerr << "rank: " << rank << "\n";

        sortArr.resize(input.size());
        for (unsigned i = 0;i < input.size();i++) {
            unsigned j = i + stride;
            j = j >= input.size() ? j - input.size() : j;

            sortArr[i][0] = rank[i];
            sortArr[i][1] = rank[j];
        }
        radix_sort(sortArr, sa.ind);
        get_rank(sortArr, sa.ind, rank);
    }
    return sa;
}


//
// Infrastructure
//

enum AlgoT {
    NAIVE,
    NLOGN,
    LINEAR
};

struct Options {
    AlgoT algorithm;
    unsigned repeatCnt;
};

const char* USAGE =
"Usage: ./sa <algorithm> [repeatCnt]\n"
"\n"
"algorithm:  The algorithm to use. One of 'naive', 'nlogn', 'linear'\n"
"repeatCnt:  How many times to run the algorithm. Useful for benchmarking.\n"
"            Default is 1\n";

std::vector<std::string> extract_args(int argc, char *argv[]) {
    std::vector<std::string> args;
    for (int i = 1;i < argc;i++) {
        args.emplace_back(argv[i]);
    }
    return args;
}

AlgoT parse_algorithm(const std::string &s) {
    if (s == "naive") {
        return NAIVE;
    } else if (s == "nlogn") {
        return NLOGN;
    } else if (s == "linear") {
        return LINEAR;
    } else {
        std::cerr << "Unrecognized algorithm '" << s << "'\n";
        std::cerr << USAGE;
        std::exit(1);
    }
}

unsigned parse_rep_cnt(const std::string &s) {
    try {
        int res = std::stoi(s);
        if (res > 0) {
            return res;
        }
    }
    catch (const std::invalid_argument &e) {}
    std::cerr << "Cannot parse repeatCnt '" << s << "'\n";
    std::cerr << USAGE;
    std::exit(1);
}

Options parse_args(const std::vector<std::string> &args) {
    Options opt;

    if (args.size() < 1 || args.size() > 2) {
        std::cerr << "Expected betwen 1 and 2 arguments, got " << args.size() << "\n";
        std::cerr << USAGE;
        std::exit(1);

    }
    opt.algorithm = parse_algorithm(args[0]);

    if (args.size() >= 2) {
        opt.repeatCnt = parse_rep_cnt(args[1]);
    } else {
        opt.repeatCnt = 1;
    }
    return opt;
}

std::vector<unsigned> read_input() {
    const unsigned BLOCK_SIZE = 1U << 12;
    char buff[BLOCK_SIZE];

    std::vector<unsigned> res;
    while (std::cin) {
        std::cin.read(buff, BLOCK_SIZE);
        for (unsigned i = 0;i < std::cin.gcount();i++) {
            res.push_back(buff[i] + 1);
        }
    }
    // Append a lexicographically minimal "$"
    res.push_back(0);

    return res;
}

void print_arr(const SuffixArray &arr) {
    std::cout << arr.ind << "\n";
    //std::cout << arr.lcp << "\n";
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    auto args = extract_args(argc, argv);
    Options opt = parse_args(args);

    std::vector<unsigned> input = read_input();

    SuffixArray arr;
    for (unsigned i = 0;i < opt.repeatCnt;i++) {
        switch(opt.algorithm) {
        case NAIVE:
            arr = compute_naive(input);
            break;
        case NLOGN:
            arr = compute_nlogn(input);
            break;
        default:
            std::cerr << "Algorithm not yet supported\n";
            std::exit(1);
        }
    }
    print_arr(arr);
}
