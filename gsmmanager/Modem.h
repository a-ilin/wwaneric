#ifndef MODEM_H
#define MODEM_H

#include "PortController.h"

#include <QEvent>
#include <QMap>
#include <QStringList>

QStringList parseAnswerLine(const QString &line, const QString &command);

class ConversationHandler;
class Modem;

struct RequestArgs
{
  virtual ~RequestArgs() {}
};

struct ModemRequest
{
  ModemRequest(int btype, int type, RequestArgs* args = NULL) :
    requestType(type),
    requestArgs(args),
    requestStage(0),
    baseType(btype)
  {}

  ~ModemRequest()
  {
    if (requestArgs)
    {
      delete requestArgs;
    }
  }

  int requestType;
  RequestArgs *requestArgs;
  int requestStage;

private:
  // base type offset for conversation handler
  int baseType;

  friend class Modem;
};

class ConversationHandler : public QObject
{
public:
  ConversationHandler() :
    QObject(NULL),
    m_baseRequestType(0),
    m_modem(NULL)
  {}

  // processes conversation (answer) for specified request
  // if request processed successful should return true, otherwise false
  // Args:
  // request - current request in queue (can be modified in this method)
  // c - modem answer
  // requestFinished - if request has no more stages should set it to true, otherwise to false
  virtual bool processConversation(ModemRequest *request, const Conversation &c, bool &requestFinished) = 0;

  // returns the data that should be sent to port for specified request
  virtual QByteArray requestData(const ModemRequest *request) const = 0;

  // returns the number of request types that can be processed by this handler
  virtual int requestTypesCount() const = 0;

  // returns the name for this handler (for internal purposes)
  virtual QString name() const = 0;

  ModemRequest* createEmptyRequest(int type, RequestArgs * args = NULL) const
  {
    ModemRequest* r = new ModemRequest(m_baseRequestType, type);
    r->requestArgs = args ? args : requestArgs();
    r->requestStage = 0;
    return r;
  }

  Modem* modem() const
  {
    return m_modem;
  }

protected:
  // returns the request args for this conversation handler
  virtual RequestArgs *requestArgs() const
  {
    return new RequestArgs();
  }

private:

  // Base request type offset for this handler. Set when handler registration occures.
  int m_baseRequestType;

  Modem * m_modem;

  friend class Modem;
};


// an ID for QEvent object constructing
extern QEvent::Type ModemEventType;

class Modem : public PortController
{
  Q_OBJECT

public:
  Modem();
  ~Modem();

  void registerConversationHandler(ConversationHandler * handler);
  void unregisterConversationHandler(ConversationHandler * handler);

public slots:
  void appendRequest(ModemRequest *request);

protected:
  // return true if conversation was recognized and successfully processed
  bool processConversation(const Conversation & c);

  // return raw data for request from queue
  QByteArray requestData() const;

private:
  QLinkedList<ModemRequest*> m_requests;

  // key is baseRequestOffset
  QMap<int, ConversationHandler*> m_conversationHandlers;

};

#endif // MODEM_H
