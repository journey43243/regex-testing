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
#include <re2/re2.h>
#include <iomanip>
#include <unordered_set>

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

// Новые функции для тестирования компиляции
void test_std_regex_compile(const string& pattern_name, const string& pattern) {
    try {
        auto start = high_resolution_clock::now();
        regex re(pattern);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        TestResult result{"compile", "std::regex", pattern_name, duration.count(), 0};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "std::regex compile error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_boost_regex_compile(const string& pattern_name, const string& pattern) {
    try {
        auto start = high_resolution_clock::now();
        boost::regex re(pattern);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        TestResult result{"compile", "boost::regex", pattern_name, duration.count(), 0};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "boost::regex compile error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_pcre_compile(const string& pattern_name, const string& pattern) {
    try {
        int errnum;
        PCRE2_SIZE erroff;
        
        auto start = high_resolution_clock::now();
        pcre2_code* re = pcre2_compile(
            (PCRE2_SPTR8)pattern.c_str(),
            PCRE2_ZERO_TERMINATED,
            0,
            &errnum,
            &erroff,
            nullptr
        );
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        TestResult result{"compile", "PCRE", pattern_name, duration.count(), 0};
        all_results.push_back(result);
        print_test_result(result);
        
        if (re) {
            pcre2_code_free(re);
        }
    } catch (const exception& e) {
        cerr << "PCRE compile error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_re2_compile(const string& pattern_name, const string& pattern) {
    try {
        auto start = high_resolution_clock::now();
        RE2 re(pattern);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        
        TestResult result{"compile", "RE2", pattern_name, duration.count(), 0};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "RE2 compile error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

// Существующие функции (остаются без изменений)
void test_std_regex_match(const vector<string>& words, const string& pattern_name, const string& pattern) {
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

void test_re2_match(const vector<string>& words, const string& pattern_name, const string& pattern) {
    try {
        RE2 re(pattern);
        if (!re.ok()) {
            cerr << "RE2 compilation failed for pattern '" << pattern << "': " << re.error() << endl;
            return;
        }
        
        size_t matches = 0;
        
        auto start = high_resolution_clock::now();
        for (const auto& word : words) {
            if (RE2::FullMatch(re2::StringPiece(word), re)) {
                matches++;
            }
        }
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end - start);
        TestResult result{"match", "RE2", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "RE2 error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

void test_std_regex_search(const string& text, const string& pattern_name, const string& pattern) {
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

void test_re2_search(const string& text, const string& pattern_name, const string& pattern) {
    try {
        RE2 re(pattern);
        if (!re.ok()) {
            cerr << "RE2 compilation failed for pattern '" << pattern << "': " << re.error() << endl;
            return;
        }
        
        size_t matches = 0;
        re2::StringPiece input(text);
        
        auto start = high_resolution_clock::now();
        while (RE2::FindAndConsume(&input, re)) {
            matches++;
        }
        auto end_time = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end_time - start);
        TestResult result{"search", "RE2", pattern_name, duration.count(), static_cast<int>(matches)};
        all_results.push_back(result);
        print_test_result(result);
    } catch (const exception& e) {
        cerr << "RE2 search error with pattern '" << pattern << "': " << e.what() << endl;
    }
}

// Новый паттерн для извлечения расширений файлов
const string file_extension_pattern = R"((?:\.([a-zA-Z0-9]+))$)";

// Функции для тестирования извлечения расширений
void test_std_regex_extensions(const vector<string>& paths) {
    try {
        regex re(file_extension_pattern);
        unordered_set<string> extensions;
        
        auto start = high_resolution_clock::now();
        for (const auto& path : paths) {
            smatch match;
            if (regex_search(path, match, re) && match.size() > 1) {
                extensions.insert(match[1].str());
            }
        }
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end - start);
        TestResult result{"extract", "std::regex", "File extensions", duration.count(), static_cast<int>(extensions.size())};
        all_results.push_back(result);
        print_test_result(result);
        
        // Вывод уникальных расширений
        cout << "std::regex found " << extensions.size() << " unique extensions:\n";
        for (const auto& ext : extensions) {
            cout << ext << " ";
        }
        cout << "\n\n";
    } catch (const exception& e) {
        cerr << "std::regex extensions error: " << e.what() << endl;
    }
}

void test_boost_regex_extensions(const vector<string>& paths) {
    try {
        boost::regex re(file_extension_pattern);
        unordered_set<string> extensions;
        
        auto start = high_resolution_clock::now();
        for (const auto& path : paths) {
            boost::smatch match;
            if (boost::regex_search(path, match, re) && match.size() > 1) {
                extensions.insert(match[1].str());
            }
        }
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end - start);
        TestResult result{"extract", "boost::regex", "File extensions", duration.count(), static_cast<int>(extensions.size())};
        all_results.push_back(result);
        print_test_result(result);
        
        cout << "boost::regex found " << extensions.size() << " unique extensions:\n";
        for (const auto& ext : extensions) {
            cout << ext << " ";
        }
        cout << "\n\n";
    } catch (const exception& e) {
        cerr << "boost::regex extensions error: " << e.what() << endl;
    }
}

void test_pcre_extensions(const vector<string>& paths) {
    try {
        int errnum;
        PCRE2_SIZE erroff;
        pcre2_code* re = pcre2_compile(
            (PCRE2_SPTR8)file_extension_pattern.c_str(),
            PCRE2_ZERO_TERMINATED,
            0,
            &errnum,
            &erroff,
            nullptr
        );
        
        if (!re) {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(errnum, buffer, sizeof(buffer));
            cerr << "PCRE compilation failed: " << buffer << endl;
            return;
        }
        
        pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);
        unordered_set<string> extensions;
        
        auto start = high_resolution_clock::now();
        for (const auto& path : paths) {
            int rc = pcre2_match(
                re,
                (PCRE2_SPTR8)path.c_str(),
                path.length(),
                0,
                0,
                match_data,
                nullptr
            );
            
            if (rc > 1) { // Есть как минимум одна группа захвата
                PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
                size_t start = ovector[2];
                size_t end = ovector[3];
                if (start != PCRE2_UNSET && end != PCRE2_UNSET) {
                    extensions.insert(path.substr(start, end - start));
                }
            }
        }
        auto end_time = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end_time - start);
        TestResult result{"extract", "PCRE", "File extensions", duration.count(), static_cast<int>(extensions.size())};
        all_results.push_back(result);
        print_test_result(result);
        
        cout << "PCRE found " << extensions.size() << " unique extensions:\n";
        for (const auto& ext : extensions) {
            cout << ext << " ";
        }
        cout << "\n\n";
        
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
    } catch (const exception& e) {
        cerr << "PCRE extensions error: " << e.what() << endl;
    }
}

void test_re2_extensions(const vector<string>& paths) {
    try {
        RE2 re(file_extension_pattern);
        if (!re.ok()) {
            cerr << "RE2 compilation failed: " << re.error() << endl;
            return;
        }
        
        unordered_set<string> extensions;
        string ext;
        
        auto start = high_resolution_clock::now();
        for (const auto& path : paths) {
            if (RE2::PartialMatch(path, re, &ext)) {
                extensions.insert(ext);
            }
        }
        auto end_time = high_resolution_clock::now();
        
        auto duration = duration_cast<microseconds>(end_time - start);
        TestResult result{"extract", "RE2", "File extensions", duration.count(), static_cast<int>(extensions.size())};
        all_results.push_back(result);
        print_test_result(result);
        
        cout << "RE2 found " << extensions.size() << " unique extensions:\n";
        for (const auto& ext : extensions) {
            cout << ext << " ";
        }
        cout << "\n\n";
    } catch (const exception& e) {
        cerr << "RE2 extensions error: " << e.what() << endl;
    }
}

int main() {
    // Тестирование компиляции регулярных выражений
    cout << "\n=== Testing REGEX COMPILATION ===\n";
    print_results_header();
    
    for (const auto& [name, pattern] : word_patterns) {
        test_std_regex_compile(name, pattern);
        test_boost_regex_compile(name, pattern);
        test_pcre_compile(name, pattern);
        test_re2_compile(name, pattern);
    }

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

    cout << "\nLoaded " << words.size() << " words for match testing\n";
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
        test_re2_match(words, name, pattern);
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
        test_re2_search(text, name, pattern);
    }

     cout << "\n\n=== Testing FILE EXTENSIONS EXTRACTION ===\n";
    ifstream paths_file("file_formats_tests.txt"); // Файл с 1 млн путей
    if (!paths_file) {
        cerr << "Cannot open file_formats_tests.txt\n";
        return 1;
    }

    vector<string> paths;
    string path;
    while (getline(paths_file, path)) {
        paths.push_back(path);
    }
    paths_file.close();

    cout << "\nLoaded " << paths.size() << " paths for extensions extraction testing\n";
    cout << "Warming up cache... ";
    warmup_cache(paths);
    cout << "done\n";

    print_results_header();
    test_std_regex_extensions(paths);
    test_boost_regex_extensions(paths);
    test_pcre_extensions(paths);
    test_re2_extensions(paths);

    return 0;

}
