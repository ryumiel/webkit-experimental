function createControls(root, video, host)
{
    return new Controller(root, video, host);
};

function Controller(root, video, host)
{
    this.video = video;
    this.root = root;
    this.host = host;
    this.controls = {};
    this.listeners = {};
    this.isLive = false;
    this.statusHidden = true;
    this.hasVisualMedia = false;
    this.hasWirelessPlaybackTargets = false;
    this.isListeningForPlaybackTargetAvailabilityEvent = false;

    this.addVideoListeners();
    this.createBase();
    this.createControls();
    this.updateBase();
    this.updateControls();
    this.updateDuration();
    this.updateProgress();
    this.updateTime();
    this.updateReadyState();
    this.updatePlaying();
    this.updateThumbnail();
    this.updateCaptionButton();
    this.updateCaptionContainer();
    this.updateFullscreenButtons();
    this.updateVolume();
    this.updateHasAudio();
    this.updateHasVideo();
    this.updateWirelessTargetAvailable();
    this.updateWirelessPlaybackStatus();
};

/* Enums */
Controller.InlineControls = 0;
Controller.FullScreenControls = 1;

Controller.PlayAfterSeeking = 0;
Controller.PauseAfterSeeking = 1;

/* Globals */
Controller.gWirelessImage = 'data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 245"><g fill="#1060FE"><path d="M193.6,6.3v121.6H6.4V6.3H193.6 M199.1,0.7H0.9v132.7h198.2V0.7L199.1,0.7z"/><path d="M43.5,139.3c15.8,8,35.3,12.7,56.5,12.7s40.7-4.7,56.5-12.7H43.5z"/></g><g text-anchor="middle" font-family="Helvetica Neue"><text x="100" y="204" fill="white" font-size="24">##DEVICE_TYPE##</text><text x="100" y="234" fill="#5C5C5C" font-size="21">##DEVICE_NAME##</text></g></svg>';
Controller.gSimulateWirelessPlaybackTarget = false; // Used for testing when there are no wireless targets.
Controller.gSimulateOptimizedFullscreenAvailable = false; // Used for testing when optimized fullscreen is not available.

