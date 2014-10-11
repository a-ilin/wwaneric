#include "common.h"
#include "Core.h"
#include "Database.h"

#include "ModemSms.h"
#include "ModemStatus.h"
#include "ModemUssd.h"

#include "MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QItemSelection>
#include <QThread>

// QObject's property of Modem class that contains it's connection ID
#define CONNECTION_ID_PROPERTY "connectionId"

#define CHECK_METACALL_RESULT(result) \
  if (!result) \
  { \
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Meta call failed!"); \
  }

Core* Core::m_instance = NULL;

Core::Core() :
  m_mainWindow(NULL),
  m_modemThread(NULL),
  m_modemThreadHelper(NULL)
{
  Q_ASSERT(!m_instance);
  m_instance = this;
}

Core::~Core()
{

}

bool Core::init()
{
  try
  {
    // logging
    {
      if (!startLogging())
      {
          return false;
      }
      logFilePath = (appUserDirectory() + QDir::separator() + QString("gsmmanager.log")).toLocal8Bit().constData();
      logAutoFlush = true;
      currentVerbosity = LOG_VERBOSE_DEBUG;
      LogFlush();
    }

    // QMetaType data types
    qRegisterMetaType<QVector<int> >("QVector<int>");  // Qt QAbstractItemModel::dataChanged()
    qRegisterMetaType<QItemSelection>("QItemSelection");

    qRegisterMetaType<Modem**>("Modem**");
    qRegisterMetaType<PortOptions>("PortOptions");
    qRegisterMetaType<ModemRequest>("ModemRequest");
    qRegisterMetaType<ModemRequest*>("ModemRequest*");
    qRegisterMetaType<ModemReply>("ModemReply");
    qRegisterMetaType<ModemReply*>("ModemReply*");

    // database
    {
      DatabaseManager::init();

      Database * database = DatabaseManager::instance();

      if (!database)
      {
        Q_LOGEX(LOG_VERBOSE_ERROR, QObject::tr("Cannot instantiate database singleton!"));
        return false;
      }

      if (!database->isValid())
      {
        Q_LOGEX(LOG_VERBOSE_ERROR, database->errorString());
        return false;
      }
    }

    // conversation handlers
    m_conversationHandlers.append(new SmsConversationHandler());
    m_conversationHandlers.append(new StatusConversationHandler());
    m_conversationHandlers.append(new UssdConversationHandler());

    // Modems thread
    m_modemThread = new QThread();
    m_modemThread->start();
    m_modemThreadHelper = new ModemThreadHelper();
    m_modemThreadHelper->moveToThread(m_modemThread);

    // MainWindow
    m_mainWindow = new MainWindow(NULL);
    m_mainWindow->init();
    m_mainWindow->show();
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void Core::tini()
{
  try
  {
    // MainWindow
    if (m_mainWindow)
    {
      m_mainWindow->tini();
      delete m_mainWindow;
      m_mainWindow = NULL;
    }

    // modems
    Q_ASSERT(m_modems.isEmpty());
    QList<QUuid> modemIds = m_modems.keys();
    foreach(const QUuid &id, modemIds)
    {
      removeConnection(id);
    }

    // modems thread
    if (m_modemThread)
    {
        m_modemThread->quit();
        m_modemThread->wait();
        if (m_modemThreadHelper)
        {
            delete m_modemThreadHelper;
        }
        delete m_modemThread;
    }

    // conversation handlers
    qDeleteAll(m_conversationHandlers);

    // database
    DatabaseManager::deinit();

    // logging
    stopLogging();
  }
  catch(...)
  {

  }

}

void Core::createConnection(const QUuid& id)
{
  Q_ASSERT(!m_modems.contains(id));

  // acquire a modem from it's thread
  Modem * m = NULL;
  int result = QMetaObject::invokeMethod(m_modemThreadHelper, "createModem",
                                         Qt::BlockingQueuedConnection,
                                         Q_ARG(Modem**, &m),
                                         Q_ARG(QUuid, id));
  CHECK_METACALL_RESULT(result);
  Q_ASSERT(m);

  m_modems.insert(id, m);

  foreach(ConversationHandler* handler, m_conversationHandlers)
  {
    m->registerConversationHandler(handler);
  }

  connect(m, SIGNAL(updatedPortStatus(bool)),
          SLOT(onConnectionStatusChanged(bool)));

  connect(m, SIGNAL(updatedPortError(QString)),
          SLOT(onConnectionErrorOccured(QString)));

  connect(m, SIGNAL(replyReceived(ModemReply*)),
          SLOT(onReplyReceived(ModemReply*)));
}

void Core::removeConnection(const QUuid& id)
{
  Modem * m = m_modems.value(id);
  Q_ASSERT(m);
  Q_ASSERT(m->property(CONNECTION_ID_PROPERTY).toUuid() == id);

  if (m)
  {
    m_modems.remove(id);

    // delete modem in it's thread
    int result = QMetaObject::invokeMethod(m_modemThreadHelper, "deleteModem",
                                           Qt::BlockingQueuedConnection,
                                           Q_ARG(Modem**, &m));
    CHECK_METACALL_RESULT(result);
    Q_ASSERT(!m);
  }
}



QString Core::appUserDirectory() const
{
  return QApplication::applicationDirPath();
}

ModemRequest* Core::modemRequest(const QUuid& connectionId,
                                 const QString& conversationHandlerName,
                                 int requestType,
                                 int requestRetries) const
{
  Modem * m = m_modems.value(connectionId);
  Q_ASSERT(m);
  Q_ASSERT(m->property(CONNECTION_ID_PROPERTY).toUuid() == connectionId);

  ModemRequest * request = NULL;

  if (m)
  {
    request = m->createRequest(conversationHandlerName, requestType, requestRetries);
    Q_ASSERT(request);
  }

  return request;
}

void Core::onReplyReceived(ModemReply* reply)
{
  Modem * modem = static_cast<Modem*>(sender());
  Q_ASSERT(modem);
  QUuid id = modem->property(CONNECTION_ID_PROPERTY).toUuid();
  // this is should be direct connection
  emit connectionEvent(id, ConnectionEventCustom, QVariant::fromValue<ModemReply*>(reply));
  delete reply;
}

void Core::onConnectionStatusChanged(bool status)
{
  Modem * modem = static_cast<Modem*>(sender());
  Q_ASSERT(modem);
  QUuid id = modem->property(CONNECTION_ID_PROPERTY).toUuid();
  emit connectionEvent(id, ConnectionEventStatus, status);
}

void Core::onConnectionErrorOccured(const QString error)
{
  Modem * modem = static_cast<Modem*>(sender());
  Q_ASSERT(modem);
  QUuid id = modem->property(CONNECTION_ID_PROPERTY).toUuid();
  emit connectionEvent(id, ConnectionEventError, error);
}

void Core::pushRequest(ModemRequest* request)
{
  int result = QMetaObject::invokeMethod(request->modem(), "appendRequest",
                                         Qt::QueuedConnection,
                                         Q_ARG(ModemRequest*, request));

  CHECK_METACALL_RESULT(result);
}

void Core::openConnection(const QUuid& id, const QString& portName, const PortOptions& options)
{
  Modem * m = m_modems.value(id);
  Q_ASSERT(m);
  Q_ASSERT(m->property(CONNECTION_ID_PROPERTY).toUuid() == id);

  if (m)
  {
    int result = 1;
    result = QMetaObject::invokeMethod(m, "setPortOptions",
                                       Qt::QueuedConnection,
                                       Q_ARG(PortOptions, options));
    CHECK_METACALL_RESULT(result);

    result = QMetaObject::invokeMethod(m, "setPortName",
                                       Qt::QueuedConnection,
                                       Q_ARG(QString, portName));
    CHECK_METACALL_RESULT(result);

    result = QMetaObject::invokeMethod(m, "openPort",
                                       Qt::QueuedConnection);
    CHECK_METACALL_RESULT(result);
  }
}

void Core::closeConnection(const QUuid& id)
{
  Modem * m = m_modems.value(id);
  Q_ASSERT(m);
  Q_ASSERT(m->property(CONNECTION_ID_PROPERTY).toUuid() == id);

  if (m)
  {
    int result = QMetaObject::invokeMethod(m, "closePort",
                                           Qt::QueuedConnection);
    CHECK_METACALL_RESULT(result);
  }
}

void Core::storeSettings()
{

}

void Core::restoreSettings()
{

}

void ModemThreadHelper::createModem(Modem** modem, QUuid id)
{
  (*modem) = new Modem();
  (*modem)->setProperty(CONNECTION_ID_PROPERTY, id);
}

void ModemThreadHelper::deleteModem(Modem** modem)
{
  (*modem)->deleteLater();
  (*modem) = NULL;
}
