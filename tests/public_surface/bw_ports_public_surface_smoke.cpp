// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#include <bw_ports/IPlugin.h>
#include <bw_ports/IPluginParameter.h>
#include <bw_ports/IPresetStatePersistence.h>
#include <bw_ports/IScheduler.h>
#include <bw_ports/IStateSerializer.h>

#include <type_traits>

int main()
{
    static_assert(std::has_virtual_destructor_v<bws::domain::IPlugin>);
    static_assert(std::has_virtual_destructor_v<bws::domain::IPluginParameter>);
    static_assert(std::has_virtual_destructor_v<bws::domain::IScheduler>);
    static_assert(std::has_virtual_destructor_v<bws::domain::IStateSerializer>);
    return 0;
}
