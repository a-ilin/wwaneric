#ifndef PORTCONTROLLER_H
#define PORTCONTROLLER_H

#include <QObject>
#include <QLinkedList>
#include <QList>
#include <QtSerialPort/QSerialPort>

class QSerialPort;
class QTimer;

// first line of Conversation has request itself
// last line of Conversation has status: OK/ERROR
typedef QList<QByteArray> Conversation;

class PortController : public QObject
{
  Q_OBJECT

public:
  PortController();

  QString portName() const { return m_serialPort->portName(); }

public slots:
  /*
   * Connection information
   *
  */
  void setPortName(const QString &portName);
  void openPort();
  void closePort();

signals:
  /*
   * Connection information
   *
  */
  void updatedPortStatus(bool opened);
  void updatedPortError(QSerialPort::SerialPortError errorCode);
  void updatedPortError(const QString& errorString);

protected:
  // return true if conversation was recognized and successfully processed
  virtual bool processConversation(const Conversation & c) = 0;

  // return raw data for request from queue
  virtual QByteArray requestData() const = 0;

protected slots:
  // retrieves next request and sends it to port sendToPort
  void sendRequest();
  void sendRequest(const QByteArray &request);

private:
  // writes raw data to port
  void sendToPort(const QByteArray &data);
  // send a request from the queue
  void sendRequestFromQueue();
  // return true if all requested data received and false otherwise
  bool parseBuffer();

  void notifyError(QSerialPort::SerialPortError errorCode);

private slots:
  // reads data from port to buffer
  void onReadyRead();
  // error processing
  void onError(QSerialPort::SerialPortError errorCode);
  void onReadChannelFinished();
  // connection timeout
  void onTimerTimeout();

private:

  enum PORT_CONTROLLER_STATUS
  {
    PORT_CONTROLLER_STATUS_READY,
    PORT_CONTROLLER_STATUS_BUSY
  } m_portControllerStatus;

  // modem detected on specified port
  bool m_modemDetected;

  // interface to serial port communication
  QSerialPort * m_serialPort;

  // received data buffer
  QByteArray m_bufferReceived;

  // timer timeout
  QTimer * m_timerTimeout;
};

#endif // PORTCONTROLLER_H
