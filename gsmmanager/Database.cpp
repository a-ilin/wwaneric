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

