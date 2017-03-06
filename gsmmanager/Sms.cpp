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

#include "Sms.h"

#include "../libPdu/src/Pdu.h"

#include "../libPdu/src/pdu_deliver.h"

#include "common.h"

#include <QSqlDatabase>
#include <QSqlError>

bool checkSmsStorage(int storage)
{
  switch (storage)
  {
  case SMS_STORAGE_PHONE:
  case SMS_STORAGE_SIM:
    return true;
  default:
    return false;
  }
}

QString smsStorageStr(SMS_STORAGE storage)
{
  if (storage == SMS_STORAGE_PHONE)
  {
    return QString(QChar('"') + QString(SMS_STORAGE_PHONE_STR) + QChar('"'));
  }
  else if (storage == SMS_STORAGE_SIM)
  {
    return QString(QChar('"') + QString(SMS_STORAGE_SIM_STR) + QChar('"'));
  }

  Q_LOGEX(LOG_VERBOSE_ERROR, "Wrong SMS storage specified!");

  return QString();
}

SMS_STORAGE strToSmsStorage(const QString& str)
{
  if (str == SMS_STORAGE_PHONE_STR)
  {
    return SMS_STORAGE_PHONE;
  }
  else if (str == SMS_STORAGE_SIM_STR)
  {
    return SMS_STORAGE_SIM;
  }

  Q_LOGEX(LOG_VERBOSE_ERROR, "Wrong SMS storage specified!");

  return (SMS_STORAGE)-1;
}

bool checkSmsStatus(int status)
{
  switch(status)
  {
  case SMS_STATUS_NEW:
  case SMS_STATUS_INCOME:
  case SMS_STATUS_DRAFT:
  case SMS_STATUS_SENT:
  case SMS_STATUS_ALL:
    return true;
  default:
    return false;
  }
}


Sms::Sms() :
  valid(false)
{
}

Sms::Sms(SMS_STORAGE storage,
         int index,
         SMS_STATUS status,
         const QByteArray &rawData) :
  valid(false),
  storage(storage),
  index(index),
  status(status),
  rawData(rawData)
{
  const char * pduString = rawData.constData();
#if 0
  PDU pdu(pduString);
  if (pdu.parse())
  {
    smsc = QString::fromLatin1(pdu.getSMSC());
    sender = QString::fromLatin1(pdu.getNumber());
    userText = QString::fromUtf8(pdu.getMessage());

    QStringList msgDate = QString::fromLatin1(pdu.getDate()).split(QString("-"));
    QStringList msgTime = QString::fromLatin1(pdu.getTime()).split(QString(":"));

    if ( (msgDate.size() == 3) && (msgTime.size() == 3) )
    {
      dateTime = QDateTime(QDate(msgDate.at(0).toInt(), msgDate.at(1).toInt(), msgDate.at(2).toInt()),
                                  QTime(msgTime.at(0).toInt(), msgTime.at(1).toInt(), msgTime.at(2).toInt()));
    }

    int udhDataLen = pdu.getUDHData()[0];
    if (udhDataLen)
    {
      udhData.append(pdu.getUDHData()+1, udhDataLen);
    }

    udhType = QString::fromLatin1(pdu.getUDHType());

    //   std::cout << "Message Sender Number Type: " << pdu.getNumberType() << std::endl;
    //   std::cout << "Message Length: " << pdu.getMessageLen() << std::endl;

    valid = true;
  }
  else
  {
    parseError = QString::fromLatin1(pdu.getError());
  }
#else

  Pdu_Deliver * pdu = NULL;

  try
  {
    pdu = dynamic_cast<Pdu_Deliver*> (Pdu::create(std::string(pduString)));
  }
  catch(const char* ex)
  {
    QString err = QString("Error decoding SMS: ") + QString(ex);
    Q_LOGEX(LOG_VERBOSE_ERROR, err);
  }
  catch(...)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unknown exception handled!");
  }

  if (pdu)
  {
    smsc = pdu->getSmsc().getValue();
    sender = pdu->getSenderNumberAsString();
    userText = pdu->getUserData()->getUserData();

    // calculate datetime
    QString timestamp = pdu->getTimeStamp();
    int tzOffsetSeconds = 0;
    if (timestamp.size() == 14)
    {
      // timezone
      {
        // highest bit is set to 1 if the TZ offset is negative
        int sign = 1;
        char tzHigh = timestamp.mid(12, 1).toUInt(NULL, 16);
        if (tzHigh & 0x80)
        {
          sign = -1;
          tzHigh &= 0x7F;
        }
        char tzLow = timestamp.mid(13, 1).toUInt(NULL, 16);

        tzOffsetSeconds = (tzHigh * 10 + tzLow) * sign * 15 * 60;
      }

      QDate date = QDate::fromString(timestamp.mid(0, 6), "yyMMdd");
      // in version Qt 5.2.1 QDate uses 1900 as default year
      if (date < QDate(2000, 1, 1))
      {
        date = date.addYears(100);
      }

      QTime time = QTime::fromString(timestamp.mid(6, 6), "hhmmss");

      dateTime = QDateTime(date, time, Qt::OffsetFromUTC, tzOffsetSeconds);
    }

    // concatenation
    {
      const Pdu_Concatenated * concat = pdu->getConcatenated();
      concatReference  = !concat ? 0 : concat->getReference();
      concatTotalCount = !concat ? 0 : concat->getMax();
      concatPartNumber = !concat ? 0 : concat->getSequence();
    }

    valid = true;
  }

