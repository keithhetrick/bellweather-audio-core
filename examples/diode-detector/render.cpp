// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

// Offline I/O adapter. The sample kernel contains no I/O or allocations.
#include "DiodeDetector.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: bws-diode-render sample_rate capacitance_F < samples.txt > output.csv\n";
        return 2;
    }
    bws::experiment::DiodeDetector model;
    try
    {
        std::size_t rateEnd = 0, capEnd = 0;
        const double rate = std::stod(argv[1], &rateEnd);
        const double cap = std::stod(argv[2], &capEnd);
        if (rateEnd != std::string(argv[1]).size() || capEnd != std::string(argv[2]).size() ||
            !model.prepare(rate, cap))
            throw std::invalid_argument("out of range");
    }
    catch (const std::exception&)
    {
        std::cerr << "Invalid configuration: use finite rate [8000,768000] and capacitance [1e-9,1e-3].\n";
        return 2;
    }
    std::cout << "voltage_V,iterations,accepted\n" << std::setprecision(17);
    double input = 0.0;
    std::size_t count = 0;
    while (std::cin >> input)
    {
        const auto sample = model.process(input);
        std::cout << sample.voltage << ',' << sample.iterations << ',' << sample.accepted << '\n';
        if (!sample.accepted)
        {
            std::cerr << "Rejected sample " << count << "; state held. Check DDM-2 input domain.\n";
            return 3;
        }
        ++count;
    }
    if (!std::cin.eof() || count == 0 || !std::cout)
    {
        std::cerr << "Render failed: empty/malformed input or output write failure.\n";
        return 4;
    }
    return 0;
}
