/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

WebInspector.DOMStorageContentView = function(representedObject)
{
    WebInspector.ContentView.call(this, representedObject);

    this.element.classList.add(WebInspector.DOMStorageContentView.StyleClassName);

    representedObject.addEventListener(WebInspector.DOMStorageObject.Event.ItemsCleared, this.itemsCleared, this);
    representedObject.addEventListener(WebInspector.DOMStorageObject.Event.ItemAdded, this.itemAdded, this);
    representedObject.addEventListener(WebInspector.DOMStorageObject.Event.ItemRemoved, this.itemRemoved, this);
    representedObject.addEventListener(WebInspector.DOMStorageObject.Event.ItemUpdated, this.itemUpdated, this);

    this.reset();
};

WebInspector.DOMStorageContentView.StyleClassName = "dom-storage";
WebInspector.DOMStorageContentView.DuplicateKeyStyleClassName = "duplicate-key";
WebInspector.DOMStorageContentView.MissingKeyStyleClassName = "missing-key";
WebInspector.DOMStorageContentView.MissingValueStyleClassName = "missing-value";


WebInspector.DOMStorageContentView.prototype = {
    constructor: WebInspector.DOMStorageContentView,
    __proto__: WebInspector.ContentView.prototype,

    // Public

    get allowedNavigationSidebarPanels()
    {
        return [WebInspector.resourceSidebarPanel.identifier];
    },

    reset: function()
    {
        this.representedObject.getEntries(function(error, entries) {
            if (error)
                return;

            if (!this._dataGrid) {
                var columns = {};
                columns.key = {title: WebInspector.UIString("Key"), sortable: true};
                columns.value = {title: WebInspector.UIString("Value"), sortable: true};

                this._dataGrid = new WebInspector.DataGrid(columns, this._editingCallback.bind(this), this._deleteCallback.bind(this));
                this._dataGrid.addEventListener(WebInspector.DataGrid.Event.SortChanged, this._sortDataGrid, this);

                this.element.appendChild(this._dataGrid.element);
            }

            console.assert(this._dataGrid);

            var nodes = [];
            for (var entry of entries) {
                if (!entry[0] || !entry[1])
                    continue;
                var data = {key: entry[0], value: entry[1]};
                var node = new WebInspector.DataGridNode(data, false);
                this._dataGrid.appendChild(node);
            }

            this._sortDataGrid();
            this._dataGrid.addPlaceholderNode();
            this._dataGrid.updateLayout();
        }.bind(this));
    },

    saveToCookie: function(cookie)
    {
        cookie.type = WebInspector.ContentViewCookieType.DOMStorage;
        cookie.isLocalStorage = this.representedObject.isLocalStorage();
        cookie.host = this.representedObject.host;
    },

    itemsCleared: function(event)
    {
        this._dataGrid.removeChildren();
        this._dataGrid.addPlaceholderNode();
    },

    itemRemoved: function(event)
    {
        for (var node of this._dataGrid.children) {
            if (node.data.key === event.data.key)
                return this._dataGrid.removeChild(node);
        }
    },

    itemAdded: function(event)
    {
        var key = event.data.key;
        var value = event.data.value;

        // Enforce key uniqueness.
        for (var node of this._dataGrid.children) {
            if (node.data.key === key)
                return;
        }

        var data = {key, value};
        this._dataGrid.appendChild(new WebInspector.DataGridNode(data, false));
        this._sortDataGrid();
    },

    itemUpdated: function(event)
    {
        var key = event.data.key;
        var value = event.data.value;

        var keyFound = false;
        for (var childNode of this._dataGrid.children) {
            if (childNode.data.key === key) {
                // Remove any rows that are now duplicates.
                if (keyFound) {
                    this._dataGrid.removeChild(childNode);
                    continue;
                }

                keyFound = true;
                childNode.data.value = value;
                childNode.refresh();
            }
        }
        this._sortDataGrid();
    },

    updateLayout: function()
    {
        if (this._dataGrid)
            this._dataGrid.updateLayout();
    },

    get scrollableElements()
    {
        if (!this._dataGrid)
            return [];
        return [this._dataGrid.scrollContainer];
    },

    // Private

    _sortDataGrid: function()
    {
        if (!this._dataGrid.sortOrder)
            return;

        var sortColumnIdentifier = this._dataGrid.sortColumnIdentifier || "key";

        function comparator(a, b)
        {
            return b.data[sortColumnIdentifier].localeCompare(a.data[sortColumnIdentifier]);
        }

        this._dataGrid.sortNodes(comparator, this._dataGrid.sortOrder);
    },

    _deleteCallback: function(node)
    {
        if (!node || node.isPlaceholderNode)
            return;

        this._dataGrid.removeChild(node);
        this.representedObject.removeItem(node.data["key"]);
    },

    _editingCallback: function(editingNode, columnIdentifier, oldText, newText, moveDirection)
    {
        var key = editingNode.data["key"].trim().removeWordBreakCharacters();
        var value = editingNode.data["value"].trim().removeWordBreakCharacters();
        var previousValue = oldText.trim().removeWordBreakCharacters();
        var enteredValue = newText.trim().removeWordBreakCharacters();
        var hasUncommittedEdits = editingNode.__hasUncommittedEdits;
        var hasChange = previousValue !== enteredValue;
        var isEditingKey = columnIdentifier === "key";
        var isEditingValue = !isEditingKey;
        var domStorage = this.representedObject;

        // Nothing changed, just bail.
        if (!hasChange && !hasUncommittedEdits)
            return;

        // Something changed, save the original key/value and enter uncommitted state.
        if (hasChange && !editingNode.__hasUncommittedEdits) {
            editingNode.__hasUncommittedEdits = true;
            editingNode.__originalKey = isEditingKey ? previousValue : key;
            editingNode.__originalValue = isEditingValue ? previousValue : value;
        }

        function cleanup()
        {
            editingNode.element.classList.remove(WebInspector.DOMStorageContentView.MissingKeyStyleClassName);
            editingNode.element.classList.remove(WebInspector.DOMStorageContentView.MissingValueStyleClassName);
            editingNode.element.classList.remove(WebInspector.DOMStorageContentView.DuplicateKeyStyleClassName);
            delete editingNode.__hasUncommittedEdits;
            delete editingNode.__originalKey;
            delete editingNode.__originalValue;
        }

        function restoreOriginalValues()
        {
            editingNode.data.key = editingNode.__originalKey;
            editingNode.data.value = editingNode.__originalValue;
            editingNode.refresh();
            cleanup();
        }

        // If the key/value field was cleared, add "missing" style.
        if (isEditingKey) {
            if (key.length)
                editingNode.element.classList.remove(WebInspector.DOMStorageContentView.MissingKeyStyleClassName);
            else
                editingNode.element.classList.add(WebInspector.DOMStorageContentView.MissingKeyStyleClassName);
        } else if (isEditingValue) {
            if (value.length)
                editingNode.element.classList.remove(WebInspector.DOMStorageContentView.MissingValueStyleClassName);
            else
                editingNode.element.classList.add(WebInspector.DOMStorageContentView.MissingValueStyleClassName);
        }

        // Check for key duplicates. If this is a new row, or an existing row that changed key.
        var keyChanged = key !== editingNode.__originalKey;
        if (keyChanged) {
            if (domStorage.entries.has(key))
                editingNode.element.classList.add(WebInspector.DOMStorageContentView.DuplicateKeyStyleClassName);
            else
                editingNode.element.classList.remove(WebInspector.DOMStorageContentView.DuplicateKeyStyleClassName);
        }

        // See if we are done editing this row or not.
        var columnIndex = this._dataGrid.orderedColumns.indexOf(columnIdentifier);
        var mayMoveToNextRow = moveDirection === "forward" && columnIndex === this._dataGrid.orderedColumns.length - 1;
        var mayMoveToPreviousRow = moveDirection === "backward" && columnIndex === 0;
        var doneEditing = mayMoveToNextRow || mayMoveToPreviousRow || !moveDirection;

        // Expecting more edits on this row.
        if (!doneEditing)
            return;

        // Key and value were cleared, remove the row.
        if (!key.length && !value.length && !editingNode.isPlaceholderNode) {
            this._dataGrid.removeChild(editingNode);
            domStorage.removeItem(editingNode.__originalKey);
            return;                
        }

        // Done editing but leaving the row in an invalid state. Leave in uncommitted state.
        var isDuplicate = editingNode.element.classList.contains(WebInspector.DOMStorageContentView.DuplicateKeyStyleClassName);
        if (!key.length || !value.length || isDuplicate)
            return;

        // Commit.
        if (keyChanged && !editingNode.isPlaceholderNode)
            domStorage.removeItem(editingNode.__originalKey);
        if (editingNode.isPlaceholderNode)
            this._dataGrid.addPlaceholderNode();
        cleanup();
        domStorage.setItem(key, value);
    }
};
