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

#include "MessageInterface.h"
#include "WireFormatter.h"
#include "ReadBufferSection.h"

namespace EmbeddedProto
{

  Error MessageInterface::deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer,
                                                 const ::EmbeddedProto::WireFormatter::WireType& wire_type)
  {
    Error return_value = ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED == wire_type 
                         ? Error::NO_ERRORS : Error::INVALID_WIRETYPE;
    if(Error::NO_ERRORS == return_value)  
    {
      uint32_t size = 0;
      return_value = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, size);
      ::EmbeddedProto::ReadBufferSection bufferSection(buffer, size);
      if(::EmbeddedProto::Error::NO_ERRORS == return_value)
      {
        return_value = deserialize(bufferSection);
      }
    }
    return return_value;
  }

#ifdef PARTIAL_SERIALIZATION_ENABLED
  Error MessageInterface::deserialize_partial_as_field(::EmbeddedProto::ReadBufferInterface& buffer,
                                                       MessageState& state)
  {
    Error return_value = Error::NO_ERRORS;

    if(::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase)
    {
      return_value = deserialize_partial_size_phase(buffer, state);
    }

    if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
    {
      if(nullptr != state.child)
      {
        // The section is clamped to what is currently in the buffer, which may be
        // less than the declared nested message size. Track progress against the
        // declared size using the number of bytes actually consumed.
        ::EmbeddedProto::ReadBufferSection section(buffer, state.bytes_remaining);
        const uint32_t section_size_before = section.get_max_size();
        return_value = this->deserialize_partial(section, *state.child);
        const uint32_t bytes_consumed = section_size_before - section.get_size();
        state.bytes_remaining -= bytes_consumed;

        if(0U == state.bytes_remaining)
        {
          // The whole nested message has been consumed. The child reports
          // END_OF_BUFFER once its bounded section runs out; for a fully received
          // nested message that simply means it is complete.
          if(Error::END_OF_BUFFER == return_value)
          {
            return_value = Error::NO_ERRORS;
          }
          if(Error::NO_ERRORS == return_value)
          {
            state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
            state.child->reset();
          }
        }
      }
      else
      {
        return_value = Error::NESTING_TOO_DEEP;
      }
    }

    return return_value;
  }

  Error MessageInterface::deserialize_partial_as_group(::EmbeddedProto::ReadBufferInterface& buffer,
                                                       MessageState& state)
  {
    Error return_value = Error::NO_ERRORS;

    // Groups have no SIZE phase. The caller consumed the START_GROUP tag and set
    // the phase to DATA.
    if(::EmbeddedProto::FieldProcessingPhase::DATA == state.phase)
    {
      if(nullptr != state.child)
      {
        // Stream the child's fields directly from the buffer. The child's partial
        // deserialize stops (reporting completion) when it consumes its matching
        // END_GROUP; otherwise it returns END_OF_BUFFER to resume later with the
        // child state preserved.
        return_value = this->deserialize_partial(buffer, *state.child);
        if((Error::NO_ERRORS == return_value)
           && (::EmbeddedProto::FieldProcessingPhase::COMPLETE == state.child->phase))
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          state.child->reset();
        }
      }
      else
      {
        return_value = Error::NESTING_TOO_DEEP;
      }
    }

    return return_value;
  }

  Error MessageInterface::skip_unknown_field_partial(::EmbeddedProto::ReadBufferInterface& buffer,
                                                     MessageState& state) const
  {
    Error return_value = Error::STATE_MISMATCH;

    if(::EmbeddedProto::WireFormatter::WireType::VARINT == state.wire_type)
    {
      if(::EmbeddedProto::FieldProcessingPhase::DATA == state.phase)
      {
        return_value = skip_varint(buffer);
        if(Error::NO_ERRORS == return_value)
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
        }
      }
    }
    else if(::EmbeddedProto::WireFormatter::WireType::FIXED32 == state.wire_type)
    {
      if(::EmbeddedProto::FieldProcessingPhase::DATA == state.phase)
      {
        if(0U == state.bytes_remaining)
        {
          state.bytes_remaining = 4U;
        }

        const uint32_t bytes_to_advance = (state.bytes_remaining < buffer.get_size())
          ? state.bytes_remaining
          : buffer.get_size();

        buffer.advance(bytes_to_advance);
        state.bytes_remaining -= bytes_to_advance;

        if(0U == state.bytes_remaining)
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          return_value = Error::NO_ERRORS;
        }
        else
        {
          return_value = Error::END_OF_BUFFER;
        }
      }
    }
    else if(::EmbeddedProto::WireFormatter::WireType::FIXED64 == state.wire_type)
    {
      if(::EmbeddedProto::FieldProcessingPhase::DATA == state.phase)
      {
        if(0U == state.bytes_remaining)
        {
          state.bytes_remaining = 8U;
        }

        const uint32_t bytes_to_advance = (state.bytes_remaining < buffer.get_size())
          ? state.bytes_remaining
          : buffer.get_size();

        buffer.advance(bytes_to_advance);
        state.bytes_remaining -= bytes_to_advance;

        if(0U == state.bytes_remaining)
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          return_value = Error::NO_ERRORS;
        }
        else
        {
          return_value = Error::END_OF_BUFFER;
        }
      }
    }
    else if(::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED == state.wire_type)
    {
      return_value = Error::NO_ERRORS;

      if(::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase)
      {
        return_value = deserialize_partial_size_phase(buffer, state);
      }

      if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
      {
        const uint32_t bytes_to_advance = (state.bytes_remaining < buffer.get_size())
          ? state.bytes_remaining
          : buffer.get_size();

        buffer.advance(bytes_to_advance);
        state.bytes_remaining -= bytes_to_advance;

        if(0U == state.bytes_remaining)
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          return_value = Error::NO_ERRORS;
        }
        else
        {
          return_value = Error::END_OF_BUFFER;
        }
      }
    }

    return return_value;
  }
