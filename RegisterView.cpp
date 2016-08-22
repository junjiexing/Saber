//
// Created by System Administrator on 16/8/22.
//

#include "RegisterView.h"
#include "EventDispatcher.h"
#include "DebugCore.h"

RegisterView::RegisterView(QWidget *parent)
	: QTreeWidget(parent)
{
	setHeaderLabels(QStringList() << "寄存器" << "值");
	setSizeAdjustPolicy(QTreeWidget::AdjustToContents);

	auto regGroup = new QTreeWidgetItem(this, QStringList() << "通用寄存器");
	m_rax = new QTreeWidgetItem(regGroup, QStringList() << "RAX");
	m_rbx = new QTreeWidgetItem(regGroup, QStringList() << "RBX");
	m_rcx = new QTreeWidgetItem(regGroup, QStringList() << "RCX");
	m_rdx = new QTreeWidgetItem(regGroup, QStringList() << "RDX");
	m_rdi = new QTreeWidgetItem(regGroup, QStringList() << "RDI");
	m_rsi = new QTreeWidgetItem(regGroup, QStringList() << "RSI");
	m_rbp = new QTreeWidgetItem(regGroup, QStringList() << "RBP");
	m_rsp = new QTreeWidgetItem(regGroup, QStringList() << "RSP");
	m_r8 = new QTreeWidgetItem(regGroup, QStringList() << "R8");
	m_r9 = new QTreeWidgetItem(regGroup, QStringList() << "R9");
	m_r10 = new QTreeWidgetItem(regGroup, QStringList() << "R10");
	m_r11 = new QTreeWidgetItem(regGroup, QStringList() << "R11");
	m_r12 = new QTreeWidgetItem(regGroup, QStringList() << "R12");
	m_r13 = new QTreeWidgetItem(regGroup, QStringList() << "R13");
	m_r14 = new QTreeWidgetItem(regGroup, QStringList() << "R14");
	m_r15 = new QTreeWidgetItem(regGroup, QStringList() << "R15");

	m_rip = new QTreeWidgetItem(this, QStringList() << "RIP");

	m_rflags = new QTreeWidgetItem(this);
	m_rflags->setText(0, "RFLAGS");

	regGroup = new QTreeWidgetItem(this, QStringList() << "段寄存器");
	m_cs = new QTreeWidgetItem(regGroup, QStringList() << "CS");
	m_fs = new QTreeWidgetItem(regGroup, QStringList() << "FS");
	m_gs = new QTreeWidgetItem(regGroup, QStringList() << "GS");
	connect(EventDispatcher::instance(), &EventDispatcher::setDebugCore, this, &RegisterView::setDebugCore);
	connect(EventDispatcher::instance(), &EventDispatcher::debugEvent, this, &RegisterView::updateContent);
}

void RegisterView::setDebugCore(std::shared_ptr<DebugCore> debugCore)
{
	m_debugCore = debugCore;
}
void RegisterView::updateContent()
{
	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		return;
	}

	auto reg = debugCore->getAllRegisterState(debugCore->excInfo().threadPort);
	m_rax->setText(1, QString::number(reg.threadState.__rax, 16));
	m_rbx->setText(1, QString::number(reg.threadState.__rbx, 16));
	m_rcx->setText(1, QString::number(reg.threadState.__rcx, 16));
	m_rdx->setText(1, QString::number(reg.threadState.__rdx, 16));
	m_rdi->setText(1, QString::number(reg.threadState.__rdi, 16));
	m_rsi->setText(1, QString::number(reg.threadState.__rsi, 16));
	m_rbp->setText(1, QString::number(reg.threadState.__rbp, 16));
	m_rsp->setText(1, QString::number(reg.threadState.__rsp, 16));
	m_r8->setText(1, QString::number(reg.threadState.__r8, 16));
	m_r9->setText(1, QString::number(reg.threadState.__r9, 16));
	m_r10->setText(1, QString::number(reg.threadState.__r10, 16));
	m_r11->setText(1, QString::number(reg.threadState.__r11, 16));
	m_r12->setText(1, QString::number(reg.threadState.__r12, 16));
	m_r13->setText(1, QString::number(reg.threadState.__r13, 16));
	m_r14->setText(1, QString::number(reg.threadState.__r14, 16));
	m_r15->setText(1, QString::number(reg.threadState.__r15, 16));

	m_rip->setText(1, QString::number(reg.threadState.__rip, 16));

	m_rflags->setText(1, QString::number(reg.threadState.__rflags, 16));

	m_cs->setText(1, QString::number(reg.threadState.__cs, 16));
	m_fs->setText(1, QString::number(reg.threadState.__fs, 16));
	m_gs->setText(1, QString::number(reg.threadState.__gs, 16));
}
