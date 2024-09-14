#ifndef _AC_HELPERS_WIN32_LIBRARY_COMMON_HEADER_
#define _AC_HELPERS_WIN32_LIBRARY_COMMON_HEADER_

#include <windows.h>

#include <chrono>
#include <memory>
#include <atomic>
#include <utility>
#include <functional>
#include <exception>
#include <limits>
#include <set>

#define AC_PLATFORM_FAIL_FAST(EC) \
    {                             \
        __debugbreak();           \
        __fastfail(EC);           \
    }

#ifndef AC_FAST_FAIL
#define AC_FAST_FAIL(EC) \
    { AC_PLATFORM_FAIL_FAST(EC); }
#endif

#ifndef AC_CRASH_APPLICATION
#define AC_CRASH_APPLICATION() AC_FAST_FAIL(ENOTRECOVERABLE)
#endif

#ifndef AC_CODDING_ERROR_IF
#define AC_CODDING_ERROR_IF(C)         \
    if (C) {                           \
        AC_FAST_FAIL(ENOTRECOVERABLE); \
    } else {                           \
        ;                              \
    }
#endif

#ifndef AC_CODDING_ERROR_IF_NOT
#define AC_CODDING_ERROR_IF_NOT(C)     \
    if (C) {                           \
        ;                              \
    } else {                           \
        AC_FAST_FAIL(ENOTRECOVERABLE); \
    }
#endif

#define AC_THROW(EC, REASON)                                                  \
    throw std::system_error {                                                 \
        std::error_code{static_cast<int>(EC), std::system_category()}, REASON \
    }

#define AC_THROW_IF(C, EC, REASON) \
    if (C) {                       \
        AC_THROW(EC, REASON);      \
    } else {                       \
    }

#define AC_THROW_IF_NOT(C, EC, REASON) \
    if (C) {                           \
    } else {                           \
        AC_THROW(EC, REASON);          \
    }

#ifndef WINBOOL
#define WINBOOL(E) ((E) ? TRUE : FALSE)
#endif // WINBOOL

#ifndef CPPBOOL
#define CPPBOOL(E) ((E) ? true : false)
#endif // CPPBOOL

namespace ac {

    using string_collection = std::vector<std::string>;
    using wstring_collection = std::vector<std::wstring>;
    using cbuffer = std::vector<char>;
    using ucbuffer = std::vector<unsigned char>;
    using wbuffer = std::vector<wchar_t>;

    [[nodiscard]] inline char const *c_str_or_null_if_empty(std::string const &str) {
        return str.empty() ? nullptr : str.c_str();
    }

    [[nodiscard]] inline wchar_t const *c_str_or_null_if_empty(std::wstring const &str) {
        return str.empty() ? nullptr : str.c_str();
    }

    [[nodiscard]] inline constexpr DWORD get_low_dword(ULONGLONG v) {
        return static_cast<DWORD>(v & 0x00000000FFFFFFFFULL);
    }

    [[nodiscard]] inline constexpr DWORD get_high_dword(ULONGLONG v) {
        return static_cast<DWORD>((v & 0xFFFFFFFF00000000ULL) >> 32ULL);
    }

    [[nodiscard]] inline constexpr long long make_longlong(DWORD low_dword, DWORD high_dword) {
        long long high = high_dword;
        long long low = low_dword;
        long long result = ((high << 32LL) | low);
        return result;
    }

    [[nodiscard]] inline constexpr unsigned long long make_ulonglong(DWORD low_dword,
                                                                     DWORD high_dword) {
        unsigned long long high = high_dword;
        unsigned long long low = low_dword;
        unsigned long long result = ((high << 32ULL) | low);
        return result;
    }

    [[nodiscard]] inline constexpr unsigned long long make_qword(DWORD low_dword,
                                                                 DWORD high_dword) {
        return make_ulonglong(low_dword, high_dword);
    }

    [[nodiscard]] inline size_t ptr_to_size_t(void *p) {
        return *reinterpret_cast<size_t *>(&p);
    }

    [[nodiscard]] inline void *size_t_to_ptr(size_t v) {
        return *reinterpret_cast<void **>(&v);
    }

