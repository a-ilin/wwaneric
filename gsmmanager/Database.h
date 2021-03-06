﻿/*
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

#ifndef DATABASE_H
#define DATABASE_H

#include "common.h"

#include <QObject>
#include <QList>
#include <QMap>
#include <QVariant>

#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>

class Database;
class QSqlDatabase;
class DatabaseManager;

// db key. table_field_name : field_value
typedef QMap<QString, QVariant> DatabaseKey;



class Database : public QObject
{
  Q_OBJECT
public:
  bool isDriverInitialized() const;
  bool isDatabaseInitialized() const;

  bool isValid() const;
  QString errorString() const;

  QSqlDatabase qDatabase() const;

signals:

public slots:


private:
  explicit Database(QObject *parent = 0);
  ~Database();

  bool m_driverInitialized;
  bool m_databaseInitialized;
  QString m_databaseName;

  friend class DatabaseManager;
};

class DatabaseManager
{
public:
  static Database * instance();
  static void init();
  static void deinit();

private:
  static Database * m_instance;
};


template <class T>
class IDatabaseEntity
{
public:
  IDatabaseEntity() {}
  virtual ~IDatabaseEntity() {}

  bool init() const
  {
    bool result = true;

    Database * db = DatabaseManager::instance();
    if (!isDatabaseReady(db))
    {
      result = initializeDatabase(db);
    }

    return result;
  }

  QList<T> select(const DatabaseKey &key = DatabaseKey()) const
  {
    Database * db = DatabaseManager::instance();

    QSqlQuery query = querySelect(db, key);

    bool result = query.exec();

    QList<T> lst;

    if (result)
    {
      int columnCount = query.record().count();

      while (query.next())
      {
        QList<QVariant> rowValues;
        for (int i=0; i< columnCount; i++)
        {
          rowValues.append(query.value(i));
        }

        lst.append(createFromSelect(rowValues));
      }
    }
    else
    {
      Q_LOGEX(LOG_VERBOSE_ERROR, QString(query.lastError().text() + QString(" ::: ") + query.lastQuery()));
    }

    return lst;
  }

  bool insert(const QList<T> &list) const
  {
    Database * db = DatabaseManager::instance();

    bool globalResult = true;

    foreach(const T& t, list)
    {
      QSqlQuery query = queryInsert(db, t);

      bool result = query.exec();
      if (!result)
      {
        Q_LOGEX(LOG_VERBOSE_ERROR, QString(query.lastError().text() + QString(" ::: ") + query.lastQuery()));
      }

      globalResult = globalResult && result;
    }

    return globalResult;
  }

  bool updat(const DatabaseKey &key, const T& t)
  {
    Database * db = DatabaseManager::instance();

    QSqlQuery query = queryUpdat(db, key, t);

    bool result = query.exec();
    if (!result)
    {
      Q_LOGEX(LOG_VERBOSE_ERROR, QString(query.lastError().text() + QString(" ::: ") + query.lastQuery()));
    }

    return result;
  }

  bool delet(const DatabaseKey &key = DatabaseKey()) const
  {
    Database * db = DatabaseManager::instance();

    QSqlQuery query = queryDelet(db, key);

    bool result = query.exec();
    if (!result)
    {
      Q_LOGEX(LOG_VERBOSE_ERROR, QString(query.lastError().text() + QString(" ::: ") + query.lastQuery()));
    }

    return result;
  }

  // TODO: To be implemented
  //QList<DatabaseKey> objectKeys(const QList<T> &list) const = 0;

protected:
  // check if database has necessary structures (tables, etc...)
  virtual bool isDatabaseReady(Database *db) const = 0;

  // initialize database (create tables, indices,...)
  virtual bool initializeDatabase(Database *db) const = 0;


  // convert DatabaseKey into query select
  virtual QSqlQuery querySelect(Database *db, const DatabaseKey& key) const = 0;
  // create an instance from values returned by querySelect
  virtual T createFromSelect(const QList<QVariant>& values) const = 0;

  // convert a value into query insert
  virtual QSqlQuery queryInsert(Database *db, const T& value) const = 0;

  // convert a value into query updat
  virtual QSqlQuery queryUpdat(Database *db, const DatabaseKey &key, const T& value) const = 0;

  // convert a value into query delete
  virtual QSqlQuery queryDelet(Database *db, const DatabaseKey &key) const = 0;
};



// Prepares query to execution
// templ          - query string template. Should contain placeholder '%1' clause will be placed to
// key            - key with values used to construct clause
// acceptedValues - list of accepted key values. Used if not empty.
void prepareQueryClause(QSqlQuery * query, const QString& templ, const DatabaseKey& key, const QStringList& acceptedValues = QStringList());



#endif // DATABASE_H
