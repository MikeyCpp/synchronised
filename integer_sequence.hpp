#pragma once

#include <tuple>

template<int ...> struct integer_sequence {};

template<int N, int ...S>
struct generate_sequence
        : generate_sequence<N - 1, N - 1, S...>
{};

template<int ...S>
struct generate_sequence<0, S...>
{
    using type = integer_sequence<S...>;
};

template<typename ...Args>
typename generate_sequence<sizeof... (Args)>::type
make_integer_sequence(const std::tuple<Args...>&)
{
    return {};
}
