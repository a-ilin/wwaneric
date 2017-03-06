﻿/*
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

#include "ModemInit.h"


#define CMD_CHARACTER_SET      "AT+CSCS=\"UTF-8\""
#define CMD_MESSAGE_INDICATION "AT+CNMI=2,1,2,1,0"
#define CMD_AT                 "AT"

#define LAST_INIT_STAGE 1


void InitConversationHandler::processConversation(ModemRequest *request,
                                                  const Conversation &c,
                                                  ModemRequest::Status& status,
                                                  AnswerData*& answerData) const
{
  Q_UNUSED(answerData);

  const int requestType = request->type();
  //const RequestArgs &requestArgs = request->requestArgs;

  status = ModemRequest::Failure;

  if (c.status == Conversation::OK)
  {
    if (requestType == INIT_REQUEST_INIT)
    {
      // set preferred storage
      if (request->stage() < LAST_INIT_STAGE)
      {
        request->setStage(request->stage() + 1);
        status = ModemRequest::SuccessNeedMoreData;
      }
      else
      {
        status = ModemRequest::SuccessCompleted;
      }
    }
    else if (requestType == INIT_REQUEST_PING)
    {
      if (request->stage() == 0)
      {
        status = ModemRequest::SuccessCompleted;
      }
    }
  }
}

QByteArray InitConversationHandler::requestData(const ModemRequest *request) const
{
  const int requestType = request->type();
  //const RequestArgs &requestArgs = request->requestArgs;
  const int requestStage = request->stage();

  QByteArray data;

  if (requestType == INIT_REQUEST_INIT)
  {
    if (requestStage == 0)
    {
      data = CMD_CHARACTER_SET;
    }
    else if (requestStage == 1)
    {
      data = CMD_MESSAGE_INDICATION;
    }
  }
  else if (requestType == INIT_REQUEST_PING)
  {
    if (requestStage == 0)
    {
      data = CMD_AT;
    }
  }

  return data;
}

int InitConversationHandler::requestTypesCount() const
{
  return INIT_REQUEST_LAST;
}

QString InitConversationHandler::name() const
{
  return QString(INIT_HANDLER_NAME);
}

