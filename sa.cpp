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
    static Span make(unsigned len, unsigned buffer = 0) {
        unsigned *base = new unsigned[len + buffer];
        for (unsigned i = 0;i < buffer;i++) {
            base[i + len] = 0;
        }
        return Span(base, len);
    }

    Span(unsigned *base = nullptr, unsigned len = 0): base(base), len(len) {}
    void free() {
        if (base != nullptr) {
            delete[] base;
        }
        base = nullptr;
        len = 0;
    }

    Span slice(unsigned len, unsigned buffer = 0) {
        Span result(base, len);

        for (unsigned i = 0;i < buffer;i++) {
            base[i + len] = 0;
        }
        base += len + buffer;
        return result;
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

void radix_sort(
    const Span in,
    const unsigned groupSize,
    const unsigned maxV,
    Span ord,
    Span temp
) {
    //TODO switch to transposed order
    const unsigned n = ord.size();
    assert(in.size() % groupSize == 0);
    assert(in.size() == n * groupSize);

    Span bPtr = temp.slice(maxV);
    Span newOrd = temp.slice(n);
    if (groupSize % 2 == 1) {
        std::swap(ord, newOrd);
    }

    for (unsigned i = 0;i < n;i++) {
        ord[i] = i;
    }

    for (unsigned o = groupSize - 1;o < groupSize;o--) {
        std::fill(bPtr.begin(), bPtr.end(), 0);
        for (unsigned i = o;i < in.size();i += groupSize) {
            bPtr[in[i]]++;
        }

        unsigned prefSum = 0;
        for (unsigned i = 0;i < bPtr.size();i++) {
            unsigned newSum = prefSum + bPtr[i];
            bPtr[i] = prefSum;
            prefSum = newSum;
        }

        for (unsigned i = 0;i < ord.size();i++) {
            unsigned bInd = in[groupSize * ord[i] + o];
            newOrd[bPtr[bInd]++] = ord[i];
        }
        std::swap(ord, newOrd);
    }
}

unsigned mod_prefix_cnt(unsigned n, unsigned mod) {
    return n / 3 + (n % 3 > mod);
}

void get_01_letter_rank(const Span s, Span rank, Span temp) {
    const unsigned out_size = s.size() - mod_prefix_cnt(s.size(), 2);
    Span letters = temp.slice(3 * out_size);
    Span ord = temp.slice(out_size);

    unsigned j = 0;
    for (unsigned mod = 0;mod < 2;mod++) {
        for (unsigned i = mod;i < s.size();i += 3){
            for (unsigned k = 0;k < 3;k++) {
                letters[j++] = s[i + k];
            }
        }
    }
    radix_sort(letters, 3, s.size(), ord, temp);

    rank[ord[0]] = 0;
    unsigned r = 0;
    unsigned *prev = letters.begin() + 3 * ord[0];

    for (unsigned i = 1;i < ord.size();i++) {
        unsigned *cur = letters.begin() + 3 * ord[i];
        for (unsigned j = 0;j < 3;j++) {
            if (prev[j] != cur[j]) {
                r++;
                break;
            }
        }
        rank[ord[i]] = r;
        prev = cur;
    }
}

void get_01_string_rank(Span ord_01, Span rank_0, Span rank_1) {
    for (unsigned i = 0;i < ord_01.size();i++) {
        unsigned j = ord_01[i];
        if (j < rank_0.size()) {
            rank_0[j] = i;
            ord_01[i] = 2 * j;
        } else {
            j -= rank_0.size();
            rank_1[j] = i;
            ord_01[i] = 2 * j + 1;
        }
    }
}

void get_2_ord(const Span s, const Span rank_0, Span ord, Span temp) {
    const unsigned out_size = mod_prefix_cnt(s.size(), 2);
    Span letters = temp.slice(2 * out_size);

    unsigned j = 0;
    unsigned k = 1;
    for (unsigned i = 2;i < s.size();i += 3) {
        letters[j++] = s[i];
        letters[j++] = rank_0[k++];
    }

    radix_sort(letters, 2, s.size(), ord, temp);
}

void recursive_linear_sa(const Span s, Span ord, Span stack) {
    for (unsigned i : s) {
        assert(i < s.size());
    }
    if (s.size() < 3) {
        for (unsigned i = 0;i < s.size();i++) {
            ord[i] = s[i];
        }
        return;
    }
    const unsigned cnt_0 = mod_prefix_cnt(s.size(), 0);
    const unsigned cnt_1 = mod_prefix_cnt(s.size(), 1);
    const unsigned cnt_2 = mod_prefix_cnt(s.size(), 2);

    Span letter_rank_01 = stack.slice(cnt_0 + cnt_1, 2);
    Span ord_01 = stack.slice(cnt_0 + cnt_1);

    get_01_letter_rank(s, letter_rank_01, stack);
    recursive_linear_sa(letter_rank_01, ord_01, stack);

    Span rank_0 = stack.slice(cnt_0, 1);
    Span rank_1 = stack.slice(cnt_1, 1);
    Span ord_2 = stack.slice(cnt_2);

    get_01_string_rank(ord_01, rank_0, rank_1);
    get_2_ord(s, rank_0, ord_2, stack);

    unsigned *p_01 = ord_01.begin();
    unsigned *p_2 = ord_2.begin();
    unsigned *p_ord = ord.begin();

    while ((p_01 != ord_01.end()) && (p_2 != ord_2.end())) {
        unsigned i_01 = (*p_01) / 2;
        unsigned i_2 = *p_2;

        unsigned j_01 = i_01 * 3 + (*p_01) % 2;
        unsigned j_2 = i_2 * 3 + 2;
        bool lt;

        if ((*p_01) % 2 == 0) {
            if (s[j_01] != s[j_2]) {
                lt = s[j_01] < s[j_2];
            } else {
                lt = rank_1[i_01] < rank_0[i_2 + 1];
            }
        } else {
            if (s[j_01] != s[j_2]) {
                lt = s[j_01] < s[j_2];
            } else if (s[j_01 + 1] != s[j_2 + 1]) {
                lt = s[j_01 + 1] < s[j_2 + 1];
            } else {
                lt = rank_0[i_01 + 1] < rank_1[i_2 + 1];
            }

        }

        if (lt) {
            *(p_ord++) = j_01;
            p_01++;
        } else {
            *(p_ord++) = j_2;
            p_2++;
        }

    }

    for (;p_01 != ord_01.end();p_01++) {
        *(p_ord++) = (*p_01) / 2 * 3 + (*p_01) % 2;
    }
    for (;p_2 != ord_2.end();p_2++) {
        *(p_ord++) = (*p_2) * 3 + 2;
    }
}

Span linear_sa(const Span rawInput) {
    Span in = Span::make(rawInput.size(), 2);
    Span st = Span::make(rawInput.size() * 6 + 5);
    Span out = Span::make(rawInput.size());

    get_char_rank(rawInput, in);
    recursive_linear_sa(in, out, st);
    in.free();
    st.free();
    return out;
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
    fwrite(sa.begin(), sizeof(unsigned), sa.size(), stdout);
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
        case LINEAR:
            sa = linear_sa(input);
            break;
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
