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

#ifndef MODEMSTATUS_H
#define MODEMSTATUS_H

#include "Modem.h"

#define STATUS_HANDLER_NAME "Status"

enum STATUS_REQUEST_TYPE
{
  STATUS_REQUEST_MANUFACTURER_INFO,
  STATUS_REQUEST_MODEL_INFO,
  STATUS_REQUEST_SERIAL_NUMBER,
  STATUS_REQUEST_REVISION_INFO,
  STATUS_REQUEST_SIGNAL_QUALITY,
  STATUS_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct StatusAnswer : public AnswerData
{
  QString data;
};

struct StatusSignalQualityAnswer : public AnswerData
{
  double signal_dbm;
  double signal_percent;
  bool   signal_detected;
  QString ber_percent_range;
  bool    ber_detected;
};

struct StatusArgs : public RequestArgs
{
};

class StatusConversationHandler : public ConversationHandler
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

#endif // MODEMSTATUS_H
