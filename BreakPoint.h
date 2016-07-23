
#pragma once

#include <cstdint>

class DebugCore;
class BreakPoint
{
public:
    explicit BreakPoint(DebugCore* debugCore);
    BreakPoint(const BreakPoint&) = delete;
    BreakPoint(BreakPoint&&) = default;

    uint64_t address() const
    {
        return m_address;
    }
    uint8_t orgByte() const
    {
        return m_orgByte;
    }
    bool enabled() const
    {
        return m_enabled;
    }
    bool setEnabled(bool enabled);
    bool isHardware() const
    {
        return m_isHardware;
    }

private:
    uint64_t m_address;
    uint8_t m_orgByte;
    bool m_enabled = false;
    bool m_isHardware = false;

    DebugCore* m_debugCore;
    friend DebugCore;
};

