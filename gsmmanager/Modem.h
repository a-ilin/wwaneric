#ifndef MODEM_H
#define MODEM_H

#include "PortController.h"

#include <QMap>
#include <QReadWriteLock>
#include <QStringList>

QStringList parseAnswerLine(const QString &line, const QString &command);

class Modem;

// data that passed to conversation handler
struct RequestArgs
{
  virtual ~RequestArgs() {}
};
// data that passed from conversation handler
struct AnswerData
{
  virtual ~AnswerData() {}
};

class ModemReply
{
public:
  ModemReply();
  ~ModemReply();

  QString handlerName() const;
  int type() const;
  bool status() const;
  AnswerData* data() const;

private:

  // hadler name
  QString m_handlerName;
  // inner reply handler type, usually equals to request type,
  // but may differs in case of processing unexpected data
  int m_type;
  // success (true) or failure(false)
  bool m_status;
  // reply data
  AnswerData * m_data;

  friend class Modem;
};

Q_DECLARE_METATYPE(ModemReply)
Q_DECLARE_METATYPE(ModemReply*)
Q_DECLARE_METATYPE(ModemReply**)

class ModemRequest
{
public:

  ~ModemRequest();

  enum Status
  {
    SuccessCompleted,
    SuccessNeedMoreData,
    Failure
  };

  int type() const;
  RequestArgs* args() const;

  int stage() const;
  void setStage(int stage);

  Modem* modem() const;

private:
  // inner request handler type
  int m_type;
  // arguments to handler
  RequestArgs *m_args;
  // stage
  int m_stage;
  // retry count in case of error, if 0 request will not be processed
  int m_retry;
  // base type offset for conversation handler
  int m_baseOffset;
  // modem
  Modem * m_modem;

  friend class Modem;
};

Q_DECLARE_METATYPE(ModemRequest)
Q_DECLARE_METATYPE(ModemRequest*)
Q_DECLARE_METATYPE(ModemRequest**)


class ConversationHandler
{
public:

  ConversationHandler() {}
  virtual ~ConversationHandler() {}

  // processes conversation (answer) for specified request
  // Args:
  // [in, out] request    - current request in queue (can be modified in this method)
  // [in]      c          - modem answer
  // [out]     status     - set to corresponding status after request processing
  // [out]     answerData - pointer that must be initialized by answer data or set to NULL
  virtual void processConversation(ModemRequest *request,
                                   const Conversation &c,
                                   ModemRequest::Status &status,
                                   AnswerData* &answerData) const
  {
    Q_UNUSED(request);
    Q_UNUSED(c);
    Q_UNUSED(answerData);
    status = ModemRequest::Failure;
  }

  // processes unexpected data from modem
  // if request processed successful should return true, otherwise false
  // Args:
  // [in]  data       - data received by modem
  // [out] replyType  - should set to reply type if can recognize the data
  // [out] answerData - pointer that must be initialized by answer data or set to NULL
  virtual bool processUnexpectedData(const QByteArray& data,
                                     int &replyType,
                                     AnswerData* &answerData) const
  {
    Q_UNUSED(data);
    Q_UNUSED(replyType);
    Q_UNUSED(answerData);
    return false;
  }

  // returns the data that should be sent to port for specified request
  virtual QByteArray requestData(const ModemRequest *request) const
  {
    Q_UNUSED(request);
    return QByteArray();
  }

  // returns the number of request types that can be processed by this handler
  virtual int requestTypesCount() const
  {
    return 0;
  }

  // returns the name for this handler (for internal purposes)
  virtual QString name() const = 0;

  // returns the request args for specified request type
  virtual RequestArgs* requestArgs(int type) const
  {
    Q_UNUSED(type);
    return NULL;
  }
};


class InitConversationHandler;
class QTimer;

class Modem : public PortController
{
  Q_OBJECT

public:
  Modem();
  ~Modem();

  void registerConversationHandler(ConversationHandler * handler);
  void unregisterConversationHandler(ConversationHandler * handler);

  // creates modem request with specified type to passed handler name
  // and specified retry count on errors
  ModemRequest * createRequest(const QString &handlerName, int type, int retries);

public slots:
  // appends request to modem queue,
  // after processing modem will delete the ModemRequest object
  void appendRequest(ModemRequest *request);

signals:
  void replyReceived(ModemReply * reply);

protected:
  bool processConversation(Conversation c);
  bool processUnexpectedData(const QByteArray& data);
  QByteArray requestData() const;
  void modemDetected(bool status);

protected slots:
  void onInitTimeout();
  void onPingTimeout();

private:
  QLinkedList<ModemRequest*> m_requests;

  // baseOffset ; handler
  QMap<int, ConversationHandler*> m_conversationHandlers;
  // handler name ; baseOffset
  QMap<QString, int> m_conversationHandlersNames;

  // is modem initialization sequence finished
  bool m_modemInited;
  // handler that processes modem initialization
  InitConversationHandler * m_initHandler;
  // init timeout timer
  QTimer * m_initTimer;
  // timer checks modem availability
  QTimer * m_pingTimer;
  bool m_pingReceived;

  mutable QReadWriteLock m_rwlock;

  Q_DISABLE_COPY(Modem)
};

Q_DECLARE_METATYPE(Modem*)
Q_DECLARE_METATYPE(Modem**)

#endif // MODEM_H
