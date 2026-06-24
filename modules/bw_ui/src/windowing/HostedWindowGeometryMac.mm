// Copyright (c) 2026 Bellweather Studios.
// SPDX-License-Identifier: Apache-2.0
// Compiled with -fobjc-arc (set per-file in CMakeLists.txt).

#if defined(__APPLE__)

#include <bw_ui/windowing/HostedWindowGeometry.h>

#import <AppKit/NSScreen.h>
#import <AppKit/NSView.h>
#import <AppKit/NSWindow.h>

namespace bws::ui::windowing
{

namespace
{
struct HostedWindowGeometry
{
    juce::Rectangle<int> nativeViewFrame;
    juce::Rectangle<int> nativeViewBounds;
    juce::Rectangle<int> nativeViewInWindow;
    juce::Rectangle<int> nativeWindowFrame;
    juce::Rectangle<int> contentLayoutFrame;
    juce::Rectangle<int> visibleFrame;
    int superviewDepth { 0 };
    bool borderless { false };
};

juce::Rectangle<int> toJuceRect(NSRect rect)
{
    return {
        juce::roundToInt(static_cast<float>(rect.origin.x)),
        juce::roundToInt(static_cast<float>(rect.origin.y)),
        juce::roundToInt(static_cast<float>(rect.size.width)),
        juce::roundToInt(static_cast<float>(rect.size.height))
    };
}

juce::String rectToString(const juce::Rectangle<int>& rect)
{
    return juce::String(rect.getX()) + ","
         + juce::String(rect.getY()) + " "
         + juce::String(rect.getWidth()) + "x"
         + juce::String(rect.getHeight());
}

std::optional<HostedWindowGeometry> measureHostedWindowGeometry(const juce::Component& component)
{
    const auto* peer = component.getPeer();

    if (peer == nullptr)
        return std::nullopt;

    void* nativeHandle = peer->getNativeHandle();

    if (nativeHandle == nullptr)
        return std::nullopt;

    NSView* view = (__bridge NSView*) nativeHandle;
    NSWindow* window = [view window];

    if (window == nil)
        return std::nullopt;

    HostedWindowGeometry geometry;
    geometry.nativeViewFrame = toJuceRect([view frame]);
    geometry.nativeViewBounds = toJuceRect([view bounds]);
    geometry.nativeViewInWindow = toJuceRect([view convertRect:[view bounds] toView:nil]);
    geometry.nativeWindowFrame = toJuceRect([window frame]);
    geometry.contentLayoutFrame = toJuceRect([window contentLayoutRect]);

    NSScreen* screen = [window screen];
    if (screen == nil)
        screen = [NSScreen mainScreen];

    geometry.visibleFrame = screen != nil ? toJuceRect([screen visibleFrame])
                                          : juce::Rectangle<int>{};
    geometry.borderless = ([window styleMask] & NSWindowStyleMaskBorderless) != 0;

    NSInteger superDepth = 0;
    for (NSView* cursor = [view superview]; cursor != nil; cursor = [cursor superview])
        ++superDepth;

    geometry.superviewDepth = static_cast<int>(superDepth);
    return geometry;
}

std::optional<HostedWindowSessionState> captureHostedWindowSessionState(const juce::Component& component)
{
    const auto geometry = measureHostedWindowGeometry(component);

    if (!geometry.has_value())
        return std::nullopt;

    HostedWindowSessionState state;
    state.nativeWindowFrame = geometry->nativeWindowFrame;
    state.nativeViewInWindow = geometry->nativeViewInWindow;
    state.borderless = geometry->borderless;

    return state;
}

bool applyHostedFullscreenExitRestore(const juce::Component& component,
                                      const HostedWindowSessionState& state)
{
    const auto geometry = measureHostedWindowGeometry(component);

    if (!geometry.has_value() || state.borderless)
        return false;

    const auto* peer = component.getPeer();
    if (peer == nullptr)
        return false;

    void* nativeHandle = peer->getNativeHandle();
    if (nativeHandle == nullptr)
        return false;

    NSView* view = (__bridge NSView*) nativeHandle;
    NSWindow* window = [view window];
    if (window == nil)
        return false;

    // Hosted fullscreen exit should restore the exact saved native frame first.
    // If that exact frame is no longer valid (display changed, etc.), clamp it
    // afterward. Preserving the current post-fullscreen frame size lets clamp
    // "win" and produces the bottom-locked restore bug we observed in-host.
    const juce::Rectangle<int> targetFrame = state.nativeWindowFrame;

    const auto currentFrame = toJuceRect([window frame]);
    if (targetFrame == currentFrame)
        return false;

    [window setFrame:NSMakeRect(targetFrame.getX(),
                                targetFrame.getY(),
                                targetFrame.getWidth(),
                                targetFrame.getHeight())
             display:YES];

    clampHostedWindowToVisibleFrame(component);
    return true;
}

bool fitHostedWindowToVisibleFrame(const juce::Component& component)
{
    const auto geometry = measureHostedWindowGeometry(component);

    if (!geometry.has_value() || geometry->borderless)
        return false;

    const auto* peer = component.getPeer();
    if (peer == nullptr)
        return false;

    void* nativeHandle = peer->getNativeHandle();
    if (nativeHandle == nullptr)
        return false;

    NSView* view = (__bridge NSView*) nativeHandle;
    NSWindow* window = [view window];
    if (window == nil)
        return false;

    const auto leftInset = geometry->nativeViewInWindow.getX();
    const auto bottomInset = geometry->nativeViewInWindow.getY();
    const auto rightInset = geometry->nativeWindowFrame.getWidth()
        - geometry->nativeViewInWindow.getRight();
    const auto topInset = geometry->nativeWindowFrame.getHeight()
        - geometry->nativeViewInWindow.getBottom();

    juce::Rectangle<int> targetFrame(
        geometry->visibleFrame.getX() - leftInset,
        geometry->visibleFrame.getY() - bottomInset,
        geometry->visibleFrame.getWidth() + leftInset + rightInset,
        geometry->visibleFrame.getHeight() + topInset + bottomInset);

    if (targetFrame == geometry->nativeWindowFrame)
        return false;

    [window setFrame:NSMakeRect(targetFrame.getX(),
                                targetFrame.getY(),
                                targetFrame.getWidth(),
                                targetFrame.getHeight())
             display:YES];
    return true;
}
}

bool exitHostedFullscreenSession(const juce::Component& component,
                                 const HostedWindowSessionState& state)
{
    bool changed = applyHostedFullscreenExitRestore(component, state);

    // JUCE/host fullscreen exit can continue mutating the hosted window after
    // the initial restore call returns. Re-apply the same native-frame restore
    // on the next message turn so Bellweather wins the final restore location.
    juce::Component::SafePointer<juce::Component> safeComponent(const_cast<juce::Component*>(&component));
    juce::MessageManager::callAsync([safeComponent, state]() {
        if (safeComponent != nullptr)
            applyHostedFullscreenExitRestore(*safeComponent, state);
    });

    return changed;
}

juce::String debugHostedWindowGeometry(const juce::Component& component)
{
    const auto geometry = measureHostedWindowGeometry(component);

    if (!geometry.has_value())
        return {};

    juce::String text;
    text << "native view frame: " << rectToString(geometry->nativeViewFrame) << "\n";
    text << "native view bounds: " << rectToString(geometry->nativeViewBounds) << "\n";
    text << "native in window: " << rectToString(geometry->nativeViewInWindow) << "\n";
    text << "native window: " << rectToString(geometry->nativeWindowFrame) << "\n";
    text << "content layout: " << rectToString(geometry->contentLayoutFrame) << "\n";
    text << "visible frame: " << rectToString(geometry->visibleFrame) << "\n";
    text << "super depth: " << juce::String(geometry->superviewDepth);
    return text;
}

bool clampHostedWindowToVisibleFrame(const juce::Component& component)
{
    const auto* peer = component.getPeer();

    if (peer == nullptr)
        return false;

    void* nativeHandle = peer->getNativeHandle();

    if (nativeHandle == nullptr)
        return false;

    NSView* view = (__bridge NSView*) nativeHandle;
    NSWindow* window = [view window];

    if (window == nil || ([window styleMask] & NSWindowStyleMaskBorderless) != 0)
        return false;

    NSScreen* screen = [window screen];
    if (screen == nil)
        screen = [NSScreen mainScreen];

    if (screen == nil)
        return false;

    NSRect visibleFrame = [screen visibleFrame];
    NSRect frame = [window frame];

    CGFloat dx = 0.0;
    CGFloat dy = 0.0;

    if (NSMinX(frame) < NSMinX(visibleFrame))
        dx = NSMinX(visibleFrame) - NSMinX(frame);
    else if (NSMaxX(frame) > NSMaxX(visibleFrame))
        dx = NSMaxX(visibleFrame) - NSMaxX(frame);

    if (NSMinY(frame) < NSMinY(visibleFrame))
        dy = NSMinY(visibleFrame) - NSMinY(frame);
    else if (NSMaxY(frame) > NSMaxY(visibleFrame))
        dy = NSMaxY(visibleFrame) - NSMaxY(frame);

    if (dx == 0.0 && dy == 0.0)
        return false;

    NSPoint origin = frame.origin;
    origin.x += dx;
    origin.y += dy;
    [window setFrameOrigin:origin];
    return true;
}

std::optional<HostedWindowSessionState> enterHostedFullscreenSession(const juce::Component& component)
{
    const auto state = captureHostedWindowSessionState(component);

    if (!state.has_value())
        return std::nullopt;

    fitHostedWindowToVisibleFrame(component);
    return state;
}

} // namespace bws::ui::windowing

#endif // __APPLE__
