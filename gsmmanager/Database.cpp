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
