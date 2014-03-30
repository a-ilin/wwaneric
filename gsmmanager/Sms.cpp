#include "Sms.h"

#include "../pdu/pdu.h"

#include "common.h"

#include <QSqlDatabase>
#include <QSqlError>

SmsMeta::SmsMeta() :
  m_valid(false)
{

}

SmsMeta::SmsMeta(SMS_STORAGE storage, SMS_STATUS status, int index) :
  m_valid(false),
  m_storage(storage),
  m_index(index),
  m_status(status)
{
}

Sms::Sms() :
  SmsMeta()
{
}

Sms::Sms(SMS_STORAGE storage, SMS_STATUS status, int index, const QByteArray &rawData) :
  SmsMeta(storage, status, index),
  m_rawData(rawData)
{
  const char * pduString = rawData.constData();
  PDU pdu(pduString);
  if (pdu.parse())
  {
    m_smsc = QString::fromLatin1(pdu.getSMSC());
    m_sender = QString::fromLatin1(pdu.getNumber());
    m_userText = QString::fromUtf8(pdu.getMessage());

    QStringList msgDate = QString::fromLatin1(pdu.getDate()).split(QString("-"));
    QStringList msgTime = QString::fromLatin1(pdu.getTime()).split(QString(":"));

    if ( (msgDate.size() == 3) && (msgTime.size() == 3) )
    {
      m_dateTime = QDateTime(QDate(msgDate.at(0).toInt(), msgDate.at(1).toInt(), msgDate.at(2).toInt()),
                                  QTime(msgTime.at(0).toInt(), msgTime.at(1).toInt(), msgTime.at(2).toInt()));
    }

    int udhDataLen = pdu.getUDHData()[0];
    if (udhDataLen)
    {
      m_udhData.append(pdu.getUDHData()+1, udhDataLen);
    }

    m_udhType = QString::fromLatin1(pdu.getUDHType());

    //   std::cout << "Message Sender Number Type: " << pdu.getNumberType() << std::endl;
    //   std::cout << "Message Length: " << pdu.getMessageLen() << std::endl;

    m_valid = true;
  }
  else
  {
    m_parseError = QString::fromLatin1(pdu.getError());
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

    Sms sms(smsStorage, smsStatus, index, rawData);

    if (!sms.isValid())
    {
      throw sms.parseError();
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
  query.bindValue(":i_storage", (int)value.storage());
  query.bindValue(":i_index", value.index());
  query.bindValue(":i_status", (int)value.status());
  query.bindValue(":a_raw", QString(value.rawData()));

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
