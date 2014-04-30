#ifndef MODEMINIT_H
#define MODEMINIT_H

#include "Modem.h"

#define INIT_HANDLER_NAME "Init"

enum INIT_REQUEST_TYPE
{
  INIT_REQUEST_INIT,
  INIT_REQUEST_LAST  // a value for type counting. not a real request type.
};

class InitConversationHandler : public ConversationHandler
{
public:
  void processConversation(ModemRequest *request,
                           const Conversation &c,
                           ModemRequest::Status &status,
                           AnswerData* &answerData) const;

  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

};

#endif // MODEMINIT_H
