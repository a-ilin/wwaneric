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

struct UssdArgs : public RequestArgs
{
};

class UssdConversationHandler : public ConversationHandler
{
  Q_OBJECT

public:
  bool processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished);
  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

public slots:
  void sendUssd(const QString &ussd, USSD_SEND_STATUS status);

signals:
  void updatedUssd(const QString &ussd, USSD_STATUS status);
};

#endif // MODEMUSSD_H
