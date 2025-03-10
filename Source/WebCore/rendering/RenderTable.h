/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2009, 2010, 2014 Apple Inc. All rights reserved.
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

#ifndef RenderTable_h
#define RenderTable_h

#include "CSSPropertyNames.h"
#include "CollapsedBorderValue.h"
#include "RenderBlock.h"
#include <memory>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class RenderTableCol;
class RenderTableCaption;
class RenderTableCell;
class RenderTableSection;
class TableLayout;

enum SkipEmptySectionsValue { DoNotSkipEmptySections, SkipEmptySections };

class RenderTable : public RenderBlock {
public:
    RenderTable(Element&, Ref<RenderStyle>&&);
    RenderTable(Document&, Ref<RenderStyle>&&);
    virtual ~RenderTable();

    // Per CSS 3 writing-mode: "The first and second values of the 'border-spacing' property represent spacing between columns
    // and rows respectively, not necessarily the horizontal and vertical spacing respectively".
    int hBorderSpacing() const { return m_hSpacing; }
    int vBorderSpacing() const { return m_vSpacing; }
    
    bool collapseBorders() const { return style().borderCollapse(); }

    virtual LayoutUnit borderStart() const override { return m_borderStart; }
    virtual LayoutUnit borderEnd() const override { return m_borderEnd; }
    virtual LayoutUnit borderBefore() const override;
    virtual LayoutUnit borderAfter() const override;

    virtual LayoutUnit borderLeft() const override
    {
        if (style().isHorizontalWritingMode())
            return style().isLeftToRightDirection() ? borderStart() : borderEnd();
        return style().isFlippedBlocksWritingMode() ? borderAfter() : borderBefore();
    }

    virtual LayoutUnit borderRight() const override
    {
        if (style().isHorizontalWritingMode())
            return style().isLeftToRightDirection() ? borderEnd() : borderStart();
        return style().isFlippedBlocksWritingMode() ? borderBefore() : borderAfter();
    }

    virtual LayoutUnit borderTop() const override
    {
        if (style().isHorizontalWritingMode())
            return style().isFlippedBlocksWritingMode() ? borderAfter() : borderBefore();
        return style().isLeftToRightDirection() ? borderStart() : borderEnd();
    }

    virtual LayoutUnit borderBottom() const override
    {
        if (style().isHorizontalWritingMode())
            return style().isFlippedBlocksWritingMode() ? borderBefore() : borderAfter();
        return style().isLeftToRightDirection() ? borderEnd() : borderStart();
    }

    Color bgColor() const { return style().visitedDependentColor(CSSPropertyBackgroundColor); }

    int outerBorderBefore() const;
    int outerBorderAfter() const;
    int outerBorderStart() const;
    int outerBorderEnd() const;

    int outerBorderLeft() const
    {
        if (style().isHorizontalWritingMode())
            return style().isLeftToRightDirection() ? outerBorderStart() : outerBorderEnd();
        return style().isFlippedBlocksWritingMode() ? outerBorderAfter() : outerBorderBefore();
    }

    int outerBorderRight() const
    {
        if (style().isHorizontalWritingMode())
            return style().isLeftToRightDirection() ? outerBorderEnd() : outerBorderStart();
        return style().isFlippedBlocksWritingMode() ? outerBorderBefore() : outerBorderAfter();
    }

    int outerBorderTop() const
    {
        if (style().isHorizontalWritingMode())
            return style().isFlippedBlocksWritingMode() ? outerBorderAfter() : outerBorderBefore();
        return style().isLeftToRightDirection() ? outerBorderStart() : outerBorderEnd();
    }

    int outerBorderBottom() const
    {
        if (style().isHorizontalWritingMode())
            return style().isFlippedBlocksWritingMode() ? outerBorderBefore() : outerBorderAfter();
        return style().isLeftToRightDirection() ? outerBorderEnd() : outerBorderStart();
    }

