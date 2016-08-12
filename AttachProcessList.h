//
// Created by System Administrator on 16/8/11.
//

#pragma once

#include <QDialog>

class QTableWidget;

class AttachProcessList : public QDialog
{
	Q_OBJECT
public:
	AttachProcessList(QWidget* parent);

	void refresh();
	int currentPid(){ return  m_currentPid; }
private:
	QTableWidget* table_;

	int m_currentPid = 0;
};


