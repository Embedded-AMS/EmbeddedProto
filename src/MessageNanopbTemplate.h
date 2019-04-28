
#ifndef _MESSAGE_NANOPB_TEMPLATE_H_
#define _MESSAGE_NANOPB_TEMPLATE_H_

#include <MessageInterface.h>

namespace EmbeddedProto {

template<class NANOPB_STRUCT>
class MessageNanopbTemplate : public MessageInterface
{
public:
    MessageNanopbTemplate() = default;
    ~MessageNanopbTemplate() = default;


    NANOPB_STRUCT& get() {
        return data;
    }

    NANOPB_STRUCT& get() const {
        return data;
    }

    NANOPB_STRUCT* operator ->() {
        return &data;
    }

protected:

    //! The actual message struct as used by nanopb.
    NANOPB_STRUCT data;

private:

}

} // End of namespace EmbeddedProto

#endif _MESSAGE_NANOPB_TEMPLATE_H_
