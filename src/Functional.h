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

#ifndef _FUNCTIONAL_H_
#define _FUNCTIONAL_H_

#include <type_traits>

namespace EmbeddedProto
{

//! Lightweight zero-allocation callback wrapper.
/*!
    This class stores callbacks without dynamic allocation and is intended for
    small embedded systems. It supports:
    - C functions with signature void(Args...)
    - C style callbacks with context pointer: void(void*, Args...)
    - C++ member functions (compile-time bound)
    - Callable objects/lambdas by reference (non-owning)

    \tparam Signature Callback signature, currently only void-return signatures.
*/
template<typename Signature>
class Functional;

template<typename... Args>
class Functional<void(Args...)>
{
  public:
    using FunctionCallback = void (*)(Args...);
    using ContextCallback = void (*)(void* context, Args...);

    Functional() = default;
    ~Functional() = default;

    //! Remove any bound callback.
    void clear()
    {
      context_ = nullptr;
      context_callback_ = nullptr;
      function_callback_ = nullptr;
    }

    //! Check if a callback is bound.
    bool is_set() const
    {
      bool result = false;
      if((nullptr != context_callback_) || (nullptr != function_callback_))
      {
        result = true;
      }
      return result;
    }

    //! Bind a plain C/C++ function pointer.
    void set(FunctionCallback function)
    {
      function_callback_ = function;
      context_callback_ = nullptr;
      context_ = nullptr;
    }

    //! Bind a C style callback with context pointer.
    void set(ContextCallback function, void* context)
    {
      function_callback_ = nullptr;
      context_callback_ = function;
      context_ = context;
    }

    //! Bind a member function (non-const) at compile time.
    template<class T, void (T::*METHOD)(Args...)>
    void set(T* instance)
    {
      function_callback_ = nullptr;
      context_callback_ = &member_thunk<T, METHOD>;
      context_ = instance;
    }

    //! Bind a member function (const) at compile time.
    template<class T, void (T::*METHOD)(Args...) const>
    void set(const T* instance)
    {
      function_callback_ = nullptr;
      context_callback_ = &const_member_thunk<T, METHOD>;
      context_ = const_cast<T*>(instance);
    }

    //! Bind a callable object by reference (non-owning).
    template<class Callable>
    void set(Callable& callable,
             typename std::enable_if<!std::is_same<Callable, Functional<void(Args...)>>::value>::type* = nullptr)
    {
      function_callback_ = nullptr;
      context_callback_ = &callable_thunk<Callable>;
      context_ = &callable;
    }

    //! Bind a const callable object by reference (non-owning).
    template<class Callable>
    void set(const Callable& callable,
             typename std::enable_if<!std::is_same<Callable, Functional<void(Args...)>>::value>::type* = nullptr)
    {
      function_callback_ = nullptr;
      context_callback_ = &const_callable_thunk<Callable>;
      context_ = const_cast<Callable*>(&callable);
    }

    //! Invoke the bound callback if set.
    void invoke(Args... args) const
    {
      if(nullptr != context_callback_)
      {
        context_callback_(context_, args...);
      }
      else if(nullptr != function_callback_)
      {
        function_callback_(args...);
      }
      else
      {
        // Intentionally empty.
      }
    }

    //! \brief Convenience operator for invoke().
    void operator()(Args... args) const
    {
      invoke(args...);
    }

  private:
    template<class T, void (T::*METHOD)(Args...)>
    static void member_thunk(void* context, Args... args)
    {
      T* instance = static_cast<T*>(context);
      if(nullptr != instance)
      {
        (instance->*METHOD)(args...);
      }
    }

    template<class T, void (T::*METHOD)(Args...) const>
    static void const_member_thunk(void* context, Args... args)
    {
      const T* instance = static_cast<const T*>(context);
      if(nullptr != instance)
      {
        (instance->*METHOD)(args...);
      }
    }

    template<class Callable>
    static void callable_thunk(void* context, Args... args)
    {
      Callable* callable = static_cast<Callable*>(context);
      if(nullptr != callable)
      {
        (*callable)(args...);
      }
    }

    template<class Callable>
    static void const_callable_thunk(void* context, Args... args)
    {
      const Callable* callable = static_cast<const Callable*>(context);
      if(nullptr != callable)
      {
        (*callable)(args...);
      }
    }

    void* context_ = nullptr;
    ContextCallback context_callback_ = nullptr;
    FunctionCallback function_callback_ = nullptr;
};

} // namespace EmbeddedProto

#endif // _FUNCTIONAL_H_
