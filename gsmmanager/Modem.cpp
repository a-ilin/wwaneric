#include "Modem.h"

#include "common.h"

#include "ModemInit.h"

#include <QTimer>


/* line should have format:
 * <command>: <data>[,<data>[,<data>[,...]]]
 * where <data> can be one of these types:
 * - a decimal number,
 * - a string edged by quotes "",
 * - a null string, i.e. empty space
*/
QStringList parseAnswerLine(const QString &line, const QString &command)
{
  // check for command
  int index = line.indexOf(command);

  // line should starts with the command
  for (int i=0; i < index; ++i)
  {
    if (!line[i].isSpace())
    {
      return QStringList();
    }
  }

  index += command.size();
  while((index < line.size()) && (line.at(index) == ' '))
  {
    ++index;
  }

  // a string with cutted command and spaces after it
  QString midLine = line.mid(index);

  QByteArray sep("\"");
  QList<QByteArray> splittedQuoted = splitByteArray(midLine.toUtf8(), sep, KeepSeparators);

  // state of current block. if true it was quoted.
  bool q = false;
  // count of quotes sequence
  int qSec = 0;
  // will contain quoted and not quoted parts
  struct Part
  {
    QByteArray part;
    bool wasQuoted;
  };

  QList<Part> preResult;

  int partIndex = 0;
  foreach(const QByteArray &part, splittedQuoted)
  {
    if (part == sep)
    {
      q = !q;
      ++qSec;

      // empty string
      if (qSec == 2)
      {
        Part p;
        p.wasQuoted = true;
        preResult.append(p);
      }
      // error, should be less then 3 ( """ <== is not accepted )
      if (qSec > 2)
      {
        return QStringList();
      }

      continue;
    }
    else
    {
      qSec = 0;
    }

    Part p;
    p.part = part;

    if (q)
    {
      p.wasQuoted = true;
    }
    else
    {
      p.wasQuoted = false;

      // check that if previous was quoted then ...
      if (preResult.size() && preResult.last().wasQuoted)
      {
        if (!part.size())
        {
          QString str("Meet empty part. In current mode this should not happen! Source: \"%1\" Arg: \"%2\"");
          Q_LOGEX(LOG_VERBOSE_CRITICAL, str.arg(line).arg(command));

          return QStringList();
        }
        // the current should starts with comma
        if (part.at(0) != ',')
        {
          return QStringList();
        }
      }
    }

    preResult.append(p);

    ++partIndex;
  }

  if (q)
  {
    // abort
    return QStringList();
  }

  QStringList result;

  for (int index = 0; index < preResult.size(); ++index)
  {
    const Part &part = preResult.at(index);
    if (part.wasQuoted)
    {
      result.append(QString(part.part));
    }
    else
    {
      QString str(part.part);

      // check that if next was quoted then current must ends with comma
      if ((preResult.size() > index + 1) &&
          (preResult.at(index + 1).wasQuoted))
      {
        if ((!str.size()) || (str.at(str.size() - 1) != ','))
        {
          return QStringList();
        }
        else
        {
          // remove last comma
          str.resize(str.size() - 1);
        }
      }

      // check that if previous was quoted then current must starts with comma
      if ((index > 0) && (preResult.at(index - 1).wasQuoted))
      {
        if ((!str.size()) || (str.at(0) != ','))
        {
          return QStringList();
        }
        else
        {
          // remove first comma
          str.remove(0, 1);
        }
      }

      foreach(const QByteArray &array, splitByteArray(str.toUtf8(), ",", KeepEmptyParts))
      {
        result.append(QString(array));
      }
    }
  }

  return result;
}





Modem::Modem() :
  PortController(),
  m_modemInited(false)
{
  m_initHandler = new InitConversationHandler();
  registerConversationHandler(m_initHandler);

  m_initTimer = new QTimer(this);
  m_initTimer->setInterval(2000);
  m_initTimer->setSingleShot(true);
  connect(m_initTimer, SIGNAL(timeout()), SLOT(onInitTimeout()));

  m_pingTimer = new QTimer(this);
  m_pingTimer->setInterval(10000);
  connect(m_pingTimer, SIGNAL(timeout()), SLOT(onPingTimeout()));
}

Modem::~Modem()
{
  unregisterConversationHandler(m_initHandler);
  delete m_initHandler;
}

void Modem::registerConversationHandler(ConversationHandler *handler)
{
  Q_ASSERT(handler);

  QWriteLocker locker(&m_rwlock);

  if (m_conversationHandlersNames.contains(handler->name()))
  {
    QString err("Handler with name \"%1\" is already registered!");
    Q_LOGEX(LOG_VERBOSE_CRITICAL, err.arg(handler->name()));
    return;
  }

  // calculate handler base offset
  int maxRequestType = 0;
  if (m_conversationHandlers.size() > 0)
  {
    // last handler will have the max base offset (QMap is sorted by key)
    const ConversationHandler * lastHandler = m_conversationHandlers.last();
    int lastKey = m_conversationHandlers.lastKey();
    maxRequestType = lastKey + lastHandler->requestTypesCount();
  }

  m_conversationHandlers.insert(maxRequestType, handler);
  m_conversationHandlersNames.insert(handler->name(), maxRequestType);
}

