/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7
#define FURFURYLIC_GUARD_E8F031F6_10D8_4585_9012_CFADC2F95BA7

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace furfurylic {
namespace commata {
namespace detail {

template <class T>
struct npos_impl
{
    constexpr static T npos = static_cast<T>(-1);
};

template <class T>
constexpr T npos_impl<T>::npos;

}

class csv_error :
    public std::exception, public detail::npos_impl<std::size_t>
{
    std::shared_ptr<std::string> what_;
    std::pair<std::size_t, std::size_t> physical_position_;

public:
    explicit csv_error(std::string what_arg) :
        what_(std::make_shared<std::string>(std::move(what_arg))),
        physical_position_(npos, npos)
    {}

    const char* what() const noexcept override
    {
        return what_->c_str(); // std::string::c_str is noexcept
    }

    void set_physical_position(std::size_t row, std::size_t col)
    {
        physical_position_ = std::make_pair(row, col);
    }

    const std::pair<std::size_t, std::size_t>* get_physical_position() const
        noexcept
    {
        // std::make_pair<std::size_t, std::size_t> shall not throw exceptions
        return (physical_position_ != std::make_pair(npos, npos)) ?
            &physical_position_ : nullptr;
    }
};

}}

#endif
