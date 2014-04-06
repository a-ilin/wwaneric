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
  /* A part of line.
   * quoted is true if the part was quoted and it's contents shouldn't be parsed for commas
   * start includes a start comma and quotes (if exists)
   * length includes a start comma and quotes, (if exists)
   * string includes only useful data, i.e. it excludes commas and quotes
  */
  struct StringPart
  {
    bool quoted;
    int start;
    int size;
    QString string;
  };

  QStringList result;

  // check for command
  int index = line.indexOf(command);
  if(index == 0)  // line should starts with the command
  {
    index = command.size();
    while(line.at(index) == ' ')
    {
      ++index;
    }

    // a string with cutted command and spaces after it
    QString midLine = line.mid(index);

    QList<int> quoteIndexes;
    for (index = midLine.indexOf('\"'); index != -1; index = midLine.indexOf('\"', index+1))
    {
      quoteIndexes.append(index);
    }

    // each quote should correspond it's pair
    if (!(quoteIndexes.size() % 2))
    {
      // key: start index
      QMap<int, StringPart> parts;

      bool syntaxOk = true;

      // insert quoted into line parts
      for(int i=0; i< quoteIndexes.size() / 2; ++i)
      {
        int openIndex = quoteIndexes.at(i);
        int closeIndex = quoteIndexes.at(i+1);

        // check for edge of ','
        if (openIndex && (midLine.at(openIndex-1) != ','))
        {
          syntaxOk = false;
          break;
        }

        if ((closeIndex < midLine.size() - 1) && (midLine.at(closeIndex+1) != ','))
        {
          syntaxOk = false;
          break;
        }

        StringPart part;
        part.quoted = true;
        part.start = openIndex ? openIndex - 1 : openIndex;
        part.size = closeIndex - part.start + 1;
        part.string = midLine.mid(openIndex+1, closeIndex-openIndex-1);

        parts.insert(part.start, part);
      }

      if (syntaxOk)
      {
        // key: start index
        QMap<int, StringPart> partsNotQuoted;

        // insert non-quoted into line parts
        QMap<int, StringPart>::const_iterator iter = parts.constBegin();
        QMap<int, StringPart>::const_iterator iterEnd = parts.constEnd();
        int previousEnd = -1;
        while (iter != iterEnd)
        {
          int quotedIndex = iter.key();
          const StringPart& quotedPart = iter.value();

          if (quotedIndex > previousEnd + 1)
          {
            StringPart part;
            part.quoted = false;
            part.start = previousEnd + 1;
            part.size = quotedIndex - previousEnd - 1;
            part.string = midLine.mid(part.start ? part.start + 1 : part.start,
                                      quotedIndex - part.start);

            partsNotQuoted.insert(part.start, part);
          }

          previousEnd = quotedIndex + quotedPart.size - 1;

          ++iter;
        }

        // insert non-quoted rest
        if (parts.size() > 0)
        {
          const StringPart& last = parts.last();
          if (last.start + last.size < midLine.size())
          {
            StringPart part;
            part.quoted = false;
            part.start = last.start + last.size;
            part.size = midLine.size() - part.start;
            part.string = midLine.mid(part.start + 1, part.size - 1);

            partsNotQuoted.insert(part.start, part);
          }
        }
        // if no quoted parts exist then insert a midline itself
        else
        {
          StringPart part;
          part.quoted = false;
          part.start = 0;
          part.size = midLine.size();
          part.string = midLine;

          partsNotQuoted.insert(part.start, part);
        }

        // unite
        parts.unite(partsNotQuoted);

        // construct result string list
        foreach(const StringPart & part, parts)
        {
          if (part.quoted)
          {
            result.append(part.string);
          }
          else
          {
            result.append(part.string.split(',', QString::KeepEmptyParts));
          }
        }
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

