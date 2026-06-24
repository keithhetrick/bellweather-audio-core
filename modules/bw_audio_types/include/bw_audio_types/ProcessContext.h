// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <bw_audio_types/BufferView.h>
#include <bw_audio_types/ParamSnapshot.h>
#include <bw_audio_types/TransportState.h>

namespace bws::domain
{

/**
 * Unified audio processing context.
 *
 * Built once per host processing callback, passed to DSP code on the stack, and
 * used immediately. Do not store this object or any view it carries.
 *
 * BufferView points into the host's audio buffer memory. Valid only for the
 * duration of the processBlock() call. Storing this struct beyond that scope
 * creates dangling pointers.
 *
 * ParamSnapshot is a pre-block snapshot: callers read parameter state once per
 * block, then process against that stable view.
 *
 * Host path: host process callback -> build ProcessContext -> DSP engine.
 * Test path: construct directly with plain C++ value types.
 */
struct ProcessContext
{
    BufferView buffer;        // non-owning, valid for processBlock scope only
    ParamSnapshot params;     // pre-block normalized values [0,1] by ParamId
    TransportState transport; // host transport - plugins declare what they need
};

} // namespace bws::domain
