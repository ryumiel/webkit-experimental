/*
 * Copyright (C) 2006, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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
 *
*/

#include "config.h"
#include "HitTestResult.h"

#include "CachedImage.h"
#include "DocumentMarkerController.h"
#include "Editor.h"
#include "File.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "HTMLAnchorElement.h"
#include "HTMLAreaElement.h"
#include "HTMLAttachmentElement.h"
#include "HTMLAudioElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLPlugInImageElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLVideoElement.h"
#include "HitTestLocation.h"
#include "PseudoElement.h"
#include "RenderBlockFlow.h"
#include "RenderImage.h"
#include "RenderInline.h"
#include "SVGAElement.h"
#include "SVGImageElement.h"
#include "SVGNames.h"
#include "Scrollbar.h"
#include "ShadowRoot.h"
#include "UserGestureIndicator.h"
#include "VisibleUnits.h"
#include "XLinkNames.h"

namespace WebCore {

using namespace HTMLNames;

HitTestResult::HitTestResult()
    : m_isOverWidget(false)
{
}

HitTestResult::HitTestResult(const LayoutPoint& point)
    : m_hitTestLocation(point)
    , m_pointInInnerNodeFrame(point)
    , m_isOverWidget(false)
{
}

HitTestResult::HitTestResult(const LayoutPoint& centerPoint, unsigned topPadding, unsigned rightPadding, unsigned bottomPadding, unsigned leftPadding)
    : m_hitTestLocation(centerPoint, topPadding, rightPadding, bottomPadding, leftPadding)
    , m_pointInInnerNodeFrame(centerPoint)
    , m_isOverWidget(false)
{
}

HitTestResult::HitTestResult(const HitTestLocation& other)
    : m_hitTestLocation(other)
    , m_pointInInnerNodeFrame(m_hitTestLocation.point())
    , m_isOverWidget(false)
{
}

HitTestResult::HitTestResult(const HitTestResult& other)
    : m_hitTestLocation(other.m_hitTestLocation)
    , m_innerNode(other.innerNode())
    , m_innerNonSharedNode(other.innerNonSharedNode())
    , m_pointInInnerNodeFrame(other.m_pointInInnerNodeFrame)
    , m_localPoint(other.localPoint())
    , m_innerURLElement(other.URLElement())
    , m_scrollbar(other.scrollbar())
    , m_isOverWidget(other.isOverWidget())
{
    // Only copy the NodeSet in case of rect hit test.
    m_rectBasedTestResult = other.m_rectBasedTestResult ? std::make_unique<NodeSet>(*other.m_rectBasedTestResult) : nullptr;
}

HitTestResult::~HitTestResult()
{
}

HitTestResult& HitTestResult::operator=(const HitTestResult& other)
{
    m_hitTestLocation = other.m_hitTestLocation;
    m_innerNode = other.innerNode();
    m_innerNonSharedNode = other.innerNonSharedNode();
    m_pointInInnerNodeFrame = other.m_pointInInnerNodeFrame;
    m_localPoint = other.localPoint();
    m_innerURLElement = other.URLElement();
    m_scrollbar = other.scrollbar();
    m_isOverWidget = other.isOverWidget();

    // Only copy the NodeSet in case of rect hit test.
    m_rectBasedTestResult = other.m_rectBasedTestResult ? std::make_unique<NodeSet>(*other.m_rectBasedTestResult) : nullptr;

    return *this;
}

void HitTestResult::setToNonShadowAncestor()
{
    Node* node = innerNode();
    if (node)
        node = node->document().ancestorInThisScope(node);
    setInnerNode(node);
    node = innerNonSharedNode();
    if (node)
        node = node->document().ancestorInThisScope(node);
    setInnerNonSharedNode(node);
}

void HitTestResult::setInnerNode(Node* node)
{
    if (is<PseudoElement>(node))
        node = downcast<PseudoElement>(*node).hostElement();
    m_innerNode = node;
}
    
void HitTestResult::setInnerNonSharedNode(Node* node)
{
    if (is<PseudoElement>(node))
        node = downcast<PseudoElement>(*node).hostElement();
    m_innerNonSharedNode = node;
}

void HitTestResult::setURLElement(Element* n) 
{ 
    m_innerURLElement = n; 
}

void HitTestResult::setScrollbar(Scrollbar* s)
{
    m_scrollbar = s;
}

Frame* HitTestResult::innerNodeFrame() const
{
    if (m_innerNonSharedNode)
        return m_innerNonSharedNode->document().frame();
    if (m_innerNode)
        return m_innerNode->document().frame();
    return 0;
}

Frame* HitTestResult::targetFrame() const
{
    if (!m_innerURLElement)
        return 0;

    Frame* frame = m_innerURLElement->document().frame();
    if (!frame)
        return 0;

    return frame->tree().find(m_innerURLElement->target());
}

bool HitTestResult::isSelected() const
{
    if (!m_innerNonSharedNode)
        return false;

    Frame* frame = m_innerNonSharedNode->document().frame();
    if (!frame)
        return false;

    return frame->selection().contains(m_hitTestLocation.point());
}

String HitTestResult::spellingToolTip(TextDirection& dir) const
{
    dir = LTR;
    // Return the tool tip string associated with this point, if any. Only markers associated with bad grammar
    // currently supply strings, but maybe someday markers associated with misspelled words will also.
    if (!m_innerNonSharedNode)
        return String();
    
    DocumentMarker* marker = m_innerNonSharedNode->document().markers().markerContainingPoint(m_hitTestLocation.point(), DocumentMarker::Grammar);
    if (!marker)
        return String();

    if (auto renderer = m_innerNonSharedNode->renderer())
        dir = renderer->style().direction();
    return marker->description();
}

String HitTestResult::replacedString() const
{
    // Return the replaced string associated with this point, if any. This marker is created when a string is autocorrected, 
    // and is used for generating a contextual menu item that allows it to easily be changed back if desired.
    if (!m_innerNonSharedNode)
        return String();
    
    DocumentMarker* marker = m_innerNonSharedNode->document().markers().markerContainingPoint(m_hitTestLocation.point(), DocumentMarker::Replacement);
    if (!marker)
        return String();
    
    return marker->description();
}    
    
String HitTestResult::title(TextDirection& dir) const
{
    dir = LTR;
    // Find the title in the nearest enclosing DOM node.
    // For <area> tags in image maps, walk the tree for the <area>, not the <img> using it.
    for (Node* titleNode = m_innerNode.get(); titleNode; titleNode = titleNode->parentNode()) {
        if (is<Element>(*titleNode)) {
            Element& titleElement = downcast<Element>(*titleNode);
            String title = titleElement.title();
            if (!title.isEmpty()) {
                if (auto renderer = titleElement.renderer())
                    dir = renderer->style().direction();
                return title;
            }
        }
    }
    return String();
}

String HitTestResult::innerTextIfTruncated(TextDirection& dir) const
{
    for (Node* truncatedNode = m_innerNode.get(); truncatedNode; truncatedNode = truncatedNode->parentNode()) {
        if (!is<Element>(*truncatedNode))
            continue;

        if (auto renderer = downcast<Element>(*truncatedNode).renderer()) {
            if (is<RenderBlockFlow>(*renderer)) {
                RenderBlockFlow& block = downcast<RenderBlockFlow>(*renderer);
                if (block.style().textOverflow()) {
                    for (RootInlineBox* line = block.firstRootBox(); line; line = line->nextRootBox()) {
                        if (line->hasEllipsisBox()) {
                            dir = block.style().direction();
                            return downcast<Element>(*truncatedNode).innerText();
                        }
                    }
                }
                break;
            }
        }
    }

    dir = LTR;
    return String();
}

String displayString(const String& string, const Node* node)
{
    if (!node)
        return string;
    return node->document().displayStringModifiedByEncoding(string);
}

String HitTestResult::altDisplayString() const
{
    if (!m_innerNonSharedNode)
        return String();
    
    if (is<HTMLImageElement>(*m_innerNonSharedNode)) {
        HTMLImageElement& image = downcast<HTMLImageElement>(*m_innerNonSharedNode);
        return displayString(image.fastGetAttribute(altAttr), m_innerNonSharedNode.get());
    }
    
    if (is<HTMLInputElement>(*m_innerNonSharedNode)) {
        HTMLInputElement& input = downcast<HTMLInputElement>(*m_innerNonSharedNode);
        return displayString(input.alt(), m_innerNonSharedNode.get());
    }

    return String();
}

Image* HitTestResult::image() const
{
    if (!m_innerNonSharedNode)
        return nullptr;
    
    auto* renderer = m_innerNonSharedNode->renderer();
    if (is<RenderImage>(renderer)) {
        auto& image = downcast<RenderImage>(*renderer);
        if (image.cachedImage() && !image.cachedImage()->errorOccurred())
            return image.cachedImage()->imageForRenderer(&image);
    }

    return nullptr;
}

IntRect HitTestResult::imageRect() const
{
    if (!image())
        return IntRect();
    return m_innerNonSharedNode->renderBox()->absoluteContentQuad().enclosingBoundingBox();
}

#if ENABLE(ATTACHMENT_ELEMENT)
URL HitTestResult::absoluteAttachmentURL() const
{
    if (!m_innerNonSharedNode)
        return URL();
    
    if (!(m_innerNonSharedNode->renderer() && m_innerNonSharedNode->renderer()->isAttachment()))
        return URL();
    
    if (!is<HTMLAttachmentElement>(*m_innerNonSharedNode))
        return URL();
    File* attachmentFile = downcast<HTMLAttachmentElement>(*m_innerNonSharedNode).file();
    if (!attachmentFile)
        return URL();
    
    return URL::fileURLWithFileSystemPath(attachmentFile->path());
}
#endif

URL HitTestResult::absoluteImageURL() const
{
    if (!m_innerNonSharedNode)
        return URL();

    if (!(m_innerNonSharedNode->renderer() && m_innerNonSharedNode->renderer()->isImage()))
        return URL();

    AtomicString urlString;
    if (is<HTMLEmbedElement>(*m_innerNonSharedNode)
        || is<HTMLImageElement>(*m_innerNonSharedNode)
        || is<HTMLInputElement>(*m_innerNonSharedNode)
        || is<HTMLObjectElement>(*m_innerNonSharedNode)
        || is<SVGImageElement>(*m_innerNonSharedNode)) {
        urlString = downcast<Element>(*m_innerNonSharedNode).imageSourceURL();
    } else
        return URL();

    return m_innerNonSharedNode->document().completeURL(stripLeadingAndTrailingHTMLSpaces(urlString));
}

URL HitTestResult::absolutePDFURL() const
{
    if (!m_innerNonSharedNode)
        return URL();

    if (!is<HTMLEmbedElement>(*m_innerNonSharedNode) && !is<HTMLObjectElement>(*m_innerNonSharedNode))
        return URL();

    HTMLPlugInImageElement& element = downcast<HTMLPlugInImageElement>(*m_innerNonSharedNode);
    URL url = m_innerNonSharedNode->document().completeURL(stripLeadingAndTrailingHTMLSpaces(element.url()));
    if (!url.isValid())
        return URL();

    if (element.serviceType() == "application/pdf" || (element.serviceType().isEmpty() && url.path().endsWith(".pdf", false)))
        return url;
    return URL();
}

URL HitTestResult::absoluteMediaURL() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->currentSrc();
    return URL();
#else
    return URL();
#endif
}

