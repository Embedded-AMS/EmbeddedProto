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

#ifndef _MESSAGE_INTERFACE_H_
#define _MESSAGE_INTERFACE_H_

#include "WireFormatter.h"
#include "Fields.h"
#include "Errors.h"
#include "MessageState.h"
#include "ReadBufferSection.h"

#include <cstdint>


namespace EmbeddedProto 
{

class MessageInterface : public ::EmbeddedProto::Field
{
  public:

    MessageInterface() = default;

    ~MessageInterface() override = default;

    //! \see Field::deserialize()
    Error deserialize(::EmbeddedProto::ReadBufferInterface& buffer) override = 0;

    //! \see Field::deserialize()
    Error deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer, 
                                 const ::EmbeddedProto::WireFormatter::WireType& wire_type) final;

    //! Clear the content of this message and set it to it's default state.
    /*!
        The defaults are to be set according to the Protobuf standard.
    */
    void clear() override = 0;

    //! Serialize this message as a delimited group (editions message_encoding =
    //! DELIMITED).
    /*!
        Writes a START_GROUP tag, this message's fields inline, then an END_GROUP
        tag. Because group framing carries no length prefix there is no size
        pre-pass, which enables single-pass encoding of (nested) messages.

        \param field_number The field number for the START/END group tags.
        \param buffer The buffer to write to.
        \return Error::NO_ERRORS when successful.
    */
    Error serialize_as_group(const uint32_t field_number, WriteBufferInterface& buffer) const
    {
      Error return_value = WireFormatter::SerializeVarint(
          WireFormatter::MakeTag(field_number, WireFormatter::WireType::START_GROUP), buffer);
      if(Error::NO_ERRORS == return_value)
      {
        return_value = this->serialize(buffer);
      }
      if(Error::NO_ERRORS == return_value)
      {
        return_value = WireFormatter::SerializeVarint(
            WireFormatter::MakeTag(field_number, WireFormatter::WireType::END_GROUP), buffer);
      }
      return return_value;
    }

#ifdef PARTIAL_SERIALIZATION_ENABLED
    //! Deserialize message with partial state support.
    /*!
        This method deserializes the message in chunks, allowing deserialization to be paused
        when the buffer ends and resumed with a fresh buffer.

        \param buffer Read buffer (may be small).
        \param state External state object (must persist between calls).
        \return Error::NO_ERRORS when complete.
        \return Error::END_OF_BUFFER when buffer ended, call again with fresh buffer.
        \return Other errors on failure (state should be reset).
    */
    virtual Error deserialize_partial(ReadBufferInterface& buffer,
                                      MessageState& state) = 0;

    //! Serialize message with partial state support.
    /*!
        This method serializes the message in chunks, allowing serialization to be paused
        when the buffer becomes full and resumed with a fresh buffer.
        
        \param buffer Write buffer (may be small).
        \param state External state object (must persist between calls).
        \return Error::NO_ERRORS when complete.
        \return Error::BUFFER_FULL when buffer full, call again with fresh buffer.
        \return Other errors on failure (state should be reset).
    */
    virtual Error serialize_partial(WriteBufferInterface& buffer,
                                    MessageState& state) const = 0;

    Error serialize_partial_as_field(uint32_t field_number,
                                     WriteBufferInterface& buffer,
                                     MessageState& state,
                                     bool optional) const override
    {
      Error return_value = Error::NO_ERRORS;

      // The size of this nested message is calculated once, when the field is entered,
      // and kept in the state. Resuming after a full buffer reuses it instead of
      // walking the whole subtree again for every chunk. Whether the field is emitted
      // at all is decided here as well, so no later phase depends on the stored size.
      if(::EmbeddedProto::FieldProcessingPhase::TAG == state.phase)
      {
        state.size_value = serialized_size();

        // Skip serializing empty fields for non-optional fields (proto3 default behavior)
        if((!optional) && (0U == state.size_value))
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
        }
      }

      // Handle TAG and SIZE phases using helper method
      if((::EmbeddedProto::FieldProcessingPhase::TAG == state.phase) || (::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase))
      {
        return_value = serialize_partial_tag_and_size(field_number, state.size_value, buffer, state, optional);
      }

