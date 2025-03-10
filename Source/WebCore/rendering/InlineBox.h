/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef InlineBox_h
#define InlineBox_h

#include "RenderBoxModelObject.h"
#include "RenderText.h"
#include "TextFlags.h"
#include <wtf/TypeCasts.h>

namespace WebCore {

class HitTestRequest;
class HitTestResult;
class RootInlineBox;

// InlineBox represents a rectangle that occurs on a line.  It corresponds to
// some RenderObject (i.e., it represents a portion of that RenderObject).
class InlineBox {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~InlineBox();

    void assertNotDeleted() const;

    virtual void deleteLine() = 0;
    virtual void extractLine() = 0;
    virtual void attachLine() = 0;

    virtual bool isLineBreak() const { return renderer().isLineBreak(); }

    WEBCORE_EXPORT virtual void adjustPosition(float dx, float dy);
    void adjustLogicalPosition(float deltaLogicalLeft, float deltaLogicalTop)
    {
        if (isHorizontal())
            adjustPosition(deltaLogicalLeft, deltaLogicalTop);
        else
            adjustPosition(deltaLogicalTop, deltaLogicalLeft);
    }
    void adjustLineDirectionPosition(float delta)
    {
        if (isHorizontal())
            adjustPosition(delta, 0);
        else
            adjustPosition(0, delta);
    }
    void adjustBlockDirectionPosition(float delta)
    {
        if (isHorizontal())
            adjustPosition(0, delta);
        else
            adjustPosition(delta, 0);
    }

    virtual void paint(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom) = 0;
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit lineTop, LayoutUnit lineBottom) = 0;

#if ENABLE(TREE_DEBUGGING)
    void showNodeTreeForThis() const;
    void showLineTreeForThis() const;
    
    virtual void showLineTreeAndMark(const InlineBox* markedBox, int depth) const;
    virtual void showLineBox(bool mark, int depth) const;
    virtual const char* boxName() const;
#endif

    bool behavesLikeText() const { return m_bitfields.behavesLikeText(); }
    void setBehavesLikeText(bool behavesLikeText) { m_bitfields.setBehavesLikeText(behavesLikeText); }

    virtual bool isInlineElementBox() const { return false; }
    virtual bool isInlineFlowBox() const { return false; }
    virtual bool isInlineTextBox() const { return false; }
    virtual bool isRootInlineBox() const { return false; }
    virtual bool isSVGInlineTextBox() const { return false; }
    virtual bool isSVGInlineFlowBox() const { return false; }
    virtual bool isSVGRootInlineBox() const { return false; }

    bool hasVirtualLogicalHeight() const { return m_bitfields.hasVirtualLogicalHeight(); }
    void setHasVirtualLogicalHeight() { m_bitfields.setHasVirtualLogicalHeight(true); }
    virtual float virtualLogicalHeight() const
    {
        ASSERT_NOT_REACHED();
        return 0;
    }

    bool isHorizontal() const { return m_bitfields.isHorizontal(); }
    void setIsHorizontal(bool isHorizontal) { m_bitfields.setIsHorizontal(isHorizontal); }

    virtual FloatRect calculateBoundaries() const
    {
        ASSERT_NOT_REACHED();
        return FloatRect();
    }

    bool isConstructed() { return m_bitfields.constructed(); }
    virtual void setConstructed() { m_bitfields.setConstructed(true); }

    void setExtracted(bool extracted = true) { m_bitfields.setExtracted(extracted); }
    
    void setIsFirstLine(bool firstLine) { m_bitfields.setFirstLine(firstLine); }
    bool isFirstLine() const { return m_bitfields.firstLine(); }

    void removeFromParent();

    InlineBox* nextOnLine() const { return m_next; }
    InlineBox* prevOnLine() const { return m_prev; }
    void setNextOnLine(InlineBox* next)
    {
        ASSERT(m_parent || !next);
        m_next = next;
    }
    void setPrevOnLine(InlineBox* prev)
    {
        ASSERT(m_parent || !prev);
        m_prev = prev;
    }
    bool nextOnLineExists() const;
    bool previousOnLineExists() const;

    virtual bool isLeaf() const { return true; }
    
    InlineBox* nextLeafChild() const;
    InlineBox* prevLeafChild() const;

    // Helper functions for editing and hit-testing code.
    // FIXME: These two functions should be moved to RenderedPosition once the code to convert between
    // Position and inline box, offset pair is moved to RenderedPosition.
    InlineBox* nextLeafChildIgnoringLineBreak() const;
    InlineBox* prevLeafChildIgnoringLineBreak() const;

    // FIXME: Hide this once all callers are using tighter types.
    RenderObject& renderer() const { return m_renderer; }

