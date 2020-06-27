#pragma once

#include "asio.hpp"

#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace reader {
namespace aux {

// backport bit_cast (C++20)
// https://en.cppreference.com/w/cpp/numeric/bit_cast
template <class To, class From>
typename std::enable_if<(sizeof(To) == sizeof(From)) && std::is_trivially_copyable<From>::value &&
                            std::is_trivial<To>::value &&
                            (std::is_copy_constructible<To>::value ||
                             std::is_move_constructible<To>::value),
                        // this implementation requires that To is trivially default constructible
                        // and that To is copy constructible or move constructible.
                        To>::type
// constexpr support needs compiler magic
bit_cast(const From& src) noexcept {
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

// Cast asio::buffer objects to a value of type To
// This is derived from bit_cast.
template <class To>
typename std::enable_if<std::is_trivial<To>::value && (std::is_copy_constructible<To>::value ||
                                                       std::is_move_constructible<To>::value),
                        To>::type
buffer_cast(const asio::const_buffer src) {
    if (src.size() != sizeof(To)) {
        throw std::length_error{"`src` is not the right size for type `To`. "};
    }

    To dst;
    std::memcpy(&dst, src.data(), src.size());
    return dst;
}

} // namespace aux
} // namespace reader