    template<typename T>
    [[nodiscard]] T *size_t_to_ptr_t(size_t v) {
        return *reinterpret_cast<T **>(&v);
    }

    template<typename T>
    void zero_pod(T *v) {
        //
        // non POD types cannot be used in unions so compiler will give
        // an error if T is not a POD
        //
        typedef union unused_type {
            T o_;
            char c_;
        } unused_type_t;

        ZeroMemory(v, sizeof(T));
    }

    [[nodiscard]] inline DWORD try_resize(cbuffer &b, size_t new_size) noexcept {
        DWORD s{ERROR_SUCCESS};
        try {
            if (0 != new_size) {
                b.resize(new_size);
            } else {
                b.clear();
            }
        } catch (std::bad_alloc const &) {
            s = ERROR_NOT_ENOUGH_MEMORY;
        } catch (...) {
            AC_CRASH_APPLICATION();
        }
        return s;
    }

    inline constexpr unsigned long long filetime_ctime_epoch_diff = 116444736000000000ULL;

    [[nodiscard]] inline FILETIME system_clock_time_point_to_filetime(
        std::chrono::system_clock::time_point time_point) noexcept {
        ULARGE_INTEGER ull;
        ull.QuadPart = time_point.time_since_epoch().count();
        ull.QuadPart += filetime_ctime_epoch_diff;

        FILETIME ft;
        ft.dwHighDateTime = ull.HighPart;
        ft.dwLowDateTime = ull.LowPart;

        return ft;
    }

    [[nodiscard]] inline std::chrono::system_clock::time_point filetime_to_system_clock_time_point(
        FILETIME const &ft) noexcept {
        long long ul = make_qword(ft.dwLowDateTime, ft.dwHighDateTime);
        if (ul > filetime_ctime_epoch_diff) {
            ul -= filetime_ctime_epoch_diff;
        } else {
            ul = 0;
        }
        return std::chrono::system_clock::time_point{
            std::chrono::system_clock::time_point::duration{ul}};
    }

    [[nodiscard]] inline DWORD try_resize(cbuffer *b, size_t new_size) noexcept {
        return try_resize(*b, new_size);
    }

    [[nodiscard]] inline DWORD try_resize(wbuffer &b, size_t new_size) noexcept {
        DWORD s{ERROR_SUCCESS};
        try {
            if (0 != new_size) {
                b.resize(new_size);
            } else {
                b.clear();
            }
        } catch (std::bad_alloc const &) {
            s = ERROR_NOT_ENOUGH_MEMORY;
        } catch (...) {
            AC_CRASH_APPLICATION();
        }
        return s;
    }

    [[nodiscard]] inline DWORD try_resize(wbuffer *b, size_t new_size) noexcept {
        return try_resize(*b, new_size);
    }

    [[nodiscard]] inline DWORD try_resize(std::string &b, size_t new_size) noexcept {
        DWORD s{ERROR_SUCCESS};
        try {
            if (0 != new_size) {
                b.resize(new_size);
            } else {
                b.clear();
            }
        } catch (std::bad_alloc const &) {
            s = ERROR_NOT_ENOUGH_MEMORY;
        } catch (...) {
            AC_CRASH_APPLICATION();
        }
        return s;
    }

    [[nodiscard]] inline DWORD try_resize(std::string *b, size_t new_size) noexcept {
        return try_resize(*b, new_size);
    }

    [[nodiscard]] inline DWORD try_resize(std::wstring &b, size_t new_size) noexcept {
        DWORD s{ERROR_SUCCESS};
        try {
            if (0 != new_size) {
                b.resize(new_size);
            } else {
                b.clear();
            }
        } catch (std::bad_alloc const &) {
            s = ERROR_NOT_ENOUGH_MEMORY;
        } catch (...) {
            AC_CRASH_APPLICATION();
        }
        return s;
    }

    [[nodiscard]] inline DWORD try_resize(std::wstring *b, size_t new_size) noexcept {
        return try_resize(*b, new_size);
    }

    [[nodiscard]] inline DWORD try_assign(std::string &b, char const *new_value) noexcept {
        DWORD s{ERROR_SUCCESS};
        try {
            if (new_value) {
                b = new_value;
            } else {
                b.clear();
            }
        } catch (std::bad_alloc const &) {
            s = ERROR_NOT_ENOUGH_MEMORY;
        } catch (...) {
            AC_CRASH_APPLICATION();
        }
        return s;
    }

