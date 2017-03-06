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

#ifndef SMS_H
#define SMS_H

#include <QDateTime>
#include <QObject>

#include "Database.h"

// from ST-Ericcsson docs
#define SMS_STORAGE_SIM_STR "SM"
#define SMS_STORAGE_PHONE_STR "ME"

enum SMS_STORAGE
{
  SMS_STORAGE_SIM   = 0x1,
  SMS_STORAGE_PHONE = 0x2
};

bool checkSmsStorage(int storage);
QString smsStorageStr(SMS_STORAGE storage);
SMS_STORAGE strToSmsStorage(const QString &str);

enum SMS_STATUS
{
  SMS_STATUS_NEW    = 0x0,  // received and not read yet
  SMS_STATUS_INCOME = 0x1,  // received and read
  SMS_STATUS_DRAFT  = 0x2,  // draft
  SMS_STATUS_SENT   = 0x3,  // sent
  SMS_STATUS_ALL    = 0x4   // all
};

bool checkSmsStatus(int status);

struct SmsBase
{
  // connection associated with the SMS
  QString connectionId;
  // storage type
  SMS_STORAGE storage;
  // SMS status
  SMS_STATUS status;
  // PDU raw data
  QByteArray rawData;
};

struct Sms
{
  Sms();
  Sms(SMS_STORAGE storage,
      int index,
      SMS_STATUS status,
      const QByteArray &rawData);

  // is this sms valid
  bool valid;

  // storage type
  SMS_STORAGE storage;
  // index of SMS in storage. Indexes can be sparsed!
  int index;
  // SMS status
  SMS_STATUS status;

  // PDU decoded values
  QString userText;
  QDateTime dateTime;
  QString sender;
  QString smsc;

  // if there was a PDU parsing error this will not be empty
  QString parseError;

  // concatenation
  uint concatReference;
  uint concatTotalCount;
  uint concatPartNumber;

  // PDU raw data
  QByteArray rawData;
};



class SmsMeta
{
public:
  SmsMeta(const QList<Sms>& smsList);

  bool isValid() const { return m_valid; }

  SMS_STORAGE storage() const { return m_storage; }
  QList<int> indexes() const { return m_indexes; }
  SMS_STATUS status() const { return m_status; }

  QString userText() const { return m_userText; }
  QDateTime dateTime() const { return m_dateTime; }
  QString sender() const { return m_sender; }
  QString smsc() const { return m_smsc; }

  QList<QByteArray> pduList() const { return m_pduList; }

protected:

  // is this sms valid
  bool m_valid;

  //
  // SMS data
  //

  // storage type
  SMS_STORAGE m_storage;
  // index of SMS in storage. Indexes can be sparsed!
  QList<int> m_indexes;
  // SMS status
  SMS_STATUS m_status;

  // PDU decoded values
  QString m_userText;
  QDateTime m_dateTime;
  QString m_sender;
  QString m_smsc;

  // PDU container
  QList<QByteArray> m_pduList;

};




class SmsDatabaseEntity : public IDatabaseEntity<SmsBase>
{
public:
  SmsDatabaseEntity() :
    IDatabaseEntity() {}

  ~SmsDatabaseEntity() {}

protected:
  bool isDatabaseReady(Database *db) const;
  bool initializeDatabase(Database *db) const;

  QSqlQuery querySelect(Database *db, const DatabaseKey &key) const;

  SmsBase createFromSelect(const QList<QVariant> &values) const;

  QSqlQuery queryInsert(Database *db, const SmsBase &value) const;

  QSqlQuery queryUpdat(Database *db, const DatabaseKey &key, const SmsBase &value) const;

  QSqlQuery queryDelet(Database *db, const DatabaseKey &key) const;
};


#endif // SMS_H
