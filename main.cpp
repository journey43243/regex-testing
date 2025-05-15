#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <regex>
#include <numeric>
#include <map>
#include <tuple>
#include <boost/regex.hpp>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// Шаблоны для тестирования отдельных слов
const vector<pair<string, string>> word_patterns = {
    {"Lowercase", "^[a-z]+$"},
    {"Digits only", "^\\d+$"},
    {"4 alnum chars", "^\\w{4}$"},
    {"Capitalized", "^[A-Z][a-z]+$"},
    {"Words ending with 'ing'", "^[a-z]+ing$"},
    {"Simple ID pattern", "^\\d{3}-\\d{2}-\\d{4}$"},
    {"Password pattern", "^(?=.*[a-z])(?=.*[A-Z])(?=.*\\d).{8,}$"},
    {"Vowel-consonant alternation", "^([aeiou][^aeiou])+[aeiou]?$"},
    {"5-letter palindromes", "^(.)(.).\\2\\1$"}
};

// Паттерны для поиска в "Войне и мире"
const vector<pair<string, string>> war_and_peace_patterns = {
    {"Russian names", "\\b[A-Z][a-z]*(ov|ev|in|sky|aya)\\b"},
    {"French phrases", "\\b[a-zA-ZÀ-ÿ]+\\s[a-zA-ZÀ-ÿ]+\\b"},
    {"Military terms", "\\b(regiment|battalion|cavalry|infantry|artillery)\\b"},
    {"Aristocratic titles", "\\b(Prince|Count|Countess|Baron|Duchess)\\s[A-Z][a-z]+\\b"},
    {"Nature descriptions", "\\b(sunset|moonlight|snow|forest|river|field)s?\\b"},
    {"Emotional expressions", "\\b(sighed|wept|laughed|exclaimed|whispered)\\b"},
    {"Historical dates", "\\b(1[0-9]{3}|20[0-9]{2})\\b"},
    {"Philosophical terms", R"(\b(life|death|love|war|peace|destiny)\b)"},
    {"Long sentences", R"(\b(\w+\s+){20,}\w+\b)"}
};

struct TestResult {
    string operation;
    string library;
    string pattern_name;
    long long time_us;
    int matches;
};

vector<TestResult> all_results;

void warmup_cache(const vector<string>& words) {
    volatile size_t dummy = 0;
    for (const auto& word : words) {
        dummy += word.length();
    }
    for (const auto& word : words) {
        dummy -= word[0];
    }
    (void)dummy;
}

string read_file_to_string(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Cannot open " << filename << endl;
        return "";
    }
    
    string content((istreambuf_iterator<char>(file)), 
                   istreambuf_iterator<char>());
    file.close();
    return content;
}

void print_test_result(const TestResult& result) {
    printf("| %-8s | %-12s | %-30s | %7d | %9lld |\n",
           result.operation.c_str(),
           result.library.c_str(),
           result.pattern_name.c_str(),
           result.matches,
           result.time_us);
}

void print_results_header() {
    cout << "| Operation | Library      | Pattern Name                   | Matches | Time (μs) |\n";
    cout << "|-----------|--------------|--------------------------------|---------|-----------|\n";
}

