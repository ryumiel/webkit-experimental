/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2005-2010, 2013-2014 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef StringImpl_h
#define StringImpl_h

#include <limits.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <wtf/ASCIICType.h>
#include <wtf/Forward.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringHasher.h>
#include <wtf/Vector.h>
#include <wtf/text/ConversionMode.h>

#if USE(CF)
typedef const struct __CFString * CFStringRef;
#endif

#ifdef __OBJC__
@class NSString;
#endif

namespace JSC {
namespace LLInt { class Data; }
class LLIntOffsetsExtractor;
}

namespace WTF {

struct CStringTranslator;
template<typename CharacterType> struct HashAndCharactersTranslator;
struct HashAndUTF8CharactersTranslator;
struct LCharBufferTranslator;
struct CharBufferFromLiteralDataTranslator;
struct SubstringTranslator;
struct UCharBufferTranslator;
template<typename> class RetainPtr;

enum TextCaseSensitivity {
    TextCaseSensitive,
    TextCaseInsensitive
};

typedef bool (*CharacterMatchFunctionPtr)(UChar);
typedef bool (*IsWhiteSpaceFunctionPtr)(UChar);

// Define STRING_STATS to 1 turn on run time statistics of string sizes and memory usage
#define STRING_STATS 0

#if STRING_STATS
struct StringStats {
    inline void add8BitString(unsigned length, bool isSubString = false)
    {
        ++m_totalNumberStrings;
        ++m_number8BitStrings;
        if (!isSubString)
            m_total8BitData += length;
    }

    inline void add16BitString(unsigned length, bool isSubString = false)
    {
        ++m_totalNumberStrings;
        ++m_number16BitStrings;
        if (!isSubString)
            m_total16BitData += length;
    }

    void removeString(StringImpl&);
    void printStats();

    static const unsigned s_printStringStatsFrequency = 5000;
    static std::atomic<unsigned> s_stringRemovesTillPrintStats;

    std::atomic<unsigned> m_refCalls;
    std::atomic<unsigned> m_derefCalls;

    std::atomic<unsigned> m_totalNumberStrings;
    std::atomic<unsigned> m_number8BitStrings;
    std::atomic<unsigned> m_number16BitStrings;
    std::atomic<unsigned long long> m_total8BitData;
    std::atomic<unsigned long long> m_total16BitData;
};

#define STRING_STATS_ADD_8BIT_STRING(length) StringImpl::stringStats().add8BitString(length)
#define STRING_STATS_ADD_8BIT_STRING2(length, isSubString) StringImpl::stringStats().add8BitString(length, isSubString)
#define STRING_STATS_ADD_16BIT_STRING(length) StringImpl::stringStats().add16BitString(length)
#define STRING_STATS_ADD_16BIT_STRING2(length, isSubString) StringImpl::stringStats().add16BitString(length, isSubString)
#define STRING_STATS_REMOVE_STRING(string) StringImpl::stringStats().removeString(string)
#define STRING_STATS_REF_STRING(string) ++StringImpl::stringStats().m_refCalls;
#define STRING_STATS_DEREF_STRING(string) ++StringImpl::stringStats().m_derefCalls;
#else
#define STRING_STATS_ADD_8BIT_STRING(length) ((void)0)
#define STRING_STATS_ADD_8BIT_STRING2(length, isSubString) ((void)0)
#define STRING_STATS_ADD_16BIT_STRING(length) ((void)0)
#define STRING_STATS_ADD_16BIT_STRING2(length, isSubString) ((void)0)
#define STRING_STATS_ADD_UPCONVERTED_STRING(length) ((void)0)
#define STRING_STATS_REMOVE_STRING(string) ((void)0)
#define STRING_STATS_REF_STRING(string) ((void)0)
#define STRING_STATS_DEREF_STRING(string) ((void)0)
#endif

class StringImpl {
    WTF_MAKE_NONCOPYABLE(StringImpl); WTF_MAKE_FAST_ALLOCATED;
    friend struct WTF::CStringTranslator;
    template<typename CharacterType> friend struct WTF::HashAndCharactersTranslator;
    friend struct WTF::HashAndUTF8CharactersTranslator;
    friend struct WTF::CharBufferFromLiteralDataTranslator;
    friend struct WTF::LCharBufferTranslator;
    friend struct WTF::SubstringTranslator;
    friend struct WTF::UCharBufferTranslator;
    friend class AtomicStringImpl;
    friend class JSC::LLInt::Data;
    friend class JSC::LLIntOffsetsExtractor;
    
private:
    enum BufferOwnership {
        BufferInternal,
        BufferOwned,
        BufferSubstring,
    };

    // The bottom 6 bits in the hash are flags.
    static const unsigned s_flagCount = 6;
    static const unsigned s_flagMask = (1u << s_flagCount) - 1;
    COMPILE_ASSERT(s_flagCount <= StringHasher::flagCount, StringHasher_reserves_enough_bits_for_StringImpl_flags);
    static const unsigned s_flagStringKindCount = 4;

    static const unsigned s_hashFlagStringKindIsAtomic = 1u << (s_flagStringKindCount);
    static const unsigned s_hashFlagStringKindIsSymbol = 1u << (s_flagStringKindCount + 1);
    static const unsigned s_hashMaskStringKind = s_hashFlagStringKindIsAtomic | s_hashFlagStringKindIsSymbol;
    static const unsigned s_hashFlag8BitBuffer = 1u << 3;
    static const unsigned s_hashFlagDidReportCost = 1u << 2;
    static const unsigned s_hashMaskBufferOwnership = (1u << 0) | (1u << 1);

    enum StringKind {
        StringNormal = 0u, // non-symbol, non-atomic
        StringAtomic = s_hashFlagStringKindIsAtomic, // non-symbol, atomic
        StringSymbol = s_hashFlagStringKindIsSymbol, // symbol, non-atomic
    };