void Modem::unregisterConversationHandler(ConversationHandler *handler)
{
  Q_ASSERT(handler);

  QWriteLocker locker(&m_rwlock);

  if (!m_conversationHandlersNames.contains(handler->name()))
  {
    QString err("Handler with name \"%1\" is not registered!");
    Q_LOGEX(LOG_VERBOSE_CRITICAL, err.arg(handler->name()));
    return;
  }

  int baseOffset = m_conversationHandlersNames.value(handler->name(), -1);
  Q_ASSERT(baseOffset != -1);

  Q_ASSERT(m_conversationHandlers.contains(baseOffset));
  m_conversationHandlers.remove(baseOffset);

  m_conversationHandlersNames.remove(handler->name());
}

ModemRequest* Modem::createRequest(const QString& handlerName, int type, int retries)
{
  QReadLocker locker(&m_rwlock);

  int baseOffset = m_conversationHandlersNames.value(handlerName, -1);
  Q_ASSERT(baseOffset != -1);

  ConversationHandler * handler = m_conversationHandlers.value(baseOffset);
  Q_ASSERT(handler);


  ModemRequest * r = NULL;
  if (handler)
  {
    RequestArgs * args = handler->requestArgs(type);

    r = new ModemRequest();
    r->m_baseOffset = baseOffset;
    r->m_type = type;
    r->m_args = args;
    r->m_retry = retries;
    r->m_stage = 0;
    r->m_modem = this;

  }

  return r;
}

void Modem::appendRequest(ModemRequest *request)
{
  // this call is thread safe when called via queued Qt meta call

  m_requests.append(request);
  sendRequest();
}

bool Modem::processConversation(Conversation c)
{
  if (!m_requests.size())
  {
    // no repeat for unexpected answers
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "No pending requests exist!");
    return true;
  }

  AnswerData * data = NULL;
  ModemRequest::Status status;
  ConversationHandler * handler = NULL;

  ModemRequest * request = m_requests.first();

  // If there was an unexpected reply between port I/O
  // data field must be checked too
  QByteArray unexpectedData;
  while ((requestData() != c.request) && (c.data.size() > 0))
  {
    unexpectedData += c.request + phraseSep;
    c.request = c.data.takeFirst();
  }

  if ( ! unexpectedData.isEmpty())
  {
    if ( ! processUnexpectedData(unexpectedData))
    {
      QString strErr("Unexpected data collision solving failed!");
      Q_LOGEX(LOG_VERBOSE_ERROR, strErr);
    }
  }

  // try to process conversation
  if (requestData() == c.request)
  {
    QReadLocker locker(&m_rwlock);
    handler = m_conversationHandlers.value(request->m_baseOffset);

    if (handler)
    {
      if ((handler != m_initHandler) && (!m_modemInited))
      {
        Q_LOGEX(LOG_VERBOSE_CRITICAL, "Modem is not initialized when answer received!");
      }

      handler->processConversation(request, c, status, data);

      if (status == ModemRequest::SuccessCompleted)
      {
        if (handler == m_initHandler)
        {
          switch(request->type())
          {
          case INIT_REQUEST_INIT:
            m_initTimer->stop();
            m_modemInited = true;
            modemDetected(true);
            break;
          case INIT_REQUEST_PING:
            m_pingReceived = true;
            Q_LOGEX(LOG_VERBOSE_DEBUG, "Ping received!");
            break;
          default:
            Q_LOGEX(LOG_VERBOSE_CRITICAL, QString("Unknown request type: %1 !").arg(request->type()));
          }
        }
      }
      else if (status == ModemRequest::SuccessNeedMoreData)
      {

      }
      else if (status == ModemRequest::Failure)
      {
        if (handler == m_initHandler)
        {
          switch(request->type())
          {
          case INIT_REQUEST_INIT:
            Q_LOGEX(LOG_VERBOSE_ERROR, "Error occured on modem initialization!");
            closePort();
            break;
          case INIT_REQUEST_PING:
            Q_LOGEX(LOG_VERBOSE_ERROR, "Error occured on ping processing")
            onPingTimeout();
            break;
          default:
            Q_LOGEX(LOG_VERBOSE_CRITICAL, QString("Unknown request type: %1 !").arg(request->type()));
          }
        }

        if (c.status == Conversation::OK)
        {
          // modem returned status OK but handler cannot process it
          QString str = QString("Unprocessed answer received. " ENDL
                                "Request:"      ENDL
                                "%1."           ENDL
                                "Answer (HEX):" ENDL
                                "%2"
                                "Status:"       ENDL
                                "%3"            ENDL)
                        .arg(QString(c.request))
                        .arg(hexString(c.data))
                        .arg(QString(c.statusData));
          Q_LOGEX(LOG_VERBOSE_ERROR, str);
        }

        --request->m_retry;
        Q_ASSERT(request->m_retry >= 0);
      }
    }
    else
    {
      QString err = QString("Handler for this kind of request was unregistered! Request: %1")
                    .arg(QString(requestData()));
      Q_LOGEX(LOG_VERBOSE_CRITICAL, err);
    }
  }
  else
  {
    QString str = QString("Unexpected answer received. Request: %1. Answer: %2")
                  .arg(QString(requestData()))
                  .arg(QString(c.request));
    Q_LOGEX(LOG_VERBOSE_ERROR, str);
  }

  // remove request with ended retries
  if ((!request->m_retry) || (status == ModemRequest::SuccessCompleted))
  {
    if (!request->m_retry)
    {
      Q_LOGEX(LOG_VERBOSE_NOTIFICATION, "Request retry count is ended. Request removed from queue.");
    }

    Q_ASSERT(handler);

    ModemReply * reply = new ModemReply();
    reply->m_handlerName = handler->name();
    reply->m_type = request->m_type;
    reply->m_status = status == ModemRequest::SuccessCompleted;
    reply->m_data = data;

    delete m_requests.takeFirst();

    emit replyReceived(reply);
  }

  return true;
}