      // A zero sized field never reaches the data phase, serialize_partial_tag_and_size()
      // marks it complete, so the size no longer has to be checked here.
      if(::EmbeddedProto::FieldProcessingPhase::DATA == state.phase)
      {
        if(nullptr != state.child)
        {
          return_value = this->serialize_partial(buffer, *state.child);
          if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::COMPLETE == state.child->phase))
          {
            state.bytes_remaining = 0U;
            state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          }
        }
        else
        {
          return_value = Error::NESTING_TOO_DEEP;
        }
      }

      return return_value;
    }

    //! Serialize this message as a delimited group with partial state support.
    /*!
        DELIMITED (group) framing has no length prefix, so there is no SIZE phase.
        The phases used are TAG (write START_GROUP) -> DATA (stream the child) ->
        SIZE (reused to write END_GROUP) -> COMPLETE. Each tag is written
        atomically; on a full buffer the phase is left unchanged so the call
        resumes from the same point with a fresh buffer.

        \param field_number The field number for the START/END group tags.
        \param buffer The (possibly small) write buffer.
        \param state The state for this group field; state.child streams the body.
        \return Error::NO_ERRORS when complete, Error::BUFFER_FULL to resume.
    */
    Error serialize_partial_as_group(uint32_t field_number,
                                     WriteBufferInterface& buffer,
                                     MessageState& state) const
    {
      Error return_value = Error::NO_ERRORS;

      // Phase TAG: write the START_GROUP tag.
      if(::EmbeddedProto::FieldProcessingPhase::TAG == state.phase)
      {
        const uint32_t tag = WireFormatter::MakeTag(field_number, WireFormatter::WireType::START_GROUP);
        if(buffer.get_available_size() >= WireFormatter::VarintSize(tag))
        {
          return_value = WireFormatter::SerializeVarint(tag, buffer);
          if(Error::NO_ERRORS == return_value)
          {
            state.phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
          }
        }
        else
        {
          return_value = Error::BUFFER_FULL;
        }
      }

      // Phase DATA: stream the child message's fields.
      if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
      {
        if(nullptr != state.child)
        {
          return_value = this->serialize_partial(buffer, *state.child);
          if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::COMPLETE == state.child->phase))
          {
            // Body complete; the END_GROUP tag still has to be written. Reuse the
            // (otherwise unused) SIZE phase to mark "END_GROUP pending".
            state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
          }
        }
        else
        {
          return_value = Error::NESTING_TOO_DEEP;
        }
      }

      // Phase SIZE (reused): write the END_GROUP tag.
      if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase))
      {
        const uint32_t tag = WireFormatter::MakeTag(field_number, WireFormatter::WireType::END_GROUP);
        if(buffer.get_available_size() >= WireFormatter::VarintSize(tag))
        {
          return_value = WireFormatter::SerializeVarint(tag, buffer);
          if(Error::NO_ERRORS == return_value)
          {
            state.bytes_remaining = 0U;
            state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          }
        }
        else
        {
          return_value = Error::BUFFER_FULL;
        }
      }

      return return_value;
    }

    //! \see Field::deserialize_partial_as_field()
    Error deserialize_partial_as_field(ReadBufferInterface& buffer,
                                       MessageState& state) override;

    //! Deserialize a delimited (group) message field with partial state support.
    /*!
        The opening START_GROUP tag has been consumed by the caller (phase DATA).
        The child is streamed until it consumes its matching END_GROUP, at which
        point the child reports completion and this field is marked COMPLETE.
    */
    Error deserialize_partial_as_group(ReadBufferInterface& buffer,
                                       MessageState& state);

    //! When partially deserializing skip bytes in the buffer of an unknown field.
    Error skip_unknown_field_partial(::EmbeddedProto::ReadBufferInterface& buffer,
                                     MessageState& state) const;
#endif

  protected:
    //! When deserializing skip the bytes in the buffer of an unknown field.
    /*!
        This function is used when a field with an unknown id is encountered to move through the
        buffer to the next tag.
    */
    Error skip_unknown_field(::EmbeddedProto::ReadBufferInterface& buffer,
                             const ::EmbeddedProto::WireFormatter::WireType& wire_type) const;

    Error skip_varint(::EmbeddedProto::ReadBufferInterface& buffer) const;
    Error skip_fixed32(::EmbeddedProto::ReadBufferInterface& buffer) const;
    Error skip_fixed64(::EmbeddedProto::ReadBufferInterface& buffer) const;
    Error skip_length_delimited(::EmbeddedProto::ReadBufferInterface& buffer) const;

    //! Skip an unknown DELIMITED (group) field up to its matching END_GROUP.
    Error skip_group(::EmbeddedProto::ReadBufferInterface& buffer) const;

};


  // Definitions of the member functions declared above. The library is header only, so they are
  // inline.

  inline Error MessageInterface::deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer,
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
  inline Error MessageInterface::deserialize_partial_as_field(::EmbeddedProto::ReadBufferInterface& buffer,
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

  inline Error MessageInterface::deserialize_partial_as_group(::EmbeddedProto::ReadBufferInterface& buffer,
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

  inline Error MessageInterface::skip_unknown_field_partial(::EmbeddedProto::ReadBufferInterface& buffer,
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


  inline Error MessageInterface::skip_unknown_field(::EmbeddedProto::ReadBufferInterface& buffer,
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

  inline Error MessageInterface::skip_group(::EmbeddedProto::ReadBufferInterface& buffer) const
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


  inline Error MessageInterface::skip_varint(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    // Use a 64 bit variable to decode the maximum possible number of bytes. As we do not know
    // the actual type.
    uint64_t dummy;
    return ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, dummy);
  }

  inline Error MessageInterface::skip_fixed32(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    float dummy;
    return ::EmbeddedProto::WireFormatter::DeserializeFloat(buffer, dummy);
  }

  inline Error MessageInterface::skip_fixed64(::EmbeddedProto::ReadBufferInterface& buffer) const
  {
    double dummy;
    return ::EmbeddedProto::WireFormatter::DeserializeDouble(buffer, dummy);
  }

  inline Error MessageInterface::skip_length_delimited(::EmbeddedProto::ReadBufferInterface& buffer) const
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

#endif // _MESSAGE_INTERFACE_H_
