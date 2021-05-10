#include <algorithm>
#include <array>
#include <iostream>
#include <cstdio>
#include <map>
#include <cassert>
#include <string>
#include <vector>


class Span {
private:
    unsigned *base;
    unsigned len;

public:
    static Span make(unsigned len) {
        return Span(new unsigned[len], len);
    }

    Span(unsigned *base = nullptr, unsigned len = 0): base(base), len(len) {}
    void free() {
        if (base != nullptr) {
            delete[] base;
        }
        base = nullptr;
        len = 0;
    }

    const unsigned* get_buffer() const {
        return base;
    }

    Span slice(unsigned startI, unsigned endI) {
        return Span(base + startI, endI - startI);
    }
    const Span slice(unsigned startI, unsigned endI) const {
        return Span(base + startI, endI - startI);
    }

    unsigned& operator[](unsigned i) {
        return base[i];
    }
    const unsigned& operator[](unsigned i) const {
        return base[i];
    }
    size_t size() const {
        return len;
    }

    unsigned* begin() {
        return base;
    }
    const unsigned* begin() const {
        return base;
    }
    unsigned* end() {
        return base + len;
    }
    const unsigned* end() const {
        return base + len;
    }
};

std::ostream& operator<<(std::ostream &s, const Span span) {
    for (const unsigned &x : span) {
        s << x << " ";
    }
    return s;
}


//
// Naive algorithm
//
unsigned naive_lcp(const Span s, unsigned i, unsigned j) {
    unsigned l=0;
    while (s[i + l] == s[j + l]) {
        l++;
    }
    return l;
}

struct SuffixComparator {
    const Span s;

    SuffixComparator(const Span s): s(s) {}

    bool operator() (unsigned i, unsigned j) {
        unsigned l = naive_lcp(s, i, j);
        return s[i + l] < s[j + l];
    }
};

Span naive_sa(const Span s) {
    Span ord = Span::make(s.size());
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


void get_char_ord(const Span in, Span ord) {
    std::map<unsigned, unsigned> rankPtr;
    for (unsigned c : in) {
        rankPtr[c]++;
    }
    unsigned ptr = 0;
    for (auto &kv : rankPtr) {
        unsigned nextPtr = kv.second + ptr;
        kv.second = ptr;
        ptr = nextPtr;
    }

    for (unsigned i = 0;i < in.size();i++) {
        ord[rankPtr[in[i]]++] = i;
    }
}

void get_char_rank(const Span in, Span rank) {
    std::map<unsigned, unsigned> rankMap;
    for (unsigned c : in) {
        rankMap[c] = 0;
    }
    unsigned ind = 0;
    for (auto &kv : rankMap) {
        kv.second = ind++;
    }
    for (unsigned i = 0;i < in.size();i++) {
        rank[i] = rankMap[in[i]];
    }

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

constexpr unsigned cyclic_prev(unsigned i, unsigned step, unsigned len) {
    return step > i
        ? i + len - step
        : i - step;
}

Span nlogn_sa(const Span in) {
    Span ord = Span::make(in.size());
    Span rankPtr = Span::make(in.size());
    Span rank = Span::make(in.size());
    Span temp = Span::make(in.size());

    get_char_ord(in, ord);
    get_char_rank(in, rank);

    for (unsigned stride = 1;stride < in.size();stride *= 2) {
        // Compute rank and rankPtr from ord and prev ranks
        unsigned prevStride = stride / 2;
        Span newRank = temp;

        unsigned r = 0;
        rankPtr[0] = 0;
        newRank[ord[0]] = 0;
        for (unsigned i = 1;i < ord.size();i++) {
            unsigned cur = ord[i];
            unsigned prev = ord[i - 1];
            if (
                rank[cur] != rank[prev]
                || (rank[cur + prevStride]) != rank[prev + prevStride]
            ) {
                r++;
                rankPtr[r] = i;
            }
            newRank[ord[i]] = r;
        }
        temp = rank;
        rank = newRank;

        Span newOrd = temp;
        for (unsigned i : ord) {
            unsigned startI = cyclic_prev(i, stride, in.size());
            newOrd[rankPtr[rank[startI]]++] = startI;
        }
        temp = ord;
        ord = newOrd;
    }
    rankPtr.free();
    rank.free();
    temp.free();
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
        //auto ord = naive_sa(v);
        //std::cerr << "naive: " << ord << "\n";
        //return ord;
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

Span read_input() {
    assert(freopen(nullptr, "rb", stdin) != nullptr);
    assert(fseek(stdin, 0, SEEK_END) == 0);
    const unsigned inputLen = ftell(stdin);

    //TODO directly write into the actual array and shift inside it
    char *inputBuffer = new char[inputLen];
    fseek(stdin, 0, SEEK_SET);
    fread(inputBuffer, 1, inputLen, stdin);

    Span res = Span::make(inputLen + 1);
    for (unsigned i = 0;i < inputLen;i++) {
        res[i] = inputBuffer[i] + 1;
    }
    res[inputLen] = 0;
    delete[] inputBuffer;

    return res;
}

void print_binary_output(const Span sa) {
    assert(freopen(nullptr, "wb", stdout) != nullptr);
    fwrite(sa.get_buffer(), sizeof(unsigned), sa.size(), stdout);
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    auto args = extract_args(argc, argv);
    Options opt = parse_args(args);

    Span input = read_input();

    Span sa;
    for (unsigned i = 0;i < opt.repeatCnt;i++) {
        sa.free();
        switch(opt.algorithm) {
        case NAIVE:
            sa = naive_sa(input);
            break;
        case NLOGN:
            sa = nlogn_sa(input);
            break;
        //case LINEAR:
        //    sa = linear_sa(input);
        //    break;
        default:
            std::cerr << "Algorithm not yet supported\n";
            std::exit(1);
        }
    }
    //std::cerr << sa << "\n";
    print_binary_output(sa);
    sa.free();
    input.free();
}
