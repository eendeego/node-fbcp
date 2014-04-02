#ifndef NODE_FBCP_H_
#define NODE_FBCP_H_

#include <node.h>
#include <v8.h>

#include "v8_helpers.h"

using namespace v8;

namespace fbcp {

V8_METHOD_DECL(Startup);
V8_METHOD_DECL(Shutdown);
V8_METHOD_DECL(FullScreenCopy);
V8_METHOD_DECL(RegionCopy);

}

#endif

