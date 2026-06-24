// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include "bw_ui/internal/PaintGuard.h"

namespace bws::ui
{
thread_local bool PaintGuard::reentered = false;
} // namespace bws::ui
