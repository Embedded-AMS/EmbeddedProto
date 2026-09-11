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

#ifndef _EMBEDDED_PROTO_H_
#define _EMBEDDED_PROTO_H_

//! Includes the whole Embedded Proto library.
/*!
    One include for users who do not want to pick headers. Every header of the library is also
    available on its own as <EmbeddedProto/Name.h>, which keeps compile times down when only a
    few are needed.
*/
#include <EmbeddedProto/BytesStringCallback.h>
#include <EmbeddedProto/Defines.h>
#include <EmbeddedProto/EmptyArray.h>
#include <EmbeddedProto/Errors.h>
#include <EmbeddedProto/Fields.h>
#include <EmbeddedProto/FieldStringBytes.h>
#include <EmbeddedProto/Functional.h>
#include <EmbeddedProto/MessageCallback.h>
#include <EmbeddedProto/MessageInterface.h>
#include <EmbeddedProto/MessageSizeCalculator.h>
#include <EmbeddedProto/MessageState.h>
#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <EmbeddedProto/ReadBufferInterface.h>
#include <EmbeddedProto/ReadBufferSection.h>
#include <EmbeddedProto/RepeatedField.h>
#include <EmbeddedProto/RepeatedFieldCallback.h>
#include <EmbeddedProto/RepeatedFieldFixedSize.h>
#include <EmbeddedProto/Version.h>
#include <EmbeddedProto/WireFormatter.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/WriteBufferInterface.h>

#endif // End of _EMBEDDED_PROTO_H_
