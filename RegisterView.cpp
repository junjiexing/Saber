//
// Created by System Administrator on 16/8/22.
//

#include "RegisterView.h"
#include "EventDispatcher.h"
#include "DebugCore.h"
#include <QtWidgets>

class ValueDlg : public QDialog
{
public:
	ValueDlg(QString const& text, std::shared_ptr<DebugCore> debugCore, RegisterType type, QWidget* parent)
		:QDialog(parent)
	{
		auto vlay = new QVBoxLayout(this);
		auto edit = new QLineEdit(text, this);
		vlay->addWidget(edit);

		auto btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		vlay->addWidget(btn_box);
		connect(btn_box, &QDialogButtonBox::accepted, [=]
		{
			bool ok = false;
			uint64_t value = edit->text().toULongLong(&ok, 16);
			if (!ok)
			{
				QMessageBox::warning(this, "错误", "输入的值不是有效的64位十六进制数");
				return;
			}

			ok = debugCore->setRegisterState(debugCore->excInfo().threadPort, type, value);
			if (!ok)
			{
				QMessageBox::warning(this, "错误", "设置寄存器值失败");
				return;
			}

			accept();
		});
		connect(btn_box, &QDialogButtonBox::rejected, this, &ValueDlg::reject);
	}
};


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
void RegisterView::mouseDoubleClickEvent(QMouseEvent *event)
{
	QTreeView::mouseDoubleClickEvent(event);

	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		QMessageBox::information(this, "提示", "请先启动调试");
		return;
	}

	RegisterType type;
	QString text;
	if (m_rax->isSelected())
	{
		type = RegisterType::RAX;
		text = m_rax->text(1);
	}
	else if (m_rbx->isSelected())
	{
		type = RegisterType::RBX;
		text = m_rbx->text(1);
	}
	else if (m_rcx->isSelected())
	{
		type = RegisterType::RCX;
		text = m_rcx->text(1);
	}
	else if (m_rdx->isSelected())
	{
		type = RegisterType::RDX;
		text = m_rdx->text(1);
	}
	else if (m_rdi->isSelected())
	{
		type = RegisterType::RDI;
		text = m_rdi->text(1);
	}
	else if (m_rsi->isSelected())
	{
		type = RegisterType::RSI;
		text = m_rsi->text(1);
	}
	else if (m_rbp->isSelected())
	{
		type = RegisterType::RBP;
		text = m_rbp->text(1);
	}
	else if (m_rsp->isSelected())
	{
		type = RegisterType::RSP;
		text = m_rsp->text(1);
	}
	else if (m_r8->isSelected())
	{
		type = RegisterType::R8;
		text = m_r8->text(1);
	}
	else if (m_r9->isSelected())
	{
		type = RegisterType::R9;
		text = m_r9->text(1);
	}
	else if (m_r10->isSelected())
	{
		type = RegisterType::R10;
		text = m_r10->text(1);
	}
	else if (m_r11->isSelected())
	{
		type = RegisterType::R11;
		text = m_r11->text(1);
	}
	else if (m_r12->isSelected())
	{
		type = RegisterType::R12;
		text = m_r12->text(1);
	}
	else if (m_r13->isSelected())
	{
		type = RegisterType::R13;
		text = m_r13->text(1);
	}
	else if (m_r14->isSelected())
	{
		type = RegisterType::R14;
		text = m_r14->text(1);
	}
	else if (m_r15->isSelected())
	{
		type = RegisterType::R15;
		text = m_r15->text(1);
	}
	else if (m_rip->isSelected())
	{
		type = RegisterType::RIP;
		text = m_rip->text(1);
	}
	else if (m_rflags->isSelected())
	{
		type = RegisterType::RFLAGS;
		text = m_rflags->text(1);
	}
	else if (m_cs->isSelected())
	{
		type = RegisterType::CS;
		text = m_cs->text(1);
	}
	else if (m_fs->isSelected())
	{
		type = RegisterType::FS;
		text = m_fs->text(1);
	}
	else if (m_gs->isSelected())
	{
		type = RegisterType::GS;
		text = m_gs->text(1);
	}

	ValueDlg dlg(text, debugCore, type, this);
	if (dlg.exec() == QDialog::Accepted)
	{
		updateContent();
	}
}
