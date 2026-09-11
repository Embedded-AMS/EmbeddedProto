/*
 *  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3 of the license.
 */

#ifndef _MOCK_BYTES_STORAGE_H_
#define _MOCK_BYTES_STORAGE_H_

#include <EmbeddedProto/FieldStringBytes.h>

namespace Mocks
{

//! A user supplied storage type for a bytes field used to test the customStorage option.
/*!
    It derives from the built in ::EmbeddedProto::FieldBytes so it provides the complete bytes
    interface. It exposes a static counter to prove the generated code instantiates this type
    instead of the default storage.
*/
template<uint32_t MAX_LENGTH>
class MockBytesStorage final : public ::EmbeddedProto::FieldBytes<MAX_LENGTH>
{
  public:
    MockBytesStorage()
    {
      ++instance_count;
    }

    ~MockBytesStorage() override = default;

    //! The number of MockBytesStorage objects currently alive. Used by the tests.
    static uint32_t instance_count;
};

template<uint32_t MAX_LENGTH>
uint32_t MockBytesStorage<MAX_LENGTH>::instance_count = 0U;

} // namespace Mocks

#endif // _MOCK_BYTES_STORAGE_H_
