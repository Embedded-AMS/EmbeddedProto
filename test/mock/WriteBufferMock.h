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

#ifndef _WRITE_BUFFER_MOCK_H_
#define _WRITE_BUFFER_MOCK_H_

#include <gmock/gmock.h>

#include <EmbeddedProto/WriteBufferInterface.h>

namespace Mocks
{

  //! Raw gmock class for the write buffer, use WriteBufferMock in tests.
  class WriteBufferMockBase : public EmbeddedProto::WriteBufferInterface
  {
    public:
      WriteBufferMockBase()
      {
        // Forward a block push byte by byte to push(uint8_t) unless a test expects the
        // block call itself. Tests that list the expected bytes keep working this way.
        ON_CALL(*this, push(::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(this, &WriteBufferMockBase::push_each_byte));
      }

      MOCK_METHOD0(clear, void());
      MOCK_CONST_METHOD0(get_size, uint32_t());
      MOCK_CONST_METHOD0(get_max_size, uint32_t());
      MOCK_CONST_METHOD0(get_available_size, uint32_t());
      
      MOCK_METHOD1(push, bool(uint8_t));
      MOCK_METHOD2(push, bool(const uint8_t*, const uint32_t));

      bool push_each_byte(const uint8_t* bytes, const uint32_t length)
      {
        bool result = true;
        for(uint32_t i = 0; result && (i < length); ++i)
        {
          result = push(bytes[i]);
        }
        return result;
      }
      
      MOCK_CONST_METHOD1(peak, bool(uint8_t&));
      MOCK_CONST_METHOD0(peak, uint8_t());
      MOCK_METHOD0(advance, void());
      MOCK_METHOD1(advance, void(uint32_t));
      
      MOCK_METHOD1(pop, bool(uint8_t&));
      MOCK_METHOD0(pop, uint8_t());
  };

  //! Nice so the forwarded block pushes do not produce uninteresting-call warnings.
  using WriteBufferMock = ::testing::NiceMock<WriteBufferMockBase>;

} // End of namespace Mocks

#endif // End of _WRITE_BUFFER_MOCK_H_
