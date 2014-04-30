#ifndef CORE_H
#define CORE_H

#include "Modem.h"

#include <QtGlobal>
#include <QList>
#include <QObject>


class MainWindow;
class ModemThreadHelper;
class QThread;

class Core : public QObject
{
  Q_OBJECT

public:

  enum ConnectionEvent
  {
    ConnectionEventStatus,
    ConnectionEventError,
    ConnectionEventCustom,
    ConnectionEventLast
  };



  Core();
  ~Core();

  bool init();
  bool tini();

  static Core * instance() { return m_instance; }

  void createConnection(const QString &id);
  void removeConnection(const QString &id);

  QString appUserDirectory() const;

  // returns the request created by modem
  ModemRequest* modemRequest(const QString &connectionId,
                             const QString &conversationHandlerName,
                             int requestType,
                             int requestRetries) const;



  MainWindow * mainWindow() const { return m_mainWindow; }

  void storeSettings();
  void restoreSettings();

public slots:
  // push request to modem, request must be acquired by modemRequest(...) before this call
  void pushRequest(ModemRequest * request);

  void openConnection(const QString &id, const QString &portName, const PortOptions &options);
  void closeConnection(const QString &id);

signals:
  void connectionEvent(const QString &connectionId, Core::ConnectionEvent event, const QVariant &data);

protected slots:
  // called via queued connection by modem when reply is ready.
  // core will delete the reply after deliver it to receivers
  void onReplyReceived(ModemReply * reply);
  // called via queued connection by modem when port status was changed.
  void onConnectionStatusChanged(bool status);
  // called via queued connection by modem when an error occured.
  void onConnectionErrorOccured(const QString error);

private:

  static Core * m_instance;

  QMap<QString, Modem* > m_modems;
  QList<ConversationHandler*> m_conversationHandlers;

  MainWindow * m_mainWindow;

  // thread for modems living
  QThread * m_modemThread;
  ModemThreadHelper * m_modemThreadHelper;

  Q_DISABLE_COPY(Core)
};


class ModemThreadHelper : public QObject
{
  Q_OBJECT

public slots:

  // we need to create/delete modem and it's children and aggregates in separate thread,
  // call this slots using blocking queued connection to achieve this
  void createModem(Modem** modem, const QString& id);
  void deleteModem(Modem** modem);
};

#endif // CORE_H
