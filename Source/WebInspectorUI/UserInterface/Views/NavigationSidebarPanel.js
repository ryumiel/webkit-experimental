/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
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

WebInspector.NavigationSidebarPanel = class NavigationSidebarPanel extends WebInspector.SidebarPanel
{
    constructor(identifier, displayName, image, keyboardShortcutKey, autoPruneOldTopLevelResourceTreeElements, autoHideToolbarItemWhenEmpty, wantsTopOverflowShadow, element, role, label)
    {
        var keyboardShortcut = null;
        if (keyboardShortcutKey)
            keyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.Control, keyboardShortcutKey);

        if (keyboardShortcut) {
            var showToolTip = WebInspector.UIString("Show the %s navigation sidebar (%s)").format(displayName, keyboardShortcut.displayName);
            var hideToolTip = WebInspector.UIString("Hide the %s navigation sidebar (%s)").format(displayName, keyboardShortcut.displayName);
        } else {
            var showToolTip = WebInspector.UIString("Show the %s navigation sidebar").format(displayName);
            var hideToolTip = WebInspector.UIString("Hide the %s navigation sidebar").format(displayName);
        }

        super(identifier, displayName, showToolTip, hideToolTip, image, element, role, label || displayName);

        this.element.classList.add("navigation");

        this._keyboardShortcut = keyboardShortcut;
        this._keyboardShortcut.callback = this.toggle.bind(this);

        this._autoHideToolbarItemWhenEmpty = autoHideToolbarItemWhenEmpty || false;

        if (autoHideToolbarItemWhenEmpty)
            this.toolbarItem.hidden = true;

        this._visibleContentTreeOutlines = new Set;

        this.contentElement.addEventListener("scroll", this._updateContentOverflowShadowVisibility.bind(this));

        this._contentTreeOutline = this.createContentTreeOutline(true);

        this._filterBar = new WebInspector.FilterBar();
        this._filterBar.addEventListener(WebInspector.FilterBar.Event.FilterDidChange, this._filterDidChange, this);
        this.element.appendChild(this._filterBar.element);

        this._bottomOverflowShadowElement = document.createElement("div");
        this._bottomOverflowShadowElement.className = WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName;
        this.element.appendChild(this._bottomOverflowShadowElement);

        if (wantsTopOverflowShadow) {
            this._topOverflowShadowElement = document.createElement("div");
            this._topOverflowShadowElement.classList.add(WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName);
            this._topOverflowShadowElement.classList.add(WebInspector.NavigationSidebarPanel.TopOverflowShadowElementStyleClassName);
            this.element.appendChild(this._topOverflowShadowElement);
        }

        window.addEventListener("resize", this._updateContentOverflowShadowVisibility.bind(this));

        this._filtersSetting = new WebInspector.Setting(identifier + "-navigation-sidebar-filters", {});
        this._filterBar.filters = this._filtersSetting.value;

        this._emptyContentPlaceholderElement = document.createElement("div");
        this._emptyContentPlaceholderElement.className = WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderElementStyleClassName;

        this._emptyContentPlaceholderMessageElement = document.createElement("div");
        this._emptyContentPlaceholderMessageElement.className = WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderMessageElementStyleClassName;
        this._emptyContentPlaceholderElement.appendChild(this._emptyContentPlaceholderMessageElement);

        this._generateStyleRulesIfNeeded();
        this._generateDisclosureTrianglesIfNeeded();

        if (autoPruneOldTopLevelResourceTreeElements) {
            WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._checkForOldResources, this);
            WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ChildFrameWasRemoved, this._checkForOldResources, this);
            WebInspector.Frame.addEventListener(WebInspector.Frame.Event.ResourceWasRemoved, this._checkForOldResources, this);
        }
    }

    // Public

    get contentTreeOutlineElement()
    {
        return this._contentTreeOutline.element;
    }

    get contentTreeOutline()
    {
        return this._contentTreeOutline;
    }

    set contentTreeOutline(newTreeOutline)
    {
        console.assert(newTreeOutline);
        if (!newTreeOutline)
            return;

        if (this._contentTreeOutline)
            this._contentTreeOutline.element.classList.add(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName);

        this._contentTreeOutline = newTreeOutline;
        this._contentTreeOutline.element.classList.remove(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName);

        this._visibleContentTreeOutlines.delete(this._contentTreeOutline);
        this._visibleContentTreeOutlines.add(newTreeOutline);

        this._updateFilter();
    }

    get contentTreeOutlineToAutoPrune()
    {
        return this._contentTreeOutline;
    }

    get visibleContentTreeOutlines()
    {
        return this._visibleContentTreeOutlines;
    }

    get hasSelectedElement()
    {
        return !!this._contentTreeOutline.selectedTreeElement;
    }

    get filterBar()
    {
        return this._filterBar;
    }

    get restoringState()
    {
        return this._restoringState;
    }

    createContentTreeOutline(dontHideByDefault, suppressFiltering)
    {
        var contentTreeOutlineElement = document.createElement("ol");
        contentTreeOutlineElement.className = WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName;
        if (!dontHideByDefault)
            contentTreeOutlineElement.classList.add(WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName);
        this.contentElement.appendChild(contentTreeOutlineElement);

        var contentTreeOutline = new WebInspector.TreeOutline(contentTreeOutlineElement);
        contentTreeOutline.allowsRepeatSelection = true;

        if (!suppressFiltering) {
            contentTreeOutline.onadd = this._treeElementAddedOrChanged.bind(this);
            contentTreeOutline.onchange = this._treeElementAddedOrChanged.bind(this);
            contentTreeOutline.onexpand = this._treeElementExpandedOrCollapsed.bind(this);
            contentTreeOutline.oncollapse = this._treeElementExpandedOrCollapsed.bind(this);
        }

        if (dontHideByDefault)
            this._visibleContentTreeOutlines.add(contentTreeOutline);

        return contentTreeOutline;
    }

    treeElementForRepresentedObject(representedObject)
    {
        return this._contentTreeOutline.getCachedTreeElement(representedObject);
    }

    showDefaultContentView()
    {
        // Implemneted by subclasses if needed to show a content view when no existing tree element is selected.
    }

    showContentViewForCurrentSelection()
    {
        // Reselect the selected tree element to cause the content view to be shown as well. <rdar://problem/10854727>
        var selectedTreeElement = this._contentTreeOutline.selectedTreeElement;
        if (selectedTreeElement)
            selectedTreeElement.select();
    }

    saveStateToCookie(cookie)
    {
        console.assert(cookie);

        // This does not save folder selections, which lack a represented object and content view.
        var selectedTreeElement = null;
        this._visibleContentTreeOutlines.forEach(function(outline) {
            if (outline.selectedTreeElement)
                selectedTreeElement = outline.selectedTreeElement;
        });

        if (!selectedTreeElement)
            return;

        if (this._isTreeElementWithoutRepresentedObject(selectedTreeElement))
            return;

        var representedObject = selectedTreeElement.representedObject;
        cookie[WebInspector.TypeIdentifierCookieKey] = representedObject.constructor.TypeIdentifier;

        if (representedObject.saveIdentityToCookie)
            representedObject.saveIdentityToCookie(cookie);
        else
            console.error("Error: TreeElement.representedObject is missing a saveIdentityToCookie implementation. TreeElement.constructor: ", selectedTreeElement.constructor);
    }

    // This can be supplemented by subclasses that admit a simpler strategy for static tree elements.
    restoreStateFromCookie(cookie, relaxedMatchDelay)
    {
        this._pendingViewStateCookie = cookie;
        this._restoringState = true;

        // Check if any existing tree elements in any outline match the cookie.
        this._checkOutlinesForPendingViewStateCookie();

        if (this._finalAttemptToRestoreViewStateTimeout)
            clearTimeout(this._finalAttemptToRestoreViewStateTimeout);

        function finalAttemptToRestoreViewStateFromCookie()
        {
            delete this._finalAttemptToRestoreViewStateTimeout;

            this._checkOutlinesForPendingViewStateCookie(true);

            delete this._pendingViewStateCookie;
            delete this._restoringState;
        }

        // If the specific tree element wasn't found, we may need to wait for the resources
        // to be registered. We try one last time (match type only) after an arbitrary amount of timeout.
        this._finalAttemptToRestoreViewStateTimeout = setTimeout(finalAttemptToRestoreViewStateFromCookie.bind(this), relaxedMatchDelay);
    }

    showEmptyContentPlaceholder(message, hideToolbarItem)
    {
        console.assert(message);

        if (this._emptyContentPlaceholderElement.parentNode && this._emptyContentPlaceholderMessageElement.textContent === message)
            return;

        this._emptyContentPlaceholderMessageElement.textContent = message;
        this.element.appendChild(this._emptyContentPlaceholderElement);

        this._hideToolbarItemWhenEmpty = hideToolbarItem || false;
        this._updateToolbarItemVisibility();
        this._updateContentOverflowShadowVisibility();
    }

    hideEmptyContentPlaceholder()
    {
        if (!this._emptyContentPlaceholderElement.parentNode)
            return;

        this._emptyContentPlaceholderElement.parentNode.removeChild(this._emptyContentPlaceholderElement);

        this._hideToolbarItemWhenEmpty = false;
        this._updateToolbarItemVisibility();
        this._updateContentOverflowShadowVisibility();
    }

    updateEmptyContentPlaceholder(message)
    {
        this._updateToolbarItemVisibility();

        if (!this._contentTreeOutline.children.length) {
            // No tree elements, so no results.
            this.showEmptyContentPlaceholder(message);
        } else if (!this._emptyFilterResults) {
            // There are tree elements, and not all of them are hidden by the filter.
            this.hideEmptyContentPlaceholder();
        }
    }

    updateFilter()
    {
        this._updateFilter();
    }

    hasCustomFilters()
    {
        // Implemented by subclasses if needed.
        return false;
    }

    matchTreeElementAgainstCustomFilters(treeElement)
    {
        // Implemented by subclasses if needed.
        return true;
    }

    matchTreeElementAgainstFilterFunctions(treeElement)
    {
        for (var filterFunction of this._filterFunctions) {
            if (filterFunction(treeElement))
                return true;
        }
        return false;
    }

    applyFiltersToTreeElement(treeElement)
    {
        if (!this._filterBar.hasActiveFilters() && !this.hasCustomFilters()) {
            // No filters, so make everything visible.
            treeElement.hidden = false;

            // If this tree element was expanded during filtering, collapse it again.
            if (treeElement.expanded && treeElement.__wasExpandedDuringFiltering) {
                delete treeElement.__wasExpandedDuringFiltering;
                treeElement.collapse();
            }

            return;
        }

        var filterableData = treeElement.filterableData || {};

        var matchedBuiltInFilters = false;

        var self = this;
        function matchTextFilter(inputs)
        {
            if (!inputs || !self._textFilterRegex)
                return true;

            // Convert to a single item array if needed.
            if (!(inputs instanceof Array))
                inputs = [inputs];

            // Loop over all the inputs and try to match them.
            for (var input of inputs) {
                if (!input)
                    continue;
                if (self._textFilterRegex.test(input)) {
                    matchedBuiltInFilters = true;
                    return true;
                }
            }

            // No inputs matched.
            return false;
        }

        function makeVisible()
        {
            // Make this element visible.
            treeElement.hidden = false;

            // Make the ancestors visible and expand them.
            var currentAncestor = treeElement.parent;
            while (currentAncestor && !currentAncestor.root) {
                currentAncestor.hidden = false;

                // Only expand if the built-in filters matched, not custom filters.
                if (matchedBuiltInFilters && !currentAncestor.expanded) {
                    currentAncestor.__wasExpandedDuringFiltering = true;
                    currentAncestor.expand();
                }

                currentAncestor = currentAncestor.parent;
            }
        }

        if (matchTextFilter(filterableData.text) && this.matchTreeElementAgainstFilterFunctions(treeElement) && this.matchTreeElementAgainstCustomFilters(treeElement)) {
            // Make this element visible since it matches.
            makeVisible();

            // If this tree element didn't match a built-in filter and was expanded earlier during filtering, collapse it again.
            if (!matchedBuiltInFilters && treeElement.expanded && treeElement.__wasExpandedDuringFiltering) {
                delete treeElement.__wasExpandedDuringFiltering;
                treeElement.collapse();
            }

            return;
        }

        // Make this element invisible since it does not match.
        treeElement.hidden = true;
    }

    show()
    {
        if (!this.parentSidebar)
            return;

        WebInspector.SidebarPanel.prototype.show.call(this);

        this.contentTreeOutlineElement.focus();
    }

    shown()
    {
        WebInspector.SidebarPanel.prototype.shown.call(this);

        this._updateContentOverflowShadowVisibility();

        // Force the navigation item to be visible. This makes sure it is
        // always visible when the panel is shown.
        this.toolbarItem.hidden = false;
    }

    hidden()
    {
        WebInspector.SidebarPanel.prototype.hidden.call(this);

        this._updateToolbarItemVisibility();
    }

    // Private
    
    _updateContentOverflowShadowVisibilitySoon()
    {
        if (this._updateContentOverflowShadowVisibilityIdentifier)
            return;

        this._updateContentOverflowShadowVisibilityIdentifier = setTimeout(this._updateContentOverflowShadowVisibility.bind(this), 0);
    }

    _updateContentOverflowShadowVisibility()
    {
        delete this._updateContentOverflowShadowVisibilityIdentifier;

        var scrollHeight = this.contentElement.scrollHeight;
        var offsetHeight = this.contentElement.offsetHeight;

        if (scrollHeight < offsetHeight) {
            if (this._topOverflowShadowElement)
                this._topOverflowShadowElement.style.opacity = 0;
            this._bottomOverflowShadowElement.style.opacity = 0;
            return;
        }

        if (WebInspector.Platform.isLegacyMacOS)
            var edgeThreshold = 10;
        else
            var edgeThreshold = 1;

        var scrollTop = this.contentElement.scrollTop;

        var topCoverage = Math.min(scrollTop, edgeThreshold);
        var bottomCoverage = Math.max(0, (offsetHeight + scrollTop) - (scrollHeight - edgeThreshold));

        if (this._topOverflowShadowElement)
            this._topOverflowShadowElement.style.opacity = (topCoverage / edgeThreshold).toFixed(1);
        this._bottomOverflowShadowElement.style.opacity = (1 - (bottomCoverage / edgeThreshold)).toFixed(1);
    }

    _updateToolbarItemVisibility()
    {
        // Hide the navigation item if requested or auto-hiding and we are not visible and we are empty.
        var shouldHide = ((this._hideToolbarItemWhenEmpty || this._autoHideToolbarItemWhenEmpty) && !this.selected && !this._contentTreeOutline.children.length);
        this.toolbarItem.hidden = shouldHide;
    }

    _checkForEmptyFilterResults()
    {
        // No tree elements, so don't touch the empty content placeholder.
        if (!this._contentTreeOutline.children.length)
            return;

        // Iterate over all the top level tree elements. If any are visible, return early.
        var currentTreeElement = this._contentTreeOutline.children[0];
        while (currentTreeElement) {
            if (!currentTreeElement.hidden) {
                // Not hidden, so hide any empty content message.
                this.hideEmptyContentPlaceholder();
                this._emptyFilterResults = false;
                return;
            }

            currentTreeElement = currentTreeElement.nextSibling;
        }

        // All top level tree elements are hidden, so filtering hid everything. Show a message.
        this.showEmptyContentPlaceholder(WebInspector.UIString("No Filter Results"));
        this._emptyFilterResults = true;
    }

    _filterDidChange()
    {
        this._updateFilter();
    }

    _updateFilter()
    {
        var selectedTreeElement = this._contentTreeOutline.selectedTreeElement;
        var selectionWasHidden = selectedTreeElement && selectedTreeElement.hidden;

        var filters = this._filterBar.filters;
        this._textFilterRegex = simpleGlobStringToRegExp(filters.text, "i");
        this._filtersSetting.value = filters;
        this._filterFunctions = filters.functions;

        // Don't populate if we don't have any active filters.
        // We only need to populate when a filter needs to reveal.
        var dontPopulate = !this._filterBar.hasActiveFilters();

        // Update the whole tree.
        var currentTreeElement = this._contentTreeOutline.children[0];
        while (currentTreeElement && !currentTreeElement.root) {
            this.applyFiltersToTreeElement(currentTreeElement);
            currentTreeElement = currentTreeElement.traverseNextTreeElement(false, null, dontPopulate);
        }

        this._checkForEmptyFilterResults();
        this._updateContentOverflowShadowVisibility();

        // Filter may have hidden the selected resource in the timeline view, which should now notify its listeners.
        if (selectedTreeElement && selectedTreeElement.hidden !== selectionWasHidden) {
            var currentContentView = WebInspector.contentBrowser.currentContentView;
            if (currentContentView instanceof WebInspector.TimelineRecordingContentView && typeof currentContentView.currentTimelineView.filterUpdated === "function")
                currentContentView.currentTimelineView.filterUpdated();
        }
    }

    _treeElementAddedOrChanged(treeElement)
    {
        // Don't populate if we don't have any active filters.
        // We only need to populate when a filter needs to reveal.
        var dontPopulate = !this._filterBar.hasActiveFilters();

        // Apply the filters to the tree element and its descendants.
        var currentTreeElement = treeElement;
        while (currentTreeElement && !currentTreeElement.root) {
            this.applyFiltersToTreeElement(currentTreeElement);
            currentTreeElement = currentTreeElement.traverseNextTreeElement(false, treeElement, dontPopulate);
        }

        this._checkForEmptyFilterResults();
        this._updateContentOverflowShadowVisibilitySoon();

        if (this.selected)
            this._checkElementsForPendingViewStateCookie(treeElement);
    }

    _treeElementExpandedOrCollapsed(treeElement)
    {
        this._updateContentOverflowShadowVisibility();
    }

    _generateStyleRulesIfNeeded()
    {
        if (WebInspector.NavigationSidebarPanel._styleElement)
            return;

        WebInspector.NavigationSidebarPanel._styleElement = document.createElement("style");

        var maximumSidebarTreeDepth = 32;
        var baseLeftPadding = 5; // Matches the padding in NavigationSidebarPanel.css for the item class. Keep in sync.
        var depthPadding = 10;

        var styleText = "";
        var childrenSubstring = "";
        for (var i = 1; i <= maximumSidebarTreeDepth; ++i) {
            // Keep all the elements at the same depth once the maximum is reached.
            childrenSubstring += i === maximumSidebarTreeDepth ? " .children" : " > .children";
            styleText += "." + WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName + childrenSubstring + " > .item { ";
            styleText += "padding-left: " + (baseLeftPadding + (depthPadding * i)) + "px; }\n";
        }

        WebInspector.NavigationSidebarPanel._styleElement.textContent = styleText;

        document.head.appendChild(WebInspector.NavigationSidebarPanel._styleElement);
    }

    _generateDisclosureTrianglesIfNeeded()
    {
        if (WebInspector.NavigationSidebarPanel._generatedDisclosureTriangles)
            return;

        // Set this early instead of in _generateDisclosureTriangle because we don't want multiple panels that are
        // created at the same time to duplicate the work (even though it would be harmless.)
        WebInspector.NavigationSidebarPanel._generatedDisclosureTriangles = true;

        var specifications = {};

        if (WebInspector.Platform.isLegacyMacOS) {
            specifications[WebInspector.NavigationSidebarPanel.DisclosureTriangleNormalCanvasIdentifierSuffix] = {
                fillColor: [112, 126, 139],
                shadowColor: [255, 255, 255, 0.8],
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowBlur: 0
            };

            specifications[WebInspector.NavigationSidebarPanel.DisclosureTriangleSelectedCanvasIdentifierSuffix] = {
                fillColor: [255, 255, 255],
                shadowColor: [61, 91, 110, 0.8],
                shadowOffsetX: 0,
                shadowOffsetY: 1,
                shadowBlur: 2
            };
        } else {
            specifications[WebInspector.NavigationSidebarPanel.DisclosureTriangleNormalCanvasIdentifierSuffix] = {
                fillColor: [140, 140, 140]
            };

            specifications[WebInspector.NavigationSidebarPanel.DisclosureTriangleSelectedCanvasIdentifierSuffix] = {
                fillColor: [255, 255, 255]
            };
        }

        generateColoredImagesForCSS("Images/DisclosureTriangleSmallOpen.svg", specifications, 13, 13, WebInspector.NavigationSidebarPanel.DisclosureTriangleOpenCanvasIdentifier);
        generateColoredImagesForCSS("Images/DisclosureTriangleSmallClosed.svg", specifications, 13, 13, WebInspector.NavigationSidebarPanel.DisclosureTriangleClosedCanvasIdentifier);
    }

    _checkForOldResources(event)
    {
        if (this._checkForOldResourcesTimeoutIdentifier)
            return;

        function delayedWork()
        {
            delete this._checkForOldResourcesTimeoutIdentifier;

            var contentTreeOutline = this.contentTreeOutlineToAutoPrune;

            // Check all the ResourceTreeElements at the top level to make sure their Resource still has a parentFrame in the frame hierarchy.
            // If the parentFrame is no longer in the frame hierarchy we know it was removed due to a navigation or some other page change and
            // we should remove the issues for that resource.
            for (var i = contentTreeOutline.children.length - 1; i >= 0; --i) {
                var treeElement = contentTreeOutline.children[i];
                if (!(treeElement instanceof WebInspector.ResourceTreeElement))
                    continue;

                var resource = treeElement.resource;
                if (!resource.parentFrame || resource.parentFrame.isDetached())
                    contentTreeOutline.removeChildAtIndex(i, true, true);
            }

            if (typeof this._updateEmptyContentPlaceholder === "function")
                this._updateEmptyContentPlaceholder();
        }

        // Check on a delay to coalesce multiple calls to _checkForOldResources.
        this._checkForOldResourcesTimeoutIdentifier = setTimeout(delayedWork.bind(this), 0);
    }

    _isTreeElementWithoutRepresentedObject(treeElement)
    {
        return treeElement instanceof WebInspector.FolderTreeElement
            || treeElement instanceof WebInspector.DatabaseHostTreeElement
            || typeof treeElement.representedObject === "string"
            || treeElement.representedObject instanceof String;
    }

    _checkOutlinesForPendingViewStateCookie(matchTypeOnly)
    {
        if (!this._pendingViewStateCookie)
            return;

        var visibleTreeElements = [];
        this._visibleContentTreeOutlines.forEach(function(outline) {
            var currentTreeElement = outline.hasChildren ? outline.children[0] : null;
            while (currentTreeElement) {
                visibleTreeElements.push(currentTreeElement);
                currentTreeElement = currentTreeElement.traverseNextTreeElement(false, null, false);
            }
        });

        return this._checkElementsForPendingViewStateCookie(visibleTreeElements, matchTypeOnly);
    }

    _checkElementsForPendingViewStateCookie(treeElements, matchTypeOnly)
    {
        if (!this._pendingViewStateCookie)
            return;

        var cookie = this._pendingViewStateCookie;

        function treeElementMatchesCookie(treeElement)
        {
            if (this._isTreeElementWithoutRepresentedObject(treeElement))
                return false;

            var representedObject = treeElement.representedObject;
            if (!representedObject)
                return false;

            var typeIdentifier = cookie[WebInspector.TypeIdentifierCookieKey];
            if (typeIdentifier !== representedObject.constructor.TypeIdentifier)
                return false;

            if (matchTypeOnly)
                return true;

            var candidateObjectCookie = {};
            if (representedObject.saveIdentityToCookie)
                representedObject.saveIdentityToCookie(candidateObjectCookie);

            var candidateCookieKeys = Object.keys(candidateObjectCookie);
            return candidateCookieKeys.length && candidateCookieKeys.every(function valuesMatchForKey(key) {
                return candidateObjectCookie[key] === cookie[key];
            });
        }

        if (!(treeElements instanceof Array))
            treeElements = [treeElements];

        var matchedElement = null;
        treeElements.some(function(element) {
            if (treeElementMatchesCookie.call(this, element)) {
                matchedElement = element;
                return true;
            }
        }, this);

        if (matchedElement) {
            matchedElement.revealAndSelect();

            delete this._pendingViewStateCookie;

            // Delay clearing the restoringState flag until the next runloop so listeners
            // checking for it in this runloop still know state was being restored.
            setTimeout(function() {
                delete this._restoringState;
            }.bind(this));

            if (this._finalAttemptToRestoreViewStateTimeout) {
                clearTimeout(this._finalAttemptToRestoreViewStateTimeout);
                delete this._finalAttemptToRestoreViewStateTimeout;
            }
        }
    }
};

WebInspector.NavigationSidebarPanel.OverflowShadowElementStyleClassName = "overflow-shadow";
WebInspector.NavigationSidebarPanel.TopOverflowShadowElementStyleClassName = "top";
WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementHiddenStyleClassName = "hidden";
WebInspector.NavigationSidebarPanel.ContentTreeOutlineElementStyleClassName = "navigation-sidebar-panel-content-tree-outline";
WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName = "hide-disclosure-buttons";
WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderElementStyleClassName = "empty-content-placeholder";
WebInspector.NavigationSidebarPanel.EmptyContentPlaceholderMessageElementStyleClassName = "message";
WebInspector.NavigationSidebarPanel.DisclosureTriangleOpenCanvasIdentifier = "navigation-sidebar-panel-disclosure-triangle-open";
WebInspector.NavigationSidebarPanel.DisclosureTriangleClosedCanvasIdentifier = "navigation-sidebar-panel-disclosure-triangle-closed";
WebInspector.NavigationSidebarPanel.DisclosureTriangleNormalCanvasIdentifierSuffix = "-normal";
WebInspector.NavigationSidebarPanel.DisclosureTriangleSelectedCanvasIdentifierSuffix = "-selected";
