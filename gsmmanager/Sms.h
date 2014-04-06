#ifndef SMS_H
#define SMS_H

#include <QDateTime>
#include <QObject>

#include "Database.h"

// from ST-Ericcsson docs
#define SMS_STORAGE_SIM_STR "\"SM\""
#define SMS_STORAGE_PHONE_STR "\"ME\""

enum SMS_STORAGE
{
  SMS_STORAGE_SIM   = 0x1,
  SMS_STORAGE_PHONE = 0x2
};

QString smsStorageStr(SMS_STORAGE storage);

enum SMS_STATUS
{
  SMS_STATUS_NEW =    0x0,  // received and not read yet
  SMS_STATUS_INCOME = 0x1,  // received and read
  SMS_STATUS_DRAFT =  0x2,  // draft
  SMS_STATUS_SENT =   0x3,  // sent
  SMS_STATUS_ALL =    0x4   // all
};

bool checkSmsStatus(int status);

class SmsMeta
{
public:
  SmsMeta();
  SmsMeta(SMS_STORAGE storage, SMS_STATUS status, int index);
  virtual ~SmsMeta() {}

  bool isValid() const { return m_valid; }

  SMS_STORAGE storage() const { return m_storage; }
  int index() const { return m_index; }
  SMS_STATUS status() const { return m_status; }

  QString userText() const { return m_userText; }
  QDateTime dateTime() const { return m_dateTime; }
  QString sender() const { return m_sender; }
  QString smsc() const { return m_smsc; }


protected:

  // is this sms valid
  bool m_valid;

  //
  // SMS data
  //

  // storage type
  SMS_STORAGE m_storage;
  // index of SMS in storage. Indexes can be sparsed!
  int m_index;
  // SMS status
  SMS_STATUS m_status;

  // PDU decoded values
  QString m_userText;
  QDateTime m_dateTime;
  QString m_sender;
  QString m_smsc;

};

Q_DECLARE_METATYPE(SmsMeta)

class Sms : public SmsMeta
{
public:
  Sms();
  Sms(SMS_STORAGE storage, SMS_STATUS status, int index, const QByteArray &rawData);
  ~Sms() {}

  QString parseError() const { return m_parseError; }

  QString udhType() const { return m_udhType; }
  QByteArray udhData() const {return m_udhData; }

  QByteArray rawData() const {return m_rawData; }

protected:

  // if there was a PDU parsing error this will not be empty
  QString m_parseError;

  QString m_udhType;
  QByteArray m_udhData;

  // PDU raw data
  QByteArray m_rawData;
};

Q_DECLARE_METATYPE(Sms)


class SmsDatabaseEntity : public IDatabaseEntity<Sms>
{
public:
  SmsDatabaseEntity() :
    IDatabaseEntity() {}

  ~SmsDatabaseEntity() {}

protected:
  bool isDatabaseReady(Database *db) const;
  bool initializeDatabase(Database *db) const;

  QSqlQuery querySelect(Database *db, const DatabaseKey &key) const;

  Sms createFromSelect(const QList<QVariant> &values) const;

  QSqlQuery queryInsert(Database *db, const Sms &value) const;

  QSqlQuery queryUpdat(Database *db, const DatabaseKey &key, const Sms &value) const;

  QSqlQuery queryDelet(Database *db, const DatabaseKey &key) const;
};


#endif // SMS_H
