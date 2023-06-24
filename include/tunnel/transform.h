#ifndef TUNNEL_TRANSFORM_H
#define TUNNEL_TRANSFORM_H

#include <stdexcept>

#include "tunnel/channel.h"
#include "tunnel/processor.h"

namespace tunnel {

template <typename T>
class Transform : public Processor<T> {};

}  // namespace tunnel

#endif
