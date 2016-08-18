//
// Created by System Administrator on 16/8/18.
//

#pragma once

#include "Common.h"

#include <QTableView>
#include <QStandardItemModel>

class OutputModel : public QStandardItemModel
{
	Q_OBJECT
public:
	OutputModel(QObject* parent);
public slots:
	void addLog(QString msg, LogType t);
};

class OutputView : public QTableView
{
	Q_OBJECT
public:
	OutputView(QWidget* parent);

private:

};


