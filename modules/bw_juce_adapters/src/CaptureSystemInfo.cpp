// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/CaptureSystemInfo.h"

#include <juce_core/juce_core.h>

namespace bws::adapters
{

bw::SystemInfo captureSystemInfo()
{
    return bw::SystemInfo {
        .os = juce::SystemStats::getOperatingSystemName().substring(0, 50).toStdString(),
        .deviceId = juce::SystemStats::getUniqueDeviceID().toStdString(),
        .machineName = juce::SystemStats::getComputerName().substring(0, 200).toStdString(),
        .appDataDir = std::filesystem::path(
            juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getFullPathName().toStdString()),
    };
}

} // namespace bws::adapters
