/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ConsoleCommandResult = function(result, wasThrown, originatingCommand, savedResultIndex)
{
    var level = (wasThrown ? WebInspector.LegacyConsoleMessage.MessageLevel.Error : WebInspector.LegacyConsoleMessage.MessageLevel.Log);
    this.originatingCommand = originatingCommand;
    this.savedResultIndex = savedResultIndex;

    if (this.savedResultIndex && this.savedResultIndex > WebInspector.ConsoleCommandResult.maximumSavedResultIndex)
        WebInspector.ConsoleCommandResult.maximumSavedResultIndex = this.savedResultIndex;

    WebInspector.LegacyConsoleMessageImpl.call(this, WebInspector.LegacyConsoleMessage.MessageSource.JS, level, "", null, WebInspector.LegacyConsoleMessage.MessageType.Result, undefined, undefined, undefined, undefined, [result]);
};

WebInspector.ConsoleCommandResult.maximumSavedResultIndex = 0;

WebInspector.ConsoleCommandResult.clearMaximumSavedResultIndex = function()
{
    WebInspector.ConsoleCommandResult.maximumSavedResultIndex = 0;
}

WebInspector.ConsoleCommandResult.prototype = {
    constructor: WebInspector.ConsoleCommandResult,

    // Public

    enforcesClipboardPrefixString: false,

    toMessageElement: function()
    {
        var element = WebInspector.LegacyConsoleMessageImpl.prototype.toMessageElement.call(this);
        element.classList.add("console-user-command-result");
        if (!element.getAttribute("data-labelprefix"))
            element.setAttribute("data-labelprefix", WebInspector.UIString("Output: "));
        return element;
    },

    get clipboardPrefixString()
    {
        return "< ";
    }
};

WebInspector.ConsoleCommandResult.prototype.__proto__ = WebInspector.LegacyConsoleMessageImpl.prototype;
