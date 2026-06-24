// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_state_types/BwStateBlob.h>
#include <bw_state_types/PresetMetadata.h>
#include <bw_state_types/SystemInfo.h>

#include <array>
#include <cstddef>

int main()
{
    const std::array<std::byte, 2> bytes {std::byte {1}, std::byte {2}};
    bws::domain::BwStateBlob blob {bytes, 3};
    bws::domain::PresetMetadata metadata {"Init", "Factory", false, "init"};
    bw::SystemInfo system {};

    return !blob.isEmpty() && blob.size() == 2 && metadata.name == "Init" && system.os.empty() ? 0 : 1;
}
