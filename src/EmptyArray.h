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

#ifndef _EMPTY_ARRAY_H_
#define _EMPTY_ARRAY_H_

#include <cstdint>

namespace EmbeddedProto
{
  namespace internal
  {

    //! Stand-in for std::array<T, 0> that is one byte on every standard library.
    /*!
        libc++ gives std::array<T, 0> the size of one T so that data() stays aligned, libstdc++
        makes it one byte. A field with a maximum length of zero holds no element, so the fixed size
        containers use this class instead of the array there. Every element access lands on one
        scratch element shared by all instances for the same T. That keeps an out of range access
        memory safe, but a value written to the scratch element is lost by design.
    */
    template<class T>
    class EmptyArray
    {
      public:
        static constexpr uint32_t size() { return 0U; }

        T* data() { return &storage(); }
        const T* data() const { return &storage(); }

        //! Begin and end are equal, a range based loop does not run.
        T* begin() { return data(); }
        T* end() { return data(); }
        const T* begin() const { return data(); }
        const T* end() const { return data(); }

        //! Any index lands on the scratch element, reset to its default value first.
        T& operator[](uint32_t) { return reset(); }
        const T& operator[](uint32_t) const { return reset(); }

        void fill(const T&) {}

      private:
        static T& storage()
        {
          static T element = T();
          return element;
        }

        static T& reset()
        {
          T& element = storage();
          element = T();
          return element;
        }
    };

  } // End of namespace internal
} // End of namespace EmbeddedProto

#endif // End of _EMPTY_ARRAY_H_
