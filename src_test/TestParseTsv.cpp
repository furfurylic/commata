/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <commata/parse_error.hpp>
#include <commata/parse_tsv.hpp>

#include "BaseTest.hpp"

using namespace commata;
using namespace commata::test;

namespace {

static_assert(std::is_trivially_copyable_v<tsv_source<streambuf_input<char>>>);

template <class T>
class logging_allocator
{
    std::vector<std::size_t>* allocations_;

public:
    using value_type = T;
    using size_type = std::size_t;

    logging_allocator() noexcept : allocations_(nullptr)
    {}

    explicit logging_allocator(std::vector<std::size_t>& allocations)
        noexcept :
        allocations_(std::addressof(allocations))
    {}

    template <class U>
    logging_allocator(const logging_allocator<U>& other) noexcept :
        allocations_(other.allocations_)
    {}

    T* allocate(std::size_t n)
    {
        if (allocations_) {
            allocations_->emplace_back();
        }
        const std::size_t nn = sizeof(T) * n;
        const auto p = static_cast<T*>(static_cast<void*>(new char[nn]));
        if (allocations_) {
            allocations_->back() = nn;
        }
        return p;
    }

    void deallocate(T* p, std::size_t)
    {
        delete [] static_cast<char*>(static_cast<void*>(p));
    }

    template <class U>
    bool operator==(const logging_allocator<U>&) const
    {
        return true;
    }

    template <class U>
    bool operator!=(const logging_allocator<U>&) const
    {
        return false;
    }
};

// <                <- buffer start (suppressible)
// {(field)(field)} <- record
// *                <- empty physical line
// {(field)(field)} <- record
// >                <- buffer end   (suppressible)
template <class Ch, class Tr = std::char_traits<Ch>>
class simple_transcriptor
{
    std::basic_ostream<Ch, Tr>* out_;
    bool in_value_;
    bool suppresses_buffer_events_;

public:
    using char_type = Ch;

    explicit simple_transcriptor(
        std::basic_ostream<Ch, Tr>& out,
        bool suppresses_buffer_events = false) :
        out_(std::addressof(out)), in_value_(false),
        suppresses_buffer_events_(suppresses_buffer_events)
    {}

    void start_buffer(Ch* /*buffer_begin*/, Ch* /*buffer_end*/)
    {
        if (!suppresses_buffer_events_) {
            out() << '<';
        }
    }

    void end_buffer(Ch* /*buffer_last*/)
    {
        if (!suppresses_buffer_events_) {
            out() << '>';
        }
    }

    void start_record(Ch* /*record_begin*/)
    {
        out() << '{';
    }

    void update(Ch* first, Ch* last)
    {
        if (!in_value_) {
            out() << '(';
            in_value_ = true;
        }
        out() << std::basic_string_view<Ch, Tr>(first, last - first);
    }

    void finalize(Ch* first, Ch* last)
    {
        update(first, last);
        out() << ')';
        in_value_ = false;
        *last = Ch();
    }

    void end_record(Ch* /*record_end*/)
    {
        out() << '}';
    }

    void empty_physical_line(Ch* /*where*/)
    {
        out() << '*';
    }

protected:
    std::basic_ostream<Ch, Tr>& out() noexcept
    {
        return *out_;
    }
};

template <class Ch, class Tr>
class empty_physical_line_intolerant_simple_transcriptor :
    public simple_transcriptor<Ch, Tr>
{
public:
    explicit empty_physical_line_intolerant_simple_transcriptor(
        std::basic_ostream<Ch, Tr>& out,
        bool suppresses_buffer_events = false) :
        simple_transcriptor<Ch, Tr>(out, suppresses_buffer_events)
    {}

    void empty_physical_line(Ch* /*where*/)
    {
        throw parse_error("I cannot stand an empty physical line");
    }

    void handle_exception()
    {
        std::throw_with_nested(std::logic_error("Bye bye"));
    }
};

// +                <- get buffer
// <                <- buffer start
// {(field)(field)} <- record
// *                <- empty physical line
// {(field)(field)} <- record
// >                <- buffer end   (suppressible)
// -                <- release buffer
template <class Ch, class Tr>
class transcriptor : public simple_transcriptor<Ch, Tr>
{
    std::size_t buffer_size_;
    std::unique_ptr<Ch[]> buffer_;

