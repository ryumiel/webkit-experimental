/*
 *  Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "PropertyNameArray.h"

#include "JSCInlines.h"
#include "JSObject.h"
#include "JSPropertyNameEnumerator.h"
#include "Structure.h"
#include "StructureChain.h"

namespace JSC {

void PropertyNameArray::add(StringImpl* identifier)
{
    ASSERT(!identifier || (identifier == StringImpl::empty() || identifier->isAtomic() || identifier->isSymbol()));
    if (!ASSERT_DISABLED) {
        uint32_t index = PropertyName(Identifier::fromUid(m_vm, identifier)).asIndex();
        ASSERT_UNUSED(index, index == PropertyName::NotAnIndex || index >= m_previouslyEnumeratedLength);
    }

    if (m_alternateSet && m_alternateSet->contains(identifier))
        return;

    if (!m_set->add(identifier).isNewEntry)
        return;

    addKnownUnique(identifier);
}

void PropertyNameArray::setPreviouslyEnumeratedProperties(const JSPropertyNameEnumerator* enumerator)
{
    m_alternateSet = enumerator->identifierSet();
}

} // namespace JSC
