/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DFGPromotedHeapLocation_h
#define DFGPromotedHeapLocation_h

#if ENABLE(DFG_JIT)

#include "DFGNode.h"
#include <wtf/PrintStream.h>

namespace JSC { namespace DFG {

enum PromotedLocationKind {
    InvalidPromotedLocationKind,
    
    StructurePLoc,
    NamedPropertyPLoc,
    ArgumentPLoc,
    ArgumentCountPLoc,
    ArgumentsCalleePLoc
};

class PromotedLocationDescriptor {
public:
    PromotedLocationDescriptor(
        PromotedLocationKind kind = InvalidPromotedLocationKind, unsigned info = 0)
        : m_kind(kind)
        , m_info(info)
    {
    }
    
    bool operator!() const { return m_kind == InvalidPromotedLocationKind; }
    
    PromotedLocationKind kind() const { return m_kind; }
    unsigned info() const { return m_info; }
    
    OpInfo imm1() const { return OpInfo(static_cast<uint32_t>(m_kind)); }
    OpInfo imm2() const { return OpInfo(static_cast<uint32_t>(m_info)); }
    
    unsigned hash() const
    {
        return m_kind + m_info;
    }
    
    bool operator==(const PromotedLocationDescriptor& other) const
    {
        return m_kind == other.m_kind
            && m_info == other.m_info;
    }
    
    bool operator!=(const PromotedLocationDescriptor& other) const
    {
        return !(*this == other);
    }

    bool isHashTableDeletedValue() const
    {
        return m_kind == InvalidPromotedLocationKind && m_info;
    }
    
    void dump(PrintStream& out) const;

private:
    PromotedLocationKind m_kind;
    unsigned m_info;
};

class PromotedHeapLocation {
public:
    PromotedHeapLocation(
        PromotedLocationKind kind = InvalidPromotedLocationKind,
        Node* base = nullptr, unsigned info = 0)
        : m_base(base)
        , m_meta(kind, info)
    {
    }
    
    PromotedHeapLocation(
        PromotedLocationKind kind, Edge base, unsigned info = 0)
        : PromotedHeapLocation(kind, base.node(), info)
    {
    }
    
    PromotedHeapLocation(Node* base, PromotedLocationDescriptor meta)
        : m_base(base)
        , m_meta(meta)
    {
    }
    
    PromotedHeapLocation(WTF::HashTableDeletedValueType)
        : m_base(nullptr)
        , m_meta(InvalidPromotedLocationKind, 1)
    {
    }
    
    Node* createHint(Graph&, NodeOrigin, Node* value);
    
    bool operator!() const { return kind() == InvalidPromotedLocationKind; }
    
    PromotedLocationKind kind() const { return m_meta.kind(); }
    Node* base() const { return m_base; }
    unsigned info() const { return m_meta.info(); }
    PromotedLocationDescriptor descriptor() const { return m_meta; }
    
    unsigned hash() const
    {
        return m_meta.hash() + WTF::PtrHash<Node*>::hash(m_base);
    }
    
    bool operator==(const PromotedHeapLocation& other) const
    {
        return m_base == other.m_base
            && m_meta == other.m_meta;
    }
    
    bool isHashTableDeletedValue() const
    {
        return m_meta.isHashTableDeletedValue();
    }
    
    void dump(PrintStream& out) const;
    
private:
    Node* m_base;
    PromotedLocationDescriptor m_meta;
};

struct PromotedHeapLocationHash {
    static unsigned hash(const PromotedHeapLocation& key) { return key.hash(); }
    static bool equal(const PromotedHeapLocation& a, const PromotedHeapLocation& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

} } // namespace JSC::DFG

namespace WTF {

void printInternal(PrintStream&, JSC::DFG::PromotedLocationKind);

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::DFG::PromotedHeapLocation> {
    typedef JSC::DFG::PromotedHeapLocationHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::DFG::PromotedHeapLocation> : SimpleClassHashTraits<JSC::DFG::PromotedHeapLocation> {
    static const bool emptyValueIsZero = false;
};

} // namespace WTF

#endif // ENABLE(DFG_JIT)

#endif // DFGPromotedHeapLocation_h

