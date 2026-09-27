/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef FURFURYLIC_94ED8C5E_6AC4_4A81_A797_4A6AC20ECF7F
#define FURFURYLIC_94ED8C5E_6AC4_4A81_A797_4A6AC20ECF7F

#include <type_traits>

namespace commata::test {

struct trivially_copy_constructible
{
    trivially_copy_constructible& operator=(
        const trivially_copy_constructible&)
    {
        return *this;
    }
};

static_assert(std::is_trivially_copy_constructible_v<
    trivially_copy_constructible>);
static_assert(!std::is_trivially_copy_assignable_v<
    trivially_copy_constructible>);
static_assert(!std::is_trivially_move_assignable_v<
    trivially_copy_constructible>);

struct trivially_move_constructible
{
    trivially_move_constructible(const trivially_move_constructible&)
    {}

    trivially_move_constructible(trivially_move_constructible&&) = default;
};

static_assert(std::is_copy_constructible_v<
    trivially_move_constructible>);
static_assert(!std::is_trivially_copy_constructible_v<
    trivially_move_constructible>);
static_assert(std::is_trivially_move_constructible_v<
    trivially_move_constructible>);
static_assert(!std::is_trivially_move_assignable_v<
    trivially_move_constructible>);

struct trivially_copy_assignable
{
    trivially_copy_assignable(
        const trivially_copy_assignable&)
    {}

    trivially_copy_assignable& operator=(
        const trivially_copy_assignable&) = default;
};

static_assert(std::is_copy_constructible_v<
    trivially_copy_assignable>);
static_assert(!std::is_trivially_copy_constructible_v<
    trivially_copy_assignable>);
static_assert(std::is_trivially_copy_assignable_v<
    trivially_copy_assignable>);

struct trivially_move_assignable
{
    trivially_move_assignable(
        const trivially_move_assignable&)
    {}

    trivially_move_assignable& operator=(
        trivially_move_assignable&&) = default;
};

static_assert(std::is_copy_constructible_v<
    trivially_move_assignable>);
static_assert(!std::is_trivially_copy_constructible_v<
    trivially_move_assignable>);
static_assert(!std::is_trivially_move_constructible_v<
    trivially_move_assignable>);
static_assert(!std::is_trivially_copy_assignable_v<
    trivially_move_assignable>);
static_assert(std::is_trivially_move_assignable_v<
    trivially_move_assignable>);

}

#endif