void test_std_regex_match(const vector<string>& words, const string& pattern_name, const string& pattern) {
    // Функция использует srd::regex для сравнения по паттерну со строкой
    // Также замеряется время работы
    try {
        regex re(pattern);
        size_t matches = 0;
        
        auto start = high_resolution_clock::now();
        for (const auto& word : words) {
            if (regex_match(word, re)) {
                matches++;
            }
        }
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end - start);
        TestResult result{"match", "std::regex", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "std::regex error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_boost_regex_match(const vector<string>& words, const string& pattern_name, const string& pattern) {
    // Функция использует boost::regex для сравнения по паттерну со строкой
    // Также замеряется время работы
    try {
        boost::regex re(pattern);
        size_t matches = 0;
        
        auto start = high_resolution_clock::now();
        for (const auto& word : words) {
            if (boost::regex_match(word, re)) {
                matches++;
            }
        }
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end - start);
        TestResult result{"match", "boost::regex", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "boost::regex error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_pcre_match(const vector<string>& words, const string& pattern_name, const string& pattern) {
    // Функция использует pcre для сравнения по паттерну со строкой
    // Также замеряется время работы
    try {
        int errnum;
        PCRE2_SIZE erroff;
        pcre2_code* re = pcre2_compile(
            (PCRE2_SPTR8)pattern.c_str(),
            PCRE2_ZERO_TERMINATED,
            0,
            &errnum,
            &erroff,
            nullptr
        );
        
        if (!re) {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(errnum, buffer, sizeof(buffer));
            cerr << "PCRE compilation failed for pattern '" << pattern << "': " << buffer << endl;
            return;
        }
        
        pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);
        size_t matches = 0;
        
        auto start = high_resolution_clock::now();
        for (const auto& word : words) {
            int rc = pcre2_match(
                re,
                (PCRE2_SPTR8)word.c_str(),
                word.length(),
                0,
                0,
                match_data,
                nullptr
            );
            if (rc >= 0) {
                matches++;
            }
        }
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end - start);
        TestResult result{"match", "PCRE", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
        
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
    } catch (const exception& e) {
        cerr << "PCRE error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_std_regex_search(const string& text, const string& pattern_name, const string& pattern) {
    // Функция использует std::regex для поиска паттерна в тексте
    // Также замеряется время работы
    try {
        regex re(pattern);
        size_t matches = 0;
        
        auto start = high_resolution_clock::now();
        sregex_iterator it(text.begin(), text.end(), re);
        sregex_iterator end;
        matches = distance(it, end);
        auto end_time = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end_time - start);
        TestResult result{"search", "std::regex", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "std::regex search error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_boost_regex_search(const string& text, const string& pattern_name, const string& pattern) {
    // Функция использует boost::regex для поиска паттерна в тексте
    // Также замеряется время работы
    try {
        boost::regex re(pattern);
        size_t matches = 0;
        
        auto start = high_resolution_clock::now();
        boost::sregex_iterator it(text.begin(), text.end(), re);
        boost::sregex_iterator end;
        matches = distance(it, end);
        auto end_time = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end_time - start);
        TestResult result{"search", "boost::regex", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "boost::regex search error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_pcre_search(const string& text, const string& pattern_name, const string& pattern) {
    // Функция использует pcre2 для поиска паттерна в тексте
    // Также замеряется время работы
    try {
        int errnum;
        PCRE2_SIZE erroff;
        pcre2_code* re = pcre2_compile(
            (PCRE2_SPTR8)pattern.c_str(),
            PCRE2_ZERO_TERMINATED,
            0,
            &errnum,
            &erroff,
            nullptr
        );
        
        if (!re) {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(errnum, buffer, sizeof(buffer));
            cerr << "PCRE compilation failed for pattern '" << pattern << "': " << buffer << endl;
            return;
        }
        
        pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);
        size_t matches = 0;
        size_t offset = 0;
        size_t length = text.length();
        const char* subject = text.c_str();
        
        auto start = high_resolution_clock::now();
        while (offset < length) {
            int rc = pcre2_match(
                re,
                (PCRE2_SPTR8)(subject + offset),
                length - offset,
                0,
                0,
                match_data,
                nullptr
            );
            
            if (rc < 0) break;
            
            matches++;
            PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
            offset += ovector[1];
        }
        auto end_time = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end_time - start);
        TestResult result{"search", "PCRE", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
        
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
    } catch (const exception& e) {
        cerr << "PCRE search error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

struct ComparisonResult {
    string library;
    long long avg_match_time;
    long long avg_search_time;
    int match_count;
    int search_count;
};


int main() {
    // Тестирование match на отдельных словах
    ifstream words_file("match.txt");
    if (!words_file) {
        cerr << "Cannot open match.txt\n";
        return 1;
    }

    vector<string> words;
    string word;
    while (getline(words_file, word)) {
        words.push_back(word);
    }
    words_file.close();

    cout << "Loaded " << words.size() << " words for match testing\n";
    cout << "Warming up cache... ";
    warmup_cache(words);
    cout << "done\n";

    cout << "\n=== Testing MATCH operations ===\n";
    print_results_header();

    // Тестирование match для каждого шаблона
    for (const auto& [name, pattern] : word_patterns) {
        cout << "\nTesting pattern: " << name << " (" << pattern << ")\n";
        
        test_std_regex_match(words, name, pattern);
        test_boost_regex_match(words, name, pattern);
        test_pcre_match(words, name, pattern);
    }

    // Тестирование search в "Войне и мире"
    cout << "\n\n=== Testing SEARCH operations ===\n";
    string text = read_file_to_string("search.txt");
    if (text.empty()) {
        cerr << "Failed to read search.txt" << endl;
        return 1;
    }

    cout << "Loaded War and Peace text (" << text.size() << " characters)\n";
    print_results_header();

    for (const auto& [name, pattern] : war_and_peace_patterns) {
        cout << "\nTesting pattern: " << name << " (" << pattern << ")\n";
        
        test_std_regex_search(text, name, pattern);
        test_boost_regex_search(text, name, pattern);
        test_pcre_search(text, name, pattern);
    }

    return 0;
}