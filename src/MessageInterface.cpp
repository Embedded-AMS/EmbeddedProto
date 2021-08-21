/*
 *  Copyright (C) 2020-2021 Embedded AMS B.V. - All Rights Reserved
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
 *  <https://EmbeddedProto.com/license/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#include "MessageInterface.h"
#include "WireFormatter.h"
#include "ReadBufferSection.h"

namespace EmbeddedProto
{

  Error MessageInterface::MessageInterface::serialize_with_id(uint32_t field_number, 
                                                              ::EmbeddedProto::WriteBufferInterface& buffer) const
  {
    Error return_value = Error::NO_ERRORS;

    // See if we have data which should be serialized.
    const uint32_t size_x = this->serialized_size();
    if(0 < size_x)
    {
      uint32_t tag = WireFormatter::MakeTag(field_number, 
                              WireFormatter::WireType::LENGTH_DELIMITED);
      return_value = WireFormatter::SerializeVarint(tag, buffer);
      
      if(Error::NO_ERRORS == return_value)
      {
        return_value = WireFormatter::SerializeVarint(size_x, buffer);
        if(Error::NO_ERRORS == return_value)
        {
          // See if there is enough space left in the buffer for the data.
          if(size_x <= buffer.get_available_size()) 
          {
            const auto* base = static_cast<const ::EmbeddedProto::Field*>(this);  
            return_value = base->serialize(buffer);
          }
          else
          {
            return_value = Error::BUFFER_FULL;
          }
        }
      }
    }
    return return_value;
  }


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

} // End of namespace EmbeddedProto
