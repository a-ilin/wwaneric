#include "Ussd.h"

#include "common.h"

bool checkUssdStatus(int status)
{
  switch(status)
  {
  case USSD_STATUS_FINISHED:
  case USSD_STATUS_USER_ACTION_NEEDED:
  case USSD_STATUS_DIALOGUE_TERMINATED:
  case USSD_STATUS_OTHER_IO_RESPONDED:
  case USSD_STATUS_OPERATION_NOT_SUPPORTED:
  case USSD_STATUS_NETWORK_TIMEOUT:
    return true;
  default:
    return false;
  }
}

bool checkUssdSendStatus(int status)
{
  switch(status)
  {
  case USSD_SEND_STATUS_CODE_PRESENTATION_OFF:
  case USSD_SEND_STATUS_CODE_PRESENTATION_ON:
  case USSD_SEND_STATUS_DIALOGUE_TERMINATE:
    return true;
  default:
    return false;
  }
}

bool UssdDatabaseEntity::isDatabaseReady(Database *db) const
{
  QStringList availableTables = db->qDatabase().tables();
  if (availableTables.contains("ussd"))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool UssdDatabaseEntity::initializeDatabase(Database *db) const
{
  const QString sqlQuery =
      "CREATE TABLE ussd "
      "(a_ussd BLOB, a_description BLOB, "
      "PRIMARY KEY (a_ussd) )";

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

QSqlQuery UssdDatabaseEntity::querySelect(Database *db, const DatabaseKey &key) const
{
  QString resultString =
      "SELECT a_ussd, a_description "
      "FROM ussd";

  if (key.isEmpty())
  {
    QSqlQuery query(resultString, db->qDatabase());
    return query;
  }

  resultString.append(" WHERE ");

  QStringList acceptedValues;
  acceptedValues << "a_ussd";

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

Ussd UssdDatabaseEntity::createFromSelect(const QList<QVariant> &values) const
{
  Q_ASSERT(values.size() == 2);

  QString ussdText = values.at(0).toString();
  QString ussdDescription = values.at(1).toString();

  Ussd ussd;
  ussd.ussd = ussdText;
  ussd.description = ussdDescription;

  return ussd;
}

QSqlQuery UssdDatabaseEntity::queryInsert(Database *db, const Ussd &value) const
{
  QString resultString =
      "INSERT OR REPLACE "
      "INTO ussd (a_ussd, a_description) "
      "VALUES (:a_ussd, :a_description)";

  QSqlQuery query(db->qDatabase());

  query.prepare(resultString);
  query.bindValue(":a_ussd", value.ussd);
  query.bindValue(":a_description", value.description);

  return query;
}

QSqlQuery UssdDatabaseEntity::queryUpdat(Database *db, const DatabaseKey &key, const Ussd &value) const
{
  QString resultString =
      "UPDATE ussd "
      "SET a_ussd = :a_ussd, a_description = :a_description "
      "WHERE a_ussd = :a_ussd_key";

  Q_ASSERT(key.size() == 1);
  QString ussdKey = key.value("a_ussd").toString();

  QSqlQuery query(db->qDatabase());

  query.prepare(resultString);
  query.bindValue(":a_ussd", value.ussd);
  query.bindValue(":a_description", value.description);
  query.bindValue(":a_ussd_key", ussdKey);

  return query;
}

QSqlQuery UssdDatabaseEntity::queryDelet(Database *db, const DatabaseKey &key) const
{
  QString queryString =
      "DELETE "
      "FROM ussd";

  QStringList acceptedValues;
  acceptedValues << "a_ussd";

  QSqlQuery query(db->qDatabase());
  prepareQueryClause(&query, queryString, key, acceptedValues);

  return query;
}