bool HitTestResult::mediaSupportsFullscreen() const
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElt(mediaElement());
    return is<HTMLVideoElement>(mediaElt) && mediaElt->supportsFullscreen();
#else
    return false;
#endif
}

#if ENABLE(VIDEO)
HTMLMediaElement* HitTestResult::mediaElement() const
{
    if (!m_innerNonSharedNode)
        return nullptr;

    if (!(m_innerNonSharedNode->renderer() && m_innerNonSharedNode->renderer()->isMedia()))
        return nullptr;

    if (is<HTMLMediaElement>(*m_innerNonSharedNode))
        return downcast<HTMLMediaElement>(m_innerNonSharedNode.get());
    return nullptr;
}
#endif

void HitTestResult::toggleMediaControlsDisplay() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->setControls(!mediaElt->controls());
#endif
}

void HitTestResult::toggleMediaLoopPlayback() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->setLoop(!mediaElt->loop());
#endif
}

bool HitTestResult::mediaIsInFullscreen() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElement = this->mediaElement())
        return mediaElement->isVideo() && mediaElement->isFullscreen();
#endif
    return false;
}

void HitTestResult::toggleMediaFullscreenState() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElement = this->mediaElement()) {
        if (mediaElement->isVideo() && mediaElement->supportsFullscreen()) {
            UserGestureIndicator indicator(DefinitelyProcessingUserGesture, &mediaElement->document());
            mediaElement->toggleFullscreenState();
        }
    }