    // Used to construct static strings, which have an special refCount that can never hit zero.
    // This means that the static string will never be destroyed, which is important because
    // static strings will be shared across threads & ref-counted in a non-threadsafe manner.
    friend class NeverDestroyed<StringImpl>;
    enum ConstructEmptyStringTag { ConstructEmptyString };
    StringImpl(ConstructEmptyStringTag)
        : m_refCount(s_refCountFlagIsStaticString)
        , m_length(0)
        , m_data8(reinterpret_cast<const LChar*>(&m_length))
        , m_hashAndFlags(s_hashFlag8BitBuffer | StringAtomic | BufferOwned)
    {
        // Ensure that the hash is computed so that AtomicStringHash can call existingHash()
        // with impunity. The empty string is special because it is never entered into
        // AtomicString's HashKey, but still needs to compare correctly.
        STRING_STATS_ADD_8BIT_STRING(m_length);

        hash();
    }

    // FIXME: there has to be a less hacky way to do this.
    enum Force8Bit { Force8BitConstructor };
    // Create a normal 8-bit string with internal storage (BufferInternal)
    StringImpl(unsigned length, Force8Bit)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data8(tailPointer<LChar>())
        , m_hashAndFlags(s_hashFlag8BitBuffer | StringNormal | BufferInternal)
    {
        ASSERT(m_data8);
        ASSERT(m_length);

        STRING_STATS_ADD_8BIT_STRING(m_length);
    }

    // Create a normal 16-bit string with internal storage (BufferInternal)
    StringImpl(unsigned length)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data16(tailPointer<UChar>())
        , m_hashAndFlags(StringNormal | BufferInternal)
    {
        ASSERT(m_data16);
        ASSERT(m_length);

        STRING_STATS_ADD_16BIT_STRING(m_length);
    }

    // Create a StringImpl adopting ownership of the provided buffer (BufferOwned)
    StringImpl(MallocPtr<LChar> characters, unsigned length)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data8(characters.leakPtr())
        , m_hashAndFlags(s_hashFlag8BitBuffer | StringNormal | BufferOwned)
    {
        ASSERT(m_data8);
        ASSERT(m_length);

        STRING_STATS_ADD_8BIT_STRING(m_length);
    }

    enum ConstructWithoutCopyingTag { ConstructWithoutCopying };
    StringImpl(const UChar* characters, unsigned length, ConstructWithoutCopyingTag)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data16(characters)
        , m_hashAndFlags(StringNormal | BufferInternal)
    {
        ASSERT(m_data16);
        ASSERT(m_length);

        STRING_STATS_ADD_16BIT_STRING(m_length);
    }

    StringImpl(const LChar* characters, unsigned length, ConstructWithoutCopyingTag)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data8(characters)
        , m_hashAndFlags(s_hashFlag8BitBuffer | StringNormal | BufferInternal)
    {
        ASSERT(m_data8);
        ASSERT(m_length);

        STRING_STATS_ADD_8BIT_STRING(m_length);
    }

    // Create a StringImpl adopting ownership of the provided buffer (BufferOwned)
    StringImpl(MallocPtr<UChar> characters, unsigned length)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data16(characters.leakPtr())
        , m_hashAndFlags(StringNormal | BufferOwned)
    {
        ASSERT(m_data16);
        ASSERT(m_length);

        STRING_STATS_ADD_16BIT_STRING(m_length);
    }

    // Used to create new strings that are a substring of an existing 8-bit StringImpl (BufferSubstring)
    StringImpl(const LChar* characters, unsigned length, PassRefPtr<StringImpl> base)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data8(characters)
        , m_hashAndFlags(s_hashFlag8BitBuffer | StringNormal | BufferSubstring)
    {
        ASSERT(is8Bit());
        ASSERT(m_data8);
        ASSERT(m_length);
        ASSERT(base->bufferOwnership() != BufferSubstring);

        substringBuffer() = base.leakRef();

        STRING_STATS_ADD_8BIT_STRING2(m_length, true);
    }

    // Used to create new strings that are a substring of an existing 16-bit StringImpl (BufferSubstring)
    StringImpl(const UChar* characters, unsigned length, PassRefPtr<StringImpl> base)
        : m_refCount(s_refCountIncrement)
        , m_length(length)
        , m_data16(characters)
        , m_hashAndFlags(StringNormal | BufferSubstring)
    {
        ASSERT(!is8Bit());
        ASSERT(m_data16);
        ASSERT(m_length);
        ASSERT(base->bufferOwnership() != BufferSubstring);

        substringBuffer() = base.leakRef();

        STRING_STATS_ADD_16BIT_STRING2(m_length, true);
    }

    enum CreateSymbolEmptyTag { CreateSymbolEmpty };
    StringImpl(CreateSymbolEmptyTag)
        : m_refCount(s_refCountIncrement)
        , m_length(0)
        // We expect m_length to be initialized to 0 as we use it
        // to represent a null terminated buffer.
        , m_data8(reinterpret_cast<const LChar*>(&m_length))
        , m_hashAndFlags(hashAndFlagsForSymbol(s_hashFlag8BitBuffer | BufferInternal))
    {
        ASSERT(m_data8);

        STRING_STATS_ADD_8BIT_STRING(m_length);
    }

    ~StringImpl();

public:
    WTF_EXPORT_STRING_API static void destroy(StringImpl*);

    WTF_EXPORT_STRING_API static Ref<StringImpl> create(const UChar*, unsigned length);
    WTF_EXPORT_STRING_API static Ref<StringImpl> create(const LChar*, unsigned length);
    WTF_EXPORT_STRING_API static Ref<StringImpl> create8BitIfPossible(const UChar*, unsigned length);
    template<size_t inlineCapacity>
    static Ref<StringImpl> create8BitIfPossible(const Vector<UChar, inlineCapacity>& vector)
    {
        return create8BitIfPossible(vector.data(), vector.size());
    }
    WTF_EXPORT_STRING_API static Ref<StringImpl> create8BitIfPossible(const UChar*);

    ALWAYS_INLINE static Ref<StringImpl> create(const char* s, unsigned length) { return create(reinterpret_cast<const LChar*>(s), length); }
    WTF_EXPORT_STRING_API static Ref<StringImpl> create(const LChar*);
    ALWAYS_INLINE static Ref<StringImpl> create(const char* s) { return create(reinterpret_cast<const LChar*>(s)); }

