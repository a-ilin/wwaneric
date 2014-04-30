#include "ModemInit.h"


#define CMD_CHARACTER_SET      "AT+CSCS=\"UTF-8\""

#define LAST_INIT_STAGE 0


void InitConversationHandler::processConversation(ModemRequest *request,
                                                  const Conversation &c,
                                                  ModemRequest::Status& status,
                                                  AnswerData*& answerData) const
{
  Q_UNUSED(answerData);

  const int requestType = request->type();
  //const RequestArgs &requestArgs = request->requestArgs;

  status = ModemRequest::Failure;

  if (c.status == Conversation::OK)
  {
    if (requestType == INIT_REQUEST_INIT)
    {
      // set preferred storage
      if (request->stage() < LAST_INIT_STAGE)
      {
        request->setStage(request->stage() + 1);
        status = ModemRequest::SuccessNeedMoreData;
      }
      else
      {
        status = ModemRequest::SuccessCompleted;
      }
    }
  }
}

QByteArray InitConversationHandler::requestData(const ModemRequest *request) const
{
  const int requestType = request->type();
  //const RequestArgs &requestArgs = request->requestArgs;
  const int requestStage = request->stage();

  QByteArray data;

  if (requestType == INIT_REQUEST_INIT)
  {
    if (requestStage == 0)
    {
      data = CMD_CHARACTER_SET;
    }
  }

  return data;
}

int InitConversationHandler::requestTypesCount() const
{
  return INIT_REQUEST_LAST;
}

QString InitConversationHandler::name() const
{
  return QString(INIT_HANDLER_NAME);
}