#endif
}

void HitTestResult::enterFullscreenForVideo() const
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement(this->mediaElement());
    if (is<HTMLVideoElement>(mediaElement)) {
        HTMLVideoElement& videoElement = downcast<HTMLVideoElement>(*mediaElement);
        if (!videoElement.isFullscreen() && mediaElement->supportsFullscreen()) {
            UserGestureIndicator indicator(DefinitelyProcessingUserGesture, &mediaElement->document());
            videoElement.enterFullscreen();
        }
    }
#endif
}

bool HitTestResult::mediaControlsEnabled() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElement = this->mediaElement())
        return mediaElement->controls();
#endif
    return false;
}

bool HitTestResult::mediaLoopEnabled() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->loop();
#endif
    return false;
}

bool HitTestResult::mediaPlaying() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return !mediaElt->paused();
#endif
    return false;
}

void HitTestResult::toggleMediaPlayState() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->togglePlayState();
#endif
}

bool HitTestResult::mediaHasAudio() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->hasAudio();
#endif
    return false;
}

bool HitTestResult::mediaIsVideo() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return is<HTMLVideoElement>(*mediaElt);
#endif
    return false;
}

bool HitTestResult::mediaMuted() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->muted();
#endif
    return false;
}

void HitTestResult::toggleMediaMuteState() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        mediaElt->setMuted(!mediaElt->muted());
#endif
}