    static ALWAYS_INLINE Ref<StringImpl> createSubstringSharingImpl8(PassRefPtr<StringImpl> rep, unsigned offset, unsigned length)
    {
        ASSERT(rep);
        ASSERT(length <= rep->length());

        if (!length)
            return *empty();

        ASSERT(rep->is8Bit());
        StringImpl* ownerRep = (rep->bufferOwnership() == BufferSubstring) ? rep->substringBuffer() : rep.get();

        // We allocate a buffer that contains both the StringImpl struct as well as the pointer to the owner string.
        StringImpl* stringImpl = static_cast<StringImpl*>(fastMalloc(allocationSize<StringImpl*>(1)));
        return adoptRef(*new (NotNull, stringImpl) StringImpl(rep->m_data8 + offset, length, ownerRep));
    }

    static ALWAYS_INLINE Ref<StringImpl> createSubstringSharingImpl(PassRefPtr<StringImpl> rep, unsigned offset, unsigned length)
    {
        ASSERT(rep);
        ASSERT(length <= rep->length());

        if (!length)
            return *empty();

        StringImpl* ownerRep = (rep->bufferOwnership() == BufferSubstring) ? rep->substringBuffer() : rep.get();

        // We allocate a buffer that contains both the StringImpl struct as well as the pointer to the owner string.
        StringImpl* stringImpl = static_cast<StringImpl*>(fastMalloc(allocationSize<StringImpl*>(1)));
        if (rep->is8Bit())
            return adoptRef(*new (NotNull, stringImpl) StringImpl(rep->m_data8 + offset, length, ownerRep));
        return adoptRef(*new (NotNull, stringImpl) StringImpl(rep->m_data16 + offset, length, ownerRep));
    }

    template<unsigned charactersCount>
    ALWAYS_INLINE static Ref<StringImpl> createFromLiteral(const char (&characters)[charactersCount])
    {
        COMPILE_ASSERT(charactersCount > 1, StringImplFromLiteralNotEmpty);
        COMPILE_ASSERT((charactersCount - 1 <= ((unsigned(~0) - sizeof(StringImpl)) / sizeof(LChar))), StringImplFromLiteralCannotOverflow);

        return createWithoutCopying(reinterpret_cast<const LChar*>(characters), charactersCount - 1);
    }

    // FIXME: Transition off of these functions to createWithoutCopying instead.
    WTF_EXPORT_STRING_API static Ref<StringImpl> createFromLiteral(const char* characters, unsigned length);
    WTF_EXPORT_STRING_API static Ref<StringImpl> createFromLiteral(const char* characters);

    WTF_EXPORT_STRING_API static Ref<StringImpl> createWithoutCopying(const UChar* characters, unsigned length);
    WTF_EXPORT_STRING_API static Ref<StringImpl> createWithoutCopying(const LChar* characters, unsigned length);

    WTF_EXPORT_STRING_API static Ref<StringImpl> createUninitialized(unsigned length, LChar*& data);
    WTF_EXPORT_STRING_API static Ref<StringImpl> createUninitialized(unsigned length, UChar*& data);
    template <typename T> static ALWAYS_INLINE PassRefPtr<StringImpl> tryCreateUninitialized(unsigned length, T*& output)
    {
        if (!length) {
            output = 0;
            return empty();
        }

        if (length > ((std::numeric_limits<unsigned>::max() - sizeof(StringImpl)) / sizeof(T))) {
            output = 0;
            return 0;
        }
        StringImpl* resultImpl;
        if (!tryFastMalloc(allocationSize<T>(length)).getValue(resultImpl)) {
            output = 0;
            return 0;
        }
        output = resultImpl->tailPointer<T>();

        return constructInternal<T>(resultImpl, length);
    }

    ALWAYS_INLINE static Ref<StringImpl> createSymbolEmpty()
    {
        return adoptRef(*new StringImpl(CreateSymbolEmpty));
    }

    WTF_EXPORT_STRING_API static Ref<StringImpl> createSymbol(PassRefPtr<StringImpl> rep);

    // Reallocate the StringImpl. The originalString must be only owned by the PassRefPtr,
    // and the buffer ownership must be BufferInternal. Just like the input pointer of realloc(),
    // the originalString can't be used after this function.
    static Ref<StringImpl> reallocate(PassRefPtr<StringImpl> originalString, unsigned length, LChar*& data);
    static Ref<StringImpl> reallocate(PassRefPtr<StringImpl> originalString, unsigned length, UChar*& data);

    static unsigned flagsOffset() { return OBJECT_OFFSETOF(StringImpl, m_hashAndFlags); }
    static unsigned flagIs8Bit() { return s_hashFlag8BitBuffer; }
    static unsigned flagIsAtomic() { return s_hashFlagStringKindIsAtomic; }
    static unsigned flagIsSymbol() { return s_hashFlagStringKindIsSymbol; }
    static unsigned maskStringKind() { return s_hashMaskStringKind; }
    static unsigned dataOffset() { return OBJECT_OFFSETOF(StringImpl, m_data8); }

    template<typename CharType, size_t inlineCapacity, typename OverflowHandler>
    static Ref<StringImpl> adopt(Vector<CharType, inlineCapacity, OverflowHandler>& vector)
    {
        if (size_t size = vector.size()) {
            ASSERT(vector.data());
            if (size > std::numeric_limits<unsigned>::max())
                CRASH();
            return adoptRef(*new StringImpl(vector.releaseBuffer(), size));
        }
        return *empty();
    }

    WTF_EXPORT_STRING_API static Ref<StringImpl> adopt(StringBuffer<UChar>&);
    WTF_EXPORT_STRING_API static Ref<StringImpl> adopt(StringBuffer<LChar>&);

    unsigned length() const { return m_length; }
    static ptrdiff_t lengthMemoryOffset() { return OBJECT_OFFSETOF(StringImpl, m_length); }
    bool is8Bit() const { return m_hashAndFlags & s_hashFlag8BitBuffer; }

    ALWAYS_INLINE const LChar* characters8() const { ASSERT(is8Bit()); return m_data8; }
    ALWAYS_INLINE const UChar* characters16() const { ASSERT(!is8Bit()); return m_data16; }

