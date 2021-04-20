#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

struct SuffixArray {
    std::vector<unsigned> ind;
    std::vector<unsigned> lcp;
};

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
    for (const unsigned &v : arr.ind) {
        std::cout << v << " ";
    }
    std::cout << "\n";
    for (const unsigned &v : arr.lcp) {
        std::cout << v << " ";
    }
    std::cout << "\n";
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
        default:
            std::cerr << "Algorithm not yet supported\n";
            std::exit(1);
        }
    }
    print_arr(arr);
}
