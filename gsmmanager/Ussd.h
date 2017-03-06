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

#ifndef USSD_H
#define USSD_H

#include "Database.h"

#include <QUuid>

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
  QUuid connectionId;
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
