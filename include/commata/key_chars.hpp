/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_BF20840C_9605_400A_A368_82EE9D338213
#define COMMATA_GUARD_BF20840C_9605_400A_A368_82EE9D338213

namespace commata {
namespace detail {

template <class Ch>
struct key_chars;

template <>
struct key_chars<char>
{
    static const char comma_c  = ',';
    static const char dquote_c = '\"';
    static const char cr_c     = '\r';
    static const char lf_c     = '\n';
};

template <>
struct key_chars<wchar_t>
{
    static const wchar_t comma_c  = L',';
    static const wchar_t dquote_c = L'\"';
    static const wchar_t cr_c     = L'\r';
    static const wchar_t lf_c     = L'\n';
};

}}

#endif
