#include <fstream>
#include <lexertl/generator.hpp>
#include <iomanip>
#include <lexertl/lookup.hpp>
#include <lexertl/memory_file.hpp>
#include <lexertl/stream_shared_iterator.hpp>
#include <lexertl/utf_iterators.hpp>

#ifdef WIN32
#include <windows.h>
#endif

void test_unicode()
{
    lexertl::cmatch r_;
    const char *i_ = "";

    r_.clear();
    r_.reset(i_, i_);

    const char utf8_[] = "\xf0\x90\x8d\x86\xe6\x97\xa5\xd1\x88\x7f";
    lexertl::basic_utf8_in_iterator<const char *, int> u8iter_(utf8_,
        utf8_ + sizeof(utf8_));
    int i = *u8iter_; // 0x10346

    i = *++u8iter_; // 0x65e5
    i = *u8iter_++; // 0x65e5
    i = *u8iter_; // 0x0448
    i = *++u8iter_; // 0x7f

    const wchar_t utf16_[] = L"\xdbff\xdfff\xd801\xdc01\xd800\xdc00\xd7ff";
    lexertl::basic_utf16_in_iterator<const wchar_t *, int> u16iter_(utf16_,
        utf16_ + sizeof(utf16_) / sizeof(wchar_t));

    i = *u16iter_; // 0x10ffff
    i = *++u16iter_; // 0x10401
    i = *u16iter_++; // 0x10401
    i = *u16iter_; // 0x10000
    i = *++u16iter_; // 0xd7ff

    // Not all compilers have char32_t, so use int for now
    lexertl::basic_rules<char, int> rules_(lexertl::icase);
    lexertl::basic_state_machine<int> sm_;
    const int in_[] = {0x393, ' ', 0x393, 0x398, ' ', 0x398,
        '1', ' ', 'i', 'd', 0x41f, 0};
    std::basic_string<int> input_(in_);
    const int *iter_ = input_.c_str();
    const int *end_ = iter_ + input_.size();
    lexertl::match_results<const int *> results_(iter_, end_);

    rules_.push("\\p{LC}[\\p{LC}0-9]*", 1);
    lexertl::basic_generator<lexertl::basic_rules<char, int>,
        lexertl::basic_state_machine<int> >::build(rules_, sm_);

#ifdef WIN32
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwBytesWritten = 0;
#endif

    do
    {
#ifdef WIN32
        std::wstring str_;
#else
        std::string str_;
#endif

        lexertl::lookup(sm_, results_);

#ifdef WIN32
        str_.assign(lexertl::basic_utf16_out_iterator<const int *, wchar_t>
            (results_.first, results_.second),
            lexertl::basic_utf16_out_iterator<const int *, wchar_t>
            (results_.second, results_.second));
        std::wcout << L"Id: " << results_.id << L", Token: '";
        ::WriteConsoleW(hStdOut, str_.c_str(), str_.size(), &dwBytesWritten, 0);
        std::wcout << '\'' << std::endl;
#else
        str_.assign(lexertl::basic_utf8_out_iterator<const int *>
            (results_.first, results_.second),
            lexertl::basic_utf8_out_iterator<const int *>
            (results_.second, results_.second));
        std::cout << "Id: " << results_.id << ", Token: '" <<
            str_ << '\'' << std::endl;
#endif
    } while (results_.id != 0);
}

int main(int /*argc*/, char ** /*argv*/)
{
    test_unicode();
    return 0;
}