#endif
}


SmsMeta::SmsMeta(const QList<Sms>& smsList)
{
  int id = 0;
  m_valid = true;
  foreach(const Sms& sms, smsList)
  {
    if (!id)
    {
      // this parameters should be the same for all SMS in the message
      m_storage = sms.storage;
      m_status = sms.status;
      m_sender = sms.sender;
      m_smsc = sms.smsc;

      // copy datetime from the first SMS
      m_dateTime = sms.dateTime;
    }
    else
    {
      if (m_storage != sms.storage)
      {
        m_valid = false;
        Q_LOGEX(LOG_VERBOSE_ERROR, "Passed SMSes with different storage types!");
      }

      if (m_status != sms.status)
      {
        m_valid = false;
        Q_LOGEX(LOG_VERBOSE_ERROR, "Passed SMSes with different status types!");
      }

      if (m_sender != sms.sender)
      {
        m_valid = false;
        Q_LOGEX(LOG_VERBOSE_ERROR, "Passed SMSes with different senders!");
      }

      if (m_smsc != sms.smsc)
      {
        m_valid = false;
        Q_LOGEX(LOG_VERBOSE_ERROR, "Passed SMSes with different SMSC!");
      }
    }

    // dateTime can differs in different SMSes, we will use the last
    if (sms.dateTime > m_dateTime)
    {
      m_dateTime = sms.dateTime;
    }

    // concat user message
    m_userText += sms.userText;

    // append SMS index
    if(!m_indexes.contains(sms.index))
    {
      m_indexes.append(sms.index);
    }
    else
    {
      m_valid = false;
      Q_LOGEX(LOG_VERBOSE_ERROR, "Passed SMSes with the same index!");
    }

    // append SMS PDU
    m_pduList.append(sms.rawData);

    ++id;
  }
}


