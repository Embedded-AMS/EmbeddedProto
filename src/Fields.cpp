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
 *    the Netherlands
 */

#include "Fields.h"
#include "MessageSizeCalculator.h"
#include "WireFormatter.h"

namespace EmbeddedProto 
{
  uint32_t Field::serialized_size() const
  {
    ::EmbeddedProto::MessageSizeCalculator calcBuffer;
    this->serialize(calcBuffer);
    return calcBuffer.get_size();
  }

  Error Field::serialize_len(const uint32_t field_number, 
                             const uint32_t size,
                             WriteBufferInterface& buffer, 
                             const bool optional) const
  {
    Error return_value = Error::NO_ERRORS;
    
    if((0 < size) || optional)
    {
      return_value = WireFormatter::SerializeVarint(
          WireFormatter::MakeTag(field_number, WireFormatter::WireType::LENGTH_DELIMITED), 
          buffer);
      
      if(Error::NO_ERRORS == return_value)
      {
        return_value = WireFormatter::SerializeVarint(size, buffer);
        
        if(Error::NO_ERRORS == return_value)
        {
          // Check if there's enough space for the data after writing tag and size
          if(size <= buffer.get_available_size())
          {
            return_value = serialize(buffer);
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

  Error Field::serialize_scalar(const uint32_t field_number,
                                const WireFormatter::WireType wire_type,
                                const bool is_default,
                                WriteBufferInterface& buffer, 
                                const bool optional) const
  {
    Error return_value = Error::NO_ERRORS;
    
    if(optional || !is_default)
    {
      return_value = WireFormatter::SerializeVarint(
          WireFormatter::MakeTag(field_number, wire_type), 
          buffer);
      
      if(Error::NO_ERRORS == return_value)
      {
        return_value = serialize(buffer);
      }
    }
    
    return return_value;
  }

} // End of namespace EmbeddedProto