bool HitTestResult::isDownloadableMedia() const
{
#if ENABLE(VIDEO)
    if (HTMLMediaElement* mediaElt = mediaElement())
        return mediaElt->canSaveMediaData();
#endif

    return false;
}

bool HitTestResult::isOverTextInsideFormControlElement() const
{
    Node* node = innerNode();
    if (!node)
        return false;

    if (!is<HTMLTextFormControlElement>(*node))
        return false;

    Frame* frame = node->document().frame();
    if (!frame)
        return false;

    IntPoint framePoint = roundedPointInInnerNodeFrame();
    if (!frame->rangeForPoint(framePoint))
        return false;

    VisiblePosition position = frame->visiblePositionForPoint(framePoint);
    if (position.isNull())
        return false;

    RefPtr<Range> wordRange = enclosingTextUnitOfGranularity(position, WordGranularity, DirectionForward);
    if (!wordRange)
        return false;

    return !wordRange->text().isEmpty();
}

bool HitTestResult::allowsCopy() const
{
    Node* node = innerNode();
    if (!node)
        return false;

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return false;

    bool isUserSelectNone = renderer->style().userSelect() == SELECT_NONE;
    bool isPasswordField = is<HTMLInputElement>(node) && downcast<HTMLInputElement>(*node).isPasswordField();
    return !isPasswordField && !isUserSelectNone;
}

URL HitTestResult::absoluteLinkURL() const
{
    if (m_innerURLElement)
        return m_innerURLElement->absoluteLinkURL();
    return URL();
}

bool HitTestResult::isLiveLink() const
{
    if (!m_innerURLElement)
        return false;

    if (is<HTMLAnchorElement>(*m_innerURLElement))
        return downcast<HTMLAnchorElement>(*m_innerURLElement).isLiveLink();

    if (is<SVGAElement>(*m_innerURLElement))
        return m_innerURLElement->isLink();

    return false;
}

bool HitTestResult::isOverLink() const
{
    return m_innerURLElement && m_innerURLElement->isLink();
}

String HitTestResult::titleDisplayString() const
{
    if (!m_innerURLElement)
        return String();
    
    return displayString(m_innerURLElement->title(), m_innerURLElement.get());
}

String HitTestResult::textContent() const
{
    if (!m_innerURLElement)
        return String();
    return m_innerURLElement->textContent();
}

// FIXME: This function needs a better name and may belong in a different class. It's not
// really isContentEditable(); it's more like needsEditingContextMenu(). In many ways, this
// function would make more sense in the ContextMenu class, except that WebElementDictionary 
// hooks into it. Anyway, we should architect this better. 
bool HitTestResult::isContentEditable() const
{
    if (!m_innerNonSharedNode)
        return false;

    if (is<HTMLTextAreaElement>(*m_innerNonSharedNode))
        return true;

    if (is<HTMLInputElement>(*m_innerNonSharedNode))
        return downcast<HTMLInputElement>(*m_innerNonSharedNode).isTextField();

    return m_innerNonSharedNode->hasEditableStyle();
}