    InlineFlowBox* parent() const
    {
        assertNotDeleted();
        ASSERT_WITH_SECURITY_IMPLICATION(!m_hasBadParent);
        return m_parent;
    }
    void setParent(InlineFlowBox* par) { m_parent = par; }

    const RootInlineBox& root() const;
    RootInlineBox& root();

    // x() is the left side of the box in the containing block's coordinate system.
    void setX(float x) { m_topLeft.setX(x); }
    float x() const { return m_topLeft.x(); }
    float left() const { return m_topLeft.x(); }

    // y() is the top side of the box in the containing block's coordinate system.
    void setY(float y) { m_topLeft.setY(y); }
    float y() const { return m_topLeft.y(); }
    float top() const { return m_topLeft.y(); }

    const FloatPoint& topLeft() const { return m_topLeft; }

    float width() const { return isHorizontal() ? logicalWidth() : logicalHeight(); }
    float height() const { return isHorizontal() ? logicalHeight() : logicalWidth(); }
    FloatSize size() const { return FloatSize(width(), height()); }
    float right() const { return left() + width(); }
    float bottom() const { return top() + height(); }

    // The logicalLeft position is the left edge of the line box in a horizontal line and the top edge in a vertical line.
    float logicalLeft() const { return isHorizontal() ? m_topLeft.x() : m_topLeft.y(); }
    float logicalRight() const { return logicalLeft() + logicalWidth(); }
    void setLogicalLeft(float left)
    {
        if (isHorizontal())
            setX(left);
        else
            setY(left);
    }
    int pixelSnappedLogicalLeft() const { return logicalLeft(); }
    int pixelSnappedLogicalRight() const { return ceilf(logicalRight()); }
    int pixelSnappedLogicalTop() const { return logicalTop(); }
    int pixelSnappedLogicalBottom() const { return ceilf(logicalBottom()); }

    // The logicalTop[ position is the top edge of the line box in a horizontal line and the left edge in a vertical line.
    float logicalTop() const { return isHorizontal() ? m_topLeft.y() : m_topLeft.x(); }
    float logicalBottom() const { return logicalTop() + logicalHeight(); }
    void setLogicalTop(float top)
    {
        if (isHorizontal())
            setY(top);
        else
            setX(top);
    }

    // The logical width is our extent in the line's overall inline direction, i.e., width for horizontal text and height for vertical text.
    void setLogicalWidth(float w) { m_logicalWidth = w; }
    float logicalWidth() const { return m_logicalWidth; }

    // The logical height is our extent in the block flow direction, i.e., height for horizontal text and width for vertical text.
    float logicalHeight() const;

    FloatRect logicalFrameRect() const { return isHorizontal() ? FloatRect(m_topLeft.x(), m_topLeft.y(), m_logicalWidth, logicalHeight()) : FloatRect(m_topLeft.y(), m_topLeft.x(), m_logicalWidth, logicalHeight()); }
    FloatRect frameRect() const { return FloatRect(topLeft(), size()); }

    WEBCORE_EXPORT virtual int baselinePosition(FontBaseline baselineType) const;
    WEBCORE_EXPORT virtual LayoutUnit lineHeight() const;

    WEBCORE_EXPORT virtual int caretMinOffset() const;
    WEBCORE_EXPORT virtual int caretMaxOffset() const;

    unsigned char bidiLevel() const { return m_bitfields.bidiEmbeddingLevel(); }
    void setBidiLevel(unsigned char level) { m_bitfields.setBidiEmbeddingLevel(level); }
    TextDirection direction() const { return bidiLevel() % 2 ? RTL : LTR; }
    bool isLeftToRightDirection() const { return direction() == LTR; }
    int caretLeftmostOffset() const { return isLeftToRightDirection() ? caretMinOffset() : caretMaxOffset(); }
    int caretRightmostOffset() const { return isLeftToRightDirection() ? caretMaxOffset() : caretMinOffset(); }

    virtual void clearTruncation() { }

    bool isDirty() const { return m_bitfields.dirty(); }
    virtual void markDirty(bool dirty = true) { m_bitfields.setDirty(dirty); }

    WEBCORE_EXPORT virtual void dirtyLineBoxes();
    
    WEBCORE_EXPORT virtual RenderObject::SelectionState selectionState();

    WEBCORE_EXPORT virtual bool canAccommodateEllipsis(bool ltr, int blockEdge, int ellipsisWidth) const;
    // visibleLeftEdge, visibleRightEdge are in the parent's coordinate system.
    WEBCORE_EXPORT virtual float placeEllipsisBox(bool ltr, float visibleLeftEdge, float visibleRightEdge, float ellipsisWidth, float &truncatedWidth, bool&);

#if !ASSERT_WITH_SECURITY_IMPLICATION_DISABLED
    void setHasBadParent();
    void invalidateParentChildList();
#endif

    bool visibleToHitTesting() const { return renderer().style().visibility() == VISIBLE && renderer().style().pointerEvents() != PE_NONE; }

