#ifndef USSD_H
#define USSD_H

#include "Database.h"

// from ST-Ericcsson docs
enum USSD_STATUS
{
  USSD_STATUS_FINISHED                 = 0,
  USSD_STATUS_USER_ACTION_NEEDED       = 1,
  USSD_STATUS_DIALOGUE_TERMINATED      = 2,
  USSD_STATUS_OTHER_IO_RESPONDED       = 3,
  USSD_STATUS_OPERATION_NOT_SUPPORTED  = 4,
  USSD_STATUS_NETWORK_TIMEOUT          = 5,
  USSD_STATUS_LAST  // this value used to type conversion check only
};

bool checkUssdStatus(int status);

enum USSD_SEND_STATUS
{
  USSD_SEND_STATUS_CODE_PRESENTATION_OFF = 0,
  USSD_SEND_STATUS_CODE_PRESENTATION_ON  = 1,
  USSD_SEND_STATUS_DIALOGUE_TERMINATE    = 2,
  USSD_SEND_STATUS_LAST  // this value used to type conversion check only
};

bool checkUssdSendStatus(int status);

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
