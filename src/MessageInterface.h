

#ifndef _MESSAGE_INTERFACE_H_
#define _MESSAGE_INTERFACE_H_

#include <cstdint>

#include "WireFormatter.h"
#include "Fields.h"

namespace EmbeddedProto 
{

class MessageInterface : public ::EmbeddedProto::Field
{
  public:

    MessageInterface() = default;

    virtual ~MessageInterface() = default;

    // TODO doc
    bool serialize_with_id(uint32_t field_number, ::EmbeddedProto::WriteBufferInterface& buffer) const final;

    //! Clear the content of this message and set it to it's default state.
    /*!
        The defaults are to be set according to the Protobuf standard.
    */
    virtual void clear() = 0;

};

} // End of namespace EmbeddedProto

#endif // _MESSAGE_INTERFACE_H_
