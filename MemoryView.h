//
// Created by System Administrator on 16/8/22.
//

#pragma once

#include <QAbstractScrollArea>

class DebugCore;
class QMenu;

class MemoryView : public QAbstractScrollArea
{
	Q_OBJECT
public:
	MemoryView(QWidget* parent);
	virtual ~MemoryView();

public slots:
	virtual void updateContent();
	void setDebugCore(std::shared_ptr<DebugCore> debugCore);
	void setAddress(uint64_t address);

	void setQwordMode(bool on);
	void reCalcLayout();
protected:
	virtual void paintEvent(QPaintEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void contextMenuEvent(QContextMenuEvent *event) override;
	std::weak_ptr<DebugCore> m_debugCore;
private:

	uint64_t m_regionStart = 0;
	uint64_t m_regionSize = 0;

	uint64_t m_hilightStart = 0;
	uint64_t m_currentAddress = 0;

	int m_fontWidth;
	int m_fontHeight;

	bool m_qwordModel;

	QMenu* m_ctxMenu;
};

class StackView : public MemoryView
{
	Q_OBJECT
public:
	StackView(QWidget* parent);
	virtual void updateContent() override;
};
