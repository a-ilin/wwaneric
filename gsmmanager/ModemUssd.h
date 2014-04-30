#ifndef MODEMUSSD_H
#define MODEMUSSD_H

#include "Modem.h"

#include "Ussd.h"

#define USSD_HANDLER_NAME "USSD"

enum USSD_REQUEST_TYPE
{
  USSD_REQUEST_SEND,
  USSD_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct UssdAnswer : public AnswerData
{
  QString ussd;
  USSD_STATUS status;
};

struct UssdArgs : public RequestArgs
{
  QString ussd;
  USSD_SEND_STATUS status;
};

class UssdConversationHandler : public ConversationHandler
{
public:
  void processConversation(ModemRequest *request,
                           const Conversation &c,
                           ModemRequest::Status &status,
                           AnswerData* &answerData) const;

  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

  bool processUnexpectedData(const QByteArray& data,
                             int &replyType,
                             AnswerData* &answerData) const;

protected:
  RequestArgs* requestArgs(int type) const;
};





#endif // MODEMUSSD_H
