/*
 * Copyright (C) 2012-2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DFGConstantFoldingPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractInterpreterInlines.h"
#include "DFGArgumentsUtilities.h"
#include "DFGBasicBlock.h"
#include "DFGGraph.h"
#include "DFGInPlaceAbstractState.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "GetByIdStatus.h"
#include "JSCInlines.h"
#include "PutByIdStatus.h"

namespace JSC { namespace DFG {

class ConstantFoldingPhase : public Phase {
public:
    ConstantFoldingPhase(Graph& graph)
        : Phase(graph, "constant folding")
        , m_state(graph)
        , m_interpreter(graph, m_state)
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        bool changed = false;
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            if (block->cfaFoundConstants)
                changed |= foldConstants(block);
        }
        
        if (changed && m_graph.m_form == SSA) {
            // It's now possible that we have Upsilons pointed at JSConstants. Fix that.
            for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
                BasicBlock* block = m_graph.block(blockIndex);
                if (!block)
                    continue;
                fixUpsilons(block);
            }
        }
         
        return changed;
    }

private:
    bool foldConstants(BasicBlock* block)
    {
        bool changed = false;
        m_state.beginBasicBlock(block);
        for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
            if (!m_state.isValid())
                break;
            
            Node* node = block->at(indexInBlock);

            bool alreadyHandled = false;
            bool eliminated = false;
                    
            switch (node->op()) {
            case BooleanToNumber: {
                if (node->child1().useKind() == UntypedUse
                    && !m_interpreter.needsTypeCheck(node->child1(), SpecBoolean))
                    node->child1().setUseKind(BooleanUse);
                break;
            }
                
            case CheckStructure:
            case ArrayifyToStructure: {
                AbstractValue& value = m_state.forNode(node->child1());
                StructureSet set;
                if (node->op() == ArrayifyToStructure)
                    set = node->structure();
                else
                    set = node->structureSet();
                if (value.m_structure.isSubsetOf(set)) {
                    m_interpreter.execute(indexInBlock); // Catch the fact that we may filter on cell.
                    node->convertToPhantom();
                    eliminated = true;
                    break;
                }
                break;
            }
                
            case CheckStructureImmediate: {
                AbstractValue& value = m_state.forNode(node->child1());
                StructureSet& set = node->structureSet();
                
                if (value.value()) {
                    if (Structure* structure = jsDynamicCast<Structure*>(value.value())) {
                        if (set.contains(structure)) {
                            m_interpreter.execute(indexInBlock);
                            node->convertToPhantom();
                            eliminated = true;
                            break;
                        }
                    }
                }
                
                if (PhiChildren* phiChildren = m_interpreter.phiChildren()) {
                    bool allGood = true;
                    phiChildren->forAllTransitiveIncomingValues(
                        node,
                        [&] (Node* incoming) {
                            if (Structure* structure = incoming->dynamicCastConstant<Structure*>()) {
                                if (set.contains(structure))
                                    return;
                            }
                            allGood = false;
                        });
                    if (allGood) {
                        m_interpreter.execute(indexInBlock);
                        node->convertToPhantom();
                        eliminated = true;
                        break;
                    }
                }
                break;
            }
                
            case CheckArray:
            case Arrayify: {
                if (!node->arrayMode().alreadyChecked(m_graph, node, m_state.forNode(node->child1())))
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                
            case PutStructure: {
                if (m_state.forNode(node->child1()).m_structure.onlyStructure() != node->transition()->next)
                    break;
                
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                
            case CheckCell: {
                if (m_state.forNode(node->child1()).value() != node->cellOperand()->value())
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }

            case CheckNotEmpty: {
                if (m_state.forNode(node->child1()).m_type & SpecEmpty)
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }

            case CheckInBounds: {
                JSValue left = m_state.forNode(node->child1()).value();
                JSValue right = m_state.forNode(node->child2()).value();
                if (left && right && left.isInt32() && right.isInt32()
                    && static_cast<uint32_t>(left.asInt32()) < static_cast<uint32_t>(right.asInt32())) {
                    node->convertToPhantom();
                    eliminated = true;
                    break;
                }
                
                break;
            }
                
            case GetMyArgumentByVal: {
                JSValue index = m_state.forNode(node->child2()).value();
                if (!index || !index.isInt32())
                    break;
                
                Node* arguments = node->child1().node();
                InlineCallFrame* inlineCallFrame = arguments->origin.semantic.inlineCallFrame;
                
                // Don't try to do anything if the index is known to be outside our static bounds. Note
                // that our static bounds are usually strictly larger than the dynamic bounds. The
                // exception is something like this, assuming foo() is not inlined:
                //
                // function foo() { return arguments[5]; }
                //
                // Here the static bound on number of arguments is 0, and we're accessing index 5. We
                // will not strength-reduce this to GetStack because GetStack is otherwise assumed by the
                // compiler to access those variables that are statically accounted for; for example if
                // we emitted a GetStack on arg6 we would have out-of-bounds access crashes anywhere that
                // uses an Operands<> map. There is not much cost to continuing to use a
                // GetMyArgumentByVal in such statically-out-of-bounds accesses; we just lose CFA unless
                // GCSE removes the access entirely.
                if (inlineCallFrame) {
                    if (index.asUInt32() >= inlineCallFrame->arguments.size() - 1)
                        break;
                } else {
                    if (index.asUInt32() >= m_state.variables().numberOfArguments() - 1)
                        break;
                }
                
                m_interpreter.execute(indexInBlock); // Push CFA over this node after we get the state before.
                
                StackAccessData* data;
                if (inlineCallFrame) {
                    data = m_graph.m_stackAccessData.add(
                        VirtualRegister(
                            inlineCallFrame->stackOffset +
                            CallFrame::argumentOffset(index.asInt32())),
                        FlushedJSValue);
                } else {
                    data = m_graph.m_stackAccessData.add(
                        virtualRegisterForArgument(index.asInt32() + 1), FlushedJSValue);
                }
                
                if (inlineCallFrame && !inlineCallFrame->isVarargs()
                    && index.asUInt32() < inlineCallFrame->arguments.size() - 1) {
                    node->convertToGetStack(data);
                    eliminated = true;
                    break;
                }
                
                Node* length = emitCodeToGetArgumentsArrayLength(
                    m_insertionSet, arguments, indexInBlock, node->origin);
                m_insertionSet.insertNode(
                    indexInBlock, SpecNone, CheckInBounds, node->origin,
                    node->child2(), Edge(length, Int32Use));
                node->convertToGetStack(data);
                eliminated = true;
                break;
            }
                
            case MultiGetByOffset: {
                Edge baseEdge = node->child1();
                Node* base = baseEdge.node();
                MultiGetByOffsetData& data = node->multiGetByOffsetData();

                // First prune the variants, then check if the MultiGetByOffset can be
                // strength-reduced to a GetByOffset.
                
                AbstractValue baseValue = m_state.forNode(base);
                
                m_interpreter.execute(indexInBlock); // Push CFA over this node after we get the state before.
                alreadyHandled = true; // Don't allow the default constant folder to do things to this.
                
                for (unsigned i = 0; i < data.variants.size(); ++i) {
                    GetByIdVariant& variant = data.variants[i];
                    variant.structureSet().filter(baseValue);
                    if (variant.structureSet().isEmpty()) {
                        data.variants[i--] = data.variants.last();
                        data.variants.removeLast();
                        changed = true;
                    }
                }
                
                if (data.variants.size() != 1)
                    break;
                
                emitGetByOffset(
                    indexInBlock, node, baseValue, data.variants[0], data.identifierNumber);
                changed = true;
                break;
            }
                
            case MultiPutByOffset: {
                Edge baseEdge = node->child1();
                Node* base = baseEdge.node();
                MultiPutByOffsetData& data = node->multiPutByOffsetData();
                
                AbstractValue baseValue = m_state.forNode(base);

                m_interpreter.execute(indexInBlock); // Push CFA over this node after we get the state before.
                alreadyHandled = true; // Don't allow the default constant folder to do things to this.
                

                for (unsigned i = 0; i < data.variants.size(); ++i) {
                    PutByIdVariant& variant = data.variants[i];
                    variant.oldStructure().filter(baseValue);
                    
                    if (variant.oldStructure().isEmpty()) {
                        data.variants[i--] = data.variants.last();
                        data.variants.removeLast();
                        changed = true;
                        continue;
                    }
                    
                    if (variant.kind() == PutByIdVariant::Transition
                        && variant.oldStructure().onlyStructure() == variant.newStructure()) {
                        variant = PutByIdVariant::replace(
                            variant.oldStructure(),
                            variant.offset());
                        changed = true;
                    }
                }

                if (data.variants.size() != 1)
                    break;
                
                emitPutByOffset(
                    indexInBlock, node, baseValue, data.variants[0], data.identifierNumber);
                changed = true;
                break;
            }
        
            case GetById:
            case GetByIdFlush: {
                Edge childEdge = node->child1();
                Node* child = childEdge.node();
                unsigned identifierNumber = node->identifierNumber();
                
                AbstractValue baseValue = m_state.forNode(child);

                m_interpreter.execute(indexInBlock); // Push CFA over this node after we get the state before.
                alreadyHandled = true; // Don't allow the default constant folder to do things to this.

                if (baseValue.m_structure.isTop() || baseValue.m_structure.isClobbered()
                    || (node->child1().useKind() == UntypedUse || (baseValue.m_type & ~SpecCell)))
                    break;
                
                GetByIdStatus status = GetByIdStatus::computeFor(
                    baseValue.m_structure.set(), m_graph.identifiers()[identifierNumber]);
                if (!status.isSimple())
                    break;
                
                for (unsigned i = status.numVariants(); i--;) {
                    if (!status[i].constantChecks().isEmpty()
                        || status[i].alternateBase()) {
                        // FIXME: We could handle prototype cases.
                        // https://bugs.webkit.org/show_bug.cgi?id=110386
                        break;
                    }
                }
                
                if (status.numVariants() == 1) {
                    emitGetByOffset(indexInBlock, node, baseValue, status[0], identifierNumber);
                    changed = true;
                    break;
                }
                
                if (!isFTL(m_graph.m_plan.mode))
                    break;
                
                MultiGetByOffsetData* data = m_graph.m_multiGetByOffsetData.add();
                data->variants = status.variants();
                data->identifierNumber = identifierNumber;
                node->convertToMultiGetByOffset(data);
                changed = true;
                break;
            }
                
            case PutById:
            case PutByIdDirect:
            case PutByIdFlush: {
                NodeOrigin origin = node->origin;
                Edge childEdge = node->child1();
                Node* child = childEdge.node();
                unsigned identifierNumber = node->identifierNumber();
                
                ASSERT(childEdge.useKind() == CellUse);
                
                AbstractValue baseValue = m_state.forNode(child);

                m_interpreter.execute(indexInBlock); // Push CFA over this node after we get the state before.
                alreadyHandled = true; // Don't allow the default constant folder to do things to this.

                if (baseValue.m_structure.isTop() || baseValue.m_structure.isClobbered())
                    break;
                
                PutByIdStatus status = PutByIdStatus::computeFor(
                    m_graph.globalObjectFor(origin.semantic),
                    baseValue.m_structure.set(),
                    m_graph.identifiers()[identifierNumber],
                    node->op() == PutByIdDirect);
                
                if (!status.isSimple())
                    break;
                
                ASSERT(status.numVariants());
                
                if (status.numVariants() > 1 && !isFTL(m_graph.m_plan.mode))
                    break;
                
                changed = true;
                
                for (unsigned i = status.numVariants(); i--;)
                    addChecks(origin, indexInBlock, status[i].constantChecks());
                
                if (status.numVariants() == 1) {
                    emitPutByOffset(indexInBlock, node, baseValue, status[0], identifierNumber);
                    break;
                }
                
                ASSERT(isFTL(m_graph.m_plan.mode));

                MultiPutByOffsetData* data = m_graph.m_multiPutByOffsetData.add();
                data->variants = status.variants();
                data->identifierNumber = identifierNumber;
                node->convertToMultiPutByOffset(data);
                break;
            }

            case ToPrimitive: {
                if (m_state.forNode(node->child1()).m_type & ~(SpecFullNumber | SpecBoolean | SpecString | SpecCellOther))
                    break;
                
                node->convertToIdentity();
                changed = true;
                break;
            }
                
            case Check: {
                alreadyHandled = true;
                m_interpreter.execute(indexInBlock);
                for (unsigned i = 0; i < AdjacencyList::Size; ++i) {
                    Edge edge = node->children.child(i);
                    if (!edge)
                        break;
                    if (edge.isProved() || edge.willNotHaveCheck()) {
                        node->children.removeEdge(i--);
                        changed = true;
                    }
                }
                break;
            }
                
            default:
                break;
            }
            
            if (eliminated) {
                changed = true;
                continue;
            }
                
            if (alreadyHandled)
                continue;
            
            m_interpreter.execute(indexInBlock);
            if (!m_state.isValid()) {
                // If we invalidated then we shouldn't attempt to constant-fold. Here's an
                // example:
                //
                //     c: JSConstant(4.2)
                //     x: ValueToInt32(Check:Int32:@const)
                //
                // It would be correct for an analysis to assume that execution cannot
                // proceed past @x. Therefore, constant-folding @x could be rather bad. But,
                // the CFA may report that it found a constant even though it also reported
                // that everything has been invalidated. This will only happen in a couple of
                // the constant folding cases; most of them are also separately defensive
                // about such things.
                break;
            }
            if (!node->shouldGenerate() || m_state.didClobber() || node->hasConstant())
                continue;
            
            // Interesting fact: this freezing that we do right here may turn an fragile value into
            // a weak value. See DFGValueStrength.h.
            FrozenValue* value = m_graph.freeze(m_state.forNode(node).value());
            if (!*value)
                continue;
            
            NodeOrigin origin = node->origin;
            AdjacencyList children = node->children;
            
            m_graph.convertToConstant(node, value);
            if (!children.isEmpty()) {
                m_insertionSet.insertNode(
                    indexInBlock, SpecNone, Phantom, origin, children);
            }
            
            changed = true;
        }
        m_state.reset();
        m_insertionSet.execute(block);
        
        return changed;
    }
        
    void emitGetByOffset(unsigned indexInBlock, Node* node, const AbstractValue& baseValue, const GetByIdVariant& variant, unsigned identifierNumber)
    {
        NodeOrigin origin = node->origin;
        Edge childEdge = node->child1();

        addBaseCheck(indexInBlock, node, baseValue, variant.structureSet());
        
        JSValue baseForLoad;
        if (variant.alternateBase())
            baseForLoad = variant.alternateBase();
        else
            baseForLoad = baseValue.m_value;
        if (JSValue value = m_graph.tryGetConstantProperty(baseForLoad, variant.baseStructure(), variant.offset())) {
            m_graph.convertToConstant(node, m_graph.freeze(value));
            return;
        }
        
        if (variant.alternateBase()) {
            Node* child = m_insertionSet.insertConstant(indexInBlock, origin, variant.alternateBase());
            childEdge = Edge(child, KnownCellUse);
        } else
            childEdge.setUseKind(KnownCellUse);
        
        Edge propertyStorage;
        
        if (isInlineOffset(variant.offset()))
            propertyStorage = childEdge;
        else {
            propertyStorage = Edge(m_insertionSet.insertNode(
                indexInBlock, SpecNone, GetButterfly, origin, childEdge));
        }
        
        StorageAccessData& data = *m_graph.m_storageAccessData.add();
        data.offset = variant.offset();
        data.identifierNumber = identifierNumber;
        
        node->convertToGetByOffset(data, propertyStorage);
    }

    void emitPutByOffset(unsigned indexInBlock, Node* node, const AbstractValue& baseValue, const PutByIdVariant& variant, unsigned identifierNumber)
    {
        NodeOrigin origin = node->origin;
        Edge childEdge = node->child1();
        
        addBaseCheck(indexInBlock, node, baseValue, variant.oldStructure());

        childEdge.setUseKind(KnownCellUse);

        Transition* transition = 0;
        if (variant.kind() == PutByIdVariant::Transition) {
            transition = m_graph.m_transitions.add(
                variant.oldStructureForTransition(), variant.newStructure());
        }

        Edge propertyStorage;

        if (isInlineOffset(variant.offset()))
            propertyStorage = childEdge;
        else if (!variant.reallocatesStorage()) {
            propertyStorage = Edge(m_insertionSet.insertNode(
                indexInBlock, SpecNone, GetButterfly, origin, childEdge));
        } else if (!variant.oldStructureForTransition()->outOfLineCapacity()) {
            ASSERT(variant.newStructure()->outOfLineCapacity());
            ASSERT(!isInlineOffset(variant.offset()));
            Node* allocatePropertyStorage = m_insertionSet.insertNode(
                indexInBlock, SpecNone, AllocatePropertyStorage,
                origin, OpInfo(transition), childEdge);
            m_insertionSet.insertNode(indexInBlock, SpecNone, StoreBarrier, origin, Edge(node->child1().node(), KnownCellUse));
            propertyStorage = Edge(allocatePropertyStorage);
        } else {
            ASSERT(variant.oldStructureForTransition()->outOfLineCapacity());
            ASSERT(variant.newStructure()->outOfLineCapacity() > variant.oldStructureForTransition()->outOfLineCapacity());
            ASSERT(!isInlineOffset(variant.offset()));

            Node* reallocatePropertyStorage = m_insertionSet.insertNode(
                indexInBlock, SpecNone, ReallocatePropertyStorage, origin,
                OpInfo(transition), childEdge,
                Edge(m_insertionSet.insertNode(
                    indexInBlock, SpecNone, GetButterfly, origin, childEdge)));
            m_insertionSet.insertNode(indexInBlock, SpecNone, StoreBarrier, origin, Edge(node->child1().node(), KnownCellUse));
            propertyStorage = Edge(reallocatePropertyStorage);
        }

        StorageAccessData& data = *m_graph.m_storageAccessData.add();
        data.offset = variant.offset();
        data.identifierNumber = identifierNumber;
        
        node->convertToPutByOffset(data, propertyStorage);

        if (variant.kind() == PutByIdVariant::Transition) {
            // FIXME: PutStructure goes last until we fix either
            // https://bugs.webkit.org/show_bug.cgi?id=142921 or
            // https://bugs.webkit.org/show_bug.cgi?id=142924.
            m_insertionSet.insertNode(
                indexInBlock + 1, SpecNone, PutStructure, origin, OpInfo(transition), childEdge);
        }
    }
    
    void addBaseCheck(
        unsigned indexInBlock, Node* node, const AbstractValue& baseValue, const StructureSet& set)
    {
        if (!baseValue.m_structure.isSubsetOf(set)) {
            // Arises when we prune MultiGetByOffset. We could have a
            // MultiGetByOffset with a single variant that checks for structure S,
            // and the input has structures S and T, for example.
            m_insertionSet.insertNode(
                indexInBlock, SpecNone, CheckStructure, node->origin,
                OpInfo(m_graph.addStructureSet(set)), node->child1());
            return;
        }
        
        if (baseValue.m_type & ~SpecCell) {
            m_insertionSet.insertNode(
                indexInBlock, SpecNone, Phantom, node->origin, node->child1());
        }
    }
    
    void addChecks(
        NodeOrigin origin, unsigned indexInBlock, const ConstantStructureCheckVector& checks)
    {
        for (unsigned i = 0; i < checks.size(); ++i) {
            addStructureTransitionCheck(
                origin, indexInBlock, checks[i].constant(), checks[i].structure());
        }
    }

    void addStructureTransitionCheck(NodeOrigin origin, unsigned indexInBlock, JSCell* cell, Structure* structure)
    {
        if (m_graph.registerStructure(cell->structure()) == StructureRegisteredAndWatched)
            return;
        
        m_graph.registerStructure(structure);

        Node* weakConstant = m_insertionSet.insertNode(
            indexInBlock, speculationFromValue(cell), JSConstant, origin,
            OpInfo(m_graph.freeze(cell)));
        
        m_insertionSet.insertNode(
            indexInBlock, SpecNone, CheckStructure, origin,
            OpInfo(m_graph.addStructureSet(structure)), Edge(weakConstant, CellUse));
    }
    
    void fixUpsilons(BasicBlock* block)
    {
        for (unsigned nodeIndex = block->size(); nodeIndex--;) {
            Node* node = block->at(nodeIndex);
            if (node->op() != Upsilon)
                continue;
            switch (node->phi()->op()) {
            case Phi:
                break;
            case JSConstant:
            case DoubleConstant:
            case Int52Constant:
                node->convertToPhantom();
                break;
            default:
                DFG_CRASH(m_graph, node, "Bad Upsilon phi() pointer");
                break;
            }
        }
    }
    
    InPlaceAbstractState m_state;
    AbstractInterpreter<InPlaceAbstractState> m_interpreter;
    InsertionSet m_insertionSet;
};

bool performConstantFolding(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Constant Folding Phase");
    return runPhase<ConstantFoldingPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


