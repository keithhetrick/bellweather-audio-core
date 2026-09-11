// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <unordered_map>

#include <juce_gui_basics/juce_gui_basics.h>

#include "bw_ui/Components/ThemedTooltipWindow.h"
#include "bw_ui/foundation/UiTheme.h"

namespace bws::ui
{

// One tooltip window per editor. Resolves the hovered control's copy from a
// pointer-keyed registry the parameter attachment populates, walking the
// ancestry from the leaf under the mouse; on a miss it chains to the base
// TooltipWindow behaviour so inherited-tooltip controls never regress.
class TooltipHub : public ThemedTooltipWindow
{
public:
    // Resolves a registered control's key (paramId or explicit) to display copy
    // at hover time, so lock-state, localization and the global on/off are read
    // when the tip is shown, not when the control is registered.
    using CopyResolver = std::function<juce::String(const juce::String& key)>;
    using EnabledResolver = std::function<bool()>;

    TooltipHub(juce::Component* parentComp, const UiThemeResolved& theme, CopyResolver resolver, int delayMs = 200,
               EnabledResolver enabledResolver = {});

    void registerControl(juce::Component& control, juce::String key);
    void unregisterControl(juce::Component& control) noexcept;

    juce::String getTipFor(juce::Component& leaf) override;

protected:
    // The inherited-tooltip fallback. Defaults to the base window behaviour
    // (TooltipClient lookup on the leaf); a seam so the fallback chain is
    // observable without the platform foreground gate.
    virtual juce::String baseTipFor(juce::Component& leaf);

private:
    std::unordered_map<juce::Component*, juce::String> registry_;
    CopyResolver resolver_;
    EnabledResolver enabledResolver_;
};

} // namespace bws::ui
