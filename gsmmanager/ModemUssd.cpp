#include "ModemUssd.h"

#define CMD_USSD_SEND "AT+CUSD=%1,\"%2\",15"



static bool processUssdData(const QString& data, QString& msg, USSD_STATUS& status)
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


void UssdConversationHandler::processConversation(ModemRequest *request,
                                                  const Conversation &c,
                                                  ModemRequest::Status& status,
                                                  AnswerData*& answerData) const
{
  const int requestType = request->type();
  //const UssdArgs* ussdArgs = static_cast<UssdArgs*> (request->requestArgs);

  status = ModemRequest::Failure;

  if (requestType == USSD_REQUEST_SEND)
  {
    if (c.status == Conversation::OK)
    {
      QByteArray toProcess;
      if (c.data.size() > 0)
      {
        toProcess = c.data.first();
      }
      // it can be just a confirmation of our termination request processing
      else
      {
        toProcess = c.request;
      }

      QString msg;
      USSD_STATUS ussdStatus;
      if (processUssdData(toProcess, msg, ussdStatus))
      {
        UssdAnswer * answer = new UssdAnswer();
        answer->status = ussdStatus;
        answer->ussd = msg;
        answerData = answer;

        status = ModemRequest::SuccessCompleted;
      }
    }
  }
}

QByteArray UssdConversationHandler::requestData(const ModemRequest *request) const
{
  const int requestType = request->type();
  const UssdArgs *ussdArgs = static_cast<const UssdArgs*> (request->args());

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

RequestArgs *UssdConversationHandler::requestArgs(int type) const
{
  Q_UNUSED(type);
  return new UssdArgs();
}

bool UssdConversationHandler::processUnexpectedData(const QByteArray& data,
                                                    int &replyType,
                                                    AnswerData* &answerData) const
{
  QString msg;
  USSD_STATUS status;

  if (processUssdData(QString(data), msg, status))
  {
    replyType = USSD_REQUEST_SEND;

    UssdAnswer * answer = new UssdAnswer();
    answer->status = status;
    answer->ussd = msg;
    answerData = answer;

    return true;
  }

  return false;
}
