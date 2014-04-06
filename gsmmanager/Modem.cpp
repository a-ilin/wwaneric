#include "Modem.h"

#include "common.h"

QEvent::Type ModemEventType = (QEvent::Type)QEvent::registerEventType();

QStringList parseAnswerLine(const QString &line, const QString &command)
{
  QStringList result;
  int index = line.indexOf(command);
  if(index == 0)  // line should start with the command
  {
    index = command.size();
    while(line.at(index) == ' ')
    {
      ++index;
    }

    result = line.mid(index).split(',');
  }

  return result;
}

Modem::Modem() :
  PortController()
{

}

Modem::~Modem()
{
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

  if (c.size() >= 2)
  {
    QByteArray answerHeader(c.first());

    // echo reply has \r at the end
    if (answerHeader.at(answerHeader.size() - 1) == '\r')
    {
      answerHeader.remove(answerHeader.size() - 2, 1);
    }

    if (requestData() == answerHeader)
    {
      ModemRequest *request = m_requests.first();
      int requestBaseTypeOffset = request->baseType;
      ConversationHandler * handler = m_conversationHandlers.value(requestBaseTypeOffset);

      if (handler)
      {
        success = handler->processConversation(request, c, requestProcessed);
        if (!success)
        {
          QString str = QString("Unprocessed answer received. Request: %1. Answer: %2")
                        .arg(QString(requestData()))
                        .arg(QString(answerHeader));
          Q_LOGEX(LOG_VERBOSE_CRITICAL, str);
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
                    .arg(QString(answerHeader));
      Q_LOGEX(LOG_VERBOSE_CRITICAL, str);
    }
  }
  else
  {
    Q_LOGEX(LOG_VERBOSE_WARNING, "Too small conversation received!");
  }

  if (requestProcessed)
  {
    delete m_requests.takeFirst();
  }

  return success;
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

