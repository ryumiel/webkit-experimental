/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008, 2009, 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "DocumentStyleSheetCollection.h"

#include "CSSStyleSheet.h"
#include "Element.h"
#include "HTMLIFrameElement.h"
#include "HTMLLinkElement.h"
#include "HTMLStyleElement.h"
#include "Page.h"
#include "PageGroup.h"
#include "ProcessingInstruction.h"
#include "SVGNames.h"
#include "SVGStyleElement.h"
#include "Settings.h"
#include "StyleInvalidationAnalysis.h"
#include "StyleResolver.h"
#include "StyleSheetContents.h"
#include "StyleSheetList.h"
#include "UserContentController.h"
#include "UserContentURLPattern.h"
#include "UserStyleSheet.h"

namespace WebCore {

using namespace HTMLNames;

DocumentStyleSheetCollection::DocumentStyleSheetCollection(Document& document)
    : m_document(document)
    , m_pendingStylesheets(0)
    , m_injectedStyleSheetCacheValid(false)
    , m_hadActiveLoadingStylesheet(false)
    , m_pendingUpdateType(NoUpdate)
    , m_usesFirstLineRules(false)
    , m_usesFirstLetterRules(false)
    , m_usesRemUnits(false)
    , m_usesStyleBasedEditability(false)
{
}

void DocumentStyleSheetCollection::combineCSSFeatureFlags()
{
    // Delay resetting the flags until after next style recalc since unapplying the style may not work without these set (this is true at least with before/after).
    const StyleResolver& styleResolver = m_document.ensureStyleResolver();
    m_usesFirstLineRules = m_usesFirstLineRules || styleResolver.usesFirstLineRules();
    m_usesFirstLetterRules = m_usesFirstLetterRules || styleResolver.usesFirstLetterRules();
}

void DocumentStyleSheetCollection::resetCSSFeatureFlags()
{
    const StyleResolver& styleResolver = m_document.ensureStyleResolver();
    m_usesFirstLineRules = styleResolver.usesFirstLineRules();
    m_usesFirstLetterRules = styleResolver.usesFirstLetterRules();
}

CSSStyleSheet* DocumentStyleSheetCollection::pageUserSheet()
{
    if (m_pageUserSheet)
        return m_pageUserSheet.get();
    
    Page* owningPage = m_document.page();
    if (!owningPage)
        return 0;
    
    String userSheetText = owningPage->userStyleSheet();
    if (userSheetText.isEmpty())
        return 0;
    
    // Parse the sheet and cache it.
    m_pageUserSheet = CSSStyleSheet::createInline(m_document, m_document.settings()->userStyleSheetLocation());
    m_pageUserSheet->contents().setIsUserStyleSheet(true);
    m_pageUserSheet->contents().parseString(userSheetText);
    return m_pageUserSheet.get();
}

void DocumentStyleSheetCollection::clearPageUserSheet()
{
    if (m_pageUserSheet) {
        m_pageUserSheet = 0;
        m_document.styleResolverChanged(DeferRecalcStyle);
    }
}

void DocumentStyleSheetCollection::updatePageUserSheet()
{
    clearPageUserSheet();
    if (pageUserSheet())
        m_document.styleResolverChanged(RecalcStyleImmediately);
}

const Vector<RefPtr<CSSStyleSheet>>& DocumentStyleSheetCollection::injectedUserStyleSheets() const
{
    updateInjectedStyleSheetCache();
    return m_injectedUserStyleSheets;
}

const Vector<RefPtr<CSSStyleSheet>>& DocumentStyleSheetCollection::injectedAuthorStyleSheets() const
{
    updateInjectedStyleSheetCache();
    return m_injectedAuthorStyleSheets;
}

void DocumentStyleSheetCollection::updateInjectedStyleSheetCache() const
{
    if (m_injectedStyleSheetCacheValid)
        return;
    m_injectedStyleSheetCacheValid = true;
    m_injectedUserStyleSheets.clear();
    m_injectedAuthorStyleSheets.clear();

    Page* owningPage = m_document.page();
    if (!owningPage)
        return;

    const auto* userContentController = owningPage->userContentController();
    if (!userContentController)
        return;

    const UserStyleSheetMap* userStyleSheets = userContentController->userStyleSheets();
    if (!userStyleSheets)
        return;

    for (auto& styleSheets : userStyleSheets->values()) {
        for (const auto& sheet : *styleSheets) {
            if (sheet->injectedFrames() == InjectInTopFrameOnly && m_document.ownerElement())
                continue;

            if (!UserContentURLPattern::matchesPatterns(m_document.url(), sheet->whitelist(), sheet->blacklist()))
                continue;

            RefPtr<CSSStyleSheet> groupSheet = CSSStyleSheet::createInline(const_cast<Document&>(m_document), sheet->url());
            bool isUserStyleSheet = sheet->level() == UserStyleUserLevel;
            if (isUserStyleSheet)
                m_injectedUserStyleSheets.append(groupSheet);
            else
                m_injectedAuthorStyleSheets.append(groupSheet);

            groupSheet->contents().setIsUserStyleSheet(isUserStyleSheet);
            groupSheet->contents().parseString(sheet->source());
        }
    }
}

void DocumentStyleSheetCollection::invalidateInjectedStyleSheetCache()
{
    if (!m_injectedStyleSheetCacheValid)
        return;
    m_injectedStyleSheetCacheValid = false;
    if (m_injectedUserStyleSheets.isEmpty() && m_injectedAuthorStyleSheets.isEmpty())
        return;
    m_document.styleResolverChanged(DeferRecalcStyle);
}

void DocumentStyleSheetCollection::addAuthorSheet(Ref<StyleSheetContents>&& authorSheet)
{
    ASSERT(!authorSheet.get().isUserStyleSheet());
    m_authorStyleSheets.append(CSSStyleSheet::create(WTF::move(authorSheet), &m_document));
    m_document.styleResolverChanged(RecalcStyleImmediately);
}

void DocumentStyleSheetCollection::addUserSheet(Ref<StyleSheetContents>&& userSheet)
{
    ASSERT(userSheet.get().isUserStyleSheet());
    m_userStyleSheets.append(CSSStyleSheet::create(WTF::move(userSheet), &m_document));
    m_document.styleResolverChanged(RecalcStyleImmediately);
}

void DocumentStyleSheetCollection::maybeAddContentExtensionSheet(const String& identifier, StyleSheetContents& sheet)
{
    ASSERT(sheet.isUserStyleSheet());

    if (m_contentExtensionSheets.contains(identifier))
        return;

    Ref<CSSStyleSheet> cssSheet = CSSStyleSheet::create(sheet, &m_document);
    m_contentExtensionSheets.set(identifier, &cssSheet.get());
    m_userStyleSheets.append(adoptRef(cssSheet.leakRef()));
}

// This method is called whenever a top-level stylesheet has finished loading.
void DocumentStyleSheetCollection::removePendingSheet(RemovePendingSheetNotificationType notification)
{
    // Make sure we knew this sheet was pending, and that our count isn't out of sync.
    ASSERT(m_pendingStylesheets > 0);

    m_pendingStylesheets--;
    
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Stylesheet loaded at time %d. %d stylesheets still remain.\n", elapsedTime(), m_pendingStylesheets);
#endif

    if (m_pendingStylesheets)
        return;

    if (notification == RemovePendingSheetNotifyLater) {
        m_document.setNeedsNotifyRemoveAllPendingStylesheet();
        return;
    }
    
    m_document.didRemoveAllPendingStylesheet();
}

void DocumentStyleSheetCollection::addStyleSheetCandidateNode(Node& node, bool createdByParser)
{
    if (!node.inDocument())
        return;
    
    // Until the <body> exists, we have no choice but to compare document positions,
    // since styles outside of the body and head continue to be shunted into the head
    // (and thus can shift to end up before dynamically added DOM content that is also
    // outside the body).
    if ((createdByParser && m_document.bodyOrFrameset()) || m_styleSheetCandidateNodes.isEmpty()) {
        m_styleSheetCandidateNodes.add(&node);
        return;
    }

    // Determine an appropriate insertion point.
    StyleSheetCandidateListHashSet::iterator begin = m_styleSheetCandidateNodes.begin();
    StyleSheetCandidateListHashSet::iterator end = m_styleSheetCandidateNodes.end();
    StyleSheetCandidateListHashSet::iterator it = end;
    Node* followingNode = 0;
    do {
        --it;
        Node* n = *it;
        unsigned short position = n->compareDocumentPosition(&node);
        if (position == Node::DOCUMENT_POSITION_FOLLOWING) {
            m_styleSheetCandidateNodes.insertBefore(followingNode, &node);
            return;
        }
        followingNode = n;
    } while (it != begin);
    
    m_styleSheetCandidateNodes.insertBefore(followingNode, &node);
}

void DocumentStyleSheetCollection::removeStyleSheetCandidateNode(Node& node)
{
    m_styleSheetCandidateNodes.remove(&node);
}

void DocumentStyleSheetCollection::collectActiveStyleSheets(Vector<RefPtr<StyleSheet>>& sheets)
{
    if (m_document.settings() && !m_document.settings()->authorAndUserStylesEnabled())
        return;

    for (auto& node : m_styleSheetCandidateNodes) {
        StyleSheet* sheet = nullptr;
        if (is<ProcessingInstruction>(*node)) {
            // Processing instruction (XML documents only).
            // We don't support linking to embedded CSS stylesheets, see <https://bugs.webkit.org/show_bug.cgi?id=49281> for discussion.
            ProcessingInstruction& pi = downcast<ProcessingInstruction>(*node);
            sheet = pi.sheet();
#if ENABLE(XSLT)
            // Don't apply XSL transforms to already transformed documents -- <rdar://problem/4132806>
            if (pi.isXSL() && !m_document.transformSourceDocument()) {
                // Don't apply XSL transforms until loading is finished.
                if (!m_document.parsing())
                    m_document.applyXSLTransform(&pi);
                return;
            }
#endif
        } else if (is<HTMLLinkElement>(*node) || is<HTMLStyleElement>(*node) || is<SVGStyleElement>(*node)) {
            Element& element = downcast<Element>(*node);
            AtomicString title = element.fastGetAttribute(titleAttr);
            bool enabledViaScript = false;
            if (is<HTMLLinkElement>(element)) {
                // <LINK> element
                HTMLLinkElement& linkElement = downcast<HTMLLinkElement>(element);
                if (linkElement.isDisabled())
                    continue;
                enabledViaScript = linkElement.isEnabledViaScript();
                if (linkElement.styleSheetIsLoading()) {
                    // it is loading but we should still decide which style sheet set to use
                    if (!enabledViaScript && !title.isEmpty() && m_preferredStylesheetSetName.isEmpty()) {
                        if (!linkElement.fastGetAttribute(relAttr).contains("alternate")) {
                            m_preferredStylesheetSetName = title;
                            m_selectedStylesheetSetName = title;
                        }
                    }
                    continue;
                }
                if (!linkElement.sheet())
                    title = nullAtom;
            }
            // Get the current preferred styleset. This is the
            // set of sheets that will be enabled.
            if (is<SVGStyleElement>(element))
                sheet = downcast<SVGStyleElement>(element).sheet();
            else if (is<HTMLLinkElement>(element))
                sheet = downcast<HTMLLinkElement>(element).sheet();
            else
                sheet = downcast<HTMLStyleElement>(element).sheet();
            // Check to see if this sheet belongs to a styleset
            // (thus making it PREFERRED or ALTERNATE rather than
            // PERSISTENT).
            auto& rel = element.fastGetAttribute(relAttr);
            if (!enabledViaScript && !title.isEmpty()) {
                // Yes, we have a title.
                if (m_preferredStylesheetSetName.isEmpty()) {
                    // No preferred set has been established. If
                    // we are NOT an alternate sheet, then establish
                    // us as the preferred set. Otherwise, just ignore
                    // this sheet.
                    if (is<HTMLStyleElement>(element) || !rel.contains("alternate"))
                        m_preferredStylesheetSetName = m_selectedStylesheetSetName = title;
                }
                if (title != m_preferredStylesheetSetName)
                    sheet = nullptr;
            }

            if (rel.contains("alternate") && title.isEmpty())
                sheet = nullptr;
        }
        if (sheet)
            sheets.append(sheet);
    }
}

void DocumentStyleSheetCollection::analyzeStyleSheetChange(UpdateFlag updateFlag, const Vector<RefPtr<CSSStyleSheet>>& newStylesheets, StyleResolverUpdateType& styleResolverUpdateType, bool& requiresFullStyleRecalc)
{
    styleResolverUpdateType = Reconstruct;
    requiresFullStyleRecalc = true;
    
    // Stylesheets of <style> elements that @import stylesheets are active but loading. We need to trigger a full recalc when such loads are done.
    bool hasActiveLoadingStylesheet = false;
    unsigned newStylesheetCount = newStylesheets.size();
    for (unsigned i = 0; i < newStylesheetCount; ++i) {
        if (newStylesheets[i]->isLoading())
            hasActiveLoadingStylesheet = true;
    }
    if (m_hadActiveLoadingStylesheet && !hasActiveLoadingStylesheet) {
        m_hadActiveLoadingStylesheet = false;
        return;
    }
    m_hadActiveLoadingStylesheet = hasActiveLoadingStylesheet;

    if (updateFlag != OptimizedUpdate)
        return;
    if (!m_document.styleResolverIfExists())
        return;
    StyleResolver& styleResolver = *m_document.styleResolverIfExists();

    // Find out which stylesheets are new.
    unsigned oldStylesheetCount = m_activeAuthorStyleSheets.size();
    if (newStylesheetCount < oldStylesheetCount)
        return;
    Vector<StyleSheetContents*> addedSheets;
    unsigned newIndex = 0;
    for (unsigned oldIndex = 0; oldIndex < oldStylesheetCount; ++oldIndex) {
        if (newIndex >= newStylesheetCount)
            return;
        while (m_activeAuthorStyleSheets[oldIndex] != newStylesheets[newIndex]) {
            addedSheets.append(&newStylesheets[newIndex]->contents());
            ++newIndex;
            if (newIndex == newStylesheetCount)
                return;
        }
        ++newIndex;
    }
    bool hasInsertions = !addedSheets.isEmpty();
    while (newIndex < newStylesheetCount) {
        addedSheets.append(&newStylesheets[newIndex]->contents());
        ++newIndex;
    }
    // If all new sheets were added at the end of the list we can just add them to existing StyleResolver.
    // If there were insertions we need to re-add all the stylesheets so rules are ordered correctly.
    styleResolverUpdateType = hasInsertions ? Reset : Additive;

    // If we are already parsing the body and so may have significant amount of elements, put some effort into trying to avoid style recalcs.
    if (!m_document.bodyOrFrameset() || m_document.hasNodesWithPlaceholderStyle())
        return;
    StyleInvalidationAnalysis invalidationAnalysis(addedSheets, styleResolver.mediaQueryEvaluator());
    if (invalidationAnalysis.dirtiesAllStyle())
        return;
    invalidationAnalysis.invalidateStyle(m_document);
    requiresFullStyleRecalc = false;
}

static void filterEnabledNonemptyCSSStyleSheets(Vector<RefPtr<CSSStyleSheet>>& result, const Vector<RefPtr<StyleSheet>>& sheets)
{
    for (unsigned i = 0; i < sheets.size(); ++i) {
        if (!is<CSSStyleSheet>(*sheets[i]))
            continue;
        CSSStyleSheet& sheet = downcast<CSSStyleSheet>(*sheets[i]);
        if (sheet.disabled())
            continue;
        if (!sheet.length())
            continue;
        result.append(&sheet);
    }
}

bool DocumentStyleSheetCollection::updateActiveStyleSheets(UpdateFlag updateFlag)
{
    if (m_document.inStyleRecalc()) {
        // SVG <use> element may manage to invalidate style selector in the middle of a style recalc.
        // https://bugs.webkit.org/show_bug.cgi?id=54344
        // FIXME: This should be fixed in SVG and the call site replaced by ASSERT(!m_inStyleRecalc).
        m_pendingUpdateType = FullUpdate;
        m_document.scheduleForcedStyleRecalc();
        return false;

    }
    if (!m_document.hasLivingRenderTree())
        return false;

    Vector<RefPtr<StyleSheet>> activeStyleSheets;
    collectActiveStyleSheets(activeStyleSheets);

    Vector<RefPtr<CSSStyleSheet>> activeCSSStyleSheets;
    activeCSSStyleSheets.appendVector(injectedAuthorStyleSheets());
    activeCSSStyleSheets.appendVector(documentAuthorStyleSheets());
    filterEnabledNonemptyCSSStyleSheets(activeCSSStyleSheets, activeStyleSheets);

    StyleResolverUpdateType styleResolverUpdateType;
    bool requiresFullStyleRecalc;
    analyzeStyleSheetChange(updateFlag, activeCSSStyleSheets, styleResolverUpdateType, requiresFullStyleRecalc);

    if (styleResolverUpdateType == Reconstruct)
        m_document.clearStyleResolver();
    else {
        StyleResolver& styleResolver = m_document.ensureStyleResolver();
        if (styleResolverUpdateType == Reset) {
            styleResolver.ruleSets().resetAuthorStyle();
            styleResolver.appendAuthorStyleSheets(0, activeCSSStyleSheets);
        } else {
            ASSERT(styleResolverUpdateType == Additive);
            styleResolver.appendAuthorStyleSheets(m_activeAuthorStyleSheets.size(), activeCSSStyleSheets);
        }
        resetCSSFeatureFlags();
    }

    m_weakCopyOfActiveStyleSheetListForFastLookup = nullptr;
    m_activeAuthorStyleSheets.swap(activeCSSStyleSheets);
    m_styleSheetsForStyleSheetList.swap(activeStyleSheets);

    for (const auto& sheet : m_activeAuthorStyleSheets) {
        if (sheet->contents().usesRemUnits())
            m_usesRemUnits = true;
        if (sheet->contents().usesStyleBasedEditability())
            m_usesStyleBasedEditability = true;
    }
    m_pendingUpdateType = NoUpdate;

    return requiresFullStyleRecalc;
}

bool DocumentStyleSheetCollection::activeStyleSheetsContains(const CSSStyleSheet* sheet) const
{
    if (!m_weakCopyOfActiveStyleSheetListForFastLookup) {
        m_weakCopyOfActiveStyleSheetListForFastLookup = std::make_unique<HashSet<const CSSStyleSheet*>>();
        for (unsigned i = 0; i < m_activeAuthorStyleSheets.size(); ++i)
            m_weakCopyOfActiveStyleSheetListForFastLookup->add(m_activeAuthorStyleSheets[i].get());
    }
    return m_weakCopyOfActiveStyleSheetListForFastLookup->contains(sheet);
}

void DocumentStyleSheetCollection::detachFromDocument()
{
    if (m_pageUserSheet)
        m_pageUserSheet->detachFromDocument();
    for (unsigned i = 0; i < m_injectedUserStyleSheets.size(); ++i)
        m_injectedUserStyleSheets[i]->detachFromDocument();
    for (unsigned i = 0; i < m_injectedAuthorStyleSheets.size(); ++i)
        m_injectedAuthorStyleSheets[i]->detachFromDocument();
    for (unsigned i = 0; i < m_userStyleSheets.size(); ++i)
        m_userStyleSheets[i]->detachFromDocument();
    for (unsigned i = 0; i < m_authorStyleSheets.size(); ++i)
        m_authorStyleSheets[i]->detachFromDocument();
}

}
