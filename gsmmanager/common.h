#ifndef COMMON_H
#define COMMON_H

#include <QByteArray>
#include <QList>
#include <QtGlobal>


// set OS flag
#if defined(Q_OS_WIN) || defined(_WIN16) || \
    defined(_WIN32) || defined(_WIN64) || \
    defined (__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

#define F_OS_WINDOWS

#else

#define F_OS_UNIX

#endif


// enable logging
#define LOG_ENABLED
#define LOG_USE_PTHREADS
#include "log.h"

// line ending
#ifdef F_OS_WINDOWS
#define ENDL "\r\n"
#else
#define ENDL "\n"
#endif

// returns data as a HEX formatted string
QString hexString(const QByteArray & data);
QString hexString(const QList<QByteArray> & listData);

// QVariant safe convertion macro
#define SAFE_CONVERT(type, method, name, variant, bad_command) \
  type name; \
  { \
    bool ok = false; \
    name = variant.method(&ok); \
    if (!ok) \
    { \
      bad_command; \
    } \
  }




/*
 * Splittes the byte array into list of arrays using separator.
 *
 * Assume that 'X' is a separator. Then:
 *
 * KeepDataOnly:   (abcXXXabcXabcXXabc) ==> (abc)(abc)(abc)(abc)
 * KeepEmptyParts: (abcXXXabcXabcXXabc) ==> (abc)()()(abc)(abc)()(abc)
 * KeepSeparators: (abcXXXabcXabcXXabc) ==> (abc)(X)(X)(X)(abc)(X)(abc)(X)(X)(abc)
 *
 */

enum SplitByteArrayMode
{
  KeepDataOnly,
  KeepEmptyParts,
  KeepSeparators
};

QList<QByteArray> splitByteArray(const QByteArray &array,
                                 const QByteArray &sep,
                                 SplitByteArrayMode mode);



#endif // COMMON_H