    [[nodiscard]] inline DWORD try_assign(std::string *b, char const *new_value) noexcept {
        return try_assign(*b, new_value);
    }

    [[nodiscard]] inline DWORD try_assign(std::wstring &b, wchar_t const *new_value) noexcept {
        DWORD s{ERROR_SUCCESS};
        try {
            if (new_value) {
                b = new_value;
            } else {
                b.clear();
            }
        } catch (std::bad_alloc const &) {
            s = ERROR_NOT_ENOUGH_MEMORY;
        } catch (...) {
            AC_CRASH_APPLICATION();
        }
        return s;
    }

    [[nodiscard]] inline DWORD try_assign(std::wstring *b, wchar_t const *new_value) noexcept {
        return try_assign(*b, new_value);
    }

    [[nodiscard]] inline DWORD try_remove_character_from_tail(std::string *str, char ch) noexcept {
        DWORD err{ERROR_SUCCESS};

        if (!str->empty()) {
            std::string::iterator i{str->end() - 1};
            while (i != str->begin()) {
                if (*i != ch) {
                    break;
                }
                --i;
            }
            try {
                if (i == str->begin()) {
                    str->clear();
                } else {
                    str->erase(i, str->end());
                }
            } catch (std::bad_alloc const &) {
                err = ERROR_NOT_ENOUGH_MEMORY;
            } catch (...) {
                AC_CRASH_APPLICATION();
            }
        }

        return err;
    }

    [[nodiscard]] inline DWORD try_remove_character_from_tail(std::wstring *str,
                                                              wchar_t ch) noexcept {
        DWORD err{ERROR_SUCCESS};

        if (!str->empty()) {
            std::wstring::iterator i{str->end() - 1};
            while (i != str->begin()) {
                if (*i != ch) {
                    break;
                }
                --i;
            }
            try {
                if (i == str->begin()) {
                    str->clear();
                } else {
                    str->erase(i, str->end());
                }
            } catch (std::bad_alloc const &) {
                err = ERROR_NOT_ENOUGH_MEMORY;
            } catch (...) {
                AC_CRASH_APPLICATION();
            }
        }

        return err;
    }

    class cpp_set_lang_guard {
    public:
        explicit cpp_set_lang_guard(wchar_t const *language) noexcept
            : original_language_(_wsetlocale(LC_CTYPE, NULL)) {
            _wsetlocale(LC_CTYPE, language);
        }

        cpp_set_lang_guard(cpp_set_lang_guard const &) = delete;
        cpp_set_lang_guard &operator=(cpp_set_lang_guard const &) = delete;

        cpp_set_lang_guard(cpp_set_lang_guard &&other) noexcept
            : original_language_{std::move(other.original_language_)} {
        }
        cpp_set_lang_guard &operator=(cpp_set_lang_guard &&other) noexcept {
            original_language_ = std::move(original_language_);
            return *this;
        }

        bool is_armed() const noexcept {
            return original_language_ != std::nullopt;
        }

        operator bool() const noexcept {
            return is_armed();
        }

        void disarm() {
        }

        ~cpp_set_lang_guard() noexcept {
            if (original_language_) {
                _wsetlocale(LC_CTYPE, original_language_->c_str());
            }
        }

    private:
        std::optional<std::wstring> original_language_;
    };

    // converts multibyte string to unicode
    inline void a_to_u(char const *str, std::wstring *dst, UINT codepage = CP_ACP, DWORD flags = 0) {
        DWORD size = ::MultiByteToWideChar(codepage, flags, str, -1, NULL, 0);
        if (size) {
            wbuffer buffer(size + 2, L'\0');
            size = ::MultiByteToWideChar(codepage, flags, str, -1, &buffer[0], size + 1);
            if (size) {
                *dst = &buffer[0];
            }
        }
        if (!size) {
            AC_THROW(GetLastError(), "MultiByteToWideChar");
        }
    }