bool Modem::processUnexpectedData(const QByteArray& data)
{
  QReadLocker locker(&m_rwlock);

  bool result = false;

  int replyType;
  AnswerData * answer = NULL;
  ConversationHandler * handler = NULL;

  QMap<int, ConversationHandler*>::const_iterator iter = m_conversationHandlers.constBegin();

  while((iter != m_conversationHandlers.constEnd()) && (!result))
  {
    handler = iter.value();
    result = handler->processUnexpectedData(data, replyType, answer);

    ++iter;
  }

  if (result)
  {
    ModemReply * reply = new ModemReply();
    reply->m_handlerName = handler->name();
    reply->m_type = replyType;
    reply->m_status = true;
    reply->m_data = answer;

    emit replyReceived(reply);
  }
  else
  {
    QString str = QString("Unexpected data was not processed: %1").arg(QString(data));
    Q_LOGEX(LOG_VERBOSE_WARNING, str);
  }

  return result;
}

QByteArray Modem::requestData() const
{
  QReadLocker locker(&m_rwlock);

  QByteArray data;

  foreach(const ModemRequest *request, m_requests)
  {
    const ConversationHandler * handler = m_conversationHandlers.value(request->m_baseOffset);

    if (handler)
    {
      data = handler->requestData(request);
      if (!data.size())
      {
        QString str = QString("Handler returned empty data for request. Base type: %1, type: %2")
                      .arg(request->m_baseOffset)
                      .arg(request->m_type);
        Q_LOGEX(LOG_VERBOSE_CRITICAL, str);
      }
    }
    else
    {
      QString err = QString("Handler for this type of request was unregistered! Base type: %1, type: %2")
                    .arg(request->m_baseOffset)
                    .arg(request->m_type);
      Q_LOGEX(LOG_VERBOSE_CRITICAL, err);
    }

    if (data.size())
    {
      break;
    }
  }

  return data;
}

void Modem::modemDetected(bool status)
{
  if (m_modemInited)
  {
    // modem was detected previously
    if (status)
    {
      m_pingReceived = true;
      onPingTimeout();
    }
    else
    {
      m_modemInited = false;
      m_pingTimer->stop();
    }
  }
  else if (status)
  {
    // modem detected but not initialized yet

    ModemRequest * initRequest = createRequest(m_initHandler->name(), INIT_REQUEST_INIT, 1);
    appendRequest(initRequest);

    m_initTimer->start();
  }

  if (m_modemInited || (!status))
  {
    PortController::modemDetected(status);
  }
}

void Modem::onInitTimeout()
{
  if (!m_modemInited)
  {
    closePort();
  }
}

void Modem::onPingTimeout()
{
  if (!m_pingReceived)
  {
    // modem disconnected or port hanged
    closePort();
  }
  else
  {
    ModemRequest * pingRequest = createRequest(m_initHandler->name(), INIT_REQUEST_PING, 1);
    appendRequest(pingRequest);
    m_pingReceived = false;
    // restart timer every shot to decrease processing jitter
    m_pingTimer->start();
  }
}



ModemReply::ModemReply() :
  m_data(NULL)
{
}

ModemReply::~ModemReply()
{
  if (m_data)
  {
    delete m_data;
  }
}

AnswerData* ModemReply::data() const
{
  return m_data;
}

QString ModemReply::handlerName() const
{
  return m_handlerName;
}

int ModemReply::type() const
{
  return m_type;
}

bool ModemReply::status() const
{
  return m_status;
}




ModemRequest::~ModemRequest()
{
  if (m_args)
  {
    delete m_args;
  }
}

int ModemRequest::type() const
{
  return m_type;
}

RequestArgs*ModemRequest::args() const
{
  return m_args;
}

int ModemRequest::stage() const
{
  return m_stage;
}

void ModemRequest::setStage(int stage)
{
  m_stage = stage;
}

Modem*ModemRequest::modem() const
{
  return m_modem;
}





