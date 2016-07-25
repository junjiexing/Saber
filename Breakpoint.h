
#pragma once

#include <cstdint>

class DebugCore;

class Breakpoint
{
public:
    explicit Breakpoint(DebugCore* debugCore);
    Breakpoint(const Breakpoint&) = delete;
    Breakpoint(Breakpoint&&) = default;

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