    // converts multibyte string to unicode
    [[nodiscard]] inline std::wstring a_to_u(char const *str,
                                             UINT codepage = CP_ACP,
                                             DWORD flags = 0) {
        std::wstring dst;
        a_to_u(str, &dst, codepage, flags);
        return dst;
    }

    // converts unicode string to multibyte
    inline void u_to_a(wchar_t const *str,
                       std::string *dst,
                       UINT codepage = CP_ACP,
                       DWORD flags = 0,
                       char const *default_char = NULL,
                       bool *is_default_used = NULL) {
        BOOL is_default_used_tmp = FALSE;
        DWORD size = ::WideCharToMultiByte(codepage, flags, str, -1, NULL, 0, NULL, NULL);
        if (size) {
            cbuffer buffer(size + 2, '\0');
            size = ::WideCharToMultiByte(
                codepage, flags, str, -1, &buffer[0], size + 1, default_char, &is_default_used_tmp);
            if (size) {
                *dst = &buffer[0];
            }
        }
        if (!size) {
            AC_THROW(GetLastError(), "MultiByteToWideChar");
        }
        if (is_default_used) {
            *is_default_used = CPPBOOL(is_default_used_tmp);
        }
    }

    // converts unicode string to multibyte
    [[nodiscard]] inline std::string u_to_a(wchar_t const *str,
                                            UINT codepage = CP_ACP,
                                            DWORD flags = 0,
                                            char const *default_char = NULL,
                                            bool *is_default_used = NULL) {
        std::string dst;
        u_to_a(str, &dst, codepage, flags, default_char, is_default_used);
        return dst;
    }

    // makes string
    [[nodiscard]] inline std::string vmake_string(size_t max_length, char const *format, va_list arg) {
        cbuffer buffer(max_length + 1, '\0');
        vsnprintf_s(&buffer[0], buffer.size(), _TRUNCATE, format, arg);
        return std::string(&buffer[0]);
    }

    [[nodiscard]] inline std::string vmake_string(char const *format, va_list arg) {
        int str_size = _vscprintf(format, arg);
        cbuffer buffer(str_size + 1, '\0');
        vsprintf_s(&buffer[0], buffer.size(), format, arg);
        return std::string(&buffer[0]);
    }

    // makes wstring
    [[nodiscard]] std::wstring inline vmake_wstring(size_t max_length,
                                                    wchar_t const *format,
                                                    va_list arg) {
        wbuffer buffer(max_length + 1, L'\0');
        EINVAL;
        _vsnwprintf_s(&buffer[0], buffer.size(), _TRUNCATE, format, arg);
        return std::wstring(&buffer[0]);
    }

    [[nodiscard]] std::wstring inline vmake_wstring(wchar_t const *format, va_list arg) {
        int str_size = _vscwprintf(format, arg);
        wbuffer buffer(str_size + 1, L'\0');
        vswprintf_s(&buffer[0], buffer.size(), format, arg);
        return std::wstring(&buffer[0]);
    }

    // makes string
    [[nodiscard]] inline std::string make_string(size_t max_length, char const *format, ...) {
        va_list args;
        va_start(args, format);
        return vmake_string(max_length, format, args);
    }

    [[nodiscard]] inline std::string make_string(char const *format, ...) {
        va_list args;
        va_start(args, format);
        return vmake_string(format, args);
    }

    // makes wstring
    [[nodiscard]] inline std::wstring make_wstring(size_t max_length, wchar_t const *format, ...) {
        va_list args;
        va_start(args, format);
        return vmake_wstring(max_length, format, args);
    }

    [[nodiscard]] inline std::wstring make_wstring(wchar_t const *format, ...) {
        va_list args;
        va_start(args, format);
        return vmake_wstring(format, args);
    }

    [[nodiscard]] inline DWORD try_get_environment_variable(wchar_t const *name,
                                                            std::wstring *out,
                                                            DWORD suggested_size = 256) noexcept {
        DWORD err{ERROR_SUCCESS};
        DWORD len{suggested_size};
        for (;;) {
            err = try_resize(out, len);
            if (ERROR_SUCCESS != err) {
                break;
            }
            DWORD n = ::GetEnvironmentVariableW(name, &(*out)[0], len);
            if (n == 0) {
                err = GetLastError();
                out->clear();
                break;
            } else if (n < len) {
                //
                // If the function succeeds, the return value is the number of
                // characters stored in the buffer pointed to by lpBuffer, not
                // including the terminating null character.
                //
                err = try_resize(out, n);
                break;
            } else {
                len = n;
            }
        }
        return err;
    }

