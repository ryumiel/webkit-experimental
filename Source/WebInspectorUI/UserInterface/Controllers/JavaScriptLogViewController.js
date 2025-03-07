/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.JavaScriptLogViewController = class JavaScriptLogViewController extends WebInspector.Object
{
    constructor(element, scrollElement, textPrompt, delegate, historySettingIdentifier)
    {
        super();

        console.assert(textPrompt instanceof WebInspector.ConsolePrompt);
        console.assert(historySettingIdentifier);

        this._element = element;
        this._scrollElement = scrollElement;

        this._promptHistorySetting = new WebInspector.Setting(historySettingIdentifier, null);

        this._prompt = textPrompt;
        this._prompt.delegate = this;
        this._prompt.history = this._promptHistorySetting.value;

        this.delegate = delegate;

        this._cleared = true;
        this._previousMessage = null;
        this._repeatCountWasInterrupted = false;

        this._sessions = [];

        this.messagesClearKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl, "K", this._handleClearShortcut.bind(this));
        this.messagesAlternateClearKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control, "L", this._handleClearShortcut.bind(this), this._element);

        this._messagesFindKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl, "F", this._handleFindShortcut.bind(this), this._element);
        this._messagesFindNextKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl, "G", this._handleFindNextShortcut.bind(this), this._element);
        this._messagesFindPreviousKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl | WebInspector.KeyboardShortcut.Modifier.Shift, "G", this._handleFindPreviousShortcut.bind(this), this._element);

        this._promptAlternateClearKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control, "L", this._handleClearShortcut.bind(this), this._prompt.element);
        this._promptFindKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl, "F", this._handleFindShortcut.bind(this), this._prompt.element);
        this._promptFindNextKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl, "G", this._handleFindNextShortcut.bind(this), this._prompt.element);
        this._promptFindPreviousKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl | WebInspector.KeyboardShortcut.Modifier.Shift, "G", this._handleFindPreviousShortcut.bind(this), this._prompt.element);

        this.startNewSession();
    }

    // Public

    get prompt()
    {
        return this._prompt;
    }

    get currentConsoleGroup()
    {
        return this._currentConsoleGroup;
    }

    clear()
    {
        this._cleared = true;

        this.startNewSession(true);

        this.prompt.focus();

        if (this.delegate && typeof this.delegate.didClearMessages === "function")
            this.delegate.didClearMessages();
    }

    startNewSession(clearPreviousSessions)
    {
        if (this._sessions.length && clearPreviousSessions) {
            for (var i = 0; i < this._sessions.length; ++i)
                this._element.removeChild(this._sessions[i].element);

            this._sessions = [];
            this._currentConsoleGroup = null;
        }

        var lastSession = this._sessions.lastValue;
        // Reuse the last session if it has no messages.
        if (lastSession && !lastSession.hasMessages()) {
            // Make sure the session is visible.
            lastSession.element.scrollIntoView();
            return;
        }

        var consoleSession = new WebInspector.ConsoleSession();

        this._previousMessage = null;
        this._repeatCountWasInterrupted = false;

        this._sessions.push(consoleSession);
        this._currentConsoleGroup = consoleSession;

        this._element.appendChild(consoleSession.element);

        // Make sure the new session is visible.
        consoleSession.element.scrollIntoView();
    }

    appendImmediateExecutionWithResult(text, result)
    {
        console.assert(result instanceof WebInspector.RemoteObject);

        var commandMessage = new WebInspector.ConsoleCommand(text);
        this._appendConsoleMessage(commandMessage, true);

        function saveResultCallback(savedResultIndex)
        {
            var commandResultMessage = new WebInspector.ConsoleCommandResult(result, false, commandMessage, savedResultIndex);
            this._appendConsoleMessage(commandResultMessage, true);
        }

        WebInspector.runtimeManager.saveResult(result, saveResultCallback.bind(this));
    }

    appendConsoleMessage(consoleMessage)
    {
        // Clone the message since there might be multiple clients using the message,
        // and since the message has a DOM element it can't be two places at once.
        var messageClone = consoleMessage.clone();

        this._appendConsoleMessage(messageClone);

        return messageClone;
    }

    updatePreviousMessageRepeatCount(count)
    {
        console.assert(this._previousMessage);
        if (!this._previousMessage)
            return;

        if (!this._repeatCountWasInterrupted) {
            this._previousMessage.repeatCount = count - (this._previousMessage.ignoredCount || 0);
            this._previousMessage.updateRepeatCount();
        } else {
            var newMessage = this._previousMessage.clone();

            // If this message is repeated after being cloned, messageRepeatCountUpdated will be called with a
            // count that includes the count of this message before cloning. We should ignore instances of this
            // log that occurred before we cloned, so set a property on the message with the number of previous
            // instances of this log that we should ignore.
            newMessage.ignoredCount = newMessage.repeatCount + (this._previousMessage.ignoredCount || 0);
            newMessage.repeatCount = 1;

            this._appendConsoleMessage(newMessage);
        }
    }

    isScrolledToBottom()
    {
        // Lie about being scrolled to the bottom if we have a pending request to scroll to the bottom soon.
        return this._scrollToBottomTimeout || this._scrollElement.isScrolledToBottom();
    }

    scrollToBottom()
    {
        if (this._scrollToBottomTimeout)
            return;

        function delayedWork()
        {
            this._scrollToBottomTimeout = null;
            this._scrollElement.scrollTop = this._scrollElement.scrollHeight;
        }

        // Don't scroll immediately so we are not causing excessive layouts when there
        // are many messages being added at once.
        this._scrollToBottomTimeout = setTimeout(delayedWork.bind(this), 0);
    }

    // Protected

    consolePromptHistoryDidChange(prompt)
    {
        this._promptHistorySetting.value = this.prompt.history;
    }

    consolePromptShouldCommitText(prompt, text, cursorIsAtLastPosition, handler)
    {
        // Always commit the text if we are not at the last position.
        if (!cursorIsAtLastPosition) {
            handler(true);
            return;
        }

        // COMPATIBILITY (iOS 6): RuntimeAgent.parse did not exist in iOS 6. Always commit.
        if (!RuntimeAgent.parse) {
            handler(true);
            return;
        }

        function parseFinished(error, result, message, range)
        {
            handler(result !== RuntimeAgent.SyntaxErrorType.Recoverable);
        }

        RuntimeAgent.parse(text, parseFinished.bind(this));
    }

    consolePromptTextCommitted(prompt, text)
    {
        console.assert(text);

        var commandMessage = new WebInspector.ConsoleCommand(text);
        this._appendConsoleMessage(commandMessage, true);

        function printResult(result, wasThrown, savedResultIndex)
        {
            if (!result || this._cleared)
                return;

            this._appendConsoleMessage(new WebInspector.ConsoleCommandResult(result, wasThrown, commandMessage, savedResultIndex), true);
        }

        text += "\n//# sourceURL=__WebInspectorConsole__\n";

        WebInspector.runtimeManager.evaluateInInspectedWindow(text, "console", true, false, false, true, true, printResult.bind(this));
    }

    // Private

    _handleClearShortcut()
    {
        this.clear();
    }

    _handleFindShortcut()
    {
        this.delegate.focusSearchBar();
    }

    _handleFindNextShortcut()
    {
        this.delegate.highlightNextSearchMatch();
    }

    _handleFindPreviousShortcut()
    {
        this.delegate.highlightPreviousSearchMatch();
    }

    _appendConsoleMessage(msg, repeatCountWasInterrupted)
    {
        var wasScrolledToBottom = this.isScrolledToBottom();

        this._cleared = false;
        this._repeatCountWasInterrupted = repeatCountWasInterrupted || false;

        if (!repeatCountWasInterrupted)
            this._previousMessage = msg;

        if (msg.type === WebInspector.LegacyConsoleMessage.MessageType.EndGroup) {
            var parentGroup = this._currentConsoleGroup.parentGroup;
            if (parentGroup)
                this._currentConsoleGroup = parentGroup;
        } else {
            if (msg.type === WebInspector.LegacyConsoleMessage.MessageType.StartGroup || msg.type === WebInspector.LegacyConsoleMessage.MessageType.StartGroupCollapsed) {
                var group = new WebInspector.ConsoleGroup(this._currentConsoleGroup);
                var groupElement = group.render(msg);
                this._currentConsoleGroup.append(groupElement);
                this._currentConsoleGroup = group;
            } else
                this._currentConsoleGroup.addMessage(msg);
        }

        if (msg.type === WebInspector.LegacyConsoleMessage.MessageType.Result || wasScrolledToBottom)
            this.scrollToBottom();

        if (this.delegate && typeof this.delegate.didAppendConsoleMessage === "function")
            this.delegate.didAppendConsoleMessage(msg);
    }
};

WebInspector.JavaScriptLogViewController.CachedPropertiesDuration = 30000;
