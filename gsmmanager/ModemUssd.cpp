#include "ModemUssd.h"

#define CMD_USSD_SEND "AT+CUSD=%1,\"%2\",15"


bool UssdConversationHandler::processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished)
{
  const int& requestType = request->requestType;
  //const UssdArgs* ussdArgs = static_cast<UssdArgs*> (request->requestArgs);

  bool success = false;

  if (requestType == USSD_REQUEST_SEND)
  {
    if (c.status == Conversation::OK)
    {
      if (c.data.size() > 0)
      {
        QString answer;
        USSD_STATUS status;
        if (processUssdData(c.data.first(), answer, status))
        {
          emit updatedUssd(answer, status);
          requestFinished = true;
          success = true;
        }
      }
      // it can be just a confirmation of our termination request processing
      else
      {
        QString answer;
        USSD_STATUS status;
        if (processUssdData(c.request, answer, status))
        {
          emit updatedUssd(answer, status);
          requestFinished = true;
          success = true;
        }
      }

      if (!success)
      {
        emit updatedUssd(tr("Cannot parse modem answer"), USSD_STATUS_LAST);
      }
    }
    else
    {
      emit updatedUssd(tr("ERROR"), USSD_STATUS_DIALOGUE_TERMINATED);
    }
  }

  return success;
}

QByteArray UssdConversationHandler::requestData(const ModemRequest *request) const
{
  const int &requestType = request->requestType;
  const UssdArgs *ussdArgs = static_cast<const UssdArgs*> (request->requestArgs);

  QByteArray data;

  if (requestType == USSD_REQUEST_SEND)
  {
    data = QString(CMD_USSD_SEND).arg(ussdArgs->status).arg(ussdArgs->ussd).toLatin1();
  }

  return data;
}

int UssdConversationHandler::requestTypesCount() const
{
  return USSD_REQUEST_LAST;
}

QString UssdConversationHandler::name() const
{
  return QString(USSD_HANDLER_NAME);
}

bool UssdConversationHandler::processUnexpectedData(const QByteArray& data)
{
  QString msg;
  USSD_STATUS status;

  if (processUssdData(QString(data), msg, status))
  {
    if (msg.size() > 0)
    {
      emit updatedUssd(msg, status);
    }
    else
    {
      emit updatedStatus(status);
    }

    return true;
  }

  return false;
}

void UssdConversationHandler::sendUssd(const QString &ussd, USSD_SEND_STATUS status)
{
  UssdArgs * ussdArgs = static_cast<UssdArgs*> (requestArgs());
  ussdArgs->ussd = ussd;
  ussdArgs->status = status;

  modem()->appendRequest(createEmptyRequest(USSD_REQUEST_SEND, ussdArgs));
}

bool UssdConversationHandler::processUssdData(const QString& data, QString& msg, USSD_STATUS& status) const
{
  bool result = false;
  QStringList headerLine = parseAnswerLine(data, "+CUSD:");

  // try checking our request
  if (!headerLine.size())
  {
    headerLine = parseAnswerLine(data, "AT+CUSD=");
  }

  if (headerLine.size() > 1)
  {
    bool decoded = true;
    SAFE_CONVERT(int, toInt, iStatus, headerLine.at(0), decoded=false;);
    if (decoded && checkUssdStatus(iStatus))
    {
      status = (USSD_STATUS)iStatus;
      msg = headerLine.at(1);
      result = true;
    }
    else
    {
      QString err = QString("Wrong USSD status received: %1").arg(headerLine.at(0));
      Q_LOGEX(LOG_VERBOSE_ERROR, err);
    }
  }
  else
  {
    Q_LOGEX(LOG_VERBOSE_ERROR, "Header size is too small!");
  }
  return result;
}

RequestArgs *UssdConversationHandler::requestArgs() const
{
  return new UssdArgs();
}