bool HitTestResult::addNodeToRectBasedTestResult(Node* node, const HitTestRequest& request, const HitTestLocation& locationInContainer, const LayoutRect& rect)
{
    // If it is not a rect-based hit test, this method has to be no-op.
    // Return false, so the hit test stops.
    if (!isRectBasedTest())
        return false;

    // If node is null, return true so the hit test can continue.
    if (!node)
        return true;

    if (request.disallowsShadowContent())
        node = node->document().ancestorInThisScope(node);

    mutableRectBasedTestResult().add(node);

    bool regionFilled = rect.contains(locationInContainer.boundingBox());
    return !regionFilled;
}

bool HitTestResult::addNodeToRectBasedTestResult(Node* node, const HitTestRequest& request, const HitTestLocation& locationInContainer, const FloatRect& rect)
{
    // If it is not a rect-based hit test, this method has to be no-op.
    // Return false, so the hit test stops.
    if (!isRectBasedTest())
        return false;

    // If node is null, return true so the hit test can continue.
    if (!node)
        return true;

    if (request.disallowsShadowContent())
        node = node->document().ancestorInThisScope(node);

    mutableRectBasedTestResult().add(node);

    bool regionFilled = rect.contains(locationInContainer.boundingBox());
    return !regionFilled;
}

void HitTestResult::append(const HitTestResult& other)
{
    ASSERT(isRectBasedTest() && other.isRectBasedTest());

    if (!m_innerNode && other.innerNode()) {
        m_innerNode = other.innerNode();
        m_innerNonSharedNode = other.innerNonSharedNode();
        m_localPoint = other.localPoint();
        m_pointInInnerNodeFrame = other.m_pointInInnerNodeFrame;
        m_innerURLElement = other.URLElement();
        m_scrollbar = other.scrollbar();
        m_isOverWidget = other.isOverWidget();
    }

    if (other.m_rectBasedTestResult) {
        NodeSet& set = mutableRectBasedTestResult();
        for (NodeSet::const_iterator it = other.m_rectBasedTestResult->begin(), last = other.m_rectBasedTestResult->end(); it != last; ++it)
            set.add(it->get());
    }
}

const HitTestResult::NodeSet& HitTestResult::rectBasedTestResult() const
{
    if (!m_rectBasedTestResult)
        m_rectBasedTestResult = std::make_unique<NodeSet>();
    return *m_rectBasedTestResult;
}

HitTestResult::NodeSet& HitTestResult::mutableRectBasedTestResult()
{
    if (!m_rectBasedTestResult)
        m_rectBasedTestResult = std::make_unique<NodeSet>();
    return *m_rectBasedTestResult;
}

Vector<String> HitTestResult::dictationAlternatives() const
{
    // Return the dictation context handle if the text at this point has DictationAlternative marker, which means this text is
    if (!m_innerNonSharedNode)
        return Vector<String>();

    DocumentMarker* marker = m_innerNonSharedNode->document().markers().markerContainingPoint(pointInInnerNodeFrame(), DocumentMarker::DictationAlternatives);
    if (!marker)
        return Vector<String>();

    Frame* frame = innerNonSharedNode()->document().frame();
    if (!frame)
        return Vector<String>();

    return frame->editor().dictationAlternativesForMarker(marker);
}

Node* HitTestResult::targetNode() const
{
    Node* node = innerNode();
    if (!node)
        return 0;
    if (node->inDocument())
        return node;

    Element* element = node->parentElement();
    if (element && element->inDocument())
        return element;

    return node;
}

Element* HitTestResult::innerElement() const
{
    Node* node = m_innerNode.get();
    if (!node)
        return nullptr;
    if (is<Element>(*node))
        return downcast<Element>(node);
    return node->parentElement();
}

Element* HitTestResult::innerNonSharedElement() const
{
    Node* node = m_innerNonSharedNode.get();
    if (!node)
        return nullptr;
    if (is<Element>(*node))
        return downcast<Element>(node);
    return node->parentElement();
}

} // namespace WebCore
