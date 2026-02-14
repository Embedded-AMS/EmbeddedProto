/*
 *  Copyright (C) 2020-2025 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation, version 3 of the license.
 *
 *  Embedded Proto  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
 *
 *  For commercial and closed source application please visit:
 *  <https://embeddedproto.com/pricing/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Atoomweg 2
 *    1627 LE, Hoorn
 *  the Netherlands
 */

#include "gtest/gtest.h"

#include <MessageState.h>

namespace test_EmbeddedAMS_MessageState 
{

//! Test Phase enum values match design document.
TEST(MessageState, PhaseEnumValues) 
{
  // Verify the enum values match the design document
  EXPECT_EQ(0, static_cast<uint8_t>(EmbeddedProto::Phase::TAG));
  EXPECT_EQ(1, static_cast<uint8_t>(EmbeddedProto::Phase::SIZE));
  EXPECT_EQ(2, static_cast<uint8_t>(EmbeddedProto::Phase::DATA));
  EXPECT_EQ(3, static_cast<uint8_t>(EmbeddedProto::Phase::COMPLETE));
}

//! Test MessageState default initialization.
TEST(MessageState, DefaultInitialization) 
{
  EmbeddedProto::MessageState state;
  
  EXPECT_EQ(EmbeddedProto::Phase::TAG, state.phase);
  EXPECT_EQ(0U, state.field_id);
  EXPECT_EQ(EmbeddedProto::WireFormatter::WireType::VARINT, state.wire_type);
  EXPECT_EQ(0U, state.element_index);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_EQ(0U, state.size_value);
  EXPECT_EQ(nullptr, state.child);
}

//! Test MessageState::reset() functionality.
TEST(MessageState, Reset) 
{
  EmbeddedProto::MessageState state;
  
  // Set some non-default values
  state.phase = EmbeddedProto::Phase::DATA;
  state.field_id = 42;
  state.wire_type = EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED;
  state.element_index = 5;
  state.bytes_remaining = 100;
  state.size_value = 200;
  
  // Reset and verify defaults
  state.reset();
  
  EXPECT_EQ(EmbeddedProto::Phase::TAG, state.phase);
  EXPECT_EQ(0U, state.field_id);
  EXPECT_EQ(EmbeddedProto::WireFormatter::WireType::VARINT, state.wire_type);
  EXPECT_EQ(0U, state.element_index);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_EQ(0U, state.size_value);
  // child pointer should not be reset
  EXPECT_EQ(nullptr, state.child);
}

//! Test MessageState reset preserves child pointer.
TEST(MessageState, ResetPreservesChildPointer) 
{
  EmbeddedProto::MessageState parent;
  EmbeddedProto::MessageState child;
  
  parent.child = &child;
  parent.field_id = 10;
  
  parent.reset();
  
  // Child pointer should still be set
  EXPECT_EQ(&child, parent.child);
  EXPECT_EQ(0U, parent.field_id);
}

//! Test MessageStateStack construction with depth 1.
TEST(MessageStateStack, ConstructionDepth1) 
{
  EmbeddedProto::MessageStateStack<1> stack;
  
  EXPECT_EQ(1U, stack.max_depth());
  EXPECT_EQ(nullptr, stack.root().child);
}

//! Test MessageStateStack construction with depth 3.
TEST(MessageStateStack, ConstructionDepth3) 
{
  EmbeddedProto::MessageStateStack<3> stack;
  
  EXPECT_EQ(3U, stack.max_depth());
  
  // Verify child chain: root -> child -> child -> nullptr
  EXPECT_NE(nullptr, stack.root().child);
  EXPECT_NE(nullptr, stack.root().child->child);
  EXPECT_EQ(nullptr, stack.root().child->child->child);
}

//! Test MessageStateStack::root() returns correct state.
TEST(MessageStateStack, RootAccessor) 
{
  EmbeddedProto::MessageStateStack<3> stack;
  
  // root() should return states_[0]
  EXPECT_EQ(&stack.root(), &stack.at(0));
}

//! Test MessageStateStack::at() returns correct states.
TEST(MessageStateStack, AtAccessor) 
{
  EmbeddedProto::MessageStateStack<3> stack;
  
  // Set different field_ids to verify at() returns different states
  stack.at(0).field_id = 1;
  stack.at(1).field_id = 2;
  stack.at(2).field_id = 3;
  
  EXPECT_EQ(1U, stack.at(0).field_id);
  EXPECT_EQ(2U, stack.at(1).field_id);
  EXPECT_EQ(3U, stack.at(2).field_id);
}

//! Test MessageStateStack::at() const version.
TEST(MessageStateStack, AtAccessorConst) 
{
  const EmbeddedProto::MessageStateStack<2> stack;
  
  // Should compile and return const references
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.at(0).phase);
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.at(1).phase);
}

