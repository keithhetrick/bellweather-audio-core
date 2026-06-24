// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_fft_adapters/BwsFFT.h>

int main()
{
    bws::fft::BwsFFT fft(5);
    return fft.size() == 32 ? 0 : 1;
}
