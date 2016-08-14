//
// Created by System Administrator on 16/8/14.
//

#include "BreakpointView.h"
#include "DebugCore.h"
#include "EventDispatcher.h"

#include <QtWidgets>

class EditBreakpointDlg : public QDialog
{
public:
	EditBreakpointDlg(QWidget* parent)
		: QDialog(parent)
	{
		auto vlay = new QVBoxLayout(this);
		auto flay = new QFormLayout;
		vlay->addLayout(flay);
		m_address = new QLineEdit(this);
		m_address->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{1,16}"), m_address));
		flay->addRow("地址", m_address);
		m_enabled = new QCheckBox(this);
		m_enabled->setChecked(true);
		flay->addRow("激活", m_enabled);
		m_oneTime = new QCheckBox(this);
		flay->addRow("一次性", m_oneTime);

		auto btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		vlay->addWidget(btnBox);

		connect(btnBox, &QDialogButtonBox::accepted, this, &EditBreakpointDlg::accept);
		connect(btnBox, &QDialogButtonBox::rejected, this, &EditBreakpointDlg::reject);
	}

	uint64_t address(){ return m_address->text().toULongLong(nullptr, 16); }
	bool enabled(){ return m_enabled->isChecked(); }
	bool oneTime(){ return m_oneTime->isChecked(); }

private:
	QLineEdit* m_address;
	QCheckBox* m_enabled;
	QCheckBox* m_oneTime;
};

BreakpointView::BreakpointView(QWidget *parent)
	: QTableWidget(0, 3, parent)
{
	setHorizontalHeaderLabels(QStringList() << "地址" << "是否激活" << "一次性");
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setEditTriggers(QAbstractItemView::NoEditTriggers);

	m_menu = new QMenu(this);
	m_menu->addAction("刷新", [this] { refreshBpList(); });
	m_menu->addAction("启用/禁用断点", [this]
	{
		auto debugCore = m_debugCore.lock();
		if (!debugCore)
		{
			QMessageBox::information(this, "提示", "请先启动调试");
			return;
		}

		auto address = getSel();
		if (address == 0)
		{
			QMessageBox::information(this, "提示", "请先选择一个断点");
			return;
		}
		auto bp = debugCore->findBreakpoint(address);

		if (!bp->setEnabled(!bp->enabled()))
		{
			QMessageBox::warning(this, "错误", "设置失败");
		}
	});
	m_menu->addAction("删除断点", [this]
	{
		auto debugCore = m_debugCore.lock();
		if (!debugCore)
		{
			QMessageBox::information(this, "提示", "请先启动调试");
			return;
		}

		auto address = getSel();
		if (address == 0)
		{
			QMessageBox::information(this, "提示", "请先选择一个断点");
			return;
		}

		if (!debugCore->removeBreakpoint(address))
		{
			QMessageBox::warning(this, "错误", "删除断点失败");
		}
	});
	m_menu->addAction("在反汇编窗口显示", [this]
	{
		auto debugCore = m_debugCore.lock();
		if (!debugCore)
		{
			QMessageBox::information(this, "提示", "请先启动调试");
			return;
		}

		auto address = getSel();
		if (address == 0)
		{
			QMessageBox::information(this, "提示", "请先选择一个断点");
			return;
		}

		emit EventDispatcher::instance()->setDisasmAddress(address);
	});

	m_menu->addAction("新建断点", [this]
	{
		auto debugCore = m_debugCore.lock();
		if (!debugCore)
		{
			QMessageBox::information(this, "提示", "请先启动调试");
			return;
		}

		EditBreakpointDlg dlg(this);
		if (dlg.exec() != QDialog::Accepted)
		{
			return;
		}

		if (!debugCore->addBreakpoint(dlg.address(), dlg.enabled(), false, dlg.oneTime()))
		{
			QMessageBox::warning(this, "错误", "添加断点失败");
		}
	});
	//TODO: 添加断点编辑功能
}

void BreakpointView::refreshBpList()
{
	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		setRowCount(0);
		return;
	}

	auto const& breakpoints = debugCore->breakpoints();
	setRowCount(breakpoints.size());
	for (int i = 0; i < breakpoints.size(); ++i)
	{
		auto bp = breakpoints[i];
		setItem(i, 0, new QTableWidgetItem(QString::number(bp->address(), 16)));
		setItem(i, 1, new QTableWidgetItem(bp->enabled()? "是" :"否"));
		setItem(i, 2, new QTableWidgetItem(bp->isOneTime()? "是" : "否"));
	}
}

void BreakpointView::setDebugCore(std::shared_ptr<DebugCore> debugCore)
{
	m_debugCore = debugCore;
	refreshBpList();
}
void BreakpointView::contextMenuEvent(QContextMenuEvent *event)
{
	m_menu->exec(event->globalPos());
	QAbstractScrollArea::contextMenuEvent(event);
}
uint64_t BreakpointView::getSel()
{
	auto i = item(currentRow(), 0);
	if (!i)
	{
		return 0;
	}

	return i->text().toULongLong(nullptr, 16);
}
