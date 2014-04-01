#include "common.h"


QList<QByteArray> splitByteArray(const QByteArray &array,
                                 const QByteArray &sep,
                                 SplitByteArrayMode mode)
{
  int arraySize = array.size();
  int sepSize = sep.size();

  if ((sepSize >= arraySize) || (!arraySize) || (!sepSize))
  {
    return QList<QByteArray>() << array;
  }
  else if (array == sep)
  {
    return QList<QByteArray>();
  }

  QList<QByteArray> splitted;
  splitted.reserve(arraySize);

  const char * arrayData = array.constData();
  const char * sepData = sep.constData();

  const char * c = arrayData;
  const char * k = c;

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
  if (rest > 0)
  {
    // there was a split just before exited cycle
    if (k == c)
    {
      splitted.append(QByteArray(c, rest));
    }
    else
    {
      Q_ASSERT(splitted.size() > 0);
      splitted.last().append(c, rest);
    }
  }

  return splitted;
}

