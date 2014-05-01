#include "Sms.h"

#include "../pdu/pdu.h"

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

Sms::Sms(SMS_STORAGE storage, int index, SMS_STATUS status, const QByteArray &rawData) :
  storage(storage),
  index(index),
  status(status),
  rawData(rawData)
{
  const char * pduString = rawData.constData();
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
    valid = false;
    parseError = QString::fromLatin1(pdu.getError());
  }
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
      "(i_storage INTEGER, i_index INTEGER, i_status INTEGER, a_raw BLOB, "
      "PRIMARY KEY (i_storage, i_index, i_status) )";

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
      "SELECT i_storage, i_index, i_status, a_raw "
      "FROM sms";

  if (key.isEmpty())
  {
    QSqlQuery query(resultString, db->qDatabase());
    return query;
  }

  resultString.append(" WHERE ");

  QStringList acceptedValues;
  acceptedValues << "i_storage";
  acceptedValues << "i_index";
  acceptedValues << "i_status";

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

Sms SmsDatabaseEntity::createFromSelect(const QList<QVariant> &values) const
{
  Q_ASSERT(values.size() == 4);

  try
  {
    SAFE_CONVERT(int, toInt, storage, values.at(0), throw QString("Error conversion storage!"););
    SAFE_CONVERT(int, toInt, index  , values.at(1), throw QString("Error conversion index!"););
    SAFE_CONVERT(int, toInt, status , values.at(2), throw QString("Error conversion status!"););

    QByteArray rawData = values.at(3).toByteArray();

    SMS_STORAGE smsStorage;
    switch(storage)
    {
    case SMS_STORAGE_PHONE:
      smsStorage = SMS_STORAGE_PHONE;
      break;
    case SMS_STORAGE_SIM:
      smsStorage = SMS_STORAGE_SIM;
      break;
    default:
      throw QString("Wrong storage type!");
    }

    SMS_STATUS smsStatus;
    switch(status)
    {
    case SMS_STATUS_NEW:
      smsStatus = SMS_STATUS_NEW;
      break;
    case SMS_STATUS_INCOME:
      smsStatus = SMS_STATUS_INCOME;
      break;
    case SMS_STATUS_DRAFT:
      smsStatus = SMS_STATUS_DRAFT;
      break;
    case SMS_STATUS_SENT:
      smsStatus = SMS_STATUS_SENT;
      break;
    default:
      throw QString("Wrong status type!");
    }

    Sms sms(smsStorage, index, smsStatus, rawData);

    if (!sms.valid)
    {
      throw sms.parseError;
    }

    return sms;
  }
  catch(const QString & str)
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, str);
  }

  return Sms();
}

QSqlQuery SmsDatabaseEntity::queryInsert(Database *db, const Sms &value) const
{
  QString resultString =
      "INSERT OR REPLACE "
      "INTO sms (i_storage, i_index, i_status, a_raw) "
      "VALUES (:i_storage, :i_index, :i_status, :a_raw)";

  QSqlQuery query(db->qDatabase());

  query.prepare(resultString);
  query.bindValue(":i_storage", (int)value.storage);
  query.bindValue(":i_index", value.index);
  query.bindValue(":i_status", (int)value.status);
  query.bindValue(":a_raw", QString(value.rawData));

  return query;
}

QSqlQuery SmsDatabaseEntity::queryUpdat(Database *db, const DatabaseKey &key, const Sms &value) const
{
  return QSqlQuery();
}

QSqlQuery SmsDatabaseEntity::queryDelet(Database *db, const DatabaseKey &key) const
{
  return QSqlQuery();
}

