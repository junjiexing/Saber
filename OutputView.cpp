//
// Created by System Administrator on 16/8/18.
//

#include "OutputView.h"
#include "EventDispatcher.h"

OutputModel::OutputModel(QObject *parent)
	: QStandardItemModel(parent)
{
	setHorizontalHeaderLabels(QStringList() << "级别" << "信息");
	QObject::connect(EventDispatcher::instance(), &EventDispatcher::addLog, this, &OutputModel::addLog);
}

void OutputModel::addLog(QString msg, LogType t)
{
	QString level;
	QColor color;
	switch (t)
	{
	case LogType::Info:
		level = "Info";
		color = Qt::white;
		break;
	case LogType::Warning:
		level = "Warning";
		color = Qt::yellow;
		break;
	case LogType::Error:
		level = "Error";
		color = Qt::red;
		break;
	}

	auto item1 = new QStandardItem(level);
	item1->setBackground(color);
	auto item2 = new QStandardItem(msg);
	item2->setBackground(color);

	appendRow(QList<QStandardItem*>() << item1 << item2);
}

OutputView::OutputView(QWidget *parent)
	: QTableView(parent)
{
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void OutputView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	scrollToBottom();
	QAbstractItemView::rowsInserted(parent, start, end);
}
