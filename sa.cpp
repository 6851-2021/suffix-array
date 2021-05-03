#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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

std::vector<unsigned> naive_sa(const std::vector<unsigned> &s) {
    std::vector<unsigned> ord(s.size());
    for (unsigned i = 0;i < s.size();i++) {
        ord[i] = i;
    }
    SuffixComparator cmp(s);
    std::sort(ord.begin(), ord.end(), cmp);
    return ord;
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
        unsigned maxX = 0;
        for (const auto &x : v) {
            maxX = std::max(maxX, x[i]);
        }
        bPtr.resize(maxX + 1);
        std::fill(bPtr.begin(), bPtr.end(), 0);
        for (const auto &x : v) {
            bPtr[x[i]]++;
        }

        unsigned prefSum = 0;
        for (unsigned j = 0;j < bPtr.size();j++) {
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

std::vector<unsigned> nlogn_sa(const std::vector<unsigned> &input) {
    static std::vector<std::array<unsigned, 2>> sortArr;
    static std::vector<unsigned> rank;

    std::vector<unsigned> ord = sort_chars(input);
    get_rank(input, ord, rank);

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
        radix_sort(sortArr, ord);
        get_rank(sortArr, ord, rank);
    }
    return ord;
}

//
// Linear
//

unsigned mod_prefix_cnt(unsigned n, unsigned mod) {
    return n / 3 + (n % 3 > mod);
}

std::vector<unsigned> get_v01_letter_rank(const std::vector<unsigned> &v) {
    const unsigned v01_cnt = v.size() - mod_prefix_cnt(v.size(), 2);
    std::vector<std::array<unsigned, 3>> v01_letters;
    v01_letters.reserve(v01_cnt);

    for (unsigned m = 0;m < 2;m++) {
        for (unsigned i = m;i < v.size();i += 3) {
            std::array<unsigned, 3> cur{};
            for (unsigned j = 0;j < 3;j++) {
                if (i + j < v.size()) {
                    cur[j] = v[i + j];
                }
            }
            v01_letters.push_back(cur);
        }
    }
    // Compress letters
    std::vector<unsigned> v01_letters_ord;
    radix_sort(v01_letters, v01_letters_ord);
    std::vector<unsigned> v01_rank;
    get_rank(v01_letters, v01_letters_ord, v01_rank);

    return v01_rank;
}

std::vector<unsigned> get_v2_ord(
    const std::vector<unsigned> &v,
    const std::vector<unsigned> &v0_rank
) {
    std::vector<std::array<unsigned, 2>> v2_letters;
    v2_letters.reserve(mod_prefix_cnt(v.size(), 2));

    for (unsigned i = 2;i < v.size();i += 3) {
        std::array<unsigned, 2> cur{};
        cur[0] = v[i];

        unsigned j = (i + 1) / 3;
        if (j < v0_rank.size()) {
            cur[1] = v0_rank[j];
        } else {
            cur[1] = 0; // end of string
        }

        v2_letters.push_back(cur);
    }

    std::vector<unsigned> v2_ord;
    radix_sort(v2_letters, v2_ord);
    return v2_ord;
}

unsigned get_or_0(const std::vector<unsigned> &v, unsigned i) {
    return i < v.size() ? v[i] : 0;
}

std::vector<unsigned> linear_sa(const std::vector<unsigned> &v) {
    //std::cerr << "linear_sa on " << v << "\n";
    if (v.size() < 3) {
        auto ord = naive_sa(v);
        //std::cerr << "naive: " << ord << "\n";
        return ord;
    }

    std::vector<unsigned> v01_letter_rank = get_v01_letter_rank(v);
    std::vector<unsigned> v01_ord = linear_sa(v01_letter_rank);

    std::vector<unsigned> v0_rank(mod_prefix_cnt(v.size(), 0));
    std::vector<unsigned> v1_rank(mod_prefix_cnt(v.size(), 1));
    for (unsigned i = 0;i < v01_ord.size();i++) {
        unsigned j = v01_ord[i];
        if (j < v0_rank.size()) {
            v0_rank[j] = i;
            v01_ord[i] = 3 * j;
        } else {
            j -= v0_rank.size();
            v1_rank[j] = i;
            v01_ord[i] = 3 * j + 1;
        }
    }
    //std::cerr << "v0_rank " << v0_rank << " v1_rank " << v1_rank << "\n";

    std::vector<unsigned> v2_ord = get_v2_ord(v, v0_rank);
    std::vector<unsigned> v2_rank(v2_ord.size());
    for (unsigned i = 0;i < v2_ord.size();i++) {
        v2_rank[v2_ord[i]] = i;
    }

    std::vector<unsigned> ord;
    ord.reserve(v.size());

    unsigned v01_i = 0;
    unsigned v2_i = 0;
    while ((v01_i < v01_ord.size()) && (v2_i < v2_ord.size())) {
        unsigned j = v2_ord[v2_i];
        unsigned i = v01_ord[v01_i] / 3;
        bool lt;

        if (v01_ord[v01_i] % 3 == 0) {
            if (get_or_0(v, 3 * i) != get_or_0(v, 3 * j + 2)) {
                lt = get_or_0(v, 3 * i) < get_or_0(v, 3 * j + 2);
            } else {
                lt = get_or_0(v1_rank, i) < get_or_0(v0_rank, j + 1);
            }
        } else {
            if (get_or_0(v, 3 * i + 1) != get_or_0(v, 3 * j + 2)) {
                lt = get_or_0(v, 3 * i + 1) < get_or_0(v, 3 * j + 2);
            } else if (get_or_0(v, 3 * i + 2) != get_or_0(v, 3 * j + 3)) {
                lt = get_or_0(v, 3 * i + 2) < get_or_0(v, 3 * j + 3);
            } else {
                lt = get_or_0(v0_rank, i + 1) < get_or_0(v1_rank, j + 1);
            }
        }
        if (lt) {
            ord.push_back(v01_ord[v01_i++]);
        } else {
            ord.push_back(v2_ord[v2_i++] * 3 + 2);
        }
    }

    while (v01_i < v01_ord.size()) {
        ord.push_back(v01_ord[v01_i++]);
    }

    while (v2_i < v2_ord.size()) {
        ord.push_back(v2_ord[v2_i++] * 3 + 2);
    }
    //std::cerr << "done with " << v << ": " << ord << "\n";
    return ord;
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

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    auto args = extract_args(argc, argv);
    Options opt = parse_args(args);

    std::vector<unsigned> input = read_input();

    std::vector<unsigned> sa;
    for (unsigned i = 0;i < opt.repeatCnt;i++) {
        switch(opt.algorithm) {
        case NAIVE:
            sa = naive_sa(input);
            break;
        case NLOGN:
            sa = nlogn_sa(input);
            break;
        default:
            sa = linear_sa(input);
            break;
            std::cerr << "Algorithm not yet supported\n";
            std::exit(1);
        }
    }
    std::cout << sa << "\n";
}
