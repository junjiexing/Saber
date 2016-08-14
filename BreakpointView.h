//
// Created by System Administrator on 16/8/14.
//

#pragma once

#include <QTableWidget>

class DebugCore;
class QMenu;

class BreakpointView : public QTableWidget
{
	Q_OBJECT
public:
	BreakpointView(QWidget* parent);

	void setDebugCore(std::shared_ptr<DebugCore> debugCore);
public slots:
	void refreshBpList();

protected:
	void contextMenuEvent(QContextMenuEvent *event);
private:
	std::weak_ptr<DebugCore> m_debugCore;

	QMenu* m_menu;

	uint64_t getSel();
};

