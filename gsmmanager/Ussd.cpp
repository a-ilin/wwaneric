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
      "(a_connection CHARACTER(38), a_ussd VARCHAR(64), a_description VARCHAR(128), "
      "PRIMARY KEY (a_connection, a_ussd) )";

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
  QString queryString =
      "SELECT a_connection, a_ussd, a_description "
      "FROM ussd %1 ";

  QStringList acceptedValues;
  acceptedValues << "a_connection" << "a_ussd";

  QSqlQuery query(db->qDatabase());
  prepareQueryClause(&query, queryString, key, acceptedValues);

  return query;
}

Ussd UssdDatabaseEntity::createFromSelect(const QList<QVariant> &values) const
{
  Q_ASSERT(values.size() == 3);

  Ussd ussd;
  ussd.connectionId = QUuid(values.at(0).toString());
  ussd.ussd = values.at(1).toString();
  ussd.description = values.at(2).toString();

  return ussd;
}

QSqlQuery UssdDatabaseEntity::queryInsert(Database *db, const Ussd &value) const
{
  QString resultString =
      "INSERT OR REPLACE "
      "INTO ussd (a_connection, a_ussd, a_description) "
      "VALUES (:a_connection, :a_ussd, :a_description)";

  QSqlQuery query(db->qDatabase());

  query.prepare(resultString);
  query.bindValue(":a_connection", value.connectionId.toString());
  query.bindValue(":a_ussd", value.ussd);
  query.bindValue(":a_description", value.description);

  return query;
}

QSqlQuery UssdDatabaseEntity::queryUpdat(Database *db, const DatabaseKey &key, const Ussd &value) const
{
  QString resultString =
      "UPDATE ussd "
      "SET a_ussd = :a_ussd, a_description = :a_description "
      "WHERE a_ussd = :a_ussd_key AND a_connection = :a_connection ";

  QSqlQuery query(db->qDatabase());

  query.prepare(resultString);
  query.bindValue(":a_ussd", value.ussd);
  query.bindValue(":a_description", value.description);
  query.bindValue(":a_ussd_key", key.value("a_ussd").toString());
  query.bindValue(":a_connection", key.value("a_connection").toString());

  return query;
}

QSqlQuery UssdDatabaseEntity::queryDelet(Database *db, const DatabaseKey &key) const
{
  QString queryString =
      "DELETE "
      "FROM ussd %1 ";

  QStringList acceptedValues;
  acceptedValues << "a_connection" << "a_ussd";

  QSqlQuery query(db->qDatabase());
  prepareQueryClause(&query, queryString, key, acceptedValues);

  return query;
}

