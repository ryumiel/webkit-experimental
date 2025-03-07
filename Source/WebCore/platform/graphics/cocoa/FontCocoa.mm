/*
 * Copyright (C) 2005, 2006, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
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

#import "config.h"
#import "Font.h"

#import "BlockExceptions.h"
#import "Color.h"
#import "CoreGraphicsSPI.h"
#import "CoreTextSPI.h"
#import "FloatRect.h"
#import "FontCache.h"
#import "FontCascade.h"
#import "FontDescription.h"
#import "SharedBuffer.h"
#import "WebCoreSystemInterface.h"
#import <float.h>
#import <unicode/uchar.h>
#import <wtf/Assertions.h>
#import <wtf/RetainPtr.h>
#import <wtf/StdLibExtras.h>

#if USE(APPKIT)
#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#else
#import "FontServicesIOS.h"
#import <CoreText/CoreText.h>
#endif

#if USE(APPKIT)
@interface NSFont (WebAppKitSecretAPI)
- (BOOL)_isFakeFixedPitch;
@end
#endif

namespace WebCore {

static bool fontHasVerticalGlyphs(CTFontRef ctFont)
{
    // The check doesn't look neat but this is what AppKit does for vertical writing...
    RetainPtr<CFArrayRef> tableTags = adoptCF(CTFontCopyAvailableTables(ctFont, kCTFontTableOptionNoOptions));
    CFIndex numTables = CFArrayGetCount(tableTags.get());
    for (CFIndex index = 0; index < numTables; ++index) {
        CTFontTableTag tag = (CTFontTableTag)(uintptr_t)CFArrayGetValueAtIndex(tableTags.get(), index);
        if (tag == kCTFontTableVhea || tag == kCTFontTableVORG)
            return true;
    }
    return false;
}

#if USE(APPKIT)
static NSString *webFallbackFontFamily(void)
{
    static NSString *webFallbackFontFamily = [[[NSFont systemFontOfSize:16.0f] familyName] retain];
    return webFallbackFontFamily;
}
#else
static bool fontFamilyShouldNotBeUsedForArabic(CFStringRef fontFamilyName)
{
    if (!fontFamilyName)
        return false;

    // Times New Roman contains Arabic glyphs, but Core Text doesn't know how to shape them. <rdar://problem/9823975>
    // FIXME <rdar://problem/12096835> remove this function once the above bug is fixed.
    // Arial and Tahoma are have performance issues so don't use them as well.
    return (CFStringCompare(CFSTR("Times New Roman"), fontFamilyName, 0) == kCFCompareEqualTo)
        || (CFStringCompare(CFSTR("Arial"), fontFamilyName, 0) == kCFCompareEqualTo)
        || (CFStringCompare(CFSTR("Tahoma"), fontFamilyName, 0) == kCFCompareEqualTo);
}
#endif

void Font::platformInit()
{
#if USE(APPKIT)
    m_syntheticBoldOffset = m_platformData.m_syntheticBold ? 1.0f : 0.f;

    bool failedSetup = false;
    if (!platformData().cgFont()) {
        // Ack! Something very bad happened, like a corrupt font.
        // Try looking for an alternate 'base' font for this renderer.

        // Special case hack to use "Times New Roman" in place of "Times".
        // "Times RO" is a common font whose family name is "Times".
        // It overrides the normal "Times" family font.
        // It also appears to have a corrupt regular variant.
        NSString *fallbackFontFamily;
        if ([[m_platformData.nsFont() familyName] isEqual:@"Times"])
            fallbackFontFamily = @"Times New Roman";
        else
            fallbackFontFamily = webFallbackFontFamily();
        
        // Try setting up the alternate font.
        // This is a last ditch effort to use a substitute font when something has gone wrong.
#if !ERROR_DISABLED
        RetainPtr<NSFont> initialFont = m_platformData.nsFont();
#endif
        if (m_platformData.font())
            m_platformData.setNSFont([[NSFontManager sharedFontManager] convertFont:m_platformData.nsFont() toFamily:fallbackFontFamily]);
        else
            m_platformData.setNSFont([NSFont fontWithName:fallbackFontFamily size:m_platformData.size()]);
        if (!platformData().cgFont()) {
            if ([fallbackFontFamily isEqual:@"Times New Roman"]) {
                // OK, couldn't setup Times New Roman as an alternate to Times, fallback
                // on the system font.  If this fails we have no alternative left.
                m_platformData.setNSFont([[NSFontManager sharedFontManager] convertFont:m_platformData.nsFont() toFamily:webFallbackFontFamily()]);
                if (!platformData().cgFont()) {
                    // We tried, Times, Times New Roman, and the system font. No joy. We have to give up.
                    LOG_ERROR("unable to initialize with font %@", initialFont.get());
                    failedSetup = true;
                }
            } else {
                // We tried the requested font and the system font. No joy. We have to give up.
                LOG_ERROR("unable to initialize with font %@", initialFont.get());
                failedSetup = true;
            }
        }

        // Report the problem.
        LOG_ERROR("Corrupt font detected, using %@ in place of %@.",
            [m_platformData.nsFont() familyName], [initialFont.get() familyName]);
    }

    // If all else fails, try to set up using the system font.
    // This is probably because Times and Times New Roman are both unavailable.
    if (failedSetup) {
        m_platformData.setNSFont([NSFont systemFontOfSize:[m_platformData.nsFont() pointSize]]);
        LOG_ERROR("failed to set up font, using system font %s", m_platformData.font());
    }

    // Work around <rdar://problem/19433490>
    CGGlyph dummyGlyphs[] = {0, 0};
    CGSize dummySize[] = { CGSizeMake(0, 0), CGSizeMake(0, 0) };
    CTFontTransformGlyphs(m_platformData.ctFont(), dummyGlyphs, dummySize, 2, kCTFontTransformApplyPositioning | kCTFontTransformApplyShaping);
    
    int iAscent;
    int iDescent;
    int iCapHeight;
    int iLineGap;
    unsigned unitsPerEm;
    iAscent = CGFontGetAscent(m_platformData.cgFont());
    // Some fonts erroneously specify a positive descender value. We follow Core Text in assuming that
    // such fonts meant the same distance, but in the reverse direction.
    iDescent = -abs(CGFontGetDescent(m_platformData.cgFont()));
    iCapHeight = CGFontGetCapHeight(m_platformData.cgFont());
    iLineGap = CGFontGetLeading(m_platformData.cgFont());
    unitsPerEm = CGFontGetUnitsPerEm(m_platformData.cgFont());

    float pointSize = m_platformData.m_size;
    float ascent = scaleEmToUnits(iAscent, unitsPerEm) * pointSize;
    float descent = -scaleEmToUnits(iDescent, unitsPerEm) * pointSize;
    float capHeight = scaleEmToUnits(iCapHeight, unitsPerEm) * pointSize;
    
    float lineGap = scaleEmToUnits(iLineGap, unitsPerEm) * pointSize;

    // We need to adjust Times, Helvetica, and Courier to closely match the
    // vertical metrics of their Microsoft counterparts that are the de facto
    // web standard. The AppKit adjustment of 20% is too big and is
    // incorrectly added to line spacing, so we use a 15% adjustment instead
    // and add it to the ascent.
    NSString *familyName = [m_platformData.nsFont() familyName];
    if ([familyName isEqualToString:@"Times"] || [familyName isEqualToString:@"Helvetica"] || [familyName isEqualToString:@"Courier"])
        ascent += floorf(((ascent + descent) * 0.15f) + 0.5f);

    // Compute and store line spacing, before the line metrics hacks are applied.
    m_fontMetrics.setLineSpacing(lroundf(ascent) + lroundf(descent) + lroundf(lineGap));

    // Hack Hiragino line metrics to allow room for marked text underlines.
    // <rdar://problem/5386183>
    if (descent < 3 && lineGap >= 3 && [familyName hasPrefix:@"Hiragino"]) {
        lineGap -= 3 - descent;
        descent = 3;
    }
    
    if (platformData().orientation() == Vertical && !isTextOrientationFallback())
        m_hasVerticalGlyphs = fontHasVerticalGlyphs(m_platformData.ctFont());

    float xHeight;

    if (platformData().orientation() == Horizontal) {
        // Measure the actual character "x", since it's possible for it to extend below the baseline, and we need the
        // reported x-height to only include the portion of the glyph that is above the baseline.
        NSGlyph xGlyph = glyphForCharacter('x');
        if (xGlyph)
            xHeight = -CGRectGetMinY(platformBoundsForGlyph(xGlyph));
        else
            xHeight = scaleEmToUnits(CGFontGetXHeight(m_platformData.cgFont()), unitsPerEm) * pointSize;
    } else
        xHeight = verticalRightOrientationFont()->fontMetrics().xHeight();

    m_fontMetrics.setUnitsPerEm(unitsPerEm);
    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setCapHeight(capHeight);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setXHeight(xHeight);
#else
    m_syntheticBoldOffset = m_platformData.m_syntheticBold ? ceilf(m_platformData.size()  / 24.0f) : 0.f;
    m_spaceGlyph = 0;
    m_spaceWidth = 0;
    unsigned unitsPerEm;
    float ascent;
    float descent;
    float capHeight;
    float lineGap;
    float lineSpacing;
    float xHeight;
    RetainPtr<CFStringRef> familyName;
    if (CTFontRef ctFont = m_platformData.font()) {
        FontServicesIOS fontService(ctFont);
        ascent = ceilf(fontService.ascent());
        descent = ceilf(fontService.descent());
        lineSpacing = fontService.lineSpacing();
        lineGap = fontService.lineGap();
        xHeight = fontService.xHeight();
        capHeight = fontService.capHeight();
        unitsPerEm = fontService.unitsPerEm();
        familyName = adoptCF(CTFontCopyFamilyName(ctFont));
    } else {
        CGFontRef cgFont = m_platformData.cgFont();

        unitsPerEm = CGFontGetUnitsPerEm(cgFont);

        float pointSize = m_platformData.size();
        ascent = lroundf(scaleEmToUnits(CGFontGetAscent(cgFont), unitsPerEm) * pointSize);
        descent = lroundf(-scaleEmToUnits(-abs(CGFontGetDescent(cgFont)), unitsPerEm) * pointSize);
        lineGap = lroundf(scaleEmToUnits(CGFontGetLeading(cgFont), unitsPerEm) * pointSize);
        xHeight = scaleEmToUnits(CGFontGetXHeight(cgFont), unitsPerEm) * pointSize;
        capHeight = scaleEmToUnits(CGFontGetCapHeight(cgFont), unitsPerEm) * pointSize;

        lineSpacing = ascent + descent + lineGap;
        familyName = adoptCF(CGFontCopyFamilyName(cgFont));
    }

    m_fontMetrics.setUnitsPerEm(unitsPerEm);
    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setLineSpacing(lineSpacing);
    m_fontMetrics.setXHeight(xHeight);
    m_fontMetrics.setCapHeight(capHeight);
    m_shouldNotBeUsedForArabic = fontFamilyShouldNotBeUsedForArabic(familyName.get());

    if (platformData().orientation() == Vertical && !isTextOrientationFallback())
        m_hasVerticalGlyphs = fontHasVerticalGlyphs(m_platformData.ctFont());

    if (!m_platformData.m_isEmoji)
        return;

    int thirdOfSize = m_platformData.size() / 3;
    m_fontMetrics.setAscent(thirdOfSize);
    m_fontMetrics.setDescent(thirdOfSize);
    m_fontMetrics.setLineGap(thirdOfSize);
    m_fontMetrics.setLineSpacing(0);
#endif
}

void Font::platformCharWidthInit()
{
    m_avgCharWidth = 0;
    m_maxCharWidth = 0;
    
#if PLATFORM(MAC)
    RetainPtr<CFDataRef> os2Table = adoptCF(CGFontCopyTableForTag(m_platformData.cgFont(), 'OS/2'));
    if (os2Table && CFDataGetLength(os2Table.get()) >= 4) {
        const UInt8* os2 = CFDataGetBytePtr(os2Table.get());
        SInt16 os2AvgCharWidth = os2[2] * 256 + os2[3];
        m_avgCharWidth = scaleEmToUnits(os2AvgCharWidth, m_fontMetrics.unitsPerEm()) * m_platformData.m_size;
    }

    RetainPtr<CFDataRef> headTable = adoptCF(CGFontCopyTableForTag(m_platformData.cgFont(), 'head'));
    if (headTable && CFDataGetLength(headTable.get()) >= 42) {
        const UInt8* head = CFDataGetBytePtr(headTable.get());
        ushort uxMin = head[36] * 256 + head[37];
        ushort uxMax = head[40] * 256 + head[41];
        SInt16 xMin = static_cast<SInt16>(uxMin);
        SInt16 xMax = static_cast<SInt16>(uxMax);
        float diff = static_cast<float>(xMax - xMin);
        m_maxCharWidth = scaleEmToUnits(diff, m_fontMetrics.unitsPerEm()) * m_platformData.m_size;
    }
#endif

    // Fallback to a cross-platform estimate, which will populate these values if they are non-positive.
    initCharWidths();
}

void Font::platformDestroy()
{
}

PassRefPtr<Font> Font::platformCreateScaledFont(const FontDescription&, float scaleFactor) const
{
    if (isCustomFont()) {
        FontPlatformData scaledFontData(m_platformData);
        scaledFontData.m_size = scaledFontData.m_size * scaleFactor;
        return Font::create(scaledFontData, true, false);
    }

    float size = m_platformData.size() * scaleFactor;

#if USE(APPKIT)
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    FontPlatformData scaledFontData([[NSFontManager sharedFontManager] convertFont:m_platformData.nsFont() toSize:size], size, false, false, m_platformData.orientation());

    if (scaledFontData.font()) {
        NSFontManager *fontManager = [NSFontManager sharedFontManager];
        NSFontTraitMask fontTraits = [fontManager traitsOfFont:m_platformData.nsFont()];

        if (m_platformData.m_syntheticBold)
            fontTraits |= NSBoldFontMask;
        if (m_platformData.m_syntheticOblique)
            fontTraits |= NSItalicFontMask;

        NSFontTraitMask scaledFontTraits = [fontManager traitsOfFont:scaledFontData.nsFont()];
        scaledFontData.m_syntheticBold = (fontTraits & NSBoldFontMask) && !(scaledFontTraits & NSBoldFontMask);
        scaledFontData.m_syntheticOblique = (fontTraits & NSItalicFontMask) && !(scaledFontTraits & NSItalicFontMask);

        return Font::create(scaledFontData);
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return nullptr;
#else
    CTFontSymbolicTraits fontTraits = CTFontGetSymbolicTraits(m_platformData.font());
    RetainPtr<CTFontDescriptorRef> fontDescriptor = adoptCF(CTFontCopyFontDescriptor(m_platformData.font()));
    RetainPtr<CTFontRef> scaledFont = adoptCF(CTFontCreateWithFontDescriptor(fontDescriptor.get(), size, nullptr));
    FontPlatformData scaledFontData(scaledFont.get(), size, false, false, m_platformData.orientation());

    if (scaledFontData.font()) {
        if (m_platformData.m_syntheticBold)
            fontTraits |= kCTFontBoldTrait;
        if (m_platformData.m_syntheticOblique)
            fontTraits |= kCTFontItalicTrait;

        CTFontSymbolicTraits scaledFontTraits = CTFontGetSymbolicTraits(scaledFontData.font());
        scaledFontData.m_syntheticBold = (fontTraits & kCTFontBoldTrait) && !(scaledFontTraits & kCTFontTraitBold);
        scaledFontData.m_syntheticOblique = (fontTraits & kCTFontItalicTrait) && !(scaledFontTraits & kCTFontTraitItalic);

        return Font::create(scaledFontData);
    }

    return nullptr;
#endif
}

void Font::determinePitch()
{
#if USE(APPKIT)
    NSFont* f = m_platformData.nsFont();
    // Special case Osaka-Mono.
    // According to <rdar://problem/3999467>, we should treat Osaka-Mono as fixed pitch.
    // Note that the AppKit does not report Osaka-Mono as fixed pitch.

    // Special case MS-PGothic.
    // According to <rdar://problem/4032938>, we should not treat MS-PGothic as fixed pitch.
    // Note that AppKit does report MS-PGothic as fixed pitch.

    // Special case MonotypeCorsiva
    // According to <rdar://problem/5454704>, we should not treat MonotypeCorsiva as fixed pitch.
    // Note that AppKit does report MonotypeCorsiva as fixed pitch.

    NSString *name = [f fontName];
    m_treatAsFixedPitch = ([f isFixedPitch]  || [f _isFakeFixedPitch] || [name caseInsensitiveCompare:@"Osaka-Mono"] == NSOrderedSame) && [name caseInsensitiveCompare:@"MS-PGothic"] != NSOrderedSame && [name caseInsensitiveCompare:@"MonotypeCorsiva"] != NSOrderedSame;
#else
    CTFontRef ctFont = m_platformData.font();
    m_treatAsFixedPitch = false;
    if (!ctFont)
        return; // CTFont is null in the case of SVG fonts for example.

    RetainPtr<CFStringRef> fullName = adoptCF(CTFontCopyFullName(ctFont));
    RetainPtr<CFStringRef> familyName = adoptCF(CTFontCopyFamilyName(ctFont));

    m_treatAsFixedPitch = CGFontIsFixedPitch(m_platformData.cgFont()) || (fullName && (CFStringCompare(fullName.get(), CFSTR("Osaka-Mono"), kCFCompareCaseInsensitive) == kCFCompareEqualTo || CFStringCompare(fullName.get(), CFSTR("MS-PGothic"), kCFCompareCaseInsensitive) == kCFCompareEqualTo));
    if (familyName && CFStringCompare(familyName.get(), CFSTR("Courier New"), kCFCompareCaseInsensitive) == kCFCompareEqualTo) {
        // Special case Courier New to not be treated as fixed pitch, as this will make use of a hacked space width which is undesireable for iPhone (see rdar://6269783).
        m_treatAsFixedPitch = false;
    }
#endif
}

FloatRect Font::platformBoundsForGlyph(Glyph glyph) const
{
    FloatRect boundingBox;
    boundingBox = CTFontGetBoundingRectsForGlyphs(m_platformData.ctFont(), platformData().orientation() == Vertical ? kCTFontOrientationVertical : kCTFontOrientationHorizontal, &glyph, 0, 1);
    boundingBox.setY(-boundingBox.maxY());
    if (m_syntheticBoldOffset)
        boundingBox.setWidth(boundingBox.width() + m_syntheticBoldOffset);

    return boundingBox;
}

static inline CGFontRenderingStyle renderingStyle(const FontPlatformData& platformData)
{
#if USE(APPKIT)
    CGFontRenderingStyle style = kCGFontRenderingStyleAntialiasing | kCGFontRenderingStyleSubpixelPositioning | kCGFontRenderingStyleSubpixelQuantization;
    NSFont *font = platformData.nsFont();
    if (font) {
        switch ([font renderingMode]) {
        case NSFontIntegerAdvancementsRenderingMode:
            style = 0;
            break;
        case NSFontAntialiasedIntegerAdvancementsRenderingMode:
            style = kCGFontRenderingStyleAntialiasing;
            break;
        default:
            break;
        }
    }
    return style;

#else
    UNUSED_PARAM(platformData);
    return kCGFontRenderingStyleAntialiasing | kCGFontRenderingStyleSubpixelPositioning | kCGFontRenderingStyleSubpixelQuantization | kCGFontAntialiasingStyleUnfiltered;
#endif
}

static inline bool advanceForColorBitmapFont(const FontPlatformData& platformData, Glyph glyph, CGSize& advance)
{
#if PLATFORM(MAC)
    NSFont *font = platformData.nsFont();
    if (!font || !platformData.isColorBitmapFont())
        return false;
    advance = NSSizeToCGSize([font advancementForGlyph:glyph]);
    return true;
#else
    UNUSED_PARAM(platformData);
    UNUSED_PARAM(glyph);
    UNUSED_PARAM(advance);
    return false;
#endif
}

static bool hasCustomTracking(CTFontRef font)
{
    return CTFontDescriptorIsSystemUIFont(adoptCF(CTFontCopyFontDescriptor(font)).get());
}

static inline bool isEmoji(const FontPlatformData& platformData)
{
#if PLATFORM(IOS)
    return platformData.m_isEmoji;
#else
    UNUSED_PARAM(platformData);
    return false;
#endif
}

static inline bool canUseFastGlyphAdvanceGetter(const FontPlatformData& platformData, Glyph glyph, CGSize& advance, bool& populatedAdvance)
{
    // Fast getter doesn't take custom tracking into account
    if (hasCustomTracking(platformData.ctFont()))
        return false;
    // Fast getter doesn't work for emoji
    if (isEmoji(platformData))
        return false;
    // ... or for any bitmap fonts in general
    if (advanceForColorBitmapFont(platformData, glyph, advance)) {
        populatedAdvance = true;
        return false;
    }
    return true;
}

float Font::platformWidthForGlyph(Glyph glyph) const
{
    CGSize advance = CGSizeZero;
    bool horizontal = platformData().orientation() == Horizontal;
    bool populatedAdvance = false;
    if ((horizontal || m_isBrokenIdeographFallback) && canUseFastGlyphAdvanceGetter(platformData(), glyph, advance, populatedAdvance)) {
        float pointSize = platformData().m_size;
        CGAffineTransform m = CGAffineTransformMakeScale(pointSize, pointSize);
        if (!CGFontGetGlyphAdvancesForStyle(platformData().cgFont(), &m, renderingStyle(platformData()), &glyph, 1, &advance)) {
            RetainPtr<CFStringRef> fullName = adoptCF(CGFontCopyFullName(platformData().cgFont()));
            LOG_ERROR("Unable to cache glyph widths for %@ %f", fullName.get(), pointSize);
            advance.width = 0;
        }
    } else if (!populatedAdvance)
        CTFontGetAdvancesForGlyphs(m_platformData.ctFont(), horizontal ? kCTFontOrientationHorizontal : kCTFontOrientationVertical, &glyph, &advance, 1);

    return advance.width + m_syntheticBoldOffset;
}

struct ProviderInfo {
    const UChar* characters;
    size_t length;
    CFDictionaryRef attributes;
};

static const UniChar* provideStringAndAttributes(CFIndex stringIndex, CFIndex* count, CFDictionaryRef* attributes, void* context)
{
    ProviderInfo* info = static_cast<struct ProviderInfo*>(context);
    if (stringIndex < 0 || static_cast<size_t>(stringIndex) >= info->length)
        return 0;

    *count = info->length - stringIndex;
    *attributes = info->attributes;
    return info->characters + stringIndex;
}

bool Font::canRenderCombiningCharacterSequence(const UChar* characters, size_t length) const
{
    ASSERT(isMainThread());

    if (!m_combiningCharacterSequenceSupport)
        m_combiningCharacterSequenceSupport = std::make_unique<HashMap<String, bool>>();

    WTF::HashMap<String, bool>::AddResult addResult = m_combiningCharacterSequenceSupport->add(String(characters, length), false);
    if (!addResult.isNewEntry)
        return addResult.iterator->value;

    RetainPtr<CFTypeRef> fontEqualityObject = platformData().objectForEqualityCheck();

    ProviderInfo info = { characters, length, getCFStringAttributes(0, platformData().orientation()) };
    RetainPtr<CTLineRef> line = adoptCF(CTLineCreateWithUniCharProvider(&provideStringAndAttributes, 0, &info));

    CFArrayRef runArray = CTLineGetGlyphRuns(line.get());
    CFIndex runCount = CFArrayGetCount(runArray);

    for (CFIndex r = 0; r < runCount; r++) {
        CTRunRef ctRun = static_cast<CTRunRef>(CFArrayGetValueAtIndex(runArray, r));
        ASSERT(CFGetTypeID(ctRun) == CTRunGetTypeID());
        CFDictionaryRef runAttributes = CTRunGetAttributes(ctRun);
        CTFontRef runFont = static_cast<CTFontRef>(CFDictionaryGetValue(runAttributes, kCTFontAttributeName));
        if (!CFEqual(fontEqualityObject.get(), FontPlatformData::objectForEqualityCheck(runFont).get()))
            return false;
    }

    addResult.iterator->value = true;
    return true;
}

#if USE(APPKIT)
const Font* Font::compositeFontReferenceFont(NSFont *key) const
{
    if (!key || CFEqual(adoptCF(CTFontCopyPostScriptName(CTFontRef(key))).get(), CFSTR("LastResort")))
        return nullptr;

    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());

    auto addResult = m_derivedFontData->compositeFontReferences.add(key, nullptr);
    if (addResult.isNewEntry) {
        NSFont *substituteFont = [key printerFont];

        CTFontSymbolicTraits traits = CTFontGetSymbolicTraits((CTFontRef)substituteFont);
        bool syntheticBold = platformData().syntheticBold() && !(traits & kCTFontBoldTrait);
        bool syntheticOblique = platformData().syntheticOblique() && !(traits & kCTFontItalicTrait);

        FontPlatformData substitutePlatform(substituteFont, platformData().size(), syntheticBold, syntheticOblique, platformData().orientation(), platformData().widthVariant());
        addResult.iterator->value = Font::create(substitutePlatform, isCustomFont());
    }
    return addResult.iterator->value.get();
}
#endif

} // namespace WebCore
