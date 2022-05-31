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

#if __cplusplus >= 201703L // C++17 and up
#include <memory>
#endif

namespace EmbeddedProto
{


#if __cplusplus >= 201703L // C++17 and up

  template<class T, class U> inline constexpr auto is_same = std::is_same_v<T, U>;
  template<class T> inline constexpr auto is_enum = std::is_enum_v<T>;
  template<class Base, class Derived> inline constexpr auto is_base_of = std::is_base_of_v<Base, Derived>;
  
  template<class T> using make_unsigned = typename std::make_unsigned_t<T>;
  template<class T> using make_signed = typename std::make_signed_t<T>;

  template<class T>
  inline constexpr void destroy_at(T* p) 
  {
    std::destroy_at(p);
  }

#elif __cplusplus >= 201402L // C++14

  template<class T, class U> constexpr auto is_same = std::is_same<T, U>::value;
  template<class T> constexpr auto is_enum = std::is_enum<T>::value;
  template<class Base, class Derived> constexpr auto is_base_of = std::is_base_of<Base, Derived>::value;
  
  template<class T> using make_unsigned = typename std::make_unsigned_t<T>;
  template<class T> using make_signed = typename std::make_signed_t<T>;

  template<class T>
  constexpr void destroy_at(T* p) 
  {
    p->~T(); 
  }

#elif __cplusplus >= 201103L // C++11

  template<class T, class U> constexpr bool is_same = std::is_same<T, U>::value;
  template<class T> constexpr bool is_enum = std::is_enum<T>::value;
  template<class Base, class Derived> constexpr bool is_base_of = std::is_base_of<Base, Derived>::value;
  
  template<class T> using make_unsigned = typename std::make_unsigned<T>::type;
  template<class T> using make_signed = typename std::make_signed<T>::type;

  template<class T>
  constexpr void destroy_at(T* p) 
  {
    p->~T(); 
  }

#else // Other

  // TODO 

#endif




}

#endif //_EMBEDDED_PROTO_DEFINES_H_