    [[nodiscard]] inline DWORD try_get_environment_variable_or_default(
        wchar_t const *name,
        std::wstring *value,
        wchar_t const *default_value = L"",
        DWORD suggested_size = 256) {
        DWORD err = try_get_environment_variable(name, value, suggested_size);
        if (ERROR_SUCCESS == err) {
        } else if (ERROR_ENVVAR_NOT_FOUND == err) {
            err = try_assign(value, default_value);
        }
        return err;
    }

    [[nodiscard]] inline std::wstring get_environment_variable_or_default(
        wchar_t const *name, wchar_t const *default_value = L"", DWORD suggested_size = 256) {
        std::wstring value(suggested_size, L'\0');
        DWORD err = try_get_environment_variable(name, &value, suggested_size);
        if (ERROR_SUCCESS == err) {
        } else if (ERROR_ENVVAR_NOT_FOUND == err) {
            if (default_value) {
                value = default_value;
            } else {
                value.clear();
            }
        } else {
            AC_THROW(err, "GetEnvironmentVariableW");
        }
        return value;
    }

    [[nodiscard]] inline DWORD try_expand_environment_variable(wchar_t const *in,
                                                               std::wstring *out) noexcept {
        DWORD err{ERROR_SUCCESS};
        DWORD len{static_cast<DWORD>(wcslen(in))};
        if (0 == len) {
            out->clear();
            return err;
        }
        for (;;) {
            err = try_resize(out, len);
            if (ERROR_SUCCESS != err) {
                return err;
            }
            DWORD n = ::ExpandEnvironmentStringsW(in, &(*out)[0], len);
            if (n == 0) {
                return GetLastError();
            } else if (n <= len) {
                //
                // If the function succeeds, the return value is the number of characters
                // stored in the destination buffer, including the terminating null character
                //
                err = try_resize(out, n - 1);
                return err;
            } else {
                len = n;
            }
        }
        return err;
    }

    [[nodiscard]] inline std::wstring expand_environment_variable(wchar_t const *in) {
        std::wstring value;
        DWORD err = try_expand_environment_variable(in, &value);
        if (ERROR_SUCCESS != err) {
            AC_THROW(err, "ExpandEnvironmentStringsW");
        }
        return value;
    }

    [[nodiscard]] inline std::wstring expand_environment_variable(std::wstring const &in) {
        std::wstring value;
        DWORD err = try_expand_environment_variable(in.c_str(), &value);
        if (ERROR_SUCCESS != err) {
            AC_THROW(err, "ExpandEnvironmentStringsW");
        }
        return value;
    }

    template<typename G>
    class scope_guard: private G {
    public:

        explicit constexpr scope_guard(G const &g)
            : G{g} {
        }

        template<typename C>
        explicit constexpr scope_guard(C &&g)
            : G{std::move(g)} {
        }

        constexpr scope_guard(scope_guard &&) = default;

        constexpr scope_guard &operator=(scope_guard &&) = default;

        constexpr scope_guard(scope_guard &) = delete;

        constexpr scope_guard &operator=(scope_guard &) = delete;

        ~scope_guard() noexcept {
            discharge();
        }

        void discharge() noexcept {
            if (armed_) {
                this->G::operator()();
                armed_ = false;
            }
        }

        constexpr void disarm() noexcept {
            armed_ = false;
        }

        constexpr void arm() noexcept {
            armed_ = true;
        }

        constexpr bool is_armed() const noexcept {
            return armed_;
        }

        constexpr explicit operator bool() const noexcept {
            return is_armed();
        }

    private:
        bool armed_{true};
    };

    template<typename G>
    inline constexpr auto make_scope_guard(G &&g) {
        return scope_guard<G>{std::forward<G>(g)};
    }


} // namespace ac

#endif //_AC_HELPERS_WIN32_LIBRARY_COMMON_HEADER_