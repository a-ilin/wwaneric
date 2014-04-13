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
