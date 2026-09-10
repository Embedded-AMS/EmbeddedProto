/*
 *  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
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
 *    the Netherlands
 */

#ifndef _MESSAGE_STATE_H_
#define _MESSAGE_STATE_H_

#include "WireFormatter.h"

#include <array>
#include <cstdint>

namespace EmbeddedProto 
{

  //! Phase of field processing for partial serialization/deserialization.
  enum class FieldProcessingPhase : uint8_t 
  {
    TAG = 0,       //<! Reading/writing field tag (field number + wire type)
    SIZE = 1,      //<! Reading/writing length prefix (for LENGTH_DELIMITED fields)
    DATA = 2,      //<! Reading/writing actual field data
    COMPLETE = 3   //<! Done with this field, ready for next
  };

  //! State for partial serialization/deserialization of a single message.
  /*!
      This structure tracks the current state of a message during partial 
      serialization or deserialization. It is used for both operations to 
      maintain consistency.
  */
  struct MessageState 
  {
      //! Default phase for a new field operation.
      static constexpr::EmbeddedProto::FieldProcessingPhase INITIAL_PHASE = ::EmbeddedProto::FieldProcessingPhase::TAG;

      //! Default wire type value.
      static constexpr WireFormatter::WireType INITIAL_WIRE_TYPE = WireFormatter::WireType::VARINT;

      //! Current phase of field processing.
     ::EmbeddedProto::FieldProcessingPhase phase = INITIAL_PHASE;
      
      //! Field number from protobuf definition (1-based, from tag or next to serialize).
      uint32_t field_id = 0;
      
      //! Wire type from tag (deserialization only, but stored here for simplicity).
      WireFormatter::WireType wire_type = INITIAL_WIRE_TYPE;
      
      //! For repeated fields: index of current element (0-based).
      uint32_t element_index = 0;
      
      //! For length-delimited fields: bytes remaining to read/write.
      uint32_t bytes_remaining = 0;
      
      //! For length-delimited fields: the size once calculated when serializing or read when deserializing.
      uint32_t size_value = 0;
      
      //! Pointer to child state for nested messages (null if leaf).
      MessageState* child = nullptr;
      
      //! Default constructor.
      MessageState() = default;
      

      //! Reset state to initial values.
      void reset()
      {
        phase = INITIAL_PHASE;
        field_id = 0;
        wire_type = INITIAL_WIRE_TYPE;
        element_index = 0;
        bytes_remaining = 0;
        size_value = 0;
        // Note: child pointer is not reset, it's set at construction
      }
      
    protected:
      // Protected copy/move to prevent slicing - state should be managed via MessageStateStack
      MessageState(const MessageState&) = default;
      MessageState(MessageState&&) = default;
      MessageState& operator=(const MessageState&) = default;
      MessageState& operator=(MessageState&&) = default;
  };

  //! Fixed-size stack of MessageState objects for nested message handling.
  /*!
      This template class provides a compile-time fixed array of MessageState 
      objects, linked together to support nested message serialization and 
      deserialization.
      
      \tparam DEPTH The maximum nesting depth this stack can handle.
  */
  template<uint32_t DEPTH>
  class MessageStateStack 
  {

    public:
      //! Constructor - links states together in parent-child chain.
      MessageStateStack() 
      {
        // Link states together: parent -> child
        // states_[0] is the root (outermost message)
        // states_[DEPTH-1] is the deepest possible nested message
        for(uint32_t i = 0; i < DEPTH - 1; ++i) 
        {
          states_[i].child = &states_[i + 1];
        }
        // states_[DEPTH-1].child remains nullptr (leaf)
      }
      
      //! Default destructor.
      ~MessageStateStack() = default;
      
      //! Get the root state (for the outermost message).
      /*!
          \return Reference to the root MessageState.
      */
      MessageState& root() 
      { 
        return states_[0]; 
      }
      
      //! Get the root state (for the outermost message) - const version.
      /*!
          \return Const reference to the root MessageState.
      */
      const MessageState& root() const 
      { 
        return states_[0]; 
      }
      
      //! Get the state at a specific depth.
      /*!
          \param depth 0 = root, 1 = first nested level, etc.
          \return Reference to the MessageState at the specified depth.
      */
      MessageState& at(uint32_t depth) 
      { 
        return states_[depth]; 
      }
      
      //! Get the state at a specific depth - const version.
      /*!
          \param depth 0 = root, 1 = first nested level, etc.
          \return Const reference to the MessageState at the specified depth.
      */
      const MessageState& at(uint32_t depth) const 
      { 
        return states_[depth]; 
      }
      
      //! Reset all states to initial values.
      void reset() 
      {
        for(uint32_t i = 0; i < DEPTH; ++i)
        {
          states_[i].reset();
        }
      }
      
      //! Get the maximum depth this stack can handle.
      /*!
          \return The template parameter DEPTH value.
      */
      static constexpr uint32_t max_depth() 
      { 
        return DEPTH; 
      }
      
    private:
      //! The array of message states.
      std::array<MessageState, DEPTH> states_{};
  };

} // namespace EmbeddedProto

#endif // End of _MESSAGE_STATE_H_