    template <typename CharType>
    ALWAYS_INLINE const CharType *characters() const;

    size_t cost() const
    {
        // For substrings, return the cost of the base string.
        if (bufferOwnership() == BufferSubstring)
            return substringBuffer()->cost();

        if (m_hashAndFlags & s_hashFlagDidReportCost)
            return 0;

        m_hashAndFlags |= s_hashFlagDidReportCost;
        size_t result = m_length;
        if (!is8Bit())
            result <<= 1;
        return result;
    }
    
    size_t costDuringGC()
    {
        if (isStatic())
            return 0;
        
        if (bufferOwnership() == BufferSubstring)
            return divideRoundedUp(substringBuffer()->costDuringGC(), refCount());
        
        size_t result = m_length;
        if (!is8Bit())
            result <<= 1;
        return divideRoundedUp(result, refCount());
    }

    WTF_EXPORT_STRING_API size_t sizeInBytes() const;

    StringKind stringKind() const { return static_cast<StringKind>(m_hashAndFlags & s_hashMaskStringKind); }
    bool isSymbol() const { return m_hashAndFlags & s_hashFlagStringKindIsSymbol; }
    bool isAtomic() const { return m_hashAndFlags & s_hashFlagStringKindIsAtomic; }

    void setIsAtomic(bool isAtomic)
    {
        ASSERT(!isStatic());
        ASSERT(!isSymbol());
        if (isAtomic) {
            m_hashAndFlags |= s_hashFlagStringKindIsAtomic;
            ASSERT(stringKind() == StringAtomic);
        } else {
            m_hashAndFlags &= ~s_hashFlagStringKindIsAtomic;
            ASSERT(stringKind() == StringNormal);
        }
    }

#if STRING_STATS
    bool isSubString() const { return bufferOwnership() == BufferSubstring; }
#endif

    static WTF_EXPORT_STRING_API CString utf8ForCharacters(const UChar* characters, unsigned length, ConversionMode = LenientConversion);
    WTF_EXPORT_STRING_API CString utf8ForRange(unsigned offset, unsigned length, ConversionMode = LenientConversion) const;
    WTF_EXPORT_STRING_API CString utf8(ConversionMode = LenientConversion) const;

private:
    static WTF_EXPORT_STRING_API bool utf8Impl(const UChar* characters, unsigned length, char*& buffer, size_t bufferSize, ConversionMode);
    
    // The high bits of 'hash' are always empty, but we prefer to store our flags
    // in the low bits because it makes them slightly more efficient to access.
    // So, we shift left and right when setting and getting our hash code.
    void setHash(unsigned hash) const
    {
        ASSERT(!hasHash());
        // Multiple clients assume that StringHasher is the canonical string hash function.
        ASSERT(hash == (is8Bit() ? StringHasher::computeHashAndMaskTop8Bits(m_data8, m_length) : StringHasher::computeHashAndMaskTop8Bits(m_data16, m_length)));
        ASSERT(!(hash & (s_flagMask << (8 * sizeof(hash) - s_flagCount)))); // Verify that enough high bits are empty.
        
        hash <<= s_flagCount;
        ASSERT(!(hash & m_hashAndFlags)); // Verify that enough low bits are empty after shift.
        ASSERT(hash); // Verify that 0 is a valid sentinel hash value.

        m_hashAndFlags |= hash; // Store hash with flags in low bits.
    }

    unsigned rawHash() const
    {
        return m_hashAndFlags >> s_flagCount;
    }

public:
    bool hasHash() const
    {
        return rawHash() != 0;
    }

    unsigned existingHash() const
    {
        ASSERT(hasHash());
        return rawHash();
    }

    unsigned hash() const
    {
        if (hasHash())
            return existingHash();
        return hashSlowCase();
    }
    
    bool isStatic() const { return m_refCount & s_refCountFlagIsStaticString; }

    inline size_t refCount() const
    {
        return m_refCount / s_refCountIncrement;
    }
    
    inline bool hasOneRef() const
    {
        return m_refCount == s_refCountIncrement;
    }
    
    // This method is useful for assertions.
    inline bool hasAtLeastOneRef() const
    {
        return !!m_refCount;
    }

    inline void ref()
    {
        ASSERT(!isCompilationThread());

        STRING_STATS_REF_STRING(*this);

        m_refCount += s_refCountIncrement;
    }

    inline void deref()
    {
        ASSERT(!isCompilationThread());

        STRING_STATS_DEREF_STRING(*this);

        unsigned tempRefCount = m_refCount - s_refCountIncrement;
        if (!tempRefCount) {
            StringImpl::destroy(this);
            return;
        }
        m_refCount = tempRefCount;
    }

    WTF_EXPORT_PRIVATE static StringImpl* empty();

    // FIXME: Does this really belong in StringImpl?
    template <typename T> static void copyChars(T* destination, const T* source, unsigned numCharacters)
    {
        if (numCharacters == 1) {
            *destination = *source;
            return;
        }

        if (numCharacters <= s_copyCharsInlineCutOff) {
            unsigned i = 0;
#if (CPU(X86) || CPU(X86_64))
            const unsigned charsPerInt = sizeof(uint32_t) / sizeof(T);

            if (numCharacters > charsPerInt) {
                unsigned stopCount = numCharacters & ~(charsPerInt - 1);

                const uint32_t* srcCharacters = reinterpret_cast<const uint32_t*>(source);
                uint32_t* destCharacters = reinterpret_cast<uint32_t*>(destination);
                for (unsigned j = 0; i < stopCount; i += charsPerInt, ++j)
                    destCharacters[j] = srcCharacters[j];
            }
#endif
            for (; i < numCharacters; ++i)
                destination[i] = source[i];
        } else
            memcpy(destination, source, numCharacters * sizeof(T));
    }

    ALWAYS_INLINE static void copyChars(UChar* destination, const LChar* source, unsigned numCharacters)
    {
        for (unsigned i = 0; i < numCharacters; ++i)
            destination[i] = source[i];
    }

