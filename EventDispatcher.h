//
// Created by System Administrator on 16/7/25.
//

#include "Common.h"

#include <QObject>

#include <vector>
#include <memory>

class DebugCore;

class EventDispatcher : public QObject
{
    Q_OBJECT
public:
    static EventDispatcher* instance();
	static void registerMetaType();

signals:
	void setDebugCore(std::shared_ptr<DebugCore> debugCore);
    void setDisasmAddress(uint64_t addr);
	void showMemoryMap(std::vector<MemoryRegion> regions);
	void showRegisters(Register regs);
	void refreshDisasmView();
};

