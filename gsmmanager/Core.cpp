﻿#include "common.h"
#include "Core.h"
#include "Database.h"

#include "ModemSms.h"
#include "ModemStatus.h"
#include "ModemUssd.h"

#include <QApplication>
#include <QDir>

Core* Core::m_instance = NULL;

Core::Core()
{
  Q_ASSERT(!m_instance);
  m_instance = this;
}

Core::~Core()
{

}

bool Core::init()
{
  // logging
  {
    int status = startLogging();
    if (status)
    {
        wprintf(L"Error initializing pthread_mutex_t! pthread_mutex_init code: %d. Terminating...\n", status);
        return -1;
    }
    logFilePath = (appUserDirectory() + QDir::separator() + QString("gsmmanager.log")).toLocal8Bit().constData();
    logAutoFlush = true;
    currentVerbosity = LOG_VERBOSE_DEBUG;
    LogFlush();
  }

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

  // modems
  {
    // TODO: support multiple modems
    Modem * m = new Modem();
    m_modems.insert(QString(), QSharedPointer<Modem>(m));
  }

  // conversation handlers
  foreach(const QSharedPointer<Modem> &modem, m_modems)
  {
    ConversationHandlersHash handlersHash;

    SmsConversationHandler * smsHandler = new SmsConversationHandler();
    modem->registerConversationHandler(smsHandler);
    handlersHash.insert(smsHandler->name(), smsHandler);

    StatusConversationHandler * statusHandler = new StatusConversationHandler();
    modem->registerConversationHandler(statusHandler);
    handlersHash.insert(statusHandler->name(), statusHandler);

    UssdConversationHandler * ussdHandler = new UssdConversationHandler();
    modem->registerConversationHandler(ussdHandler);
    handlersHash.insert(ussdHandler->name(), ussdHandler);

    m_conversationHandlers.insert(modem.data(), handlersHash);
  }

  return true;
}

bool Core::tini()
{
  // conversation handlers
  ConversationHandlersModemHash::iterator iter = m_conversationHandlers.begin();
  while(iter != m_conversationHandlers.constEnd())
  {
    Modem * modem = iter.key();

    ConversationHandlersHash::iterator iterHandlers = iter.value().begin();
    while (iterHandlers != iter.value().constEnd())
    {
      ConversationHandler * cHandler = iterHandlers.value();
      modem->unregisterConversationHandler(cHandler);
      delete cHandler;
      iterHandlers = iter.value().erase(iterHandlers);
    }

    iter = m_conversationHandlers.erase(iter);
  }

  // modems
  m_modems.clear();

  // database
  DatabaseManager::deinit();

  // logging
  {
    int status = stopLogging();
    if (status)
    {
        wprintf(L"Error deinitializing pthread_mutex_t! pthread_mutex_destroy code: %d. Terminating...\n", status);
        return false;
    }
  }

  return true;
}

Modem *Core::modem(const QString &id) const
{
  Modem * m = m_modems.value(id).data();
  Q_ASSERT(m);
  return m;
}

QString Core::appUserDirectory() const
{
  return QApplication::applicationDirPath();
}

ConversationHandler* Core::conversationHandler(Modem *modem, const QString &name) const
{
  ConversationHandler * handler = NULL;

  if (m_conversationHandlers.contains(modem))
  {
    const ConversationHandlersHash &cHash = m_conversationHandlers[modem];
    handler = cHash.value(name);
  }

  if (!handler)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "No handlers for specified modem found!");
  }

  return handler;
}
