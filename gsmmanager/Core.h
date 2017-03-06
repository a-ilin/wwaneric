/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 by Aleksei Ilin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef CORE_H
#define CORE_H

#include "Modem.h"

#include <QtGlobal>
#include <QList>
#include <QObject>
#include <QUuid>

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
  void tini();

  static Core * instance() { return m_instance; }

  void createConnection(const QUuid& id);
  void removeConnection(const QUuid& id);

  static QString appUserDirectory();

  // returns the request created by modem
  ModemRequest* modemRequest(const QUuid &connectionId,
                             const QString &conversationHandlerName,
                             int requestType,
                             int requestRetries) const;



  MainWindow * mainWindow() const { return m_mainWindow; }

  void storeSettings();
  void restoreSettings();

public slots:
  // push request to modem, request must be acquired by modemRequest(...) before this call
  void pushRequest(ModemRequest * request);

  void openConnection(const QUuid& id, const QString &portName, const PortOptions &options);
  void closeConnection(const QUuid& id);

signals:
  void connectionEvent(const QUuid& connectionId, Core::ConnectionEvent event, const QVariant &data);

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

  QMap<QUuid, Modem* > m_modems;
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
  void createModem(Modem** modem, QUuid id);
  void deleteModem(Modem** modem);
};

#endif // CORE_H
