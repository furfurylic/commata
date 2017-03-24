/**
 * These codes are licensed under the Unlisence.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_BF20840C_9605_400A_A368_82EE9D338213
#define FURFURYLIC_GUARD_BF20840C_9605_400A_A368_82EE9D338213

namespace furfurylic {
namespace commata {
namespace detail {

template <class Ch>
struct key_chars;

template <>
struct key_chars<char>
{
    static const char COMMA  = ',';
    static const char DQUOTE = '\"';
    static const char CR     = '\r';
    static const char LF     = '\n';
};

template <>
struct key_chars<wchar_t>
{
    static const wchar_t COMMA  = L',';
    static const wchar_t DQUOTE = L'\"';
    static const wchar_t CR     = L'\r';
    static const wchar_t LF     = L'\n';
};

}}}

#endif
