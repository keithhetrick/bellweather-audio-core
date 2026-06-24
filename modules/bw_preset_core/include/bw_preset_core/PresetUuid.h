// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <random>
#include <string>

namespace bws::preset
{

[[nodiscard]] inline bool isValidUuidV4(const std::string& value)
{
    if (value.size() != 36)
        return false;

    for (size_t i = 0; i < value.size(); ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (value[i] != '-')
                return false;
            continue;
        }

        if (std::isxdigit(static_cast<unsigned char>(value[i])) == 0)
            return false;
    }

    return value[14] == '4' && (value[19] == '8' || value[19] == '9' || value[19] == 'a' || value[19] == 'b' ||
                                value[19] == 'A' || value[19] == 'B');
}

[[nodiscard]] inline std::string generateUuidV4()
{
    std::array<uint8_t, 16> bytes {};
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<int> distribution(0, 255);

    std::generate(bytes.begin(), bytes.end(), [&] { return static_cast<uint8_t>(distribution(generator)); });

    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0f) | 0x40);
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3f) | 0x80);

    constexpr char hex[] = "0123456789abcdef";
    std::string uuid;
    uuid.reserve(36);

    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            uuid.push_back('-');

        uuid.push_back(hex[(bytes[i] >> 4) & 0x0f]);
        uuid.push_back(hex[bytes[i] & 0x0f]);
    }

    return uuid;
}

} // namespace bws::preset
