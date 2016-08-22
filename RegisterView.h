//
// Created by System Administrator on 16/8/22.
//

#pragma once
#include <memory>
#include <QTreeWidget>

class DebugCore;

class RegisterView : public QTreeWidget
{
	Q_OBJECT
public:
	RegisterView(QWidget* parent);

public slots:
	void setDebugCore(std::shared_ptr<DebugCore> debugCore);
	void updateContent();

private:
	std::weak_ptr<DebugCore> m_debugCore;

	QTreeWidgetItem* m_rax;
	QTreeWidgetItem* m_rbx;
	QTreeWidgetItem* m_rcx;
	QTreeWidgetItem* m_rdx;
	QTreeWidgetItem* m_rdi;
	QTreeWidgetItem* m_rsi;
	QTreeWidgetItem* m_rbp;
	QTreeWidgetItem* m_rsp;
	QTreeWidgetItem* m_r8;
	QTreeWidgetItem* m_r9;
	QTreeWidgetItem* m_r10;
	QTreeWidgetItem* m_r11;
	QTreeWidgetItem* m_r12;
	QTreeWidgetItem* m_r13;
	QTreeWidgetItem* m_r14;
	QTreeWidgetItem* m_r15;

	QTreeWidgetItem* m_rip;

	QTreeWidgetItem* m_rflags;

	QTreeWidgetItem* m_cs;
	QTreeWidgetItem* m_fs;
	QTreeWidgetItem* m_gs;
};
