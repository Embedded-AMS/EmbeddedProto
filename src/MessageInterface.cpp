/*
 *  Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
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
 *  Postal adress:
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#include "MessageInterface.h"
#include "WireFormatter.h"

namespace EmbeddedProto
{

  bool MessageInterface::MessageInterface::serialize_with_id(uint32_t field_number, ::EmbeddedProto::WriteBufferInterface& buffer) const
  {
    const uint32_t size_x = this->serialized_size();
    bool result = (size_x < buffer.get_available_size());
    if(result && (0 < size_x))
    {
      uint32_t tag = ::EmbeddedProto::WireFormatter::MakeTag(field_number, 
                              ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED);
      result = ::EmbeddedProto::WireFormatter::SerializeVarint(tag, buffer);
      result = result && ::EmbeddedProto::WireFormatter::SerializeVarint(size_x, buffer);
      const ::EmbeddedProto::Field* base = static_cast<const ::EmbeddedProto::Field*>(this);
      result = result && base->serialize(buffer);
    }
    return result;
  }

} // End of namespace EmbeddedProto
