/*
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Identifier_h
#define Identifier_h

#include "VM.h"
#include <wtf/ThreadSpecific.h>
#include <wtf/WTFThreadData.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace JSC {

class ExecState;

class Identifier {
    friend class Structure;
public:
    Identifier() { }
    enum EmptyIdentifierFlag { EmptyIdentifier };
    Identifier(EmptyIdentifierFlag) : m_string(StringImpl::empty()) { ASSERT(m_string.impl()->isAtomic()); }

    const String& string() const { return m_string; }
    AtomicStringImpl* impl() const { return static_cast<AtomicStringImpl*>(m_string.impl()); }

    int length() const { return m_string.length(); }

    CString ascii() const { return m_string.ascii(); }
    CString utf8() const { return m_string.utf8(); }

    // There's 2 functions to construct Identifier from string, (1) fromString and (2) fromUid.
    // They have different meanings in keeping or discarding symbol-ness of strings.
    // (1): fromString
    // Just construct Identifier from string. String held by Identifier is always atomized.
    // Symbol-ness of StringImpl*, which represents that the string is inteded to be used for ES6 Symbols, is discarded.
    // So a constructed Identifier never represents a symbol.
    // (2): fromUid
    // `StringImpl* uid` represents ether String or Symbol property.
    // fromUid keeps symbol-ness of provided StringImpl* while fromString discards it.
    // Use fromUid when constructing Identifier from StringImpl* which may represent symbols.

    // Only to be used with string literals.
    template<unsigned charactersCount>
    static Identifier fromString(VM*, const char (&characters)[charactersCount]);
    template<unsigned charactersCount>
    static Identifier fromString(ExecState*, const char (&characters)[charactersCount]);
    static Identifier fromString(VM*, const LChar*, int length);
    static Identifier fromString(VM*, const UChar*, int length);
    static Identifier fromString(VM*, const String&);
    static Identifier fromString(ExecState*, AtomicStringImpl*);
    static Identifier fromString(ExecState*, const AtomicString&);
    static Identifier fromString(ExecState*, const String&);
    static Identifier fromString(ExecState*, const char*);

    static Identifier fromUid(VM*, StringImpl* uid);
    static Identifier fromUid(ExecState*, StringImpl* uid);
    static Identifier fromUid(const PrivateName&);

    static Identifier createLCharFromUChar(VM* vm, const UChar* s, int length) { return Identifier(vm, add8(vm, s, length)); }

    JS_EXPORT_PRIVATE static Identifier from(ExecState*, unsigned y);
    JS_EXPORT_PRIVATE static Identifier from(ExecState*, int y);
    static Identifier from(ExecState*, double y);
    static Identifier from(VM*, unsigned y);
    static Identifier from(VM*, int y);
    static Identifier from(VM*, double y);

    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return m_string.isEmpty(); }
    bool isSymbol() const { return impl()->isSymbol(); }

    friend bool operator==(const Identifier&, const Identifier&);
    friend bool operator!=(const Identifier&, const Identifier&);

    friend bool operator==(const Identifier&, const LChar*);
    friend bool operator==(const Identifier&, const char*);
    friend bool operator!=(const Identifier&, const LChar*);
    friend bool operator!=(const Identifier&, const char*);

    static bool equal(const StringImpl*, const LChar*);
    static inline bool equal(const StringImpl*a, const char*b) { return Identifier::equal(a, reinterpret_cast<const LChar*>(b)); };
    static bool equal(const StringImpl*, const LChar*, unsigned length);
    static bool equal(const StringImpl*, const UChar*, unsigned length);
    static bool equal(const StringImpl* a, const StringImpl* b) { return ::equal(a, b); }

    // Only to be used with string literals.
    JS_EXPORT_PRIVATE static Ref<StringImpl> add(VM*, const char*);
    JS_EXPORT_PRIVATE static Ref<StringImpl> add(ExecState*, const char*);

    void dump(PrintStream&) const;

private:
    String m_string;

    // Only to be used with string literals.
    template<unsigned charactersCount>
    Identifier(VM* vm, const char (&characters)[charactersCount]) : m_string(add(vm, characters)) { ASSERT(m_string.impl()->isAtomic()); }

    Identifier(VM* vm, const LChar* s, int length) : m_string(add(vm, s, length)) { ASSERT(m_string.impl()->isAtomic()); }
    Identifier(VM* vm, const UChar* s, int length) : m_string(add(vm, s, length)) { ASSERT(m_string.impl()->isAtomic()); }
    Identifier(ExecState*, AtomicStringImpl*);
    Identifier(ExecState*, const AtomicString&);
    Identifier(VM* vm, const String& string) : m_string(add(vm, string.impl())) { ASSERT(m_string.impl()->isAtomic()); }
    Identifier(VM* vm, StringImpl* rep) : m_string(add(vm, rep)) { ASSERT(m_string.impl()->isAtomic()); }

    enum UniqueIdentifierFlag { UniqueIdentifier };
    Identifier(UniqueIdentifierFlag, StringImpl* uid) : m_string(uid) { ASSERT(m_string.impl()->isSymbol()); }

    template <typename CharType>
    ALWAYS_INLINE static uint32_t toUInt32FromCharacters(const CharType* characters, unsigned length, bool& ok);

    static bool equal(const Identifier& a, const Identifier& b) { return a.m_string.impl() == b.m_string.impl(); }
    static bool equal(const Identifier& a, const LChar* b) { return equal(a.m_string.impl(), b); }

    template <typename T> static Ref<StringImpl> add(VM*, const T*, int length);
    static Ref<StringImpl> add8(VM*, const UChar*, int length);
    template <typename T> ALWAYS_INLINE static bool canUseSingleCharacterString(T);

    static Ref<StringImpl> add(ExecState*, StringImpl*);
    static Ref<StringImpl> add(VM*, StringImpl*);

#ifndef NDEBUG
    JS_EXPORT_PRIVATE static void checkCurrentAtomicStringTable(ExecState*);
    JS_EXPORT_PRIVATE static void checkCurrentAtomicStringTable(VM*);
#else
    JS_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH static void checkCurrentAtomicStringTable(ExecState*);
    JS_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH static void checkCurrentAtomicStringTable(VM*);
#endif
};

template <> ALWAYS_INLINE bool Identifier::canUseSingleCharacterString(LChar)
{
    ASSERT(maxSingleCharacterString == 0xff);
    return true;
}

template <> ALWAYS_INLINE bool Identifier::canUseSingleCharacterString(UChar c)
{
    return (c <= maxSingleCharacterString);
}

template <typename T>
Ref<StringImpl> Identifier::add(VM* vm, const T* s, int length)
{
    if (length == 1) {
        T c = s[0];
        if (canUseSingleCharacterString(c))
            return *vm->smallStrings.singleCharacterStringRep(c);
    }
    if (!length)
        return *StringImpl::empty();

    return *AtomicString::add(s, length);
}

inline bool operator==(const Identifier& a, const Identifier& b)
{
    return Identifier::equal(a, b);
}

inline bool operator!=(const Identifier& a, const Identifier& b)
{
    return !Identifier::equal(a, b);
}

inline bool operator==(const Identifier& a, const LChar* b)
{
    return Identifier::equal(a, b);
}

inline bool operator==(const Identifier& a, const char* b)
{
    return Identifier::equal(a, reinterpret_cast<const LChar*>(b));
}

inline bool operator!=(const Identifier& a, const LChar* b)
{
    return !Identifier::equal(a, b);
}

inline bool operator!=(const Identifier& a, const char* b)
{
    return !Identifier::equal(a, reinterpret_cast<const LChar*>(b));
}

inline bool Identifier::equal(const StringImpl* r, const LChar* s)
{
    return WTF::equal(r, s);
}

inline bool Identifier::equal(const StringImpl* r, const LChar* s, unsigned length)
{
    return WTF::equal(r, s, length);
}

inline bool Identifier::equal(const StringImpl* r, const UChar* s, unsigned length)
{
    return WTF::equal(r, s, length);
}

struct IdentifierRepHash : PtrHash<RefPtr<StringImpl>> {
    static unsigned hash(const RefPtr<StringImpl>& key) { return key->existingHash(); }
    static unsigned hash(StringImpl* key) { return key->existingHash(); }
};

struct IdentifierMapIndexHashTraits : HashTraits<int> {
    static int emptyValue() { return std::numeric_limits<int>::max(); }
    static const bool emptyValueIsZero = false;
};

typedef HashMap<RefPtr<StringImpl>, int, IdentifierRepHash, HashTraits<RefPtr<StringImpl>>, IdentifierMapIndexHashTraits> IdentifierMap;
typedef HashMap<StringImpl*, int, IdentifierRepHash, HashTraits<StringImpl*>, IdentifierMapIndexHashTraits> BorrowedIdentifierMap;

} // namespace JSC

namespace WTF {

template <> struct VectorTraits<JSC::Identifier> : SimpleClassVectorTraits { };

} // namespace WTF

#endif // Identifier_h