    int calcBorderStart() const;
    int calcBorderEnd() const;
    void recalcBordersInRowDirection();

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0) override final;

    struct ColumnStruct {
        explicit ColumnStruct(unsigned initialSpan = 1)
            : span(initialSpan)
        {
        }

        unsigned span;
    };

    void forceSectionsRecalc()
    {
        setNeedsSectionRecalc();
        recalcSections();
    }

    const Vector<ColumnStruct>& columns() const { return m_columns; }
    const Vector<int>& columnPositions() const { return m_columnPos; }
    void setColumnPosition(unsigned index, int position)
    {
        // Note that if our horizontal border-spacing changed, our position will change but not
        // our column's width. In practice, horizontal border-spacing won't change often.
        m_columnLogicalWidthChanged |= m_columnPos[index] != position;
        m_columnPos[index] = position;
    }

    RenderTableSection* header() const { return m_head; }
    RenderTableSection* footer() const { return m_foot; }
    RenderTableSection* firstBody() const { return m_firstBody; }

    // This function returns 0 if the table has no section.
    RenderTableSection* topSection() const;
    RenderTableSection* bottomSection() const;

    // This function returns 0 if the table has no non-empty sections.
    RenderTableSection* topNonEmptySection() const;

    unsigned lastColumnIndex() const { return numEffCols() - 1; }

    void splitColumn(unsigned position, unsigned firstSpan);
    void appendColumn(unsigned span);
    unsigned numEffCols() const { return m_columns.size(); }
    unsigned spanOfEffCol(unsigned effCol) const { return m_columns[effCol].span; }
    
    unsigned colToEffCol(unsigned column) const
    {
        if (!m_hasCellColspanThatDeterminesTableWidth)
            return column;

        unsigned effColumn = 0;
        unsigned numColumns = numEffCols();
        for (unsigned c = 0; effColumn < numColumns && c + m_columns[effColumn].span - 1 < column; ++effColumn)
            c += m_columns[effColumn].span;
        return effColumn;
    }
    
    unsigned effColToCol(unsigned effCol) const
    {
        if (!m_hasCellColspanThatDeterminesTableWidth)
            return effCol;

        unsigned c = 0;
        for (unsigned i = 0; i < effCol; i++)
            c += m_columns[i].span;
        return c;
    }

    LayoutUnit borderSpacingInRowDirection() const
    {
        if (unsigned effectiveColumnCount = numEffCols())
            return (effectiveColumnCount + 1) * hBorderSpacing();

        return 0;
    }

    // Override paddingStart/End to return pixel values to match behavor of RenderTableCell.
    virtual LayoutUnit paddingEnd() const override final { return static_cast<int>(RenderBlock::paddingEnd()); }
    virtual LayoutUnit paddingStart() const override final { return static_cast<int>(RenderBlock::paddingStart()); }

    LayoutUnit bordersPaddingAndSpacingInRowDirection() const
    {
        // 'border-spacing' only applies to separate borders (see 17.6.1 The separated borders model).
        return borderStart() + borderEnd() + (collapseBorders() ? LayoutUnit() : (paddingStart() + paddingEnd() + borderSpacingInRowDirection()));
    }

    // Return the first column or column-group.
    RenderTableCol* firstColumn() const;

    RenderTableCol* colElement(unsigned col, bool* startEdge = 0, bool* endEdge = 0) const
    {
        // The common case is to not have columns, make that case fast.
        if (!m_hasColElements)
            return 0;
        return slowColElement(col, startEdge, endEdge);
    }

    bool needsSectionRecalc() const { return m_needsSectionRecalc; }
    void setNeedsSectionRecalc()
    {
        if (documentBeingDestroyed())
            return;
        m_needsSectionRecalc = true;
        setNeedsLayout();
    }

    RenderTableSection* sectionAbove(const RenderTableSection*, SkipEmptySectionsValue = DoNotSkipEmptySections) const;
    RenderTableSection* sectionBelow(const RenderTableSection*, SkipEmptySectionsValue = DoNotSkipEmptySections) const;

    RenderTableCell* cellAbove(const RenderTableCell*) const;
    RenderTableCell* cellBelow(const RenderTableCell*) const;
    RenderTableCell* cellBefore(const RenderTableCell*) const;
    RenderTableCell* cellAfter(const RenderTableCell*) const;
 
    typedef Vector<CollapsedBorderValue> CollapsedBorderValues;
    void invalidateCollapsedBorders()
    {
        m_collapsedBordersValid = false;
        m_collapsedBorders.clear();
    }
    const CollapsedBorderValue* currentBorderValue() const { return m_currentBorder; }
    
    bool hasSections() const { return m_head || m_foot || m_firstBody; }

    void recalcSectionsIfNeeded() const
    {
        if (m_needsSectionRecalc)
            recalcSections();
    }

    static RenderTable* createAnonymousWithParentRenderer(const RenderObject*);
    virtual RenderBox* createAnonymousBoxWithSameTypeAs(const RenderObject* parent) const override
    {
        return createAnonymousWithParentRenderer(parent);
    }

    const BorderValue& tableStartBorderAdjoiningCell(const RenderTableCell*) const;
    const BorderValue& tableEndBorderAdjoiningCell(const RenderTableCell*) const;

    void addCaption(const RenderTableCaption*);
    void removeCaption(const RenderTableCaption*);
    void addColumn(const RenderTableCol*);
    void removeColumn(const RenderTableCol*);

    LayoutUnit offsetTopForColumn(const RenderTableCol&) const;
    LayoutUnit offsetLeftForColumn(const RenderTableCol&) const;
    LayoutUnit offsetWidthForColumn(const RenderTableCol&) const;
    LayoutUnit offsetHeightForColumn(const RenderTableCol&) const;
    
    virtual void markForPaginationRelayoutIfNeeded() override final;
    
protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override final;
    virtual void simplifiedNormalFlowLayout() override final;

private:
    virtual const char* renderName() const override { return "RenderTable"; }

    virtual bool isTable() const override final { return true; }

    virtual bool avoidsFloats() const override final { return true; }

    virtual void paint(PaintInfo&, const LayoutPoint&) override final;
    virtual void paintObject(PaintInfo&, const LayoutPoint&) override final;
    virtual void paintBoxDecorations(PaintInfo&, const LayoutPoint&) override final;
    virtual void paintMask(PaintInfo&, const LayoutPoint&) override final;
    virtual void layout() override final;
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minWidth, LayoutUnit& maxWidth) const override final;
    virtual void computePreferredLogicalWidths() override;
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) override;

    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const override final;
    virtual Optional<int> firstLineBaseline() const override;
    virtual Optional<int> inlineBlockBaseline(LineDirectionMode) const override final;

    RenderTableCol* slowColElement(unsigned col, bool* startEdge, bool* endEdge) const;

    void updateColumnCache() const;
    void invalidateCachedColumns();

    void invalidateCachedColumnOffsets();

    virtual RenderBlock* firstLineBlock() const override final;
    virtual void updateFirstLetter() override final;
    
    virtual void updateLogicalWidth() override final;

    LayoutUnit convertStyleLogicalWidthToComputedWidth(const Length& styleLogicalWidth, LayoutUnit availableWidth);
    LayoutUnit convertStyleLogicalHeightToComputedHeight(const Length& styleLogicalHeight);

    virtual LayoutRect overflowClipRect(const LayoutPoint& location, RenderRegion*, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, PaintPhase = PaintPhaseBlockBackground) override final;
    virtual LayoutRect overflowClipRectForChildLayers(const LayoutPoint& location, RenderRegion* region, OverlayScrollbarSizeRelevancy relevancy) override { return RenderBox::overflowClipRect(location, region, relevancy); }

    virtual void addOverflowFromChildren() override final;

    void subtractCaptionRect(LayoutRect&) const;

    void recalcCollapsedBorders();
    void recalcSections() const;
    void layoutCaption(RenderTableCaption*);

    void distributeExtraLogicalHeight(int extraLogicalHeight);

    mutable Vector<int> m_columnPos;
    mutable Vector<ColumnStruct> m_columns;
    mutable Vector<RenderTableCaption*> m_captions;
    mutable Vector<RenderTableCol*> m_columnRenderers;

    unsigned effectiveIndexOfColumn(const RenderTableCol&) const;
    typedef HashMap<const RenderTableCol*, unsigned> EffectiveColumnIndexMap;
    mutable EffectiveColumnIndexMap m_effectiveColumnIndexMap;

    mutable RenderTableSection* m_head;
    mutable RenderTableSection* m_foot;
    mutable RenderTableSection* m_firstBody;

    std::unique_ptr<TableLayout> m_tableLayout;

    CollapsedBorderValues m_collapsedBorders;
    const CollapsedBorderValue* m_currentBorder;
    bool m_collapsedBordersValid : 1;

    mutable bool m_hasColElements : 1;
    mutable bool m_needsSectionRecalc : 1;

    bool m_columnLogicalWidthChanged : 1;
    mutable bool m_columnRenderersValid: 1;
    mutable bool m_hasCellColspanThatDeterminesTableWidth : 1;

    bool hasCellColspanThatDeterminesTableWidth() const
    {
        for (unsigned c = 0; c < numEffCols(); c++) {
            if (m_columns[c].span > 1)
                return true;
        }
        return false;
    }

    short m_hSpacing;
    short m_vSpacing;
    int m_borderStart;
    int m_borderEnd;
    mutable LayoutUnit m_columnOffsetTop;
    mutable LayoutUnit m_columnOffsetHeight;
};

inline RenderTableSection* RenderTable::topSection() const
{
    ASSERT(!needsSectionRecalc());
    if (m_head)
        return m_head;
    if (m_firstBody)
        return m_firstBody;
    return m_foot;
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderTable, isTable())

#endif // RenderTable_h
