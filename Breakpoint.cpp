#include "Breakpoint.h"
#include "DebugCore.h"
#include "Log.h"

Breakpoint::Breakpoint(DebugCore *debugCore)
    :m_debugCore(debugCore)
{

}

const uint8_t Breakpoint::bpData = 0xCC;

bool Breakpoint::setEnabled(bool enabled)
{
    log(QString("bp set enabled: %1").arg(enabled));
    if (enabled == m_enabled)
    {
        return true;
    }

    if (enabled)
    {
        bool r = m_debugCore->readMemory(m_address, &m_orgByte, 1, false);
        if (!r)
        {
            log(QString("无法启用断点 %1：readMemory()失败。").arg(QString::number(m_address, 16)), LogType::Warning);
            return false;
        }
        r = m_debugCore->writeMemory(m_address, &bpData, 1, false);
        if (!r)
        {
            log(QString("无法启用断点 %1：writeMemory()失败。").arg(QString::number(m_address, 16)), LogType::Warning);
            return false;
        }

        m_enabled = true;
        return true;
    }

    log("disable bp");
    uint8_t tmp;
    bool r = m_debugCore->readMemory(m_address, &tmp, 1, false);
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

    r = m_debugCore->writeMemory(m_address, &m_orgByte, 1, false);
    if (!r)
    {
        log(QString("无法禁用断点 %1：writeMemory()失败。").arg(QString::number(m_address, 16)), LogType::Warning);
        return false;
    }

    m_enabled = false;
    return true;
}

Breakpoint::~Breakpoint()
{
    setEnabled(false);
}

