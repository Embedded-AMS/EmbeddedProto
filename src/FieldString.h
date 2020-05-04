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
 *  Postal address:
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#ifndef _FIELD_STRING_H_
#define _FIELD_STRING_H_

#include "Fields.h"
#include "Errors.h"

#include <string.h>

namespace EmbeddedProto
{

  template<uint32_t LENGTH>
  class FieldString : public Field 
  {
    public:

      FieldString()
        : current_length_(0),
          string_{}
      {

      }

      virtual ~FieldString()
      {
        clear();
      }

      //! Obtain the number of characters in the string right now.
      uint32_t get_length() const { return current_length_; }

      //! Obtain the maximum number characters in the string.
      uint32_t get_max_length() const { return LENGTH; }

      //! Assignment operator for character array's, only use null terminated strings!
      void operator=(const char* const &&rhs)
      {
        const uint32_t rhs_length = strlen(rhs);
        current_length_ = std::min(rhs_length, LENGTH);
        strncpy(string_, rhs, current_length_);

        // Make sure the string is null terminated.
        if(LENGTH > current_length_)
        {
          string_[current_length_] = 0;
        }
      }

      const char* get() const { return string_; }

      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer) const override 
      {
        Error return_value = Error::NO_ERRORS;

        if(0 < current_length_) 
        {
          if(current_length_ <= buffer.get_available_size())
          {
            uint32_t tag = WireFormatter::MakeTag(field_number, 
                                                  WireFormatter::WireType::LENGTH_DELIMITED);
            return_value = WireFormatter::SerializeVarint(tag, buffer);
            if(Error::NO_ERRORS == return_value) 
            {
              return_value = serialize(buffer);
            }
          }
          else 
          {
            return_value = Error::BUFFER_FULL;
          }
        }

        return return_value;
      }

      Error serialize(WriteBufferInterface& buffer) const override 
      { 
        Error return_value = WireFormatter::SerializeVarint(current_length_, buffer);
        if(Error::NO_ERRORS == return_value) 
        {
          const void* void_pointer = static_cast<const void*>(&(string_[0]));
          const uint8_t* byte_pointer = static_cast<const uint8_t*>(void_pointer);
          if(!buffer.push(byte_pointer, current_length_))
          {
            return_value = Error::BUFFER_FULL;
          }
        }
        return return_value;
      }

      Error deserialize(ReadBufferInterface& buffer) override 
      {
        uint32_t availiable;
        Error return_value = WireFormatter::DeserializeVarint(buffer, availiable);
        if(Error::NO_ERRORS == return_value)
        {
          if(LENGTH >= availiable) 
          {
            clear();

            uint8_t byte;
            while((current_length_ < availiable) && buffer.pop(byte)) 
            {
              string_[current_length_] = static_cast<char>(byte);
              ++current_length_;
            }

            if(current_length_ != availiable)
            {
              // If at the end we did not read the same number of characters something went wrong.
              return_value = Error::END_OF_BUFFER;
            }
          }
          else 
          {
            return_value = Error::ARRAY_FULL;
          }
        }
        else 
        {
          current_length_ = 0;
        }

        return return_value;
      }

      //! Reset the field to it's initial value.
      void clear() override 
      { 
        memset(string_, 0, current_length_);
        current_length_ = 0; 
      }
  
    private:

      //! Number of item in the data array.
      uint32_t current_length_;

      //! The text.
      char string_[LENGTH];
  };

} // End of namespace EmbeddedProto

#endif // End of _FIELD_STRING_H_