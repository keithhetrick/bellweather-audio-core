// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "Baseline.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv)
{
    try
    {
        if (argc != 10)
            throw std::runtime_error("input output channels fs threshold ratio attack release makeup required");
        const std::uint16_t endian = 1;
        if (*reinterpret_cast<const unsigned char*>(&endian) != 1 || sizeof(double) != 8)
            throw std::runtime_error("little-endian float64 required");
        std::array<double, 7> v {};
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            std::size_t used = 0;
            const std::string arg(argv[i + 3]);
            v[i] = std::stod(arg, &used);
            if (used != arg.size() || !std::isfinite(v[i]))
                throw std::runtime_error("invalid finite argument");
        }
        if ((v[0] != 1 && v[0] != 2) || v[1] <= 0 || v[3] < 1 || v[4] <= 0 || v[5] <= 0)
            throw std::runtime_error("invalid model configuration");
        const auto channels = static_cast<int>(v[0]);
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        if (!input)
            throw std::runtime_error("cannot open input");
        const auto size = input.tellg();
        const auto frameBytes = static_cast<std::streamoff>(channels * sizeof(double));
        if (size <= 0 || size % frameBytes != 0)
            throw std::runtime_error("empty or truncated input");
        input.seekg(0);
        std::ofstream output(argv[2], std::ios::binary);
        if (!output)
            throw std::runtime_error("cannot create output");
        Baseline model(v[1], v[2], v[3], v[4], v[5], v[6]);
        for (std::streamoff offset = 0; offset < size; offset += frameBytes)
        {
            std::array<double, 2> frame {};
            input.read(reinterpret_cast<char*>(frame.data()), frameBytes);
            if (!input || !std::isfinite(frame[0]) || !std::isfinite(frame[1]))
                throw std::runtime_error("invalid input frame");
            const double gain = model.process(frame[0], frame[1]);
            if (!std::isfinite(gain))
                throw std::runtime_error("nonfinite output");
            output.write(reinterpret_cast<const char*>(&gain), sizeof(gain));
        }
        output.flush();
        if (!output)
            throw std::runtime_error("output write failed");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
