#include "Modem.h"

#include "common.h"

QEvent::Type ModemEventType = (QEvent::Type)QEvent::registerEventType();


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
  if(index != 0)  // line should starts with the command
  {
    return QStringList();
  }

  index = command.size();
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
        if (str.at(str.length() - 1) != ',')
        {
          return QStringList();
        }
        else
        {
          // remove last comma
          str.resize(str.size() - 1);
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
  PortController()
{

}

Modem::~Modem()
{
  if (m_conversationHandlers.size() > 0)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "There is a conversation handler still registered!");
  }

  if (m_unexpectedDataHandlers.size() > 0)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "There is an unexpected data handler still registered!");
  }
}

void Modem::registerConversationHandler(ConversationHandler *handler)
{
  if (!handler)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Null pointer passed!");
    return;
  }

  if (handler->m_baseRequestType || handler->m_modem)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Passed handler already was in use!");
    return;
  }

  // the Map is sorted
  int maxRequestType = 0;
  if (m_conversationHandlers.size() > 0)
  {
    const ConversationHandler * lastHandler = m_conversationHandlers.last();
    maxRequestType = lastHandler->m_baseRequestType + lastHandler->requestTypesCount();
  }

  handler->m_modem = this;
  handler->m_baseRequestType = maxRequestType;
  m_conversationHandlers.insert(maxRequestType, handler);
}

void Modem::unregisterConversationHandler(ConversationHandler *handler)
{
  if (!handler)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Null pointer passed!");
    return;
  }

  ConversationHandler *handlerFound = m_conversationHandlers.value(handler->m_baseRequestType);
  if (handlerFound == handler)
  {
    m_conversationHandlers.remove(handler->m_baseRequestType);
    handler->m_baseRequestType = 0;
    handler->m_modem = NULL;
  }
  else
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Unregistered handler passed!");
  }
}

void Modem::registerUnexpectedDataHandler(UnexpectedDataHandler* handler)
{
  if (!handler)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Null pointer passed!");
    return;
  }

  if (!m_unexpectedDataHandlers.contains(handler))
  {
    m_unexpectedDataHandlers.append(handler);
  }
}

void Modem::unregisterUnexpectedDataHandler(UnexpectedDataHandler* handler)
{
  if (!handler)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Null pointer passed!");
    return;
  }

  int removed = m_unexpectedDataHandlers.removeAll(handler);

  if(!removed)
  {
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "Handler was not registered!");
  }
}

void Modem::appendRequest(ModemRequest *request)
{
  m_requests.append(request);
  sendRequest();
}

bool Modem::processConversation(const Conversation &c)
{
  if (!m_requests.size())
  {
    // no repeat for unexpected answers
    Q_LOGEX(LOG_VERBOSE_CRITICAL, "No pending requests exist!");
    return true;
  }

  // conversation successfully processed
  bool success = false;
  // conversation processed and corresponding request should be removed
  bool requestProcessed = false;

  if (requestData() == c.request)
  {
    ModemRequest *request = m_requests.first();
    int requestBaseTypeOffset = request->baseType;
    ConversationHandler * handler = m_conversationHandlers.value(requestBaseTypeOffset);

    if (handler)
    {
      success = handler->processConversation(request, c, requestProcessed);

      if (c.status == Conversation::OK)
      {
        if (!success)
        {
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
      }

      if (!success)
      {
        --request->retryCount;
        Q_ASSERT(request->retryCount >= 0);

        // remove request with ended retries
        if (!request->retryCount)
        {
          requestProcessed = true;
          success = true;
          Q_LOGEX(LOG_VERBOSE_NOTIFICATION, "Request retry count is ended. Request removed from queue.");
        }
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

  if (requestProcessed)
  {
    delete m_requests.takeFirst();
  }

  return success;
}

bool Modem::processUnexpectedData(const QByteArray& data)
{
  bool result = false;

  for(int i=0; (i< m_unexpectedDataHandlers.size()) && (!result); ++i)
  {
    result = m_unexpectedDataHandlers.at(i)->processUnexpectedData(data);
  }

  if (!result)
  {
    QString str = QString("Unexpected data was not processed: %1").arg(QString(data));
    Q_LOGEX(LOG_VERBOSE_WARNING, str);
  }

  return result;
}

QByteArray Modem::requestData() const
{
  QByteArray data;

  foreach(const ModemRequest *request, m_requests)
  {
    int requestBaseTypeOffset = request->baseType;
    const ConversationHandler * handler = m_conversationHandlers.value(requestBaseTypeOffset);

    if (handler)
    {
      data = handler->requestData(request);
      if (!data.size())
      {
        QString str = QString("Handler returned empty data for request. Base type: %1, type: %2")
                      .arg(requestBaseTypeOffset)
                      .arg(request->requestType);
        Q_LOGEX(LOG_VERBOSE_CRITICAL, str);
      }
    }
    else
    {
      QString err = QString("Handler for this type of request was unregistered! Base type: %1, type: %2")
                    .arg(requestBaseTypeOffset)
                    .arg(request->requestType);
      Q_LOGEX(LOG_VERBOSE_CRITICAL, err);
    }

    if (data.size())
    {
      break;
    }
  }

  return data;
}

