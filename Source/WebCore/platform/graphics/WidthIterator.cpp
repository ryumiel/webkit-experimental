/*
 * Copyright (C) 2003, 2006, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Holger Hans Peter Freyther
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
#include "WidthIterator.h"

#include "Font.h"
#include "FontCascade.h"
#include "GlyphBuffer.h"
#include "Latin1TextIterator.h"
#include "SurrogatePairAwareTextIterator.h"
#include <wtf/MathExtras.h>

using namespace WTF;
using namespace Unicode;

namespace WebCore {

WidthIterator::WidthIterator(const FontCascade* font, const TextRun& run, HashSet<const Font*>* fallbackFonts, bool accountForGlyphBounds, bool forTextEmphasis)
    : m_font(font)
    , m_run(run)
    , m_currentCharacter(0)
    , m_runWidthSoFar(0)
    , m_isAfterExpansion(!run.allowsLeadingExpansion())
    , m_finalRoundingWidth(0)
    , m_typesettingFeatures(font->typesettingFeatures())
    , m_fallbackFonts(fallbackFonts)
    , m_accountForGlyphBounds(accountForGlyphBounds)
    , m_maxGlyphBoundingBoxY(std::numeric_limits<float>::min())
    , m_minGlyphBoundingBoxY(std::numeric_limits<float>::max())
    , m_firstGlyphOverflow(0)
    , m_lastGlyphOverflow(0)
    , m_forTextEmphasis(forTextEmphasis)
{
    // If the padding is non-zero, count the number of spaces in the run
    // and divide that by the padding for per space addition.
    m_expansion = m_run.expansion();
    if (!m_expansion)
        m_expansionPerOpportunity = 0;
    else {
        bool isAfterExpansion = m_isAfterExpansion;
        unsigned expansionOpportunityCount = FontCascade::expansionOpportunityCount(m_run.text(), m_run.ltr() ? LTR : RTL, isAfterExpansion);
        if (isAfterExpansion && !m_run.allowsTrailingExpansion())
            expansionOpportunityCount--;

        if (!expansionOpportunityCount)
            m_expansionPerOpportunity = 0;
        else
            m_expansionPerOpportunity = m_expansion / expansionOpportunityCount;
    }
}

GlyphData WidthIterator::glyphDataForCharacter(UChar32 character, bool mirror, int currentCharacter, unsigned& advanceLength, String& normalizedSpacesStringCache)
{
    ASSERT(m_font);

#if ENABLE(SVG_FONTS)
    if (TextRun::RenderingContext* renderingContext = m_run.renderingContext())
        return renderingContext->glyphDataForCharacter(*m_font, *this, character, mirror, currentCharacter, advanceLength, normalizedSpacesStringCache);
#else
    UNUSED_PARAM(currentCharacter);
    UNUSED_PARAM(advanceLength);
    UNUSED_PARAM(normalizedSpacesStringCache);
#endif

    return m_font->glyphDataForCharacter(character, mirror);
}

struct OriginalAdvancesForCharacterTreatedAsSpace {
public:
    OriginalAdvancesForCharacterTreatedAsSpace(bool isSpace, float advanceBefore, float advanceAt)
        : characterIsSpace(isSpace)
        , advanceBeforeCharacter(advanceBefore)
        , advanceAtCharacter(advanceAt)
    {
    }

    bool characterIsSpace;
    float advanceBeforeCharacter;
    float advanceAtCharacter;
};

typedef Vector<std::pair<int, OriginalAdvancesForCharacterTreatedAsSpace>, 64> CharactersTreatedAsSpace;

static inline float applyFontTransforms(GlyphBuffer* glyphBuffer, bool ltr, int& lastGlyphCount, const Font* font, WidthIterator& iterator, TypesettingFeatures typesettingFeatures, CharactersTreatedAsSpace& charactersTreatedAsSpace)
{
    ASSERT(typesettingFeatures & (Kerning | Ligatures));

    if (!glyphBuffer)
        return 0;

    int glyphBufferSize = glyphBuffer->size();
    if (glyphBuffer->size() <= lastGlyphCount + 1) {
        lastGlyphCount = glyphBufferSize;
        return 0;
    }

    GlyphBufferAdvance* advances = glyphBuffer->advances(0);
    float widthDifference = 0;
    for (int i = lastGlyphCount; i < glyphBufferSize; ++i)
        widthDifference -= advances[i].width();

    if (!ltr)
        glyphBuffer->reverse(lastGlyphCount, glyphBufferSize - lastGlyphCount);

#if !ENABLE(SVG_FONTS)
    UNUSED_PARAM(iterator);
#else
    // We need to handle transforms on SVG fonts internally, since they are rendered internally.
    if (font->isSVGFont()) {
        // SVG font ligatures are handled during glyph selection, only kerning remaining.
        if (iterator.run().renderingContext() && (typesettingFeatures & Kerning)) {
            // FIXME: We could pass the necessary context down to this level so we can lazily create rendering contexts at this point.
            // However, a larger refactoring of SVG fonts might necessary to sidestep this problem completely.
            iterator.run().renderingContext()->applySVGKerning(font, iterator, glyphBuffer, lastGlyphCount);
        }
    } else
#endif
        font->applyTransforms(glyphBuffer->glyphs(lastGlyphCount), advances + lastGlyphCount, glyphBufferSize - lastGlyphCount, typesettingFeatures);

    if (!ltr)
        glyphBuffer->reverse(lastGlyphCount, glyphBufferSize - lastGlyphCount);

    for (size_t i = 0; i < charactersTreatedAsSpace.size(); ++i) {
        int spaceOffset = charactersTreatedAsSpace[i].first;
        const OriginalAdvancesForCharacterTreatedAsSpace& originalAdvances = charactersTreatedAsSpace[i].second;
        if (spaceOffset && !originalAdvances.characterIsSpace)
            glyphBuffer->advances(spaceOffset - 1)->setWidth(originalAdvances.advanceBeforeCharacter);
        glyphBuffer->advances(spaceOffset)->setWidth(originalAdvances.advanceAtCharacter);
    }
    charactersTreatedAsSpace.clear();

#if PLATFORM(MAC) || PLATFORM(IOS)
    // Workaround for <rdar://problem/20230073> FIXME: Please remove this when no longer needed.
    GlyphBufferGlyph* glyphs = glyphBuffer->glyphs(0);
    int filteredIndex = lastGlyphCount;
    for (int i = lastGlyphCount; i < glyphBufferSize; ++i) {
        glyphs[filteredIndex] = glyphs[i];
        advances[filteredIndex] = advances[i];
        if (glyphs[filteredIndex] != kCGFontIndexInvalid)
            ++filteredIndex;
    }
    glyphBufferSize = filteredIndex;
#endif

    for (int i = lastGlyphCount; i < glyphBufferSize; ++i)
        widthDifference += advances[i].width();

    lastGlyphCount = glyphBufferSize;
    return widthDifference;
}

template <typename TextIterator>
inline unsigned WidthIterator::advanceInternal(TextIterator& textIterator, GlyphBuffer* glyphBuffer)
{
    bool rtl = m_run.rtl();
    bool hasExtraSpacing = (m_font->letterSpacing() || m_font->wordSpacing() || m_expansion) && !m_run.spacingDisabled();

    float widthSinceLastRounding = m_runWidthSoFar;
    m_runWidthSoFar = floorf(m_runWidthSoFar);
    widthSinceLastRounding -= m_runWidthSoFar;

    float lastRoundingWidth = m_finalRoundingWidth;
    FloatRect bounds;

    const Font& primaryFont = m_font->primaryFont();
    const Font* lastFontData = &primaryFont;
    int lastGlyphCount = glyphBuffer ? glyphBuffer->size() : 0;

    UChar32 character = 0;
    unsigned clusterLength = 0;
    CharactersTreatedAsSpace charactersTreatedAsSpace;
    String normalizedSpacesStringCache;
    while (textIterator.consume(character, clusterLength)) {
        unsigned advanceLength = clusterLength;
        int currentCharacter = textIterator.currentCharacter();
        const GlyphData& glyphData = glyphDataForCharacter(character, rtl, currentCharacter, advanceLength, normalizedSpacesStringCache);
        Glyph glyph = glyphData.glyph;
        if (!glyph) {
            textIterator.advance(advanceLength);
            continue;
        }
        const Font* font = glyphData.font;
        ASSERT(font);

        // Now that we have a glyph and font data, get its width.
        float width;
        if (character == '\t' && m_run.allowTabs())
            width = m_font->tabWidth(*font, m_run.tabSize(), m_run.xPos() + m_runWidthSoFar + widthSinceLastRounding);
        else {
            width = font->widthForGlyph(glyph);

            // SVG uses horizontalGlyphStretch(), when textLength is used to stretch/squeeze text.
            width *= m_run.horizontalGlyphStretch();

            // We special case spaces in two ways when applying word rounding.
            // First, we round spaces to an adjusted width in all fonts.
            // Second, in fixed-pitch fonts we ensure that all characters that
            // match the width of the space character have the same width as the space character.
            if (m_run.applyWordRounding() && width == font->spaceWidth() && (font->pitch() == FixedPitch || glyph == font->spaceGlyph()))
                width = font->adjustedSpaceWidth();
        }

        if (font != lastFontData && width) {
            if (shouldApplyFontTransforms()) {
                m_runWidthSoFar += applyFontTransforms(glyphBuffer, m_run.ltr(), lastGlyphCount, lastFontData, *this, m_typesettingFeatures, charactersTreatedAsSpace);
                if (glyphBuffer)
                    glyphBuffer->shrink(lastGlyphCount);
            }

            lastFontData = font;
            if (m_fallbackFonts && font != &primaryFont) {
                // FIXME: This does a little extra work that could be avoided if
                // glyphDataForCharacter() returned whether it chose to use a small caps font.
                if (!m_font->isSmallCaps() || character == u_toupper(character))
                    m_fallbackFonts->add(font);
                else {
                    const GlyphData& uppercaseGlyphData = m_font->glyphDataForCharacter(u_toupper(character), rtl);
                    if (uppercaseGlyphData.font != &primaryFont)
                        m_fallbackFonts->add(uppercaseGlyphData.font);
                }
            }
        }

        if (hasExtraSpacing) {
            // Account for letter-spacing.
            if (width && m_font->letterSpacing())
                width += m_font->letterSpacing();

            static bool expandAroundIdeographs = FontCascade::canExpandAroundIdeographsInComplexText();
            bool treatAsSpace = FontCascade::treatAsSpace(character);
            if (treatAsSpace || (expandAroundIdeographs && FontCascade::isCJKIdeographOrSymbol(character))) {
                // Distribute the run's total expansion evenly over all expansion opportunities in the run.
                if (m_expansion) {
                    float previousExpansion = m_expansion;
                    if (!treatAsSpace && !m_isAfterExpansion) {
                        // Take the expansion opportunity before this ideograph.
                        m_expansion -= m_expansionPerOpportunity;
                        float expansionAtThisOpportunity = !m_run.applyWordRounding() ? m_expansionPerOpportunity : roundf(previousExpansion) - roundf(m_expansion);
                        m_runWidthSoFar += expansionAtThisOpportunity;
                        if (glyphBuffer) {
                            if (glyphBuffer->isEmpty()) {
                                if (m_forTextEmphasis)
                                    glyphBuffer->add(font->zeroWidthSpaceGlyph(), font, m_expansionPerOpportunity, currentCharacter);
                                else
                                    glyphBuffer->add(font->spaceGlyph(), font, expansionAtThisOpportunity, currentCharacter);
                            } else
                                glyphBuffer->expandLastAdvance(expansionAtThisOpportunity);
                        }
                        previousExpansion = m_expansion;
                    }
                    if (m_run.allowsTrailingExpansion() || (m_run.ltr() && currentCharacter + advanceLength < static_cast<size_t>(m_run.length()))
                        || (m_run.rtl() && currentCharacter)) {
                        m_expansion -= m_expansionPerOpportunity;
                        width += !m_run.applyWordRounding() ? m_expansionPerOpportunity : roundf(previousExpansion) - roundf(m_expansion);
                        m_isAfterExpansion = true;
                    }
                } else
                    m_isAfterExpansion = false;

                // Account for word spacing.
                // We apply additional space between "words" by adding width to the space character.
                if (treatAsSpace && (character != '\t' || !m_run.allowTabs()) && (currentCharacter || character == noBreakSpace) && m_font->wordSpacing())
                    width += m_font->wordSpacing();
            } else
                m_isAfterExpansion = false;
        }

        if (shouldApplyFontTransforms() && glyphBuffer && FontCascade::treatAsSpace(character)) {
            charactersTreatedAsSpace.append(std::make_pair(glyphBuffer->size(),
                OriginalAdvancesForCharacterTreatedAsSpace(character == ' ', glyphBuffer->size() ? glyphBuffer->advanceAt(glyphBuffer->size() - 1).width() : 0, width)));
        }

        if (m_accountForGlyphBounds) {
            bounds = font->boundsForGlyph(glyph);
            if (!currentCharacter)
                m_firstGlyphOverflow = std::max<float>(0, -bounds.x());
        }

        if (m_forTextEmphasis && !FontCascade::canReceiveTextEmphasis(character))
            glyph = 0;

        // Advance past the character we just dealt with.
        textIterator.advance(advanceLength);

        float oldWidth = width;

        // Force characters that are used to determine word boundaries for the rounding hack
        // to be integer width, so following words will start on an integer boundary.
        if (m_run.applyWordRounding() && FontCascade::isRoundingHackCharacter(character)) {
            width = ceilf(width);

            // Since widthSinceLastRounding can lose precision if we include measurements for
            // preceding whitespace, we bypass it here.
            m_runWidthSoFar += width;

            // Since this is a rounding hack character, we should have reset this sum on the previous
            // iteration.
            ASSERT(!widthSinceLastRounding);
        } else {
            // Check to see if the next character is a "rounding hack character", if so, adjust
            // width so that the total run width will be on an integer boundary.
            if ((m_run.applyWordRounding() && static_cast<unsigned>(textIterator.currentCharacter()) < m_run.length() && FontCascade::isRoundingHackCharacter(*(textIterator.characters())))
                || (m_run.applyRunRounding() && static_cast<unsigned>(textIterator.currentCharacter()) >= m_run.length())) {
                float totalWidth = widthSinceLastRounding + width;
                widthSinceLastRounding = ceilf(totalWidth);
                width += widthSinceLastRounding - totalWidth;
                m_runWidthSoFar += widthSinceLastRounding;
                widthSinceLastRounding = 0;
            } else
                widthSinceLastRounding += width;
        }

        if (glyphBuffer)
            glyphBuffer->add(glyph, font, (rtl ? oldWidth + lastRoundingWidth : width), currentCharacter);

        lastRoundingWidth = width - oldWidth;

        if (m_accountForGlyphBounds) {
            m_maxGlyphBoundingBoxY = std::max(m_maxGlyphBoundingBoxY, bounds.maxY());
            m_minGlyphBoundingBoxY = std::min(m_minGlyphBoundingBoxY, bounds.y());
            m_lastGlyphOverflow = std::max<float>(0, bounds.maxX() - width);
        }
    }

    if (shouldApplyFontTransforms()) {
        m_runWidthSoFar += applyFontTransforms(glyphBuffer, m_run.ltr(), lastGlyphCount, lastFontData, *this, m_typesettingFeatures, charactersTreatedAsSpace);
        if (glyphBuffer)
            glyphBuffer->shrink(lastGlyphCount);
    }

    unsigned consumedCharacters = textIterator.currentCharacter() - m_currentCharacter;
    m_currentCharacter = textIterator.currentCharacter();
    m_runWidthSoFar += widthSinceLastRounding;
    m_finalRoundingWidth = lastRoundingWidth;
    return consumedCharacters;
}

unsigned WidthIterator::advance(int offset, GlyphBuffer* glyphBuffer)
{
    int length = m_run.length();

    if (offset > length)
        offset = length;

    if (m_currentCharacter >= static_cast<unsigned>(offset))
        return 0;

    if (m_run.is8Bit()) {
        Latin1TextIterator textIterator(m_run.data8(m_currentCharacter), m_currentCharacter, offset, length);
        return advanceInternal(textIterator, glyphBuffer);
    }

    SurrogatePairAwareTextIterator textIterator(m_run.data16(m_currentCharacter), m_currentCharacter, offset, length);
    return advanceInternal(textIterator, glyphBuffer);
}

bool WidthIterator::advanceOneCharacter(float& width, GlyphBuffer& glyphBuffer)
{
    int oldSize = glyphBuffer.size();
    advance(m_currentCharacter + 1, &glyphBuffer);
    float w = 0;
    for (int i = oldSize; i < glyphBuffer.size(); ++i)
        w += glyphBuffer.advanceAt(i).width();
    width = w;
    return glyphBuffer.size() > oldSize;
}

}
