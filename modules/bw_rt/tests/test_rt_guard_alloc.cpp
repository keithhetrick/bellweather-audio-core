// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Public contract tests for RT allocation observation.

#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>
#include "bw_rt/AudioThreadScope.h"
#include "bw_rt/RtGuard.h"

using namespace bws::rt;

TEST_CASE("RtViolationBuffer records allocation on audio thread", "[rt_guard][alloc]")
{
    if constexpr (!kRtGuardEnabled)
    {
        SUCCEED("RT guard allocation overrides disabled for this instrumentation lane");
        return;
    }

    auto& buffer = globalViolationBuffer();
    buffer.clear();

    SECTION("allocation inside AudioThreadScope is recorded")
    {
        {
            AudioThreadScope scope;
            auto* ptr = new int(42);
            REQUIRE(ptr != nullptr);
            delete ptr;
        }
        auto violations = buffer.snapshot();
        REQUIRE_FALSE(violations.empty());
    }

    SECTION("allocation outside AudioThreadScope is not recorded")
    {
        buffer.clear();
        auto* ptr = new int(7);
        delete ptr;
        auto violations = buffer.snapshot();
        REQUIRE(violations.empty());
    }
}

TEST_CASE("RtViolationBuffer capacity and clear", "[rt_guard][buffer]")
{
    auto& buffer = globalViolationBuffer();

    SECTION("clear resets size to zero")
    {
        buffer.clear();
        REQUIRE(buffer.size() == 0);
    }

    SECTION("kCapacity is 128")
    {
        static_assert(RtViolationBuffer::kCapacity == 128);
        REQUIRE(RtViolationBuffer::kCapacity == 128);
    }
}

TEST_CASE("isAudioThread reflects AudioThreadScope", "[rt_guard][thread]")
{
    SECTION("false outside scope")
    {
        REQUIRE_FALSE(isAudioThread());
    }

    SECTION("true inside scope")
    {
        AudioThreadScope scope;
        REQUIRE(isAudioThread());
    }

    SECTION("false after scope exits")
    {
        {
            AudioThreadScope scope;
        }
        REQUIRE_FALSE(isAudioThread());
    }

    SECTION("nested scope remains active until outer scope exits")
    {
        AudioThreadScope outer;
        {
            AudioThreadScope inner;
            REQUIRE(isAudioThread());
        }
        REQUIRE(isAudioThread());
    }

    SECTION("fresh thread is tracked without prewarming")
    {
        std::atomic<bool> before {};
        std::atomic<bool> during {};
        std::atomic<bool> after {};
        std::thread worker([&] {
            before.store(isAudioThread(), std::memory_order_relaxed);
            {
                AudioThreadScope scope;
                during.store(isAudioThread(), std::memory_order_relaxed);
            }
            after.store(isAudioThread(), std::memory_order_relaxed);
        });
        worker.join();

        REQUIRE_FALSE(before.load(std::memory_order_relaxed));
        REQUIRE(during.load(std::memory_order_relaxed));
        REQUIRE_FALSE(after.load(std::memory_order_relaxed));
    }
}
