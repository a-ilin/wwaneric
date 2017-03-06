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

#ifndef PORTCONTROLLER_H
#define PORTCONTROLLER_H

#include <QObject>
#include <QLinkedList>
#include <QList>
#include <QtSerialPort/QSerialPort>

class QSerialPort;
class QTimer;

const QByteArray phraseSep("\r\n");

// first line of Conversation has request itself
// last line of Conversation has status: OK/ERROR
struct Conversation
{
  enum STATUS
  {
    OK,
    ERROR,
    UNKNOWN
  };

  // user's request echo-reply
  QByteArray request;
  // answer's data
  QList<QByteArray> data;
  // status detection flag
  STATUS status;
  // status data (typically this is corresponds to status)
  QByteArray statusData;

};

struct PortOptions
{
  PortOptions() :
    baudRateDefault(true),
    dataBitsDefault(true),
    flowControlDefault(true),
    parityDefault(true),
    stopBitsDefault(true)
  {}

  QSerialPort::BaudRate baudRate;
  bool baudRateDefault;
  QSerialPort::DataBits dataBits;
  bool dataBitsDefault;
  QSerialPort::FlowControl flowControl;
  bool flowControlDefault;
  QSerialPort::Parity parity;
  bool parityDefault;
  QSerialPort::StopBits stopBits;
  bool stopBitsDefault;
};

Q_DECLARE_METATYPE(PortOptions)

class PortController : public QObject
{
  Q_OBJECT

public:
  PortController();

  QString portName() const { return m_serialPort->portName(); }
  const PortOptions& portOptions() const { return m_options; }

public slots:
  /*
   * Connection information
   *
  */
  void setPortName(const QString portName);
  void setPortOptions(const PortOptions options);
  void openPort();
  void closePort();

signals:
  /*
   * Connection information
   *
  */
  void updatedPortStatus(bool opened);
  void updatedPortError(QSerialPort::SerialPortError errorCode);
  void updatedPortError(const QString errorString);

protected:
  // return true if conversation was recognized and successfully processed
  virtual bool processConversation(Conversation c) = 0;
  // return true if unexpected modem data has been processed
  virtual bool processUnexpectedData(const QByteArray& data) = 0;
  // return raw data for request from queue
  virtual QByteArray requestData() const = 0;
  // called when modem detection status changed
  virtual void modemDetected(bool status);

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
    PORT_CONTROLLER_STATUS_PROCESS_REQUEST,
    PORT_CONTROLLER_STATUS_PROCESS_UNEXPECTED_DATA
  } m_portControllerStatus;

  // modem detected on specified port
  bool m_modemDetected;

  // interface to serial port communication
  QSerialPort * m_serialPort;

  // received data buffer
  QByteArray m_bufferReceived;

  // timer timeout
  QTimer * m_timerTimeout;

  // serial port options
  PortOptions m_options;

  Q_DISABLE_COPY(PortController)
};

#endif // PORTCONTROLLER_H
