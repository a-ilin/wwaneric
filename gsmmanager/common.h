/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

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
    name = (type)variant.method(&ok); \
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
 * KeepDataOnly:   (XXabcXXXabcXabcXXabcXX) ==>        (abc)         (abc)   (abc)      (abc)
 * KeepEmptyParts: (XXabcXXXabcXabcXXabcXX) ==> () ()  (abc)  () ()  (abc)   (abc)  ()  (abc)  () ()
 * KeepSeparators: (XXabcXXXabcXabcXXabcXX) ==>  (X)(X)(abc)(X)(X)(X)(abc)(X)(abc)(X)(X)(abc)(X)(X)
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


/*
 * Byte-to-byte comparison of byte arrays.
 * Return value:
 * 0 if arrays are the same,
 * positive value if first's size is bigger or first non-equal byte is bigger,
 * negative value otherwise.
 * Note that return value does not correspond to quantity of different bytes!
 */
int compareByteArray(const QByteArray& first, const QByteArray& second);

#endif // COMMON_H