#endif


  Error MessageInterface::skip_unknown_field(::EmbeddedProto::ReadBufferInterface& buffer,
                                             const ::EmbeddedProto::WireFormatter::WireType& wire_type) const
  {
    Error return_value = Error::NO_ERRORS;

    // Depending on the wire type select one of its valid variable types and deserialize the value.
    switch(wire_type) {
      case ::EmbeddedProto::WireFormatter::WireType::VARINT:
        return_value = skip_varint(buffer);
        break;

      case ::EmbeddedProto::WireFormatter::WireType::FIXED64:
        return_value = skip_fixed64(buffer);
        break;

      case ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED:
        return_value = skip_length_delimited(buffer);
        break;

      case ::EmbeddedProto::WireFormatter::WireType::FIXED32:
        return_value = skip_fixed32(buffer);
        break;

      case ::EmbeddedProto::WireFormatter::WireType::START_GROUP:
        // An unknown DELIMITED field: skip the whole group up to its END_GROUP.
        return_value = skip_group(buffer);
        break;

      default:
        // We should never get here. DeserializeTag catches this case.
        break;
    }

    return return_value;
  }

  Error MessageInterface::skip_group(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    // The opening START_GROUP tag has already been consumed. Read and discard
    // fields until the matching END_GROUP. Nested groups are skipped recursively
    // (skip_unknown_field routes START_GROUP back here), so the first END_GROUP
    // seen at this level is the matching one.
    Error return_value = Error::NO_ERRORS;
    bool done = false;
    while((!done) && (Error::NO_ERRORS == return_value))
    {
      ::EmbeddedProto::WireFormatter::WireType wire_type;
      uint32_t id = 0;
      return_value = ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id);
      if(Error::NO_ERRORS == return_value)
      {
        if(::EmbeddedProto::WireFormatter::WireType::END_GROUP == wire_type)
        {
          done = true;
        }
        else
        {
          return_value = skip_unknown_field(buffer, wire_type);
        }
      }
    }
    return return_value;
  }


  Error MessageInterface::skip_varint(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    // Use a 64 bit variable to decode the maximum possible number of bytes. As we do not know
    // the actual type.
    uint64_t dummy;
    return ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, dummy);
  }

  Error MessageInterface::skip_fixed32(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    float dummy;
    return ::EmbeddedProto::WireFormatter::DeserializeFloat(buffer, dummy);
  }

  Error MessageInterface::skip_fixed64(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    double dummy;
    return ::EmbeddedProto::WireFormatter::DeserializeDouble(buffer, dummy);
  }

  Error MessageInterface::skip_length_delimited(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    // First read the number of bytes 
    uint32_t n_bytes = 0;
    const Error return_value = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, n_bytes);
    if(Error::NO_ERRORS == return_value)
    {
      buffer.advance(n_bytes);
    }
    return return_value;
  }

} // End of namespace EmbeddedProto
