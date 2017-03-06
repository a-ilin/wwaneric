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

#ifndef MODEMSMS_H
#define MODEMSMS_H

#include "Modem.h"

#include "Sms.h"

#define SMS_HANDLER_NAME "SMS"

enum SMS_REQUEST_TYPE
{
  SMS_REQUEST_CAPACITY,
  SMS_REQUEST_READ,
  SMS_REQUEST_READ_BY_INDEX,
  SMS_REQUEST_READ_UNEXPECTED,
  SMS_REQUEST_SEND,
  SMS_REQUEST_DELETE,
  SMS_REQUEST_LAST  // a value for type counting. not a real request type.
};

struct SmsArgsRead : public RequestArgs
{
  SMS_STATUS smsStatus;
  SMS_STORAGE smsStorage;
};

struct SmsArgsReadByIndex : public RequestArgs
{
  SMS_STORAGE smsStorage;
  int smsIndex;
};

struct SmsArgsDelete : public RequestArgs
{
  SMS_STORAGE smsStorage;
  int smsIndex;
};

struct SmsAnswerDeleted : public AnswerData
{
  SMS_STORAGE smsStorage;
  int smsIndex;
};

struct SmsAnswerCapacity : public AnswerData
{
  int simUsed;
  int simTotal;
  int phoneUsed;
  int phoneTotal;
};

struct SmsAnswerRead : public AnswerData
{
  QList<Sms> smsList;
};

struct SmsAnswerReadByIndex : public AnswerData
{
  Sms sms;
};

struct SmsAnswerReadUnexpected : public AnswerData
{
  SMS_STORAGE smsStorage;
  int smsIndex;
};

class SmsConversationHandler : public ConversationHandler
{
public:
  void processConversation(ModemRequest *request,
                           const Conversation &c,
                           ModemRequest::Status &status,
                           AnswerData* &answerData) const;

  QByteArray requestData(const ModemRequest *request) const;
  int requestTypesCount() const;
  QString name() const;

  bool processUnexpectedData(const QByteArray& data,
                             int &replyType,
                             AnswerData* &answerData) const;

protected:
  RequestArgs* requestArgs(int type) const;
};

#endif // MODEMSMS_H
