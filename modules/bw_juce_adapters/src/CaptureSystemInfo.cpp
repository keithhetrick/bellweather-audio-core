// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_juce_adapters/CaptureSystemInfo.h"

#include <juce_core/juce_core.h>

namespace bws::adapters
{

namespace
{
std::string captureStableOsIdentifier()
{
#if JUCE_WINDOWS
    return juce::WindowsRegistry::getValue("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid", {},
                                           juce::WindowsRegistry::WoW64_64bit)
        .toStdString();
#elif JUCE_LINUX
    // Match Weather Station's node-machine-id source precedence. Invalid source
    // values are rejected by the shared identity normalizer, never replaced by a hostname.
    for (const auto* path : {"/var/lib/dbus/machine-id", "/etc/machine-id"})
    {
        const auto value = juce::File(path).loadFileAsString().trim();
        if (value.isNotEmpty())
            return value.toStdString();
    }
    return {};
#elif JUCE_MAC
    return juce::SystemStats::getUniqueDeviceID().toStdString();
#else
    return {};
#endif
}
} // namespace

bw::SystemInfo captureSystemInfo()
{
    return bw::SystemInfo {
        .os = juce::SystemStats::getOperatingSystemName().substring(0, 50).toStdString(),
        .deviceId = juce::SystemStats::getUniqueDeviceID().toStdString(),
        .machineName = juce::SystemStats::getComputerName().substring(0, 200).toStdString(),
        .appDataDir = std::filesystem::path(
            juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getFullPathName().toStdString()),
        .stableOsIdentifier = captureStableOsIdentifier(),
    };
}

} // namespace bws::adapters