    const RenderStyle& lineStyle() const { return m_bitfields.firstLine() ? renderer().firstLineStyle() : renderer().style(); }
    
    EVerticalAlign verticalAlign() const { return lineStyle().verticalAlign(); }

    // Use with caution! The type is not checked!
    RenderBoxModelObject* boxModelObject() const
    { 
        if (!is<RenderText>(m_renderer))
            return &downcast<RenderBoxModelObject>(m_renderer);
        return nullptr;
    }

    FloatPoint locationIncludingFlipping();
    void flipForWritingMode(FloatRect&);
    FloatPoint flipForWritingMode(const FloatPoint&);
    void flipForWritingMode(LayoutRect&);
    LayoutPoint flipForWritingMode(const LayoutPoint&);

    bool knownToHaveNoOverflow() const { return m_bitfields.knownToHaveNoOverflow(); }
    void clearKnownToHaveNoOverflow();

    bool dirOverride() const { return m_bitfields.dirOverride(); }
    void setDirOverride(bool dirOverride) { m_bitfields.setDirOverride(dirOverride); }

    void setExpansion(float newExpansion)
    {
        m_logicalWidth -= m_expansion;
        m_expansion = newExpansion;
        m_logicalWidth += m_expansion;
    }
    void setExpansionWithoutGrowing(float newExpansion)
    {
        ASSERT(!m_expansion);
        m_expansion = newExpansion;
    }
    float expansion() const { return m_expansion; }

private:
    InlineBox* m_next; // The next element on the same line as us.
    InlineBox* m_prev; // The previous element on the same line as us.

    InlineFlowBox* m_parent; // The box that contains us.

