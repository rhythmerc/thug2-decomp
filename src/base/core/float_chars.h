#pragma once

#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <string>
#include <system_error>

// NOT ORIGINAL - std::from_chars for a float or double, where the C++ library has none for floating point (the
// Android NDK r26's libc++ - before LLVM 20): the C library's strtod / strtof over a terminated copy, with
// from_chars's results (the end of the number read, invalid_argument when none, result_out_of_range past the
// type's range, the value left unchanged then). chars_format::hex reads the digits without their "0x".
namespace core {

template <typename Float>
std::from_chars_result float_from_chars(const char* first, const char* last, Float& value,
                                        std::chars_format format = std::chars_format::general)
{
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000
    const bool hex = format == std::chars_format::hex;
    std::string text = hex ? "0x" : "";
    const std::size_t prefix = text.size();
    text.append(first, last);
    // (from_chars takes no leading white space or '+': strtod would)
    if (first == last || *first == ' ' || *first == '\t' || *first == '+' || (hex && *first == '-')) {
        return {first, std::errc::invalid_argument};
    }
    char* end = nullptr;
    errno = 0;
    Float read{};
    if constexpr (sizeof(Float) == sizeof(float)) {
        read = std::strtof(text.c_str(), &end);
    } else {
        read = static_cast<Float>(std::strtod(text.c_str(), &end));
    }
    const std::size_t used = static_cast<std::size_t>(end - text.c_str());
    if (used <= prefix) {
        return {first, std::errc::invalid_argument};
    }
    const char* stop = first + (used - prefix);
    if (errno == ERANGE) {
        return {stop, std::errc::result_out_of_range};
    }
    value = read;
    return {stop, std::errc{}};
#else
    return std::from_chars(first, last, value, format);
#endif
}

} // namespace core
