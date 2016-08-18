//
// Created by System Administrator on 16/8/17.
//

#pragma once

#include <QTableWidget>

class DebugCore;

class MemoryMapView : public QTableWidget
{
	Q_OBJECT
public:
	MemoryMapView(QWidget* parent);

public slots:
	void setDebugCore(std::shared_ptr<DebugCore> debugCore);
	void updateContent();

private:
	std::weak_ptr<DebugCore> m_debugCore;
};