    // Some string features, like refcounting and the atomicity flag, are not
    // thread-safe. We achieve thread safety by isolation, giving each thread
    // its own copy of the string.
    Ref<StringImpl> isolatedCopy() const;

    WTF_EXPORT_STRING_API Ref<StringImpl> substring(unsigned pos, unsigned len = UINT_MAX);

    UChar at(unsigned i) const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_length);
        if (is8Bit())
            return m_data8[i];
        return m_data16[i];
    }
    UChar operator[](unsigned i) const { return at(i); }
    WTF_EXPORT_STRING_API UChar32 characterStartingAt(unsigned);

    WTF_EXPORT_STRING_API bool containsOnlyWhitespace();

    int toIntStrict(bool* ok = 0, int base = 10);
    unsigned toUIntStrict(bool* ok = 0, int base = 10);
    int64_t toInt64Strict(bool* ok = 0, int base = 10);
    uint64_t toUInt64Strict(bool* ok = 0, int base = 10);
    intptr_t toIntPtrStrict(bool* ok = 0, int base = 10);

    WTF_EXPORT_STRING_API int toInt(bool* ok = 0); // ignores trailing garbage
    unsigned toUInt(bool* ok = 0); // ignores trailing garbage
    int64_t toInt64(bool* ok = 0); // ignores trailing garbage
    uint64_t toUInt64(bool* ok = 0); // ignores trailing garbage
    intptr_t toIntPtr(bool* ok = 0); // ignores trailing garbage

    // FIXME: Like the strict functions above, these give false for "ok" when there is trailing garbage.
    // Like the non-strict functions above, these return the value when there is trailing garbage.
    // It would be better if these were more consistent with the above functions instead.
    double toDouble(bool* ok = 0);
    float toFloat(bool* ok = 0);

    WTF_EXPORT_STRING_API Ref<StringImpl> convertToASCIILowercase();
    WTF_EXPORT_STRING_API Ref<StringImpl> lower();
    WTF_EXPORT_STRING_API Ref<StringImpl> upper();
    WTF_EXPORT_STRING_API Ref<StringImpl> lower(const AtomicString& localeIdentifier);
    WTF_EXPORT_STRING_API Ref<StringImpl> upper(const AtomicString& localeIdentifier);

    Ref<StringImpl> foldCase();

    Ref<StringImpl> stripWhiteSpace();
    Ref<StringImpl> stripWhiteSpace(IsWhiteSpaceFunctionPtr);
    WTF_EXPORT_STRING_API Ref<StringImpl> simplifyWhiteSpace();
    Ref<StringImpl> simplifyWhiteSpace(IsWhiteSpaceFunctionPtr);

    Ref<StringImpl> removeCharacters(CharacterMatchFunctionPtr);
    template <typename CharType>
    ALWAYS_INLINE Ref<StringImpl> removeCharacters(const CharType* characters, CharacterMatchFunctionPtr);

    size_t find(LChar character, unsigned start = 0);
    size_t find(char character, unsigned start = 0);
    size_t find(UChar character, unsigned start = 0);
    WTF_EXPORT_STRING_API size_t find(CharacterMatchFunctionPtr, unsigned index = 0);
    size_t find(const LChar*, unsigned index = 0);
    ALWAYS_INLINE size_t find(const char* s, unsigned index = 0) { return find(reinterpret_cast<const LChar*>(s), index); }
    WTF_EXPORT_STRING_API size_t find(StringImpl*);
    WTF_EXPORT_STRING_API size_t find(StringImpl*, unsigned index);
    size_t findIgnoringCase(const LChar*, unsigned index = 0);
    ALWAYS_INLINE size_t findIgnoringCase(const char* s, unsigned index = 0) { return findIgnoringCase(reinterpret_cast<const LChar*>(s), index); }
    WTF_EXPORT_STRING_API size_t findIgnoringCase(StringImpl*, unsigned index = 0);
    WTF_EXPORT_STRING_API size_t findIgnoringASCIICase(const StringImpl&) const;
    WTF_EXPORT_STRING_API size_t findIgnoringASCIICase(const StringImpl&, unsigned startOffset) const;
    WTF_EXPORT_STRING_API size_t findIgnoringASCIICase(const StringImpl*) const;
    WTF_EXPORT_STRING_API size_t findIgnoringASCIICase(const StringImpl*, unsigned startOffset) const;

    WTF_EXPORT_STRING_API size_t findNextLineStart(unsigned index = UINT_MAX);

    WTF_EXPORT_STRING_API size_t reverseFind(UChar, unsigned index = UINT_MAX);
    WTF_EXPORT_STRING_API size_t reverseFind(StringImpl*, unsigned index = UINT_MAX);
    WTF_EXPORT_STRING_API size_t reverseFindIgnoringCase(StringImpl*, unsigned index = UINT_MAX);

    WTF_EXPORT_STRING_API bool startsWith(const StringImpl*) const;
    WTF_EXPORT_STRING_API bool startsWith(const StringImpl&) const;
    WTF_EXPORT_STRING_API bool startsWithIgnoringASCIICase(const StringImpl*) const;
    WTF_EXPORT_STRING_API bool startsWithIgnoringASCIICase(const StringImpl&) const;
    bool startsWith(StringImpl* str, bool caseSensitive) { return caseSensitive ? startsWith(str) : (reverseFindIgnoringCase(str, 0) == 0); }
    WTF_EXPORT_STRING_API bool startsWith(UChar) const;
    WTF_EXPORT_STRING_API bool startsWith(const char*, unsigned matchLength, bool caseSensitive) const;
    template<unsigned matchLength>
    bool startsWith(const char (&prefix)[matchLength], bool caseSensitive = true) const { return startsWith(prefix, matchLength - 1, caseSensitive); }
    WTF_EXPORT_STRING_API bool hasInfixStartingAt(const StringImpl&, unsigned startOffset) const;

    WTF_EXPORT_STRING_API bool endsWith(StringImpl*);
    WTF_EXPORT_STRING_API bool endsWith(StringImpl&);
    WTF_EXPORT_STRING_API bool endsWithIgnoringASCIICase(const StringImpl*) const;
    WTF_EXPORT_STRING_API bool endsWithIgnoringASCIICase(const StringImpl&) const;
    WTF_EXPORT_STRING_API bool endsWith(StringImpl*, bool caseSensitive);
    WTF_EXPORT_STRING_API bool endsWith(UChar) const;
    WTF_EXPORT_STRING_API bool endsWith(const char*, unsigned matchLength, bool caseSensitive) const;
    template<unsigned matchLength>
    bool endsWith(const char (&prefix)[matchLength], bool caseSensitive = true) const { return endsWith(prefix, matchLength - 1, caseSensitive); }
    WTF_EXPORT_STRING_API bool hasInfixEndingAt(const StringImpl&, unsigned endOffset) const;

    WTF_EXPORT_STRING_API Ref<StringImpl> replace(UChar, UChar);
    WTF_EXPORT_STRING_API Ref<StringImpl> replace(UChar, StringImpl*);
    ALWAYS_INLINE Ref<StringImpl> replace(UChar pattern, const char* replacement, unsigned replacementLength) { return replace(pattern, reinterpret_cast<const LChar*>(replacement), replacementLength); }
    WTF_EXPORT_STRING_API Ref<StringImpl> replace(UChar, const LChar*, unsigned replacementLength);
    Ref<StringImpl> replace(UChar, const UChar*, unsigned replacementLength);
    WTF_EXPORT_STRING_API Ref<StringImpl> replace(StringImpl*, StringImpl*);
    WTF_EXPORT_STRING_API Ref<StringImpl> replace(unsigned index, unsigned len, StringImpl*);

    WTF_EXPORT_STRING_API UCharDirection defaultWritingDirection(bool* hasStrongDirectionality = nullptr);

