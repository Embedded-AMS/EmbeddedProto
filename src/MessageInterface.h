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

      // Skip serializing empty fields for non-optional fields (proto3 default behavior)
      if(optional || (0 != serialized_size()))
      {
        // Handle TAG and SIZE phases using helper method
        if((::EmbeddedProto::FieldProcessingPhase::TAG == state.phase) || (::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase))
        {
          return_value = serialize_partial_tag_and_size(field_number, serialized_size(), buffer, state, optional);
        }

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
      }
      else
      {
        state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
      }

      return return_value;
    }

    //! \see Field::deserialize_partial_as_field()
    Error deserialize_partial_as_field(ReadBufferInterface& buffer,
                                       MessageState& state) override;

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

} // End of namespace EmbeddedProto

#endif // _MESSAGE_INTERFACE_H_