//! Test MessageStateStack::root() const version.
TEST(MessageStateStack, RootAccessorConst) 
{
  const EmbeddedProto::MessageStateStack<2> stack;
  
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.root().phase);
}

//! Test MessageStateStack::reset() resets all states.
TEST(MessageStateStack, ResetAll) 
{
  EmbeddedProto::MessageStateStack<3> stack;
  
  // Set non-default values on all states
  stack.at(0).phase = EmbeddedProto::Phase::COMPLETE;
  stack.at(0).field_id = 100;
  stack.at(1).phase = EmbeddedProto::Phase::DATA;
  stack.at(1).field_id = 200;
  stack.at(2).phase = EmbeddedProto::Phase::SIZE;
  stack.at(2).field_id = 300;
  
  // Reset all
  stack.reset();
  
  // Verify all are reset
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.at(0).phase);
  EXPECT_EQ(0U, stack.at(0).field_id);
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.at(1).phase);
  EXPECT_EQ(0U, stack.at(1).field_id);
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.at(2).phase);
  EXPECT_EQ(0U, stack.at(2).field_id);
  
  // Child pointers should still be linked
  EXPECT_NE(nullptr, stack.at(0).child);
  EXPECT_NE(nullptr, stack.at(1).child);
  EXPECT_EQ(nullptr, stack.at(2).child);
}

//! Test MessageStateStack::max_depth() is constexpr.
TEST(MessageStateStack, MaxDepthConstexpr) 
{
  // This should compile as a constexpr expression
  static_assert(EmbeddedProto::MessageStateStack<1>::max_depth() == 1, "max_depth should be constexpr");
  static_assert(EmbeddedProto::MessageStateStack<5>::max_depth() == 5, "max_depth should be constexpr");
  
  EXPECT_EQ(1U, EmbeddedProto::MessageStateStack<1>::max_depth());
  EXPECT_EQ(5U, EmbeddedProto::MessageStateStack<5>::max_depth());
}

//! Test that child chain is correctly set up after construction.
TEST(MessageStateStack, ChildChainCorrect) 
{
  EmbeddedProto::MessageStateStack<4> stack;
  
  // Verify the chain: root.child == &at(1), at(1).child == &at(2), etc.
  EXPECT_EQ(&stack.at(1), stack.at(0).child);
  EXPECT_EQ(&stack.at(2), stack.at(1).child);
  EXPECT_EQ(&stack.at(3), stack.at(2).child);
  EXPECT_EQ(nullptr, stack.at(3).child);
}

//! Test MessageState with various wire types.
TEST(MessageState, WireTypes) 
{
  EmbeddedProto::MessageState state;
  
  state.wire_type = EmbeddedProto::WireFormatter::WireType::VARINT;
  EXPECT_EQ(EmbeddedProto::WireFormatter::WireType::VARINT, state.wire_type);
  
  state.wire_type = EmbeddedProto::WireFormatter::WireType::FIXED64;
  EXPECT_EQ(EmbeddedProto::WireFormatter::WireType::FIXED64, state.wire_type);
  
  state.wire_type = EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED;
  EXPECT_EQ(EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED, state.wire_type);
  
  state.wire_type = EmbeddedProto::WireFormatter::WireType::FIXED32;
  EXPECT_EQ(EmbeddedProto::WireFormatter::WireType::FIXED32, state.wire_type);
}

//! Test typical usage pattern for partial serialization.
TEST(MessageStateStack, TypicalUsagePattern) 
{
  // Simulate a message with nested message (depth 2)
  EmbeddedProto::MessageStateStack<2> stack;
  
  // Start with root message in TAG phase
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.root().phase);
  
  // Simulate transitioning through phases
  stack.root().phase = EmbeddedProto::Phase::SIZE;
  stack.root().field_id = 1;
  stack.root().size_value = 10;
  
  EXPECT_EQ(EmbeddedProto::Phase::SIZE, stack.root().phase);
  EXPECT_EQ(1U, stack.root().field_id);
  EXPECT_EQ(10U, stack.root().size_value);
  
  // Transition to nested message via child
  stack.root().phase = EmbeddedProto::Phase::DATA;
  stack.root().bytes_remaining = 10;
  
  // Nested message starts in TAG phase
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.root().child->phase);
  
  // Reset for next message
  stack.reset();
  EXPECT_EQ(EmbeddedProto::Phase::TAG, stack.root().phase);
  EXPECT_EQ(0U, stack.root().field_id);
}

} // namespace test_EmbeddedAMS_MessageState