#if USE(CF)
    RetainPtr<CFStringRef> createCFString();
#endif
#ifdef __OBJC__
    WTF_EXPORT_STRING_API operator NSString*();
#endif

#if STRING_STATS
    ALWAYS_INLINE static StringStats& stringStats() { return m_stringStats; }
#endif

    WTF_EXPORT_STRING_API static const UChar latin1CaseFoldTable[256];

    WTF_EXPORT_STRING_API StringImpl& extractFoldedStringInSymbol()
    {
        ASSERT(length());
        ASSERT(isSymbol());
        ASSERT(bufferOwnership() == BufferSubstring);
        ASSERT(substringBuffer());
        ASSERT(!substringBuffer()->isSymbol());
        return *substringBuffer();
    }

private:
    bool requiresCopy() const
    {
        if (bufferOwnership() != BufferInternal)
            return true;

        if (is8Bit())
            return m_data8 == tailPointer<LChar>();
        return m_data16 == tailPointer<UChar>();
    }

    template<typename T>
    static size_t allocationSize(unsigned tailElementCount)
    {
        return tailOffset<T>() + tailElementCount * sizeof(T);
    }

    template<typename T>
    static ptrdiff_t tailOffset()
    {
#if COMPILER(MSVC)
        // MSVC doesn't support alignof yet.
        return roundUpToMultipleOf<sizeof(T)>(sizeof(StringImpl));
#else
        return roundUpToMultipleOf<alignof(T)>(offsetof(StringImpl, m_hashAndFlags) + sizeof(StringImpl::m_hashAndFlags));
#endif
    }

    template<typename T>
    const T* tailPointer() const
    {
        return reinterpret_cast_ptr<const T*>(reinterpret_cast<const uint8_t*>(this) + tailOffset<T>());
    }

    template<typename T>
    T* tailPointer()
    {
        return reinterpret_cast_ptr<T*>(reinterpret_cast<uint8_t*>(this) + tailOffset<T>());
    }

    StringImpl* const& substringBuffer() const
    {
        ASSERT(bufferOwnership() == BufferSubstring);

        return *tailPointer<StringImpl*>();
    }

    StringImpl*& substringBuffer()
    {
        ASSERT(bufferOwnership() == BufferSubstring);

        return *tailPointer<StringImpl*>();
    }

    // This number must be at least 2 to avoid sharing empty, null as well as 1 character strings from SmallStrings.
    static const unsigned s_copyCharsInlineCutOff = 20;

    BufferOwnership bufferOwnership() const { return static_cast<BufferOwnership>(m_hashAndFlags & s_hashMaskBufferOwnership); }
    template <class UCharPredicate> Ref<StringImpl> stripMatchedCharacters(UCharPredicate);
    template <typename CharType, class UCharPredicate> Ref<StringImpl> simplifyMatchedCharactersToSpace(UCharPredicate);
    template <typename CharType> static Ref<StringImpl> constructInternal(StringImpl*, unsigned);
    template <typename CharType> static Ref<StringImpl> createUninitializedInternal(unsigned, CharType*&);
    template <typename CharType> static Ref<StringImpl> createUninitializedInternalNonEmpty(unsigned, CharType*&);
    template <typename CharType> static Ref<StringImpl> reallocateInternal(PassRefPtr<StringImpl>, unsigned, CharType*&);
    template <typename CharType> static Ref<StringImpl> createInternal(const CharType*, unsigned);
    WTF_EXPORT_PRIVATE NEVER_INLINE unsigned hashSlowCase() const;
    WTF_EXPORT_PRIVATE static unsigned hashAndFlagsForSymbol(unsigned flags);

    // The bottom bit in the ref count indicates a static (immortal) string.
    static const unsigned s_refCountFlagIsStaticString = 0x1;
    static const unsigned s_refCountIncrement = 0x2; // This allows us to ref / deref without disturbing the static string flag.

#if STRING_STATS
    WTF_EXPORTDATA static StringStats m_stringStats;
#endif

public:
    struct StaticASCIILiteral {
        // These member variables must match the layout of StringImpl.
        unsigned m_refCount;
        unsigned m_length;
        const LChar* m_data8;
        unsigned m_hashAndFlags;

        // These values mimic ConstructFromLiteral.
        static const unsigned s_initialRefCount = s_refCountIncrement;
        static const unsigned s_initialFlags = s_hashFlag8BitBuffer | StringNormal | BufferInternal;
        static const unsigned s_hashShift = s_flagCount;
    };

