/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_41EB4C67_D08B_4038_9AB0_5894A078E2E3
#define COMMATA_GUARD_41EB4C67_D08B_4038_9AB0_5894A078E2E3

#include <ostream>
#include <string_view>
#include <type_traits>

namespace commata::test {

// <                <- buffer start (suppressible)
// {(field)(field)} <- record
// *                <- empty physical line
// {(field)(field)} <- record
// >                <- buffer end   (suppressible)
template <class Ch, class Tr = std::char_traits<std::remove_const_t<Ch>>>
class simple_transcriptor
{
    std::basic_ostream<std::remove_const_t<Ch>, Tr>* out_;
    bool in_value_;
    bool suppresses_buffer_events_;

public:
    using char_type = Ch;

    explicit simple_transcriptor(
        std::basic_ostream<std::remove_const_t<Ch>, Tr>& out,
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
        out() << std::basic_string_view<std::remove_const_t<Ch>, Tr>(
                    first, last - first);
    }

    void finalize(Ch* first, Ch* last)
    {
        update(first, last);
        out() << ')';
        in_value_ = false;
        if constexpr (!std::is_const_v<Ch>) {
            *last = Ch();
        }
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
    std::basic_ostream<std::remove_const_t<Ch>, Tr>& out() noexcept
    {
        return *out_;
    }
};

template <class Ch, class Tr>
simple_transcriptor(std::basic_ostream<Ch, Tr>&, bool = false)
 -> simple_transcriptor<Ch, Tr>;

}

#endif
