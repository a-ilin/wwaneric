#ifndef COMMON_H
#define COMMON_H

#include <QtGlobal>

// enable logging
#define LOG_ENABLED
#define LOG_USE_PTHREADS
#include "log.h"





// QVariant safe convertion macro
#define SAFE_CONVERT(type, vtype, name, variant, bad_command) \
  type name; \
  { \
    bool ok = false; \
    name = variant.vtype(&ok); \
    if (!ok) \
    { \
      bad_command; \
    } \
  }


#endif // COMMON_H