#ifndef NDEBUG
    void assertHashIsCorrect()
    {
        ASSERT(hasHash());
        ASSERT(existingHash() == StringHasher::computeHashAndMaskTop8Bits(characters8(), length()));
    }
#endif

private:
    // These member variables must match the layout of StaticASCIILiteral.
    unsigned m_refCount;
    unsigned m_length;
    union {
        const LChar* m_data8;
        const UChar* m_data16;
    };
    mutable unsigned m_hashAndFlags;
};

static_assert(sizeof(StringImpl) == sizeof(StringImpl::StaticASCIILiteral), "");

#if !ASSERT_DISABLED
// StringImpls created from StaticASCIILiteral will ASSERT
// in the generic ValueCheck<T>::checkConsistency
// as they are not allocated by fastMalloc.
// We don't currently have any way to detect that case
// so we ignore the consistency check for all StringImpl*.
template<> struct
ValueCheck<StringImpl*> {
    static void checkConsistency(const StringImpl*) { }
};
#endif

template <>
ALWAYS_INLINE Ref<StringImpl> StringImpl::constructInternal<LChar>(StringImpl* impl, unsigned length) { return adoptRef(*new (NotNull, impl) StringImpl(length, Force8BitConstructor)); }
template <>
ALWAYS_INLINE Ref<StringImpl> StringImpl::constructInternal<UChar>(StringImpl* impl, unsigned length) { return adoptRef(*new (NotNull, impl) StringImpl(length)); }

template <>
ALWAYS_INLINE const LChar* StringImpl::characters<LChar>() const { return characters8(); }

template <>
ALWAYS_INLINE const UChar* StringImpl::characters<UChar>() const { return characters16(); }

WTF_EXPORT_STRING_API bool equal(const StringImpl*, const StringImpl*);
WTF_EXPORT_STRING_API bool equal(const StringImpl*, const LChar*);
inline bool equal(const StringImpl* a, const char* b) { return equal(a, reinterpret_cast<const LChar*>(b)); }
WTF_EXPORT_STRING_API bool equal(const StringImpl*, const LChar*, unsigned);
WTF_EXPORT_STRING_API bool equal(const StringImpl*, const UChar*, unsigned);
inline bool equal(const StringImpl* a, const char* b, unsigned length) { return equal(a, reinterpret_cast<const LChar*>(b), length); }
inline bool equal(const LChar* a, StringImpl* b) { return equal(b, a); }
inline bool equal(const char* a, StringImpl* b) { return equal(b, reinterpret_cast<const LChar*>(a)); }
WTF_EXPORT_STRING_API bool equal(const StringImpl& a, const StringImpl& b);

WTF_EXPORT_STRING_API bool equalIgnoringCase(const StringImpl*, const StringImpl*);
WTF_EXPORT_STRING_API bool equalIgnoringCase(const StringImpl*, const LChar*);
inline bool equalIgnoringCase(const LChar* a, const StringImpl* b) { return equalIgnoringCase(b, a); }
WTF_EXPORT_STRING_API bool equalIgnoringCase(const LChar*, const LChar*, unsigned);
WTF_EXPORT_STRING_API bool equalIgnoringCase(const UChar*, const LChar*, unsigned);
inline bool equalIgnoringCase(const UChar* a, const char* b, unsigned length) { return equalIgnoringCase(a, reinterpret_cast<const LChar*>(b), length); }
inline bool equalIgnoringCase(const LChar* a, const UChar* b, unsigned length) { return equalIgnoringCase(b, a, length); }
inline bool equalIgnoringCase(const char* a, const UChar* b, unsigned length) { return equalIgnoringCase(b, reinterpret_cast<const LChar*>(a), length); }
inline bool equalIgnoringCase(const char* a, const LChar* b, unsigned length) { return equalIgnoringCase(b, reinterpret_cast<const LChar*>(a), length); }
inline bool equalIgnoringCase(const UChar* a, const UChar* b, int length)
{
    ASSERT(length >= 0);
    return !u_memcasecmp(a, b, length, U_FOLD_CASE_DEFAULT);
}
WTF_EXPORT_STRING_API bool equalIgnoringCaseNonNull(const StringImpl*, const StringImpl*);

WTF_EXPORT_STRING_API bool equalIgnoringNullity(StringImpl*, StringImpl*);
WTF_EXPORT_STRING_API bool equalIgnoringNullity(const UChar*, size_t length, StringImpl*);

WTF_EXPORT_STRING_API bool equalIgnoringASCIICase(const StringImpl&, const StringImpl&);
WTF_EXPORT_STRING_API bool equalIgnoringASCIICase(const StringImpl*, const StringImpl*);
WTF_EXPORT_STRING_API bool equalIgnoringASCIICaseNonNull(const StringImpl*, const StringImpl*);

template<typename CharacterType>
inline size_t find(const CharacterType* characters, unsigned length, CharacterType matchCharacter, unsigned index = 0)
{
    while (index < length) {
        if (characters[index] == matchCharacter)
            return index;
        ++index;
    }
    return notFound;
}

ALWAYS_INLINE size_t find(const UChar* characters, unsigned length, LChar matchCharacter, unsigned index = 0)
{
    return find(characters, length, static_cast<UChar>(matchCharacter), index);
}

inline size_t find(const LChar* characters, unsigned length, UChar matchCharacter, unsigned index = 0)
{
    if (matchCharacter & ~0xFF)
        return notFound;
    return find(characters, length, static_cast<LChar>(matchCharacter), index);
}

inline size_t find(const LChar* characters, unsigned length, CharacterMatchFunctionPtr matchFunction, unsigned index = 0)
{
    while (index < length) {
        if (matchFunction(characters[index]))
            return index;
        ++index;
    }
    return notFound;
}

inline size_t find(const UChar* characters, unsigned length, CharacterMatchFunctionPtr matchFunction, unsigned index = 0)
{
    while (index < length) {
        if (matchFunction(characters[index]))
            return index;
        ++index;
    }
    return notFound;
}