    using base_t = simple_transcriptor<Ch, Tr>;

public:
    using char_type = Ch;

    transcriptor(std::basic_ostream<Ch, Tr>& out, std::size_t buffer_size) :
        simple_transcriptor<Ch, Tr>(out),
        buffer_size_(buffer_size), buffer_(new Ch[buffer_size_])
    {}

    std::pair<Ch*, std::size_t> get_buffer()
    {
        this->out() << '+';
        return { buffer_.get(), buffer_size_ };
    }

    void release_buffer(Ch* buffer)
    {
        EXPECT_EQ(buffer_.get(), buffer);
        this->out() << '-';
    }

    void start_buffer(Ch* buffer_begin, Ch* buffer_end)
    {
        EXPECT_EQ(buffer_.get(),               buffer_begin);
        EXPECT_EQ(buffer_.get() + buffer_size_, buffer_end);
        this->base_t::start_buffer(buffer_begin, buffer_end);
    }

    void end_buffer(Ch* buffer_last)
    {
        EXPECT_GE(buffer_.get() + buffer_size_, buffer_last);
        this->base_t::end_buffer(buffer_last);
    }
};

// Precondition
static_assert(std::is_nothrow_move_constructible_v<simple_transcriptor<char>>);

// tsv_source invocation
// ... with an rvalue of test_collector (move)
static_assert(std::is_nothrow_invocable_v<
    const tsv_source<string_input<char>>&,
    simple_transcriptor<char>>);
static_assert(std::is_nothrow_invocable_v<
    tsv_source<string_input<char>>,
    simple_transcriptor<char>>);
// ... with a reference-wrapped test_collector
static_assert(std::is_nothrow_invocable_v<
    const tsv_source<string_input<char>>&,
    const std::reference_wrapper<simple_transcriptor<char>>&>);
static_assert(std::is_nothrow_invocable_v<
    tsv_source<string_input<char>>,
    const std::reference_wrapper<simple_transcriptor<char>>&>);
// ... with a reference-wrapped test_collector and an allocator
static_assert(std::is_nothrow_invocable_v<
    const tsv_source<string_input<char>>&,
    const std::reference_wrapper<simple_transcriptor<char>>&,
    std::size_t,
    logging_allocator<char>>);
static_assert(std::is_nothrow_invocable_v<
    tsv_source<string_input<char>>,
    const std::reference_wrapper<simple_transcriptor<char>>&,
    std::size_t,
    logging_allocator<char>>);

// Parser's nothrow move constructibility
static_assert(std::is_nothrow_move_constructible_v<
    std::invoke_result_t<
        tsv_source<string_input<char>>, simple_transcriptor<char>>>);

} // end unnamed

struct TestParseTsv :
    commata::test::BaseTest
{};

// Tests full events emitted by the parser but "yield" with a buffer whose
// length is 1
TEST_F(TestParseTsv, FullEvents1)
{
    std::ostringstream str;
    transcriptor handler(str, 1);
    parse_tsv(make_char_input("AB\tDEF\t\n\r\n\tXYZ"), std::move(handler));
    ASSERT_STREQ(
        "+<{(A>-+<B>-+<)>-+<(D>-+<E>-+<F>-+<)>-+<()}>-"
        "+<*>-+<>-"
        "+<{()>-+<(X>-+<Y>-+<Z>-+<)}>-",
        std::move(str).str().c_str());
}

// Tests full events emitted by the parser but "yield" with a buffer whose
// length is 4
TEST_F(TestParseTsv, FullEvents4)
{
    std::ostringstream str;
    transcriptor handler(str, 4);
    parse_tsv("AB\tD" "EF\t\n" "\r\n\tX" "YZ", std::move(handler));
    ASSERT_STREQ(
        "+<{(AB)(D>-+<EF)()}>-"
        "+<*"
        "{()(X>-+<YZ)}>-",
        std::move(str).str().c_str());
}

// Tests full events emitted by the parser but "yield" with a profuse buffer
TEST_F(TestParseTsv, FullEvents1024)
{
    std::wostringstream str;
    transcriptor handler(str, 1024);
    parse_tsv(make_tsv_source(L"AB\tDEF\t\n\r\n\tXYZ\n"), std::move(handler));
        // Ends with LF
    ASSERT_STREQ(
        L"+<{(AB)(DEF)()}"
        L"*"
        L"{()(XYZ)}>-",
        std::move(str).str().c_str());
}

