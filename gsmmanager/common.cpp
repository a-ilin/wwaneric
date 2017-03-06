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

#include "common.h"

#include <QByteArray>
#include <QList>
#include <QString>

QList<QByteArray> splitByteArray(const QByteArray &array,
                                 const QByteArray &sep,
                                 SplitByteArrayMode mode)
{
  int arraySize = array.size();
  int sepSize = sep.size();

  if (!arraySize)
  {
    return QList<QByteArray>();
  }
  else if (!sepSize)
  {
    return QList<QByteArray>() << array;
  }

  QList<QByteArray> splitted;
  splitted.reserve(arraySize);

  const char * arrayData = array.constData();
  const char * sepData = sep.constData();

  const char * c = arrayData;
  const char * k = c;

  /* current_pos + sep_size <= array_size */
  while(c + sepSize <= arrayData + arraySize)
  {
    if (!memcmp(c, sepData, sepSize))
    {
      if ((mode == KeepEmptyParts) || (c != k))
      {
        splitted.append(QByteArray(k, c - k));
      }

      if (mode == KeepSeparators)
      {
        splitted.append(QByteArray(c, sepSize));
      }

      c += sepSize;
      k = c;
    }
    else
    {
      ++c;
    }
  }

  if (k < c)
  {
    splitted.append(QByteArray(k, c - k));
  }

  // tail
  int rest = arraySize - (c - arrayData);
  Q_ASSERT(rest >= 0);

  // there was a split just before exited cycle
  if (k == c)
  {
    if ((rest > 0) || (mode == KeepEmptyParts))
    {
      splitted.append(QByteArray(c, rest));
    }
  }
  // add rest (without splitting)
  else if (rest > 0)
  {
    Q_ASSERT(splitted.size() > 0);
    splitted.last().append(c, rest);
  }

  return splitted;
}



QString hexString(const QByteArray& data)
{
  QString str;
  for(int i=0; i< data.size(); ++i)
  {
    uchar byte = data.constData()[i];
    QString hex = QString::number(byte, 16).left(2).toUpper();
    hex = hex.size() > 1 ? hex : QChar('0') + hex;
    str += hex + QChar(' ');
  }
  return str;
}

QString hexString(const QList<QByteArray>& listData)
{
  QString str;
  int n = 0;
  foreach(const QByteArray & data, listData)
  {
    ++n;
    str += QString::number(n) + QString(". ") + hexString(data) + ENDL;
  }
  return str;
}


int compareByteArray(const QByteArray& first, const QByteArray& second)
{
  if (first.size() < second.size())
  {
    return -1;
  }
  else if (first.size() == second.size())
  {
    const char* firstData = first.constData();
    const char* secondData = second.constData();

    return memcmp(firstData, secondData, first.size());
  }

  return 1;
}