template<typename CharacterType>
inline size_t findNextLineStart(const CharacterType* characters, unsigned length, unsigned index = 0)
{
    while (index < length) {
        CharacterType c = characters[index++];
        if ((c != '\n') && (c != '\r'))
            continue;

        // There can only be a start of a new line if there are more characters
        // beyond the current character.
        if (index < length) {
            // The 3 common types of line terminators are 1. \r\n (Windows), 
            // 2. \r (old MacOS) and 3. \n (Unix'es).

            if (c == '\n')
                return index; // Case 3: just \n.

            CharacterType c2 = characters[index];
            if (c2 != '\n')
                return index; // Case 2: just \r.

            // Case 1: \r\n.
            // But, there's only a start of a new line if there are more
            // characters beyond the \r\n.
            if (++index < length)
                return index; 
        }
    }
    return notFound;
}

template<typename CharacterType>
inline size_t reverseFindLineTerminator(const CharacterType* characters, unsigned length, unsigned index = UINT_MAX)
{
    if (!length)
        return notFound;
    if (index >= length)
        index = length - 1;
    CharacterType c = characters[index];
    while ((c != '\n') && (c != '\r')) {
        if (!index--)
            return notFound;
        c = characters[index];
    }
    return index;
}

template<typename CharacterType>
inline size_t reverseFind(const CharacterType* characters, unsigned length, CharacterType matchCharacter, unsigned index = UINT_MAX)
{
    if (!length)
        return notFound;
    if (index >= length)
        index = length - 1;
    while (characters[index] != matchCharacter) {
        if (!index--)
            return notFound;
    }
    return index;
}

ALWAYS_INLINE size_t reverseFind(const UChar* characters, unsigned length, LChar matchCharacter, unsigned index = UINT_MAX)
{
    return reverseFind(characters, length, static_cast<UChar>(matchCharacter), index);
}

inline size_t reverseFind(const LChar* characters, unsigned length, UChar matchCharacter, unsigned index = UINT_MAX)
{
    if (matchCharacter & ~0xFF)
        return notFound;
    return reverseFind(characters, length, static_cast<LChar>(matchCharacter), index);
}

inline size_t StringImpl::find(LChar character, unsigned start)
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, character, start);
    return WTF::find(characters16(), m_length, character, start);
}

ALWAYS_INLINE size_t StringImpl::find(char character, unsigned start)
{
    return find(static_cast<LChar>(character), start);
}

inline size_t StringImpl::find(UChar character, unsigned start)
{
    if (is8Bit())
        return WTF::find(characters8(), m_length, character, start);
    return WTF::find(characters16(), m_length, character, start);
}

template<size_t inlineCapacity> inline bool equalIgnoringNullity(const Vector<UChar, inlineCapacity>& a, StringImpl* b)
{
    return equalIgnoringNullity(a.data(), a.size(), b);
}

template<typename CharacterType1, typename CharacterType2>
inline int codePointCompare(unsigned l1, unsigned l2, const CharacterType1* c1, const CharacterType2* c2)
{
    const unsigned lmin = l1 < l2 ? l1 : l2;
    unsigned pos = 0;
    while (pos < lmin && *c1 == *c2) {
        ++c1;
        ++c2;
        ++pos;
    }

    if (pos < lmin)
        return (c1[0] > c2[0]) ? 1 : -1;

    if (l1 == l2)
        return 0;

    return (l1 > l2) ? 1 : -1;
}

inline int codePointCompare8(const StringImpl* string1, const StringImpl* string2)
{
    return codePointCompare(string1->length(), string2->length(), string1->characters8(), string2->characters8());
}

inline int codePointCompare16(const StringImpl* string1, const StringImpl* string2)
{
    return codePointCompare(string1->length(), string2->length(), string1->characters16(), string2->characters16());
}

inline int codePointCompare8To16(const StringImpl* string1, const StringImpl* string2)
{
    return codePointCompare(string1->length(), string2->length(), string1->characters8(), string2->characters16());
}

inline int codePointCompare(const StringImpl* string1, const StringImpl* string2)
{
    if (!string1)
        return (string2 && string2->length()) ? -1 : 0;

    if (!string2)
        return string1->length() ? 1 : 0;

    bool string1Is8Bit = string1->is8Bit();
    bool string2Is8Bit = string2->is8Bit();
    if (string1Is8Bit) {
        if (string2Is8Bit)
            return codePointCompare8(string1, string2);
        return codePointCompare8To16(string1, string2);
    }
    if (string2Is8Bit)
        return -codePointCompare8To16(string2, string1);
    return codePointCompare16(string1, string2);
}

inline bool isSpaceOrNewline(UChar c)
{
    // Use isASCIISpace() for basic Latin-1.
    // This will include newlines, which aren't included in Unicode DirWS.
    return c <= 0x7F ? isASCIISpace(c) : u_charDirection(c) == U_WHITE_SPACE_NEUTRAL;
}

template<typename CharacterType>
inline unsigned lengthOfNullTerminatedString(const CharacterType* string)
{
    ASSERT(string);
    size_t length = 0;
    while (string[length])
        ++length;

    RELEASE_ASSERT(length < std::numeric_limits<unsigned>::max());
    return static_cast<unsigned>(length);
}

inline Ref<StringImpl> StringImpl::isolatedCopy() const
{
    if (!requiresCopy()) {
        if (is8Bit())
            return StringImpl::createWithoutCopying(m_data8, m_length);
        return StringImpl::createWithoutCopying(m_data16, m_length);
    }

    if (is8Bit())
        return create(m_data8, m_length);
    return create(m_data16, m_length);
}

struct StringHash;

// StringHash is the default hash for StringImpl* and RefPtr<StringImpl>
template<typename T> struct DefaultHash;
template<> struct DefaultHash<StringImpl*> {
    typedef StringHash Hash;
};
template<> struct DefaultHash<RefPtr<StringImpl>> {
    typedef StringHash Hash;
};

} // namespace WTF

using WTF::StringImpl;
using WTF::equal;
using WTF::equalIgnoringASCIICase;
using WTF::TextCaseSensitivity;
using WTF::TextCaseSensitive;
using WTF::TextCaseInsensitive;

#endif