    RenderObject& m_renderer;

public:
    FloatPoint m_topLeft;
    float m_logicalWidth;

#define ADD_BOOLEAN_BITFIELD(name, Name) \
    private:\
    unsigned m_##name : 1;\
    public:\
    bool name() const { return m_##name; }\
    void set##Name(bool name) { m_##name = name; }\

    class InlineBoxBitfields {
    public:
        explicit InlineBoxBitfields(bool firstLine = false, bool constructed = false, bool dirty = false, bool extracted = false, bool isHorizontal = true)
            : m_firstLine(firstLine)
            , m_constructed(constructed)
            , m_bidiEmbeddingLevel(0)
            , m_dirty(dirty)
            , m_extracted(extracted)
            , m_hasVirtualLogicalHeight(false)
            , m_isHorizontal(isHorizontal)
            , m_endsWithBreak(false)
            , m_hasSelectedChildrenOrCanHaveLeadingExpansion(false)
            , m_knownToHaveNoOverflow(true)  
            , m_hasEllipsisBoxOrHyphen(false)
            , m_dirOverride(false)
            , m_behavesLikeText(false)
            , m_determinedIfNextOnLineExists(false)
            , m_nextOnLineExists(false)
        {
        }

        // Some of these bits are actually for subclasses and moved here to compact the structures.
        // for this class
        ADD_BOOLEAN_BITFIELD(firstLine, FirstLine);
        ADD_BOOLEAN_BITFIELD(constructed, Constructed);

    private:
        unsigned m_bidiEmbeddingLevel : 6; // The maximium bidi level is 62: http://unicode.org/reports/tr9/#Explicit_Levels_and_Directions

    public:
        unsigned char bidiEmbeddingLevel() const { return m_bidiEmbeddingLevel; }
        void setBidiEmbeddingLevel(unsigned char bidiEmbeddingLevel) { m_bidiEmbeddingLevel = bidiEmbeddingLevel; }

        ADD_BOOLEAN_BITFIELD(dirty, Dirty);
        ADD_BOOLEAN_BITFIELD(extracted, Extracted);
        ADD_BOOLEAN_BITFIELD(hasVirtualLogicalHeight, HasVirtualLogicalHeight);
        ADD_BOOLEAN_BITFIELD(isHorizontal, IsHorizontal);
        // for RootInlineBox
        ADD_BOOLEAN_BITFIELD(endsWithBreak, EndsWithBreak); // Whether the line ends with a <br>.
        // shared between RootInlineBox and InlineTextBox
        ADD_BOOLEAN_BITFIELD(hasSelectedChildrenOrCanHaveLeadingExpansion, HasSelectedChildrenOrCanHaveLeadingExpansion);
        ADD_BOOLEAN_BITFIELD(knownToHaveNoOverflow, KnownToHaveNoOverflow);
        ADD_BOOLEAN_BITFIELD(hasEllipsisBoxOrHyphen, HasEllipsisBoxOrHyphen);
        // for InlineTextBox
        ADD_BOOLEAN_BITFIELD(dirOverride, DirOverride);
        ADD_BOOLEAN_BITFIELD(behavesLikeText, BehavesLikeText); // Whether or not this object represents text with a non-zero height. Includes non-image list markers, text boxes, br.

    private:
        mutable unsigned m_determinedIfNextOnLineExists : 1;

    public:
        bool determinedIfNextOnLineExists() const { return m_determinedIfNextOnLineExists; }
        void setDeterminedIfNextOnLineExists(bool determinedIfNextOnLineExists) const { m_determinedIfNextOnLineExists = determinedIfNextOnLineExists; }

    private:
        mutable unsigned m_nextOnLineExists : 1;
        
    public:
        bool nextOnLineExists() const { return m_nextOnLineExists; }
        void setNextOnLineExists(bool nextOnLineExists) const { m_nextOnLineExists = nextOnLineExists; }
    };
#undef ADD_BOOLEAN_BITFIELD

private:
    float m_expansion;
    InlineBoxBitfields m_bitfields;

protected:
    explicit InlineBox(RenderObject& renderer)
        : m_next(nullptr)
        , m_prev(nullptr)
        , m_parent(nullptr)
        , m_renderer(renderer)
        , m_logicalWidth(0)
        , m_expansion(0)
#if !ASSERT_WITH_SECURITY_IMPLICATION_DISABLED
        , m_deletionSentinel(deletionSentinelNotDeletedValue)
        , m_hasBadParent(false)
#endif
    {
    }

    InlineBox(RenderObject& renderer, FloatPoint topLeft, float logicalWidth, bool firstLine, bool constructed, bool dirty, bool extracted, bool isHorizontal, InlineBox* next, InlineBox* prev, InlineFlowBox* parent)
        : m_next(next)
        , m_prev(prev)
        , m_parent(parent)
        , m_renderer(renderer)
        , m_topLeft(topLeft)
        , m_logicalWidth(logicalWidth)
        , m_expansion(0)
        , m_bitfields(firstLine, constructed, dirty, extracted, isHorizontal)
#if !ASSERT_WITH_SECURITY_IMPLICATION_DISABLED
        , m_deletionSentinel(deletionSentinelNotDeletedValue)
        , m_hasBadParent(false)
#endif
    {
    }

    // For RootInlineBox
    bool endsWithBreak() const { return m_bitfields.endsWithBreak(); }
    void setEndsWithBreak(bool endsWithBreak) { m_bitfields.setEndsWithBreak(endsWithBreak); }
    bool hasEllipsisBox() const { return m_bitfields.hasEllipsisBoxOrHyphen(); }
    bool hasSelectedChildren() const { return m_bitfields.hasSelectedChildrenOrCanHaveLeadingExpansion(); }
    void setHasSelectedChildren(bool hasSelectedChildren) { m_bitfields.setHasSelectedChildrenOrCanHaveLeadingExpansion(hasSelectedChildren); }
    void setHasEllipsisBox(bool hasEllipsisBox) { m_bitfields.setHasEllipsisBoxOrHyphen(hasEllipsisBox); }

    // For InlineTextBox
    bool hasHyphen() const { return m_bitfields.hasEllipsisBoxOrHyphen(); }
    void setHasHyphen(bool hasHyphen) { m_bitfields.setHasEllipsisBoxOrHyphen(hasHyphen); }    
    bool canHaveLeadingExpansion() const { return m_bitfields.hasSelectedChildrenOrCanHaveLeadingExpansion(); }
    void setCanHaveLeadingExpansion(bool canHaveLeadingExpansion) { m_bitfields.setHasSelectedChildrenOrCanHaveLeadingExpansion(canHaveLeadingExpansion); }
    
    // For InlineFlowBox and InlineTextBox
    bool extracted() const { return m_bitfields.extracted(); }

#if !ASSERT_WITH_SECURITY_IMPLICATION_DISABLED
private:
    static const unsigned deletionSentinelNotDeletedValue = 0xF0F0F0F0U;
    static const unsigned deletionSentinelDeletedValue = 0xF0DEADF0U;
    unsigned m_deletionSentinel;
    bool m_hasBadParent;
#endif
};

#if ASSERT_WITH_SECURITY_IMPLICATION_DISABLED

inline InlineBox::~InlineBox()
{
}

inline void InlineBox::assertNotDeleted() const
{
}

#endif

} // namespace WebCore

#define SPECIALIZE_TYPE_TRAITS_INLINE_BOX(ToValueTypeName, predicate) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ToValueTypeName) \
    static bool isType(const WebCore::InlineBox& box) { return box.predicate; } \
SPECIALIZE_TYPE_TRAITS_END()

#if ENABLE(TREE_DEBUGGING)
// Outside the WebCore namespace for ease of invocation from gdb.
void showNodeTree(const WebCore::InlineBox*);
void showLineTree(const WebCore::InlineBox*);
#endif

#endif // InlineBox_h
