/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_BF20840C_9605_400A_A368_82EE9D338213
#define COMMATA_GUARD_BF20840C_9605_400A_A368_82EE9D338213

namespace commata::detail {

template <class Ch>
struct key_chars;

template <>
struct key_chars<char>
{
    static constexpr char comma_c  = ',';
    static constexpr char tab_c    = '\t';
    static constexpr char dquote_c = '\"';
    static constexpr char cr_c     = '\r';
    static constexpr char lf_c     = '\n';
};

template <>
struct key_chars<wchar_t>
{
    static constexpr wchar_t comma_c  = L',';
    static constexpr wchar_t tab_c    = L'\t';
    static constexpr wchar_t dquote_c = L'\"';
    static constexpr wchar_t cr_c     = L'\r';
    static constexpr wchar_t lf_c     = L'\n';
};

}

#endif