bool SmsDatabaseEntity::isDatabaseReady(Database *db) const
{
  QStringList availableTables = db->qDatabase().tables();
  if (availableTables.contains("sms"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool SmsDatabaseEntity::initializeDatabase(Database *db) const
{
  const QString sqlQuery =
      "CREATE TABLE sms "
      "(a_connection BLOB, i_storage INTEGER, i_status INTEGER, a_raw BLOB, "
      "PRIMARY KEY (a_connection, i_storage, i_status, a_raw) )";

  db->qDatabase().exec(sqlQuery);

  if (db->qDatabase().lastError().isValid())
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, db->qDatabase().lastError().text());
    return false;
  }
  else
  {
    return true;
  }
}

QSqlQuery SmsDatabaseEntity::querySelect(Database *db, const DatabaseKey &key) const
{
  QString resultString =
      "SELECT a_connection, i_storage, i_status, a_raw "
      "FROM sms";

  if (key.isEmpty())
  {
    QSqlQuery query(resultString, db->qDatabase());
    return query;
  }

  resultString.append(" WHERE ");

  QStringList acceptedValues;
  acceptedValues << "a_connection";
  acceptedValues << "i_storage";
  acceptedValues << "i_status";
  acceptedValues << "a_raw";

  bool needAnd = false;

  DatabaseKey::const_iterator iter = key.constBegin();
  DatabaseKey::const_iterator iterEnd = key.constEnd();

  // first pass, construct base query string
  while (iter != iterEnd)
  {
    if (acceptedValues.contains(iter.key()))
    {
      if (needAnd)
      {
        resultString.append(" AND ");
      }

      resultString.append(QChar(' ') + iter.key() + QString(" = :")+ iter.key() + QChar(' '));

      needAnd = true;
    }
    else
    {
      Q_ASSERT(false);
    }

    ++iter;
  }

  QSqlQuery query(db->qDatabase());
  query.prepare(resultString);

  iter = key.constBegin();

  // second pass, bind values
  while (iter != iterEnd)
  {
    if (acceptedValues.contains(iter.key()))
    {
      query.bindValue(QChar(':')+iter.key(), iter.value());
    }
    else
    {
      Q_ASSERT(false);
    }

    ++iter;
  }

  return query;
}

SmsBase SmsDatabaseEntity::createFromSelect(const QList<QVariant> &values) const
{
  Q_ASSERT(values.size() == 4);

  try
  {
    QString connectionId = values.at(0).toString();

    SAFE_CONVERT(int, toInt, storage, values.at(1), throw QString("Error conversion storage!"););
    SAFE_CONVERT(int, toInt, status , values.at(2), throw QString("Error conversion status!"););

    QByteArray rawData = values.at(3).toByteArray();

    if (!checkSmsStorage(storage))
    {
      throw QString("Wrong storage type!");
    }

    if (!checkSmsStatus(status))
    {
      throw QString("Wrong status type!");
    }

    SmsBase smsBase;
    smsBase.connectionId = connectionId;
    smsBase.storage = (SMS_STORAGE)storage;
    smsBase.status = (SMS_STATUS)status;
    smsBase.rawData = rawData;

    return smsBase;
  }
  catch(const QString & str)
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, str);
  }

  return SmsBase();
}

QSqlQuery SmsDatabaseEntity::queryInsert(Database *db, const SmsBase& value) const
{
  QString resultString =
      "INSERT OR REPLACE "
      "INTO sms (a_connection, i_storage, i_status, a_raw) "
      "VALUES (:a_connection, :i_storage, :i_status, :a_raw)";

  QSqlQuery query(db->qDatabase());

  query.prepare(resultString);
  query.bindValue(":a_connection", value.connectionId);
  query.bindValue(":i_storage", (int)value.storage);
  query.bindValue(":i_status", (int)value.status);
  query.bindValue(":a_raw", QString(value.rawData));

  return query;
}

QSqlQuery SmsDatabaseEntity::queryUpdat(Database *db, const DatabaseKey &key, const SmsBase& value) const
{
  return QSqlQuery();
}

QSqlQuery SmsDatabaseEntity::queryDelet(Database *db, const DatabaseKey &key) const
{
  QString queryString =
      "DELETE "
      "FROM sms %1 ";

  QStringList acceptedValues;
  acceptedValues << "a_connection" << "i_storage" << "i_status" << "a_raw";

  QSqlQuery query(db->qDatabase());
  prepareQueryClause(&query, queryString, key, acceptedValues);

  return query;
}


