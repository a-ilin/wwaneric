#include "common.h"
#include "Core.h"
#include "Database.h"

#include "ModemSms.h"
#include "ModemStatus.h"
#include "ModemUssd.h"

#include "MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QThread>

#define MODEM_ID_PROPERTY "modemId"

#define CHECK_METACALL_RESULT(result) \
  if (!result) \
  { \
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Meta call failed!"); \
  }

Core* Core::m_instance = NULL;

Core::Core() :
  m_mainWindow(NULL)
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
      int status = startLogging();
      if (status)
      {
          wprintf(L"Error initializing pthread_mutex_t! pthread_mutex_init code: %d. Terminating..." ENDL, status);
          return -1;
      }
      logFilePath = (appUserDirectory() + QDir::separator() + QString("gsmmanager.log")).toLocal8Bit().constData();
      logAutoFlush = true;
      currentVerbosity = LOG_VERBOSE_DEBUG;
      LogFlush();
    }

    // QMetaType data types
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

bool Core::tini()
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
    QStringList modemNames = m_modems.keys();
    foreach(const QString &name, modemNames)
    {
      removeConnection(name);
    }

    // modems thread
    m_modemThread->quit();
    m_modemThread->wait();
    delete m_modemThreadHelper;
    delete m_modemThread;

    // conversation handlers
    qDeleteAll(m_conversationHandlers);

    // database
    DatabaseManager::deinit();

    // logging
    {
      int status = stopLogging();
      if (status)
      {
          wprintf(L"Error deinitializing pthread_mutex_t! pthread_mutex_destroy code: %d. Terminating..." ENDL, status);
          return false;
      }
    }
  }
  catch(...)
  {
    return false;
  }

  return true;
}

void Core::createConnection(const QString& id)
{
  Q_ASSERT(!m_modems.contains(id));

  // acquire a modem from it's thread
  Modem * m = NULL;
  int result = QMetaObject::invokeMethod(m_modemThreadHelper, "createModem",
                                         Qt::BlockingQueuedConnection,
                                         Q_ARG(Modem**, &m),
                                         Q_ARG(QString, id));
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

void Core::removeConnection(const QString& id)
{
  Modem * m = m_modems.value(id);
  Q_ASSERT(m);
  Q_ASSERT(m->property(MODEM_ID_PROPERTY).toString() == id);

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

ModemRequest* Core::modemRequest(const QString& connectionId,
                                 const QString& conversationHandlerName,
                                 int requestType,
                                 int requestRetries) const
{
  Modem * m = m_modems.value(connectionId);
  Q_ASSERT(m);
  Q_ASSERT(m->property(MODEM_ID_PROPERTY).toString() == connectionId);

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
  QString id = modem->property(MODEM_ID_PROPERTY).toString();
  // this is should be direct connection
  emit connectionEvent(id, ConnectionEventCustom, QVariant::fromValue<ModemReply*>(reply));
  delete reply;
}

void Core::onConnectionStatusChanged(bool status)
{
  Modem * modem = static_cast<Modem*>(sender());
  Q_ASSERT(modem);
  QString id = modem->property(MODEM_ID_PROPERTY).toString();

  emit connectionEvent(id, ConnectionEventStatus, status);
}

void Core::onConnectionErrorOccured(const QString error)
{
  Modem * modem = static_cast<Modem*>(sender());
  Q_ASSERT(modem);
  QString id = modem->property(MODEM_ID_PROPERTY).toString();
  emit connectionEvent(id, ConnectionEventError, error);
}

void Core::pushRequest(ModemRequest* request)
{
  int result = QMetaObject::invokeMethod(request->modem(), "appendRequest",
                                         Qt::QueuedConnection,
                                         Q_ARG(ModemRequest*, request));

  CHECK_METACALL_RESULT(result);
}

void Core::openConnection(const QString& id, const QString& portName, const PortOptions& options)
{
  Modem * m = m_modems.value(id);
  Q_ASSERT(m);
  Q_ASSERT(m->property(MODEM_ID_PROPERTY).toString() == id);

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

void Core::closeConnection(const QString& id)
{
  Modem * m = m_modems.value(id);
  Q_ASSERT(m);
  Q_ASSERT(m->property(MODEM_ID_PROPERTY).toString() == id);

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

void ModemThreadHelper::createModem(Modem** modem, const QString& id)
{
  (*modem) = new Modem();
  (*modem)->setProperty(MODEM_ID_PROPERTY, id);
}

void ModemThreadHelper::deleteModem(Modem** modem)
{
  (*modem)->deleteLater();
  (*modem) = NULL;
}
