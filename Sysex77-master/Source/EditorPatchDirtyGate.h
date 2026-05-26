#pragma once

#include <functional>

/** Callbacks wired from MidiDemo (avoids Sy99ParamRegistry ↔ VoicesTableModel cycle). */
inline std::function<void()>& editorPatchDirtyMarkCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline std::function<bool()>& editorPatchDirtySuppressCallback() noexcept
{
    static std::function<bool()> cb;
    return cb;
}

/** Refresh editor sliders from LiveSynthState without clearing dirty (after synth knob edits). */
inline std::function<void()>& editorRefreshFromLiveSynthCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline void editorPatchDirtyMaybeMarkFromLiveIngest (int baselineRaw, int liveRaw) noexcept
{
    if (editorPatchDirtySuppressCallback() && editorPatchDirtySuppressCallback())
        return;

    if (baselineRaw < 0 || liveRaw == baselineRaw)
        return;

    if (auto& mark = editorPatchDirtyMarkCallback(); mark != nullptr)
        mark();
}
