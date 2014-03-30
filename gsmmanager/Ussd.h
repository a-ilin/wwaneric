#ifndef USSD_H
#define USSD_H

#include "Database.h"

struct Ussd
{
  QString ussd;
  QString description;
};

class UssdDatabaseEntity : public IDatabaseEntity<Ussd>
{
public:
  UssdDatabaseEntity() :
    IDatabaseEntity() {}

  ~UssdDatabaseEntity() {}

protected:
  bool isDatabaseReady(Database *db) const;
  bool initializeDatabase(Database *db) const;

  QSqlQuery querySelect(Database *db, const DatabaseKey &key) const;

  Ussd createFromSelect(const QList<QVariant> &values) const;

  QSqlQuery queryInsert(Database *db, const Ussd &value) const;

  QSqlQuery queryUpdat(Database *db, const DatabaseKey &key, const Ussd &value) const;

  QSqlQuery queryDelet(Database *db, const DatabaseKey &key) const;
};

#endif // USSD_H
