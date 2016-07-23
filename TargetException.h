//
// Created by 邢俊杰 on 16/7/22.
//

#pragma once

#include "Common.h"

using ExceptionCallback = std::function<bool(ExceptionInfo const&)>;

bool waitException(task_t task);
bool replyException(task_t task);
bool setExceptionPort(task_t task, ExceptionCallback callback);
