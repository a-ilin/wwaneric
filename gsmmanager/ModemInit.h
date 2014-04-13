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
  Q_OBJECT

public:
  bool processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished);
  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

public slots:
  void initModem();

signals:
  void modemInited();

};

#endif // MODEMINIT_H
