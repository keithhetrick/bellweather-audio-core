// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
#pragma once

/// @file
/// @brief Process-lifetime-immutable host-environment snapshot, injected by value.

#include <filesystem>
#include <string>

namespace bw
{

// Process-lifetime-immutable host environment snapshot.
//
// Captured once at startup and injected by value into the modules that need it.
// A field that is not process-lifetime-immutable (e.g. available disk bytes)
// does not belong here - model it as a service port instead.
struct SystemInfo
{
    std::string os;                   // OS display name, truncated to 50 chars
    std::string deviceId;             // Legacy runtime device identifier (platform-dependent)
    std::string machineName;          // Human-readable hostname (optional; empty = omit on the wire)
    std::filesystem::path appDataDir; // User application data directory
    std::string stableOsIdentifier;   // Local-only canonical OS source; never a receipt binding
};

} // namespace bw
