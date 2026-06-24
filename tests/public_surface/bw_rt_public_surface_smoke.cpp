// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_rt/AudioThreadScope.h>
#include <bw_rt/CompositionFuzz.h>
#include <bw_rt/CrossStageInvariant.h>
#include <bw_rt/LockFreeRingBuffer.h>
#include <bw_rt/RtAssertions.h>
#include <bw_rt/RtGuard.h>
#include <bw_rt/RtSafety.h>
#include <bw_rt/RtViolationBuffer.h>
#include <bw_rt/ScopedFlushDenormals.h>
#include <bw_rt/SnapToZero.h>

int main()
{
    bws::rt::LockFreeRingBuffer ring;
    ring.prepare(2, 8);

    bws::rt::RtViolationBuffer violations;
    violations.record(bws::rt::ViolationType::Other, "smoke");

    return ring.getCapacity() == 8 && violations.size() == 1 ? 0 : 1;
}
