//
// Created by System Administrator on 16/8/18.
//

#include "OutputView.h"
#include "EventDispatcher.h"

QString logTypeToString(LogType type)
{
	switch (type)
	{
	case LogType::Info:
		return "Info";
	case LogType::Warning:
		return "Warning";
	case LogType::Error:
		return "Error";
	}
}

OutputModel::OutputModel(QObject *parent)
	: QStandardItemModel(parent)
{
	setHorizontalHeaderLabels(QStringList() << "级别" << "信息");
	QObject::connect(EventDispatcher::instance(), &EventDispatcher::addLog, this, &OutputModel::addLog);
}

void OutputModel::addLog(QString msg, LogType t)
{
	appendRow(QList<QStandardItem*>()
				  <<(new QStandardItem(logTypeToString(t)))
				  <<(new QStandardItem(msg)));
}

OutputView::OutputView(QWidget *parent)
	: QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
}