// Tests if full events emitted by the parser but "yield" reaches the handler
// through a reference wrapper
TEST_F(TestParseTsv, ReferenceWrapper)
{
    std::ostringstream str;
    transcriptor handler(str, 1024);
    const auto source = make_tsv_source("\"AB\tDEF\"\t\n\r\r\n\t XYZ");
    parse_tsv(source, std::ref(handler));   // lvalue of the table source
    ASSERT_STREQ(
        "+<{(\"AB)(DEF\")()}"   // double quotes have no special meaning
        "*"
        "{()( XYZ)}>-",
        std::move(str).str().c_str());
}

// Tests behaviour with a handler that is deemed to have no buffer control
TEST_F(TestParseTsv, NoBufferControl)
{
    std::ostringstream str;
    simple_transcriptor handler(str, true);
    parse_tsv("12\t345\t6789", handler);
    ASSERT_STREQ("{(12)(345)(6789)}", std::move(str).str().c_str());
}

// Tests if buffer allocations take place with the specified buffer length
TEST_F(TestParseTsv, NoBufferControlWithBufferSize)
{
    std::ostringstream str;
    simple_transcriptor handler(str);
    parse_tsv("12\t3" "45\t6" "789", handler, 4);
    ASSERT_STREQ("<{(12)(3><45)(6><789)}>", std::move(str).str().c_str());
}

// Tests if buffer allocations take place with the specified buffer length and
// the allocator
TEST_F(TestParseTsv, NoBufferControlWithBufferSizeAndAllocator)
{
    std::vector<std::size_t> allocations;
    logging_allocator<char> a(allocations);
    std::ostringstream str;
    simple_transcriptor handler(str);
    parse_tsv("12\t3" "45\t6" "789", handler, 4, a);
    ASSERT_STREQ("<{(12)(3><45)(6><789)}>", std::move(str).str().c_str());
    ASSERT_EQ(1U, allocations.size());
    ASSERT_EQ(4U, allocations.front());
}

// Tests if a correct physical position information is added to the exception
// thrown and "handle_exception" is correctly called
TEST_F(TestParseTsv, Error)
{
    std::ostringstream str;
    empty_physical_line_intolerant_simple_transcriptor handler(str, true);
    try {
        parse_tsv("ABC \n\nDEF", std::move(handler));
        FAIL();
    } catch (const std::logic_error& e) {
        try {
            std::rethrow_if_nested(e);
            FAIL();
        } catch (const parse_error& e2) {
            ASSERT_TRUE(e2.get_physical_position().has_value());
            ASSERT_EQ(1U, e2.get_physical_position()->first);
            ASSERT_EQ(0U, e2.get_physical_position()->second);
        }
    }
    ASSERT_STREQ("{(ABC )}", std::move(str).str().c_str());
}

TEST_F(TestParseTsv, SourceCopyAssign)
{
    auto source = make_tsv_source("12\t345\t6789");
    const auto source2 = make_tsv_source("ABCDE\tFGHI\tJKL");
    source = source2;

    std::ostringstream str;
    simple_transcriptor handler(str, true);
    parse_tsv(source, handler);
    ASSERT_STREQ("{(ABCDE)(FGHI)(JKL)}", std::move(str).str().c_str());
}

TEST_F(TestParseTsv, SourceMoveAssign)
{
    auto source = make_tsv_source(std::wstring(L"12\t345\t6789"));
    source = make_tsv_source(std::wstring(L"ABCDE\tFGHI\tJKL"));

    std::wostringstream str;
    simple_transcriptor handler(str, true);
    parse_tsv(std::move(source), handler);
    ASSERT_STREQ(L"{(ABCDE)(FGHI)(JKL)}", std::move(str).str().c_str());
}

TEST_F(TestParseTsv, SourceSwap)
{
    auto source = make_tsv_source(std::stringbuf("12\t345\t6789"));
    auto source2 = make_tsv_source(std::stringbuf("ABCDE\tFGHI\tJKL"));
    using std::swap;
    swap(source, source2);

    std::ostringstream str;
    simple_transcriptor handler(str, true);
    parse_tsv(std::move(source), handler);
    ASSERT_STREQ("{(ABCDE)(FGHI)(JKL)}", std::move(str).str().c_str());
}