Controller.prototype = {

    /* Constants */
    HandledVideoEvents: {
        loadstart: 'handleLoadStart',
        error: 'handleError',
        abort: 'handleAbort',
        suspend: 'handleSuspend',
        stalled: 'handleStalled',
        waiting: 'handleWaiting',
        emptied: 'handleReadyStateChange',
        loadedmetadata: 'handleReadyStateChange',
        loadeddata: 'handleReadyStateChange',
        canplay: 'handleReadyStateChange',
        canplaythrough: 'handleReadyStateChange',
        timeupdate: 'handleTimeUpdate',
        durationchange: 'handleDurationChange',
        playing: 'handlePlay',
        pause: 'handlePause',
        progress: 'handleProgress',
        volumechange: 'handleVolumeChange',
        webkitfullscreenchange: 'handleFullscreenChange',
        webkitbeginfullscreen: 'handleFullscreenChange',
        webkitendfullscreen: 'handleFullscreenChange',
    },
    HideControlsDelay: 4 * 1000,
    RewindAmount: 30,
    MaximumSeekRate: 8,
    SeekDelay: 1500,
    ClassNames: {
        active: 'active',
        exit: 'exit',
        failed: 'failed',
        hidden: 'hidden',
        hiding: 'hiding',
        threeDigitTime: 'three-digit-time',
        fourDigitTime: 'four-digit-time',
        fiveDigitTime: 'five-digit-time',
        sixDigitTime: 'six-digit-time',
        list: 'list',
        muteBox: 'mute-box',
        muted: 'muted',
        paused: 'paused',
        playing: 'playing',
        selected: 'selected',
        show: 'show',
        thumbnail: 'thumbnail',
        thumbnailImage: 'thumbnail-image',
        thumbnailTrack: 'thumbnail-track',
        volumeBox: 'volume-box',
        noVideo: 'no-video',
        down: 'down',
        out: 'out',
    },
    KeyCodes: {
        enter: 13,
        escape: 27,
        space: 32,
        pageUp: 33,
        pageDown: 34,
        end: 35,
        home: 36,
        left: 37,
        up: 38,
        right: 39,
        down: 40
    },

    extend: function(child)
    {
        for (var property in this) {
            if (!child.hasOwnProperty(property))
                child[property] = this[property];
        }
    },

    UIString: function(developmentString, replaceString, replacementString)
    {
        var localized = UIStringTable[developmentString];
        if (replaceString && replacementString)
            return localized.replace(replaceString, replacementString);

        if (localized)
            return localized;

        console.error("Localization for string \"" + developmentString + "\" not found.");
        return "LOCALIZED STRING NOT FOUND";
    },

    listenFor: function(element, eventName, handler, useCapture)
    {
        if (typeof useCapture === 'undefined')
            useCapture = false;

        if (!(this.listeners[eventName] instanceof Array))
            this.listeners[eventName] = [];
        this.listeners[eventName].push({element:element, handler:handler, useCapture:useCapture});
        element.addEventListener(eventName, this, useCapture);
    },

    stopListeningFor: function(element, eventName, handler, useCapture)
    {
        if (typeof useCapture === 'undefined')
            useCapture = false;

        if (!(this.listeners[eventName] instanceof Array))
            return;

        this.listeners[eventName] = this.listeners[eventName].filter(function(entry) {
            return !(entry.element === element && entry.handler === handler && entry.useCapture === useCapture);
        });
        element.removeEventListener(eventName, this, useCapture);
    },

    addVideoListeners: function()
    {
        for (var name in this.HandledVideoEvents) {
            this.listenFor(this.video, name, this.HandledVideoEvents[name]);
        };

        /* text tracks */
        this.listenFor(this.video.textTracks, 'change', this.handleTextTrackChange);
        this.listenFor(this.video.textTracks, 'addtrack', this.handleTextTrackAdd);
        this.listenFor(this.video.textTracks, 'removetrack', this.handleTextTrackRemove);

        /* audio tracks */
        this.listenFor(this.video.audioTracks, 'change', this.handleAudioTrackChange);
        this.listenFor(this.video.audioTracks, 'addtrack', this.handleAudioTrackAdd);
        this.listenFor(this.video.audioTracks, 'removetrack', this.handleAudioTrackRemove);

        /* video tracks */
        this.listenFor(this.video.videoTracks, 'change', this.updateHasVideo);
        this.listenFor(this.video.videoTracks, 'addtrack', this.updateHasVideo);
        this.listenFor(this.video.videoTracks, 'removetrack', this.updateHasVideo);

        /* controls attribute */
        this.controlsObserver = new MutationObserver(this.handleControlsChange.bind(this));
        this.controlsObserver.observe(this.video, { attributes: true, attributeFilter: ['controls'] });

        this.listenFor(this.video, 'webkitcurrentplaybacktargetiswirelesschanged', this.handleWirelessPlaybackChange);
    },

    removeVideoListeners: function()
    {
        for (var name in this.HandledVideoEvents) {
            this.stopListeningFor(this.video, name, this.HandledVideoEvents[name]);
        };

        /* text tracks */
        this.stopListeningFor(this.video.textTracks, 'change', this.handleTextTrackChange);
        this.stopListeningFor(this.video.textTracks, 'addtrack', this.handleTextTrackAdd);
        this.stopListeningFor(this.video.textTracks, 'removetrack', this.handleTextTrackRemove);

        /* audio tracks */
        this.stopListeningFor(this.video.audioTracks, 'change', this.handleAudioTrackChange);
        this.stopListeningFor(this.video.audioTracks, 'addtrack', this.handleAudioTrackAdd);
        this.stopListeningFor(this.video.audioTracks, 'removetrack', this.handleAudioTrackRemove);

        /* video tracks */
        this.stopListeningFor(this.video.videoTracks, 'change', this.updateHasVideo);
        this.stopListeningFor(this.video.videoTracks, 'addtrack', this.updateHasVideo);
        this.stopListeningFor(this.video.videoTracks, 'removetrack', this.updateHasVideo);

        /* controls attribute */
        this.controlsObserver.disconnect();
        delete(this.controlsObserver);

        this.stopListeningFor(this.video, 'webkitcurrentplaybacktargetiswirelesschanged', this.handleWirelessPlaybackChange);
        this.setShouldListenForPlaybackTargetAvailabilityEvent(false);
    },

    handleEvent: function(event)
    {
        var preventDefault = false;

        try {
            if (event.target === this.video) {
                var handlerName = this.HandledVideoEvents[event.type];
                var handler = this[handlerName];
                if (handler && handler instanceof Function)
                    handler.call(this, event);
            }

            if (!(this.listeners[event.type] instanceof Array))
                return;

            this.listeners[event.type].forEach(function(entry) {
                if (entry.element === event.currentTarget && entry.handler instanceof Function)
                    preventDefault |= entry.handler.call(this, event);
            }, this);
        } catch(e) {
            if (window.console)
                console.error(e);
        }

        if (preventDefault) {
            event.stopPropagation();
            event.preventDefault();
        }
    },

    createBase: function()
    {
        var base = this.base = document.createElement('div');
        base.setAttribute('pseudo', '-webkit-media-controls');
        this.listenFor(base, 'mousemove', this.handleWrapperMouseMove);
        this.listenFor(base, 'mouseout', this.handleWrapperMouseOut);
        if (this.host.textTrackContainer)
            base.appendChild(this.host.textTrackContainer);
    },

    shouldHaveAnyUI: function()
    {
        return this.shouldHaveControls() || (this.video.textTracks && this.video.textTracks.length);
    },

    shouldHaveControls: function()
    {
        if (!this.isAudio() && !this.host.mediaPlaybackAllowsInline)
            return true;

        return this.video.controls || this.isFullScreen();
    },

    setNeedsTimelineMetricsUpdate: function()
    {
        this.timelineMetricsNeedsUpdate = true;
    },

    updateTimelineMetricsIfNeeded: function()
    {
        if (this.timelineMetricsNeedsUpdate) {
            this.timelineLeft = this.controls.timeline.offsetLeft;
            this.timelineWidth = this.controls.timeline.offsetWidth;
            this.timelineHeight = this.controls.timeline.offsetHeight;
            this.timelineMetricsNeedsUpdate = false;
        }
    },

    updateBase: function()
    {
        if (this.shouldHaveAnyUI()) {
            if (!this.base.parentNode) {
                this.root.appendChild(this.base);
            }
        } else {
            if (this.base.parentNode) {
                this.base.parentNode.removeChild(this.base);
            }
        }
    },

    createControls: function()
    {
        var panel = this.controls.panel = document.createElement('div');
        panel.setAttribute('pseudo', '-webkit-media-controls-panel');
        panel.setAttribute('aria-label', (this.isAudio() ? this.UIString('Audio Playback') : this.UIString('Video Playback')));
        panel.setAttribute('role', 'toolbar');
        this.listenFor(panel, 'mousedown', this.handlePanelMouseDown);
        this.listenFor(panel, 'transitionend', this.handlePanelTransitionEnd);
        this.listenFor(panel, 'click', this.handlePanelClick);
        this.listenFor(panel, 'dblclick', this.handlePanelClick);
        this.listenFor(panel, 'dragstart', this.handlePanelDragStart);
        
        var panelTint = this.controls.panelTint = document.createElement('div');
        panelTint.setAttribute('pseudo', '-webkit-media-controls-panel-tint');
        this.listenFor(panelTint, 'mousedown', this.handlePanelMouseDown);
        this.listenFor(panelTint, 'transitionend', this.handlePanelTransitionEnd);
        this.listenFor(panelTint, 'click', this.handlePanelClick);
        this.listenFor(panelTint, 'dblclick', this.handlePanelClick);
        this.listenFor(panelTint, 'dragstart', this.handlePanelDragStart);

        var panelBackground = this.controls.panelBackground = document.createElement('div');
        panelBackground.setAttribute('pseudo', '-webkit-media-controls-panel-background');

        var rewindButton = this.controls.rewindButton = document.createElement('button');
        rewindButton.setAttribute('pseudo', '-webkit-media-controls-rewind-button');
        rewindButton.setAttribute('aria-label', this.UIString('Rewind ##sec## Seconds', '##sec##', this.RewindAmount));
        this.listenFor(rewindButton, 'click', this.handleRewindButtonClicked);

        var seekBackButton = this.controls.seekBackButton = document.createElement('button');
        seekBackButton.setAttribute('pseudo', '-webkit-media-controls-seek-back-button');
        seekBackButton.setAttribute('aria-label', this.UIString('Rewind'));
        this.listenFor(seekBackButton, 'mousedown', this.handleSeekBackMouseDown);
        this.listenFor(seekBackButton, 'mouseup', this.handleSeekBackMouseUp);

        var seekForwardButton = this.controls.seekForwardButton = document.createElement('button');
        seekForwardButton.setAttribute('pseudo', '-webkit-media-controls-seek-forward-button');
        seekForwardButton.setAttribute('aria-label', this.UIString('Fast Forward'));
        this.listenFor(seekForwardButton, 'mousedown', this.handleSeekForwardMouseDown);
        this.listenFor(seekForwardButton, 'mouseup', this.handleSeekForwardMouseUp);

        var playButton = this.controls.playButton = document.createElement('button');
        playButton.setAttribute('pseudo', '-webkit-media-controls-play-button');
        playButton.setAttribute('aria-label', this.UIString('Play'));
        this.listenFor(playButton, 'click', this.handlePlayButtonClicked);

        var statusDisplay = this.controls.statusDisplay = document.createElement('div');
        statusDisplay.setAttribute('pseudo', '-webkit-media-controls-status-display');
        statusDisplay.classList.add(this.ClassNames.hidden);

        var timelineBox = this.controls.timelineBox = document.createElement('div');
        timelineBox.setAttribute('pseudo', '-webkit-media-controls-timeline-container');

        var currentTime = this.controls.currentTime = document.createElement('div');
        currentTime.setAttribute('pseudo', '-webkit-media-controls-current-time-display');
        currentTime.setAttribute('aria-label', this.UIString('Elapsed'));
        currentTime.setAttribute('role', 'timer');

        var timeline = this.controls.timeline = document.createElement('input');
        timeline.setAttribute('pseudo', '-webkit-media-controls-timeline');
        timeline.setAttribute('aria-label', this.UIString('Duration'));
        timeline.type = 'range';
        timeline.value = 0;
        this.listenFor(timeline, 'input', this.handleTimelineInput);
        this.listenFor(timeline, 'change', this.handleTimelineChange);
        this.listenFor(timeline, 'mouseover', this.handleTimelineMouseOver);
        this.listenFor(timeline, 'mouseout', this.handleTimelineMouseOut);
        this.listenFor(timeline, 'mousemove', this.handleTimelineMouseMove);
        this.listenFor(timeline, 'mousedown', this.handleTimelineMouseDown);
        this.listenFor(timeline, 'mouseup', this.handleTimelineMouseUp);
        timeline.step = .01;

        this.timelineContextName = "_webkit-media-controls-timeline-" + this.host.generateUUID();
        timeline.style.backgroundImage = '-webkit-canvas(' + this.timelineContextName + ')';
        
        var thumbnailTrack = this.controls.thumbnailTrack = document.createElement('div');
        thumbnailTrack.classList.add(this.ClassNames.thumbnailTrack);

        var thumbnail = this.controls.thumbnail = document.createElement('div');
        thumbnail.classList.add(this.ClassNames.thumbnail);

        var thumbnailImage = this.controls.thumbnailImage = document.createElement('img');
        thumbnailImage.classList.add(this.ClassNames.thumbnailImage);

        var remainingTime = this.controls.remainingTime = document.createElement('div');
        remainingTime.setAttribute('pseudo', '-webkit-media-controls-time-remaining-display');
        remainingTime.setAttribute('aria-label', this.UIString('Remaining'));
        remainingTime.setAttribute('role', 'timer');

        var muteBox = this.controls.muteBox = document.createElement('div');
        muteBox.classList.add(this.ClassNames.muteBox);

        var muteButton = this.controls.muteButton = document.createElement('button');
        muteButton.setAttribute('pseudo', '-webkit-media-controls-mute-button');
        muteButton.setAttribute('aria-label', this.UIString('Mute'));
        this.listenFor(muteButton, 'click', this.handleMuteButtonClicked);

        var minButton = this.controls.minButton = document.createElement('button');
        minButton.setAttribute('pseudo', '-webkit-media-controls-volume-min-button');
        minButton.setAttribute('aria-label', this.UIString('Minimum Volume'));
        this.listenFor(minButton, 'click', this.handleMinButtonClicked);

        var maxButton = this.controls.maxButton = document.createElement('button');
        maxButton.setAttribute('pseudo', '-webkit-media-controls-volume-max-button');
        maxButton.setAttribute('aria-label', this.UIString('Maximum Volume'));
        this.listenFor(maxButton, 'click', this.handleMaxButtonClicked);

        var volumeBox = this.controls.volumeBox = document.createElement('div');
        volumeBox.setAttribute('pseudo', '-webkit-media-controls-volume-slider-container');
        volumeBox.classList.add(this.ClassNames.volumeBox);
        
        var volumeBoxBackground = this.controls.volumeBoxBackground = document.createElement('div');
        volumeBoxBackground.setAttribute('pseudo', '-webkit-media-controls-volume-slider-container-background');
        
        var volumeBoxTint = this.controls.volumeBoxTint = document.createElement('div');
        volumeBoxTint.setAttribute('pseudo', '-webkit-media-controls-volume-slider-container-tint');

        var volume = this.controls.volume = document.createElement('input');
        volume.setAttribute('pseudo', '-webkit-media-controls-volume-slider');
        volume.setAttribute('aria-label', this.UIString('Volume'));
        volume.type = 'range';
        volume.min = 0;
        volume.max = 1;
        volume.step = .01;
        this.listenFor(volume, 'input', this.handleVolumeSliderInput);

        this.volumeContextName = "_webkit-media-controls-volume-" + this.host.generateUUID();
        volume.style.backgroundImage = '-webkit-canvas(' + this.volumeContextName + ')';
        
        var captionButton = this.controls.captionButton = document.createElement('button');
        captionButton.setAttribute('pseudo', '-webkit-media-controls-toggle-closed-captions-button');
        captionButton.setAttribute('aria-label', this.UIString('Captions'));
        captionButton.setAttribute('aria-haspopup', 'true');
        this.listenFor(captionButton, 'click', this.handleCaptionButtonClicked);

        var fullscreenButton = this.controls.fullscreenButton = document.createElement('button');
        fullscreenButton.setAttribute('pseudo', '-webkit-media-controls-fullscreen-button');
        fullscreenButton.setAttribute('aria-label', this.UIString('Display Full Screen'));
        this.listenFor(fullscreenButton, 'click', this.handleFullscreenButtonClicked);

        var optimizedFullscreenButton = this.controls.optimizedFullscreenButton = document.createElement('button');
        optimizedFullscreenButton.setAttribute('pseudo', '-webkit-media-controls-optimized-fullscreen-button');
        optimizedFullscreenButton.setAttribute('aria-label', this.UIString('Display Optimized Full Screen'));
        this.listenFor(optimizedFullscreenButton, 'click', this.handleOptimizedFullscreenButtonClicked);

        var inlinePlaybackPlaceholder = this.controls.inlinePlaybackPlaceholder = document.createElement('div');
        inlinePlaybackPlaceholder.setAttribute('pseudo', '-webkit-media-controls-wireless-playback-status');
        if (!Controller.gSimulateOptimizedFullscreenAvailable)
            inlinePlaybackPlaceholder.classList.add(this.ClassNames.hidden);
        inlinePlaybackPlaceholder.setAttribute('aria-label', this.UIString('Display Optimized Full Screen'));

        var wirelessTargetPicker = this.controls.wirelessTargetPicker = document.createElement('button');
        wirelessTargetPicker.setAttribute('pseudo', '-webkit-media-controls-wireless-playback-picker-button');
        wirelessTargetPicker.setAttribute('aria-label', this.UIString('Choose Wireless Display'));
        this.listenFor(wirelessTargetPicker, 'click', this.handleWirelessPickerButtonClicked);

        if (!Controller.gSimulateWirelessPlaybackTarget)
            wirelessTargetPicker.classList.add(this.ClassNames.hidden);
    },

    setControlsType: function(type)
    {
        if (type === this.controlsType)
            return;
        this.controlsType = type;

        this.reconnectControls();
        this.updateShouldListenForPlaybackTargetAvailabilityEvent();
    },

    setIsLive: function(live)
    {
        if (live === this.isLive)
            return;
        this.isLive = live;

        this.updateStatusDisplay();

        this.reconnectControls();
    },

    reconnectControls: function()
    {
        this.disconnectControls();

        if (this.controlsType === Controller.InlineControls)
            this.configureInlineControls();
        else if (this.controlsType == Controller.FullScreenControls)
            this.configureFullScreenControls();

        if (this.shouldHaveControls())
            this.addControls();
    },

    disconnectControls: function(event)
    {
        for (var item in this.controls) {
            var control = this.controls[item];
            if (control && control.parentNode)
                control.parentNode.removeChild(control);
       }
    },

    configureInlineControls: function()
    {
        this.controls.panel.appendChild(this.controls.panelBackground);
        this.controls.panel.appendChild(this.controls.panelTint);
        this.controls.panel.appendChild(this.controls.playButton);
        if (!this.isLive)
            this.controls.panel.appendChild(this.controls.rewindButton);
        this.controls.panel.appendChild(this.controls.statusDisplay);
        if (!this.isLive) {
            this.controls.panel.appendChild(this.controls.timelineBox);
            this.controls.timelineBox.appendChild(this.controls.currentTime);
            this.controls.timelineBox.appendChild(this.controls.thumbnailTrack);
            this.controls.thumbnailTrack.appendChild(this.controls.timeline);
            this.controls.thumbnailTrack.appendChild(this.controls.thumbnail);
            this.controls.thumbnail.appendChild(this.controls.thumbnailImage);
            this.controls.timelineBox.appendChild(this.controls.remainingTime);
        }
        this.controls.panel.appendChild(this.controls.muteBox);
        this.controls.muteBox.appendChild(this.controls.volumeBox);
        this.controls.volumeBox.appendChild(this.controls.volumeBoxBackground);
        this.controls.volumeBox.appendChild(this.controls.volumeBoxTint);
        this.controls.volumeBox.appendChild(this.controls.volume);
        this.controls.muteBox.appendChild(this.controls.muteButton);
        this.controls.panel.appendChild(this.controls.wirelessTargetPicker);
        this.controls.panel.appendChild(this.controls.captionButton);
        if (!this.isAudio())
            this.controls.panel.appendChild(this.controls.fullscreenButton);

        this.controls.panel.style.removeProperty('left');
        this.controls.panel.style.removeProperty('top');
        this.controls.panel.style.removeProperty('bottom');
    },

    configureFullScreenControls: function()
    {
        this.controls.panel.appendChild(this.controls.panelBackground);
        this.controls.panel.appendChild(this.controls.panelTint);
        this.controls.panel.appendChild(this.controls.volumeBox);
        this.controls.volumeBox.appendChild(this.controls.minButton);
        this.controls.volumeBox.appendChild(this.controls.volume);
        this.controls.volumeBox.appendChild(this.controls.maxButton);
        this.controls.panel.appendChild(this.controls.seekBackButton);
        this.controls.panel.appendChild(this.controls.playButton);
        this.controls.panel.appendChild(this.controls.seekForwardButton);
        this.controls.panel.appendChild(this.controls.captionButton);
        if (!this.isAudio())
            this.controls.panel.appendChild(this.controls.fullscreenButton);
        if (!this.isLive) {
            this.controls.panel.appendChild(this.controls.timelineBox);
            this.controls.timelineBox.appendChild(this.controls.currentTime);
            this.controls.timelineBox.appendChild(this.controls.thumbnailTrack);
            this.controls.thumbnailTrack.appendChild(this.controls.timeline);
            this.controls.thumbnailTrack.appendChild(this.controls.thumbnail);
            this.controls.thumbnail.appendChild(this.controls.thumbnailImage);
            this.controls.timelineBox.appendChild(this.controls.remainingTime);
        } else
            this.controls.panel.appendChild(this.controls.statusDisplay);
    },

    updateControls: function()
    {
        if (this.isFullScreen())
            this.setControlsType(Controller.FullScreenControls);
        else
            this.setControlsType(Controller.InlineControls);

        this.setNeedsTimelineMetricsUpdate();
    },

    updateStatusDisplay: function(event)
    {
        this.updateShouldListenForPlaybackTargetAvailabilityEvent();
        if (this.video.error !== null)
            this.controls.statusDisplay.innerText = this.UIString('Error');
        else if (this.isLive && this.video.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA)
            this.controls.statusDisplay.innerText = this.UIString('Live Broadcast');
        else if (this.video.networkState === HTMLMediaElement.NETWORK_LOADING)
            this.controls.statusDisplay.innerText = this.UIString('Loading');
        else
            this.controls.statusDisplay.innerText = '';

        this.setStatusHidden(!this.isLive && this.video.readyState > HTMLMediaElement.HAVE_NOTHING && !this.video.error);
    },

    handleLoadStart: function(event)
    {
        this.updateStatusDisplay();
        this.updateProgress();
    },

    handleError: function(event)
    {
        this.updateStatusDisplay();
    },

    handleAbort: function(event)
    {
        this.updateStatusDisplay();
    },

    handleSuspend: function(event)
    {
        this.updateStatusDisplay();
    },

    handleStalled: function(event)
    {
        this.updateStatusDisplay();
        this.updateProgress();
    },

    handleWaiting: function(event)
    {
        this.updateStatusDisplay();
    },

    handleReadyStateChange: function(event)
    {
        this.hasVisualMedia = this.video.videoTracks && this.video.videoTracks.length > 0;
        this.updateReadyState();
        this.updateDuration();
        this.updateCaptionButton();
        this.updateCaptionContainer();
        this.updateFullscreenButtons();
        this.updateProgress();
    },

    handleTimeUpdate: function(event)
    {
        if (!this.scrubbing)
            this.updateTime();
        this.drawTimelineBackground();
    },

    handleDurationChange: function(event)
    {
        this.updateDuration();
        this.updateTime(true);
        this.updateProgress(true);
    },

    handlePlay: function(event)
    {
        this.setPlaying(true);
    },

    handlePause: function(event)
    {
        this.setPlaying(false);
    },

    handleProgress: function(event)
    {
        this.updateProgress();
    },

    handleVolumeChange: function(event)
    {
        this.updateVolume();
    },

    handleTextTrackChange: function(event)
    {
        this.updateCaptionContainer();
    },

    handleTextTrackAdd: function(event)
    {
        var track = event.track;

        if (this.trackHasThumbnails(track) && track.mode === 'disabled')
            track.mode = 'hidden';

        this.updateThumbnail();
        this.updateCaptionButton();
        this.updateCaptionContainer();
    },

    handleTextTrackRemove: function(event)
    {
        this.updateThumbnail();
        this.updateCaptionButton();
        this.updateCaptionContainer();
    },

    handleAudioTrackChange: function(event)
    {
        this.updateHasAudio();
    },

    handleAudioTrackAdd: function(event)
    {
        this.updateHasAudio();
        this.updateCaptionButton();
    },

    handleAudioTrackRemove: function(event)
    {
        this.updateHasAudio();
        this.updateCaptionButton();
    },

    isFullScreen: function()
    {
        return this.video.webkitDisplayingFullscreen;
    },

    handleFullscreenChange: function(event)
    {
        this.updateBase();
        this.updateControls();

        if (this.isFullScreen()) {
            this.controls.fullscreenButton.classList.add(this.ClassNames.exit);
            this.controls.fullscreenButton.setAttribute('aria-label', this.UIString('Exit Full Screen'));
            this.host.enteredFullscreen();
        } else {
            this.controls.fullscreenButton.classList.remove(this.ClassNames.exit);
            this.controls.fullscreenButton.setAttribute('aria-label', this.UIString('Display Full Screen'));
            this.host.exitedFullscreen();
        }
    },

    handleWrapperMouseMove: function(event)
    {
        if (this.controlsAreHidden())
            this.showControls();
        this.resetHideControlsTimer();
        
        if (this.scrubbing)
            this.updateTime();

        if (!this.isDragging)
            return;
        var delta = new WebKitPoint(event.clientX - this.initialDragLocation.x, event.clientY - this.initialDragLocation.y);
        this.controls.panel.style.left = this.initialOffset.x + delta.x + 'px';
        this.controls.panel.style.top = this.initialOffset.y + delta.y + 'px';
        event.stopPropagation()
    },

    handleWrapperMouseOut: function(event)
    {
        this.hideControls();
        this.clearHideControlsTimer();
    },

    handleWrapperMouseUp: function(event)
    {
        this.isDragging = false;
        this.stopListeningFor(this.base, 'mouseup', 'handleWrapperMouseUp', true);
    },

    handlePanelMouseDown: function(event)
    {
        if (event.target != this.controls.panelTint && event.target != this.controls.inlinePlaybackPlaceholder)
            return;

        if (!this.isFullScreen())
            return;

        this.listenFor(this.base, 'mouseup', this.handleWrapperMouseUp, true);
        this.isDragging = true;
        this.initialDragLocation = new WebKitPoint(event.clientX, event.clientY);
        this.initialOffset = new WebKitPoint(
            parseInt(this.controls.panel.style.left) | 0,
            parseInt(this.controls.panel.style.top) | 0
        );
    },

    handlePanelTransitionEnd: function(event)
    {
        var opacity = window.getComputedStyle(this.controls.panel).opacity;
        if (parseInt(opacity) > 0) {
            this.controls.panel.classList.remove(this.ClassNames.hidden);
            if (this.controls.panelBackground)
                this.controls.panelBackground.classList.remove(this.ClassNames.hidden);
        } else {
            this.controls.panel.classList.add(this.ClassNames.hidden);
            if (this.controls.panelBackground)
                this.controls.panelBackground.classList.add(this.ClassNames.hidden);
        }
    },

    handlePanelClick: function(event)
    {
        // Prevent clicks in the panel from playing or pausing the video in a MediaDocument.
        event.preventDefault();
    },

    handlePanelDragStart: function(event)
    {
        // Prevent drags in the panel from triggering a drag event on the <video> element.
        event.preventDefault();
    },

    handleRewindButtonClicked: function(event)
    {
        var newTime = Math.max(
                               this.video.currentTime - this.RewindAmount,
                               this.video.seekable.start(0));
        this.video.currentTime = newTime;
        return true;
    },

    canPlay: function()
    {
        return this.video.paused || this.video.ended || this.video.readyState < HTMLMediaElement.HAVE_METADATA;
    },

    handlePlayButtonClicked: function(event)
    {
        if (this.canPlay())
            this.video.play();
        else
            this.video.pause();
        return true;
    },

    handleTimelineInput: function(event)
    {
        this.video.fastSeek(this.controls.timeline.value);
    },

    handleTimelineChange: function(event)
    {
        this.video.currentTime = this.controls.timeline.value;
    },

    handleTimelineDown: function(event)
    {
        this.controls.thumbnail.classList.add(this.ClassNames.show);
    },

    handleTimelineUp: function(event)
    {
        this.controls.thumbnail.classList.remove(this.ClassNames.show);
    },

    handleTimelineMouseOver: function(event)
    {
        this.controls.thumbnail.classList.add(this.ClassNames.show);
    },

    handleTimelineMouseOut: function(event)
    {
        this.controls.thumbnail.classList.remove(this.ClassNames.show);
    },

    handleTimelineMouseMove: function(event)
    {
        if (this.controls.thumbnail.classList.contains(this.ClassNames.hidden))
            return;

        this.updateTimelineMetricsIfNeeded();
        this.controls.thumbnail.classList.add(this.ClassNames.show);
        var localPoint = webkitConvertPointFromPageToNode(this.controls.timeline, new WebKitPoint(event.clientX, event.clientY));
        var percent = (localPoint.x - this.timelineLeft) / this.timelineWidth;
        percent = Math.max(Math.min(1, percent), 0);
        this.controls.thumbnail.style.left = percent * 100 + '%';

        var thumbnailTime = percent * this.video.duration;
        for (var i = 0; i < this.video.textTracks.length; ++i) {
            var track = this.video.textTracks[i];
            if (!this.trackHasThumbnails(track))
                continue;

            if (!track.cues)
                continue;

            for (var j = 0; j < track.cues.length; ++j) {
                var cue = track.cues[j];
                if (thumbnailTime >= cue.startTime && thumbnailTime < cue.endTime) {
                    this.controls.thumbnailImage.src = cue.text;
                    return;
                }
            }
        }
    },

    handleTimelineMouseDown: function(event)
    {
        this.scrubbing = true;
    },

    handleTimelineMouseUp: function(event)
    {
        this.scrubbing = false;
    },

    handleMuteButtonClicked: function(event)
    {
        this.video.muted = !this.video.muted;
        if (this.video.muted)
            this.controls.muteButton.setAttribute('aria-label', this.UIString('Unmute'));
        this.drawVolumeBackground();
        return true;
    },

    handleMinButtonClicked: function(event)
    {
        if (this.video.muted) {
            this.video.muted = false;
            this.controls.muteButton.setAttribute('aria-label', this.UIString('Mute'));
        }
        this.video.volume = 0;
        return true;
    },

    handleMaxButtonClicked: function(event)
    {
        if (this.video.muted) {
            this.video.muted = false;
            this.controls.muteButton.setAttribute('aria-label', this.UIString('Mute'));
        }
        this.video.volume = 1;
    },

    handleVolumeSliderInput: function(event)
    {
        if (this.video.muted) {
            this.video.muted = false;
            this.controls.muteButton.setAttribute('aria-label', this.UIString('Mute'));
        }
        this.video.volume = this.controls.volume.value;
        this.drawVolumeBackground();
    },

    handleCaptionButtonClicked: function(event)
    {
        if (this.captionMenu)
            this.destroyCaptionMenu();
        else
            this.buildCaptionMenu();
        return true;
    },

    updateFullscreenButtons: function()
    {
        var shouldBeHidden = !this.video.webkitSupportsFullscreen || !this.hasVisualMedia;
        this.controls.fullscreenButton.classList.toggle(this.ClassNames.hidden, shouldBeHidden);
        this.controls.optimizedFullscreenButton.classList.toggle(this.ClassNames.hidden, shouldBeHidden);
    },

    handleFullscreenButtonClicked: function(event)
    {
        if (this.isFullScreen())
            this.video.webkitExitFullscreen();
        else
            this.video.webkitEnterFullscreen();
        return true;
    },

    handleControlsChange: function()
    {
        try {
            this.updateBase();

            if (this.shouldHaveControls())
                this.addControls();
            else
                this.removeControls();
        } catch(e) {
            if (window.console)
                console.error(e);
        }
    },

    nextRate: function()
    {
        return Math.min(this.MaximumSeekRate, Math.abs(this.video.playbackRate * 2));
    },

    handleSeekBackMouseDown: function(event)
    {
        this.actionAfterSeeking = (this.canPlay() ? Controller.PauseAfterSeeking : Controller.PlayAfterSeeking);
        this.video.play();
        this.video.playbackRate = this.nextRate() * -1;
        this.seekInterval = setInterval(this.seekBackFaster.bind(this), this.SeekDelay);
    },

    seekBackFaster: function()
    {
        this.video.playbackRate = this.nextRate() * -1;
    },

    handleSeekBackMouseUp: function(event)
    {
        this.video.playbackRate = this.video.defaultPlaybackRate;
        if (this.actionAfterSeeking === Controller.PauseAfterSeeking)
            this.video.pause();
        else if (this.actionAfterSeeking === Controller.PlayAfterSeeking)
            this.video.play();
        if (this.seekInterval)
            clearInterval(this.seekInterval);
    },

    handleSeekForwardMouseDown: function(event)
    {
        this.actionAfterSeeking = (this.canPlay() ? Controller.PauseAfterSeeking : Controller.PlayAfterSeeking);
        this.video.play();
        this.video.playbackRate = this.nextRate();
        this.seekInterval = setInterval(this.seekForwardFaster.bind(this), this.SeekDelay);
    },

    seekForwardFaster: function()
    {
        this.video.playbackRate = this.nextRate();
    },

    handleSeekForwardMouseUp: function(event)
    {
        this.video.playbackRate = this.video.defaultPlaybackRate;
        if (this.actionAfterSeeking === Controller.PauseAfterSeeking)
            this.video.pause();
        else if (this.actionAfterSeeking === Controller.PlayAfterSeeking)
            this.video.play();
        if (this.seekInterval)
            clearInterval(this.seekInterval);
    },

    updateDuration: function()
    {
        var duration = this.video.duration;
        this.controls.timeline.min = 0;
        this.controls.timeline.max = duration;

        this.setIsLive(duration === Number.POSITIVE_INFINITY);

        // Reset existing style.
        this.controls.currentTime.classList.remove(this.ClassNames.threeDigitTime);
        this.controls.currentTime.classList.remove(this.ClassNames.fourDigitTime);
        this.controls.currentTime.classList.remove(this.ClassNames.fiveDigitTime);
        this.controls.currentTime.classList.remove(this.ClassNames.sixDigitTime);
        this.controls.remainingTime.classList.remove(this.ClassNames.threeDigitTime);
        this.controls.remainingTime.classList.remove(this.ClassNames.fourDigitTime);
        this.controls.remainingTime.classList.remove(this.ClassNames.fiveDigitTime);
        this.controls.remainingTime.classList.remove(this.ClassNames.sixDigitTime);

        if (duration >= 60*60*10) {
            this.controls.currentTime.classList.add(this.ClassNames.sixDigitTime);
            this.controls.remainingTime.classList.add(this.ClassNames.sixDigitTime);
        } else if (duration >= 60*60) {
            this.controls.currentTime.classList.add(this.ClassNames.fiveDigitTime);
            this.controls.remainingTime.classList.add(this.ClassNames.fiveDigitTime);
        } else if (duration >= 60*10) {
            this.controls.currentTime.classList.add(this.ClassNames.fourDigitTime);
            this.controls.remainingTime.classList.add(this.ClassNames.fourDigitTime);
        } else {
            this.controls.currentTime.classList.add(this.ClassNames.threeDigitTime);
            this.controls.remainingTime.classList.add(this.ClassNames.threeDigitTime);
        }
    },

    progressFillStyle: function(context)
    {
        var height = this.timelineHeight;
        var gradient = context.createLinearGradient(0, 0, 0, height);
        gradient.addColorStop(0, 'rgb(2, 2, 2)');
        gradient.addColorStop(1, 'rgb(23, 23, 23)');
        return gradient;
    },

    updateProgress: function(forceUpdate)
    {
        if (!forceUpdate && this.controlsAreHidden())
            return;

        this.updateTimelineMetricsIfNeeded();
        this.drawTimelineBackground();
    },

    addRoundedRect: function(ctx, x, y, width, height, radius) {
        ctx.moveTo(x + radius, y);
        ctx.arcTo(x + width, y, x + width, y + radius, radius);
        ctx.lineTo(x + width, y + height - radius);
        ctx.arcTo(x + width, y + height, x + width - radius, y + height, radius);
        ctx.lineTo(x + radius, y + height);
        ctx.arcTo(x, y + height, x, y + height - radius, radius);
        ctx.lineTo(x, y + radius);
        ctx.arcTo(x, y, x + radius, y, radius);
    },

    drawTimelineBackground: function() {
        var dpr = window.devicePixelRatio;
        var width = this.controls.timeline.offsetWidth * dpr;
        var height = this.controls.timeline.offsetHeight * dpr;
        
        if (!width || !height)
            return;
        
        var played = this.video.currentTime / this.video.duration;
        var buffered = 0;
        for (var i = 0, end = this.video.buffered.length; i < end; ++i)
            buffered = Math.max(this.video.buffered.end(i), buffered);
        
        buffered /= this.video.duration;
        
        var ctx = document.getCSSCanvasContext('2d', this.timelineContextName, width, height);
        
        width /= dpr;
        height /= dpr;
        
        ctx.save();
        ctx.scale(dpr, dpr);
        ctx.clearRect(0, 0, width, height);
        
        var timelineHeight = 3;
        var trackHeight = 1;
        var scrubberWidth = 3;
        var scrubberHeight = 15;
        var borderSize = 2;
        var scrubberPosition = Math.max(0, Math.min(width - scrubberWidth, Math.round(width * played)));
        
        // Draw buffered section.
        ctx.save();
        ctx.fillStyle = "rgb(100, 100, 100)";
        ctx.fillRect(1, 8, Math.round(width * buffered) - borderSize, trackHeight);
        ctx.restore();
        
        // Draw timeline border.
        ctx.save();
        ctx.beginPath();
        this.addRoundedRect(ctx, scrubberPosition, 7, width - scrubberPosition, timelineHeight, timelineHeight / 2.0);
        this.addRoundedRect(ctx, scrubberPosition + 1, 8, width - scrubberPosition - borderSize , trackHeight, trackHeight / 2.0);
        ctx.closePath();
        ctx.clip("evenodd");
        ctx.fillStyle = "rgba(140, 140, 140, 0.68)";
        ctx.fillRect(0, 0, width, height);
        ctx.restore();
        
        // Draw played section.
        ctx.save(); 
        ctx.beginPath();
        this.addRoundedRect(ctx, 0, 7, width, timelineHeight, timelineHeight / 2.0);
        ctx.closePath();
        ctx.clip();
        ctx.fillStyle = "rgb(75, 75, 75)";
        ctx.fillRect(0, 0, width * played, height);
        ctx.restore();
        
        // Draw the scrubber.
        ctx.save();
        ctx.clearRect(scrubberPosition - 1, 0, scrubberWidth + borderSize, height, 0);
        ctx.beginPath();
        this.addRoundedRect(ctx, scrubberPosition, 1, scrubberWidth, scrubberHeight, 1);
        ctx.closePath();
        ctx.clip();
        ctx.fillStyle = "rgb(140, 140, 140)";
        ctx.fillRect(0, 0, width, height);
        ctx.restore();
        
        ctx.restore();
    },

    drawVolumeBackground: function() {
        var dpr = window.devicePixelRatio;
        var width = this.controls.volume.offsetWidth * dpr;
        var height = this.controls.volume.offsetHeight * dpr;

        if (!width || !height)
            return;
            
        var ctx = document.getCSSCanvasContext('2d', this.volumeContextName, width, height);
        
        width /= dpr;
        height /= dpr;
        
        ctx.save();
        ctx.scale(dpr, dpr);
        ctx.clearRect(0, 0, width, height);
        
        var seekerPosition = this.controls.volume.value;
        var trackHeight = 1;
        var timelineHeight = 3;
        var scrubberRadius = 3.5;
        var scrubberDiameter = 2 * scrubberRadius;
        var borderSize = 2;
        
        var scrubberPosition = Math.round(seekerPosition * (width - scrubberDiameter - borderSize));
        
        // Draw volume track border.
        ctx.save();
        ctx.beginPath();
        this.addRoundedRect(ctx, 0, 3, width, timelineHeight, timelineHeight / 2.0);
        this.addRoundedRect(ctx, 1, 4, width - borderSize, trackHeight, trackHeight / 2.0);
        ctx.closePath();
        ctx.clip("evenodd");
        ctx.fillStyle = "rgba(140, 140, 140, 0.68)";
        ctx.fillRect(0, 0, width, height);
        ctx.restore();
        
        // Clear a hole in the slider for the scrubber.
        ctx.save();
        ctx.beginPath();
        this.addRoundedRect(ctx, scrubberPosition, 0, scrubberDiameter + borderSize, height, (scrubberDiameter + borderSize) / 2.0);
        ctx.closePath();
        ctx.clip();
        ctx.clearRect(0, 0, width, height);
        ctx.restore();
        
        // Draw scrubber.
        ctx.save();
        ctx.beginPath();
        this.addRoundedRect(ctx, scrubberPosition + 1, 1, scrubberDiameter, scrubberDiameter, scrubberRadius);
        ctx.closePath();
        ctx.clip();
        ctx.fillStyle = "rgb(140, 140, 140)";
        ctx.fillRect(0, 0, width, height);
        ctx.restore();
        
        ctx.restore();
        
    },
    
    formatTime: function(time)
    {
        if (isNaN(time))
            time = 0;
        var absTime = Math.abs(time);
        var intSeconds = Math.floor(absTime % 60).toFixed(0);
        var intMinutes = Math.floor((absTime / 60) % 60).toFixed(0);
        var intHours = Math.floor(absTime / (60 * 60)).toFixed(0);
        var sign = time < 0 ? '-' : String();

        if (intHours > 0)
            return sign + intHours + ':' + String('00' + intMinutes).slice(-2) + ":" + String('00' + intSeconds).slice(-2);

        return sign + String('00' + intMinutes).slice(-2) + ":" + String('00' + intSeconds).slice(-2)
    },

    updatePlaying: function()
    {
        this.setPlaying(!this.canPlay());
    },

    setPlaying: function(isPlaying)
    {
        if (this.isPlaying === isPlaying)
            return;
        this.isPlaying = isPlaying;

        if (!isPlaying) {
            this.controls.panel.classList.add(this.ClassNames.paused);
            if (this.controls.panelBackground)
                this.controls.panelBackground.classList.add(this.ClassNames.paused);
            if (this.controls.panelTint)
                this.controls.panelTint.classList.add(this.ClassNames.paused);
            this.controls.playButton.classList.add(this.ClassNames.paused);
            this.controls.playButton.setAttribute('aria-label', this.UIString('Play'));
        } else {
            this.controls.panel.classList.remove(this.ClassNames.paused);
            if (this.controls.panelBackground)
                this.controls.panelBackground.classList.remove(this.ClassNames.paused);
            if (this.controls.panelTint)
                this.controls.panelTint.classList.add(this.ClassNames.paused);
            this.controls.playButton.classList.remove(this.ClassNames.paused);
            this.controls.playButton.setAttribute('aria-label', this.UIString('Pause'));
            this.resetHideControlsTimer();
        }
    },

    showControls: function()
    {
        this.setNeedsTimelineMetricsUpdate();

        this.updateTime();
        this.updateProgress(true);
        this.drawVolumeBackground();

        this.controls.panel.classList.add(this.ClassNames.show);
        this.controls.panel.classList.remove(this.ClassNames.hidden);

        if (this.controls.panelBackground) {
            this.controls.panelBackground.classList.add(this.ClassNames.show);
            this.controls.panelBackground.classList.remove(this.ClassNames.hidden);
        }

        this.updateShouldListenForPlaybackTargetAvailabilityEvent();
    },

    hideControls: function()
    {
        this.updateShouldListenForPlaybackTargetAvailabilityEvent();
        this.controls.panel.classList.remove(this.ClassNames.show);
        if (this.controls.panelBackground)
            this.controls.panelBackground.classList.remove(this.ClassNames.show);
    },

    controlsAreHidden: function()
    {
        return !this.isAudio() && !this.controls.panel.classList.contains(this.ClassNames.show);
    },

    removeControls: function()
    {
        if (this.controls.panel.parentNode)
            this.controls.panel.parentNode.removeChild(this.controls.panel);
        this.destroyCaptionMenu();
    },

    addControls: function()
    {
        this.base.appendChild(this.controls.inlinePlaybackPlaceholder);
        this.base.appendChild(this.controls.panel);
        this.setNeedsTimelineMetricsUpdate();
    },

    updateTime: function(forceUpdate)
    {
        if (!forceUpdate && this.controlsAreHidden())
            return;

        var currentTime = this.video.currentTime;
        var timeRemaining = currentTime - this.video.duration;
        this.controls.currentTime.innerText = this.formatTime(currentTime);
        this.controls.timeline.value = this.video.currentTime;
        this.controls.remainingTime.innerText = this.formatTime(timeRemaining);
    },

    updateReadyState: function()
    {
        this.updateStatusDisplay();
    },

    setStatusHidden: function(hidden)
    {
        if (this.statusHidden === hidden)
            return;

        this.statusHidden = hidden;

        if (hidden) {
            this.controls.statusDisplay.classList.add(this.ClassNames.hidden);
            this.controls.currentTime.classList.remove(this.ClassNames.hidden);
            this.controls.timeline.classList.remove(this.ClassNames.hidden);
            this.controls.remainingTime.classList.remove(this.ClassNames.hidden);
            this.setNeedsTimelineMetricsUpdate();
            this.showControls();
        } else {
            this.controls.statusDisplay.classList.remove(this.ClassNames.hidden);
            this.controls.currentTime.classList.add(this.ClassNames.hidden);
            this.controls.timeline.classList.add(this.ClassNames.hidden);
            this.controls.remainingTime.classList.add(this.ClassNames.hidden);
            this.hideControls();
        }
    },

    trackHasThumbnails: function(track)
    {
        return track.kind === 'thumbnails' || (track.kind === 'metadata' && track.label === 'thumbnails');
    },

    updateThumbnail: function()
    {
        for (var i = 0; i < this.video.textTracks.length; ++i) {
            var track = this.video.textTracks[i];
            if (this.trackHasThumbnails(track)) {
                this.controls.thumbnail.classList.remove(this.ClassNames.hidden);
                return;
            }
        }

        this.controls.thumbnail.classList.add(this.ClassNames.hidden);
    },

    updateCaptionButton: function()
    {
        if (this.video.webkitHasClosedCaptions || this.video.audioTracks.length > 1)
            this.controls.captionButton.classList.remove(this.ClassNames.hidden);
        else
            this.controls.captionButton.classList.add(this.ClassNames.hidden);
    },

    updateCaptionContainer: function()
    {
        if (!this.host.textTrackContainer)
            return;

        var hasClosedCaptions = this.video.webkitHasClosedCaptions;
        var hasHiddenClass = this.host.textTrackContainer.classList.contains(this.ClassNames.hidden);

        if (hasClosedCaptions && hasHiddenClass)
            this.host.textTrackContainer.classList.remove(this.ClassNames.hidden);
        else if (!hasClosedCaptions && !hasHiddenClass)
            this.host.textTrackContainer.classList.add(this.ClassNames.hidden);

        this.updateBase();
        this.host.updateTextTrackContainer();
    },

    buildCaptionMenu: function()
    {
        var audioTracks = this.host.sortedTrackListForMenu(this.video.audioTracks);
        var textTracks = this.host.sortedTrackListForMenu(this.video.textTracks);

        if ((!textTracks || !textTracks.length) && (!audioTracks || !audioTracks.length))
            return;

        this.captionMenu = document.createElement('div');
        this.captionMenu.setAttribute('pseudo', '-webkit-media-controls-closed-captions-container');
        this.base.appendChild(this.captionMenu);
        this.captionMenuItems = [];

        var offItem = this.host.captionMenuOffItem;
        var automaticItem = this.host.captionMenuAutomaticItem;
        var displayMode = this.host.captionDisplayMode;

        var list = document.createElement('div');
        this.captionMenu.appendChild(list);
        list.classList.add(this.ClassNames.list);

        if (audioTracks && audioTracks.length > 1) {
            var heading = document.createElement('h3');
            heading.id = 'webkitMediaControlsAudioTrackHeading'; // for AX menu label
            list.appendChild(heading);
            heading.innerText = this.UIString('Audio');

            var ul = document.createElement('ul');
            ul.setAttribute('role', 'menu');
            ul.setAttribute('aria-labelledby', 'webkitMediaControlsAudioTrackHeading');
            list.appendChild(ul);

            for (var i = 0; i < audioTracks.length; ++i) {
                var menuItem = document.createElement('li');
                menuItem.setAttribute('role', 'menuitemradio');
                menuItem.setAttribute('tabindex', '-1');
                this.captionMenuItems.push(menuItem);
                this.listenFor(menuItem, 'click', this.audioTrackItemSelected);
                this.listenFor(menuItem, 'keyup', this.handleAudioTrackItemKeyUp);
                ul.appendChild(menuItem);

                var track = audioTracks[i];
                menuItem.innerText = this.host.displayNameForTrack(track);
                menuItem.track = track;

                if (track.enabled) {
                    var trackMenuItemSelected = true;
                    menuItem.classList.add(this.ClassNames.selected);
                    menuItem.setAttribute('tabindex', '0');
                    menuItem.setAttribute('aria-checked', 'true');
                }
            }

            if (offMenu && displayMode === 'forced-only' && !trackMenuItemSelected) {
                offMenu.classList.add(this.ClassNames.selected);
                menuItem.setAttribute('tabindex', '0');
                menuItem.setAttribute('aria-checked', 'true');
            }
        }

        if (textTracks && textTracks.length > 2) {
            var heading = document.createElement('h3');
            heading.id = 'webkitMediaControlsClosedCaptionsHeading'; // for AX menu label
            list.appendChild(heading);
            heading.innerText = this.UIString('Subtitles');

            var ul = document.createElement('ul');
            ul.setAttribute('role', 'menu');
            ul.setAttribute('aria-labelledby', 'webkitMediaControlsClosedCaptionsHeading');
            list.appendChild(ul);

            for (var i = 0; i < textTracks.length; ++i) {
                var menuItem = document.createElement('li');
                menuItem.setAttribute('role', 'menuitemradio');
                menuItem.setAttribute('tabindex', '-1');
                this.captionMenuItems.push(menuItem);
                this.listenFor(menuItem, 'click', this.captionItemSelected);
                this.listenFor(menuItem, 'keyup', this.handleCaptionItemKeyUp);
                ul.appendChild(menuItem);

                var track = textTracks[i];
                menuItem.innerText = this.host.displayNameForTrack(track);
                menuItem.track = track;

                if (track === offItem) {
                    var offMenu = menuItem;
                    continue;
                }

                if (track === automaticItem) {
                    if (displayMode === 'automatic') {
                        menuItem.classList.add(this.ClassNames.selected);
                        menuItem.setAttribute('tabindex', '0');
                        menuItem.setAttribute('aria-checked', 'true');
                    }
                    continue;
                }

                if (displayMode != 'automatic' && track.mode === 'showing') {
                    var trackMenuItemSelected = true;
                    menuItem.classList.add(this.ClassNames.selected);
                    menuItem.setAttribute('tabindex', '0');
                    menuItem.setAttribute('aria-checked', 'true');
                }

            }

            if (offMenu && displayMode === 'forced-only' && !trackMenuItemSelected) {
                offMenu.classList.add(this.ClassNames.selected);
                menuItem.setAttribute('tabindex', '0');
                menuItem.setAttribute('aria-checked', 'true');
            }
        }
        
        // focus first selected menuitem
        for (var i = 0, c = this.captionMenuItems.length; i < c; i++) {
            var item = this.captionMenuItems[i];
            if (item.classList.contains(this.ClassNames.selected)) {
                item.focus();
                break;
            }
        }
        
    },

    captionItemSelected: function(event)
    {
        this.host.setSelectedTextTrack(event.target.track);
        this.destroyCaptionMenu();
    },

    focusSiblingCaptionItem: function(event)
    {
        var currentItem = event.target;
        var pendingItem = false;
        switch(event.keyCode) {
        case this.KeyCodes.left:
        case this.KeyCodes.up:
            pendingItem = currentItem.previousSibling;
            break;
        case this.KeyCodes.right:
        case this.KeyCodes.down:
            pendingItem = currentItem.nextSibling;
            break;
        }
        if (pendingItem) {
            currentItem.setAttribute('tabindex', '-1');
            pendingItem.setAttribute('tabindex', '0');
            pendingItem.focus();
        }
    },

    handleCaptionItemKeyUp: function(event)
    {
        switch (event.keyCode) {
        case this.KeyCodes.enter:
        case this.KeyCodes.space:
            this.captionItemSelected(event);
            break;
        case this.KeyCodes.escape:
            this.destroyCaptionMenu();
            break;
        case this.KeyCodes.left:
        case this.KeyCodes.up:
        case this.KeyCodes.right:
        case this.KeyCodes.down:
            this.focusSiblingCaptionItem(event);
            break;
        default:
            return;
        }
        // handled
        event.stopPropagation();
        event.preventDefault();
    },

    audioTrackItemSelected: function(event)
    {
        for (var i = 0; i < this.video.audioTracks.length; ++i) {
            var track = this.video.audioTracks[i];
            track.enabled = (track == event.target.track);
        }

        this.destroyCaptionMenu();
    },

    focusSiblingAudioTrackItem: function(event)
    {
        var currentItem = event.target;
        var pendingItem = false;
        switch(event.keyCode) {
            case this.KeyCodes.left:
            case this.KeyCodes.up:
                pendingItem = currentItem.previousSibling;
                break;
            case this.KeyCodes.right:
            case this.KeyCodes.down:
                pendingItem = currentItem.nextSibling;
                break;
        }
        if (pendingItem) {
            currentItem.setAttribute('tabindex', '-1');
            pendingItem.setAttribute('tabindex', '0');
            pendingItem.focus();
        }
    },

    handleAudioTrackItemKeyUp: function(event)
    {
        switch (event.keyCode) {
            case this.KeyCodes.enter:
            case this.KeyCodes.space:
                this.audioTrackItemSelected(event);
                break;
            case this.KeyCodes.escape:
                this.destroyCaptionMenu();
                break;
            case this.KeyCodes.left:
            case this.KeyCodes.up:
            case this.KeyCodes.right:
            case this.KeyCodes.down:
                this.focusSiblingAudioTrackItem(event);
                break;
            default:
                return;
        }
        // handled
        event.stopPropagation();
        event.preventDefault();
    },

    destroyCaptionMenu: function()
    {
        if (!this.captionMenu)
            return;

        this.captionMenuItems.forEach(function(item){
            this.stopListeningFor(item, 'click', this.captionItemSelected);
            this.stopListeningFor(item, 'keyup', this.handleCaptionItemKeyUp);
        }, this);

        // FKA and AX: focus the trigger before destroying the element with focus
        if (this.controls.captionButton)
            this.controls.captionButton.focus();

        if (this.captionMenu.parentNode)
            this.captionMenu.parentNode.removeChild(this.captionMenu);
        delete this.captionMenu;
        delete this.captionMenuItems;
    },

    updateHasAudio: function()
    {
        if (this.video.audioTracks.length)
            this.controls.muteBox.classList.remove(this.ClassNames.hidden);
        else
            this.controls.muteBox.classList.add(this.ClassNames.hidden);
    },

    updateHasVideo: function()
    {
        if (this.video.videoTracks.length)
            this.controls.panel.classList.remove(this.ClassNames.noVideo);
        else
            this.controls.panel.classList.add(this.ClassNames.noVideo);
    },

    updateVolume: function()
    {
        if (this.video.muted || !this.video.volume) {
            this.controls.muteButton.classList.add(this.ClassNames.muted);
            this.controls.volume.value = 0;
        } else {
            this.controls.muteButton.classList.remove(this.ClassNames.muted);
            this.controls.volume.value = this.video.volume;
        }
        this.drawVolumeBackground();
    },

    isAudio: function()
    {
        return this.video instanceof HTMLAudioElement;
    },

    clearHideControlsTimer: function()
    {
        if (this.hideTimer)
            clearTimeout(this.hideTimer);
        this.hideTimer = null;
    },

    resetHideControlsTimer: function()
    {
        if (this.hideTimer)
            clearTimeout(this.hideTimer);
        this.hideTimer = setTimeout(this.hideControls.bind(this), this.HideControlsDelay);
    },

    handleOptimizedFullscreenButtonClicked: function(event) {
    },

    currentPlaybackTargetIsWireless: function() {
        return Controller.gSimulateWirelessPlaybackTarget || (('webkitCurrentPlaybackTargetIsWireless' in this.video) && this.video.webkitCurrentPlaybackTargetIsWireless);
    },

    updateShouldListenForPlaybackTargetAvailabilityEvent: function() {
        var shouldListen = true;
        if (this.video.error)
            shouldListen = false;
        if (!this.isAudio() && !this.video.paused && this.controlsAreHidden())
            shouldListen = false;
        if (document.hidden)
            shouldListen = false;

        this.setShouldListenForPlaybackTargetAvailabilityEvent(shouldListen);
    },

    updateWirelessPlaybackStatus: function() {
        if (this.currentPlaybackTargetIsWireless()) {
            var backgroundImageSVG = "url('" + Controller.gWirelessImage + "')";

            var deviceName = "";
            var deviceType = "";
            var type = this.host.externalDeviceType;
            if (type == "airplay") {
                deviceType = this.UIString('##WIRELESS_PLAYBACK_DEVICE_TYPE##');
                deviceName = this.UIString('##WIRELESS_PLAYBACK_DEVICE_NAME##', '##DEVICE_NAME##', this.host.externalDeviceDisplayName || "Apple TV");
            } else if (type == "tvout") {
                deviceType = this.UIString('##TVOUT_DEVICE_TYPE##');
                deviceName = this.UIString('##TVOUT_DEVICE_NAME##');
            }

            backgroundImageSVG = backgroundImageSVG.replace('##DEVICE_TYPE##', deviceType);
            backgroundImageSVG = backgroundImageSVG.replace('##DEVICE_NAME##', deviceName);

            this.controls.inlinePlaybackPlaceholder.style.backgroundImage = backgroundImageSVG;
            this.controls.inlinePlaybackPlaceholder.setAttribute('aria-label', deviceType + ", " + deviceName);

            this.controls.inlinePlaybackPlaceholder.classList.remove(this.ClassNames.hidden);
            this.controls.wirelessTargetPicker.classList.add(this.ClassNames.playing);
        } else {
            this.controls.inlinePlaybackPlaceholder.classList.add(this.ClassNames.hidden);
            this.controls.wirelessTargetPicker.classList.remove(this.ClassNames.playing);
        }
    },

    updateWirelessTargetAvailable: function() {
        if (Controller.gSimulateWirelessPlaybackTarget || this.hasWirelessPlaybackTargets)
            this.controls.wirelessTargetPicker.classList.remove(this.ClassNames.hidden);
        else
            this.controls.wirelessTargetPicker.classList.add(this.ClassNames.hidden);
    },

    handleWirelessPickerButtonClicked: function(event)
    {
        this.video.webkitShowPlaybackTargetPicker();
        return true;
    },

    handleWirelessPlaybackChange: function(event) {
        this.updateWirelessPlaybackStatus();
        this.setNeedsTimelineMetricsUpdate();
    },

    handleWirelessTargetAvailableChange: function(event) {
        var wirelessPlaybackTargetsAvailable = event.availability == "available";
        if (this.hasWirelessPlaybackTargets === wirelessPlaybackTargetsAvailable)
            return;

        this.hasWirelessPlaybackTargets = wirelessPlaybackTargetsAvailable;
        this.updateWirelessTargetAvailable();
        this.setNeedsTimelineMetricsUpdate();
    },

    setShouldListenForPlaybackTargetAvailabilityEvent: function(shouldListen) {
        if (!window.WebKitPlaybackTargetAvailabilityEvent || this.isListeningForPlaybackTargetAvailabilityEvent == shouldListen)
            return;

        if (shouldListen && this.video.error)
            return;

        this.isListeningForPlaybackTargetAvailabilityEvent = shouldListen;
        if (shouldListen)
            this.listenFor(this.video, 'webkitplaybacktargetavailabilitychanged', this.handleWirelessTargetAvailableChange);
        else
            this.stopListeningFor(this.video, 'webkitplaybacktargetavailabilitychanged', this.handleWirelessTargetAvailableChange);
    },
};
