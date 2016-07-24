#include "BreakPoint.h"
#include "DebugCore.h"
#include "Log.h"

BreakPoint::BreakPoint(DebugCore *debugCore)
    :m_debugCore(debugCore), m_address(0),
      m_orgByte(0), m_enabled(false), m_isHardware(false)
{

}

static const uint8_t bpData = 0xCC;

bool BreakPoint::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
    {
        return true;
    }

    if (enabled)
    {
        bool r = m_debugCore->readMemory(m_address, &m_orgByte, 1);
        if (!r)
        {
            log(QString("无法启用断点 %1：readMemory()失败。").arg(QString::number(m_address, 16)), LogType::Warning);
            return false;
        }
        r = m_debugCore->writeMemory(m_address, &bpData, 1);
        if (!r)
        {
            log(QString("无法启用断点 %1：writeMemory()失败。").arg(QString::number(m_address, 16)), LogType::Warning);
            return false;
        }

        return true;
    }

    uint8_t tmp;
    bool r = m_debugCore->readMemory(m_address, &tmp, 1);
    if (!r)
    {
        log(QString("读取断点 %1 处内存失败。").arg(QString::number(m_address, 16)), LogType::Warning);
    }

    if (tmp != bpData)
    {
        log(QString("断点 %1 的数据不为0xCC，已被重写为 0x%2")
                                   .arg(QString::number(m_address, 16)).arg(QString::number(tmp, 16)),
                                   LogType::Warning);
    }

    r = m_debugCore->writeMemory(m_address, &m_orgByte, 1);
    if (!r)
    {
        log(QString("无法禁用断点 %1：writeMemory()失败。").arg(QString::number(m_address, 16)), LogType::Warning);
        return false;
    }

    return true;

}
