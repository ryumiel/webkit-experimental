/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLMediaSession_h
#define HTMLMediaSession_h

#if ENABLE(VIDEO)

#include "MediaPlayer.h"
#include "MediaSession.h"
#include "Timer.h"

namespace WebCore {

class Document;
class HTMLMediaElement;
class SourceBuffer;

class HTMLMediaSession final : public MediaSession {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit HTMLMediaSession(MediaSessionClient&);
    virtual ~HTMLMediaSession() { }

    void registerWithDocument(Document&);
    void unregisterWithDocument(Document&);

    bool playbackPermitted(const HTMLMediaElement&) const;
    bool dataLoadingPermitted(const HTMLMediaElement&) const;
    bool fullscreenPermitted(const HTMLMediaElement&) const;
    bool pageAllowsDataLoading(const HTMLMediaElement&) const;
    bool pageAllowsPlaybackAfterResuming(const HTMLMediaElement&) const;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    bool showingPlaybackTargetPickerPermitted(const HTMLMediaElement&) const;
    bool currentPlaybackTargetIsWireless(const HTMLMediaElement&) const;
    void showPlaybackTargetPicker(const HTMLMediaElement&);
    bool hasWirelessPlaybackTargets(const HTMLMediaElement&) const;

    bool wirelessVideoPlaybackDisabled(const HTMLMediaElement&) const;
    void setWirelessVideoPlaybackDisabled(const HTMLMediaElement&, bool);

    void setHasPlaybackTargetAvailabilityListeners(const HTMLMediaElement&, bool);
#endif

    bool requiresFullscreenForVideoPlayback(const HTMLMediaElement&) const;
    WEBCORE_EXPORT bool allowsAlternateFullscreen(const HTMLMediaElement&) const;
    MediaPlayer::Preload effectivePreloadForElement(const HTMLMediaElement&) const;

    void applyMediaPlayerRestrictions(const HTMLMediaElement&);

    // Restrictions to modify default behaviors.
    enum BehaviorRestrictionFlags {
        NoRestrictions = 0,
        RequireUserGestureForLoad = 1 << 0,
        RequireUserGestureForRateChange = 1 << 1,
        RequireUserGestureForFullscreen = 1 << 2,
        RequirePageConsentToLoadMedia = 1 << 3,
        RequirePageConsentToResumeMedia = 1 << 4,
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        RequireUserGestureToShowPlaybackTargetPicker = 1 << 5,
        WirelessVideoPlaybackDisabled =  1 << 6,
#endif
    };
    typedef unsigned BehaviorRestrictions;

    void addBehaviorRestriction(BehaviorRestrictions);
    void removeBehaviorRestriction(BehaviorRestrictions);

#if ENABLE(MEDIA_SOURCE)
    size_t maximumMediaSourceBufferSize(const SourceBuffer&) const;
#endif

private:

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void targetAvailabilityChangedTimerFired();

    // MediaPlaybackTargetPickerClient
    virtual void didChoosePlaybackTarget(const MediaPlaybackTarget&) override;
    virtual void externalOutputDeviceAvailableDidChange(bool) const override;
    virtual bool requiresPlaybackTargetRouteMonitoring() const override;
    virtual bool requestedPlaybackTargetPicker() const override { return m_haveRequestedPlaybackTargetPicker; }
#endif

    BehaviorRestrictions m_restrictions;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    mutable Timer m_targetAvailabilityChangedTimer;
    bool m_hasPlaybackTargetAvailabilityListeners { false };
    mutable bool m_hasPlaybackTargets { false };
    mutable bool m_haveRequestedPlaybackTargetPicker { false };
#endif
};

}

#endif // MediaSession_h

#endif // ENABLE(VIDEO)
