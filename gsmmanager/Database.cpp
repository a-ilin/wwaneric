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

#include "Database.h"

#include "Core.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QDir>

Database * DatabaseManager::m_instance = NULL;

Database * DatabaseManager::instance()
{
  if(!m_instance)
  {
    m_instance = new Database();
  }

  return m_instance;
}

void DatabaseManager::init()
{
  Q_ASSERT(!m_instance);

  m_instance = new Database();
}

void DatabaseManager::deinit()
{
  Q_ASSERT(m_instance);

  delete m_instance;
}

Database::Database(QObject *parent) :
  QObject(parent),
  m_driverInitialized(false),
  m_databaseInitialized(false)
{
  m_databaseName = Core::instance()->appUserDirectory()  + QDir::separator() + QString("data.sqlite");

  const QString driverName = "QSQLITE";

  if (QSqlDatabase::drivers().contains(driverName))
  {
    m_driverInitialized = true;

    QSqlDatabase db = QSqlDatabase::addDatabase(driverName);
    db.setDatabaseName(m_databaseName);
    if (db.open())
    {
      m_databaseInitialized = true;
    }
  }
}

Database::~Database()
{
  QSqlDatabase db = QSqlDatabase::database();
  if(db.isValid())
  {
    db.close();
  }
}

bool Database::isValid() const
{
  if (m_driverInitialized && m_databaseInitialized)
  {
    return true;
  }

  return false;
}

QString Database::errorString() const
{
  if (!m_driverInitialized)
  {
    return tr("Cannot initialize SQL driver!");
  }

  if (!m_databaseInitialized)
  {
    return tr("Cannot connect to the database!");
  }

  return QString();
}

QSqlDatabase Database::qDatabase() const
{
  return QSqlDatabase::database();
}


void prepareQueryClause(QSqlQuery * query, const QString& templ, const DatabaseKey& key, const QStringList& acceptedValues)
{
  QString resultString;

  DatabaseKey::const_iterator iter = key.constBegin();
  DatabaseKey::const_iterator iterEnd = key.constEnd();

  // first pass, construct base query string
  bool needAnd = false;
  while (iter != iterEnd)
  {
    if ((!acceptedValues.size()) || acceptedValues.contains(iter.key()))
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
      Q_LOGEX(LOG_VERBOSE_CRITICAL, QString("Wrong DB key field: ") + iter.key())
    }

    ++iter;
  }

  if (resultString.size())
  {
    resultString.prepend(" WHERE ");
  }

  query->prepare(templ.arg(resultString));

  iter = key.constBegin();

  // second pass, bind values
  while (iter != iterEnd)
  {
    if ((!acceptedValues.size()) || acceptedValues.contains(iter.key()))
    {
      query->bindValue(QChar(':')+iter.key(), iter.value());
    }
    else
    {
      Q_ASSERT(false);
      Q_LOGEX(LOG_VERBOSE_CRITICAL, QString("Wrong DB key field: ") + iter.key())
    }

    ++iter;
  }
}

