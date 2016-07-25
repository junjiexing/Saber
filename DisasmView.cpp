#include "DisasmView.h"
#include "libasmx64.h"
#include "DebugCore.h"
#include "Log.h"
#include "EventDispatcher.h"

#include <QtWidgets>

DisasmView::DisasmView(QWidget *parent)
    : QAbstractScrollArea(parent),
      m_regionStart(0),m_regionSize(0),
      m_hilightLine(0)
{
    EventDispatcher::instance()->registerReceiver(SetDisasmAddressEvent::eventType, this);
    EventDispatcher::instance()->registerReceiver(SetDebugCoreEvent::eventType, this);
}

void DisasmView::gotoAddress(uint64_t address)
{
    log(QString("In DisasmView::gotoAddress: %1").arg(address,0,16));
    if (!(address >= m_regionStart && address <= (m_regionStart + m_regionSize)))
    {
        setRegion(address);
    }

    bool found = false;
    std::size_t i = 0;
    log(QString("In DisasmView::gotoAddress, m_insnStart.size = %1").arg(m_insnStart.size()));
    for (; i < m_insnStart.size(); ++i)
    {
        if (m_insnStart[i] == address)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        //FIXME:单独处理
        //到了某个指令中间的地方..
        log("到了某个指令中间的地方");
        return;
    }

    verticalScrollBar()->setValue(i);
}

void DisasmView::setRegion(uint64_t address)
{
    if (!m_debugCore)
    {
        log("setRegion without debugcore ptr", LogType::Warning);
        return;
    }
    if (!m_debugCore->findRegion(address, m_regionStart, m_regionSize))
    {
        log("DisasmView::setRegion findRegion failed", LogType::Error);
        return;
    }
    analysis();
}

void DisasmView::analysis()
{
    log(QString("in analysis: %1, %2").arg(m_regionStart,0,16).arg(m_regionSize,0,16));
    std::vector<uint8_t> buf(m_regionSize);
    m_debugCore->readMemory(m_regionStart, buf.data(), buf.size());

    uint64_t addr = m_regionStart;
    x64dis decoder;
    for (int i = 0;i < buf.size();)
    {
        x86dis_insn* insn = decoder.decode(buf.data() + i, buf.size() - i, addr);
        //const char* pcsIns = decoder.str(insn, DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD | DIS_STYLE_SIGNED | X86DIS_STYLE_EXPLICIT_MEMSIZE);
        //printf("0x%016" PRIX64 "\t%s\n", addr, pcsIns);
        m_insnStart.emplace_back(addr);
        addr += insn->size;
        i += insn->size;
    }

    verticalScrollBar()->setMaximum(static_cast<int>(m_insnStart.size() - 1));
}

void DisasmView::paintEvent(QPaintEvent * e)
{
    if (m_insnStart.size() <= verticalScrollBar()->value())
    {
        return;
    }
    QPainter p(viewport());

    x64dis decoder;
    uint64_t addr = m_insnStart[verticalScrollBar()->value()];

    int h = viewport()->fontMetrics().height();
    for (int i = 0; i < viewport()->height(); i += h)
    {
        if ((m_regionStart + m_regionSize) <= addr)
        {
            break;
        }
        int size = std::min(15ull, m_regionStart + m_regionSize - addr);
        uint8_t buff[15];
        m_debugCore->readMemory(addr, buff, size);
        x86dis_insn* insn = decoder.decode(buff, size, addr);
        const char* insnStr = decoder.str(insn, DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD | DIS_STYLE_SIGNED | X86DIS_STYLE_EXPLICIT_MEMSIZE);
        //printf("0x%016" PRIX64 "\t%s\n", addr, pcsIns);

        QRect rc(0, i, viewport()->width(), h);
        if (addr == m_hilightLine)
        {
            p.fillRect(rc, Qt::lightGray);
        }
        p.drawText(rc, 0, QString::number(addr, 16).append("\t\t").append(insnStr));
        addr += insn->size;
    }

    QAbstractScrollArea::paintEvent(e);
}


void DisasmView::mousePressEvent(QMouseEvent *event)
{
    if (m_insnStart.size() <= verticalScrollBar()->value())
    {
        return;
    }

    x64dis decoder;
    uint64_t addr = m_insnStart[verticalScrollBar()->value()];
    auto y = event->pos().y();

    int h = viewport()->fontMetrics().height();
    for (int i = 0; i < viewport()->height(); i += h)
    {
        if ((m_regionStart + m_regionSize) <= addr)
        {
            break;
        }
        int size = std::min(15ull, m_regionStart + m_regionSize - addr);
        uint8_t buff[15];
        m_debugCore->readMemory(addr, buff, size);
        x86dis_insn* insn = decoder.decode(buff, size, addr);
        if (y >= i && y < i + h)
        {
            m_hilightLine = addr;
            break;
        }

        addr += insn->size;
    }
    viewport()->update();
    QAbstractScrollArea::mousePressEvent(event);
}

void DisasmView::setDebugCore(DebugCore *debugCore)
{
    m_debugCore = debugCore;
}

DisasmView::~DisasmView()
{
    EventDispatcher::instance()->removeReceiver(SetDisasmAddressEvent::eventType, this);
    EventDispatcher::instance()->removeReceiver(SetDebugCoreEvent::eventType, this);
}

bool DisasmView::event(QEvent *event)
{
    if (event->type() == SetDisasmAddressEvent::eventType)
    {
        log("On event SetDisasmAddress");
        gotoAddress(static_cast<SetDisasmAddressEvent*>(event)->address);
        return true;
    }
    else if (event->type() == SetDebugCoreEvent::eventType)
    {
        log("On event SetDebugCoreEvent");
        setDebugCore(static_cast<SetDebugCoreEvent*>(event)->debugCore);
        return true;
    }
    return QAbstractScrollArea::event(event);
}
