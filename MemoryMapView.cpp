//
// Created by System Administrator on 16/8/17.
//

#include "MemoryMapView.h"
#include "DebugCore.h"
#include "EventDispatcher.h"

MemoryMapView::MemoryMapView(QWidget *parent)
	: QTableWidget(0, 3, parent)
{
	setHorizontalHeaderLabels(QStringList()<<"起始地址"<<"大小"<<"权限");
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	connect(EventDispatcher::instance(), &EventDispatcher::debugEvent, this, &MemoryMapView::updateContent);
	connect(EventDispatcher::instance(), &EventDispatcher::setDebugCore, this, &MemoryMapView::setDebugCore);
}

void MemoryMapView::updateContent()
{
	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		return;
	}

	auto regions = debugCore->getMemoryMap();
	setRowCount(regions.size());
	for (unsigned i = 0; i < regions.size(); ++i)
	{
		setItem(i, 0, new QTableWidgetItem(QString("%1").arg(regions[i].start, 0, 16)));
		setItem(i, 1, new QTableWidgetItem(QString("%1").arg(regions[i].size, 0, 16)));
		setItem(i, 2, new QTableWidgetItem(QString("%1").arg(regions[i].info.protection, 0, 16)));
	}
}

void MemoryMapView::setDebugCore(std::shared_ptr<DebugCore> debugCore)
{
	m_debugCore = debugCore;
}
