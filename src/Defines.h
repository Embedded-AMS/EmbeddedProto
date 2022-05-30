/*
 *  Copyright (C) 2020-2022 Embedded AMS B.V. - All Rights Reserved
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

#ifndef _EMBEDDED_PROTO_DEFINES_H_
#define _EMBEDDED_PROTO_DEFINES_H_

#include <type_traits>

namespace EmbeddedProto
{
  template<class T, class U>
  inline constexpr auto is_same =
#if __cplusplus >= 201703L // C++17 and up
    std::is_same_v<T, U>;
#elif __cplusplus >= 201103L // C++11 and C++14
    std::is_same<T, U>::value;
#else // Other
    false;
#endif
}

#endif //_EMBEDDED_PROTO_DEFINES_H_