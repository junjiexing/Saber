//
// Created by System Administrator on 16/8/11.
//

#include "AttachProcessList.h"

#include <QtWidgets>

#include <vector>
#include <algorithm>

#include <sys/time.h>
#include <libproc.h>

AttachProcessList::AttachProcessList(QWidget *parent)
	:QDialog(parent)
{
	setWindowTitle("附加到进程");
	auto vlay = new QVBoxLayout(this);
	vlay->setMargin(0);
	table_ = new QTableWidget(0, 2, this);
	table_->setHorizontalHeaderLabels(QStringList() << "PID" << "路径");
	table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table_->setSelectionBehavior(QAbstractItemView::SelectRows);
	table_->horizontalHeader()->setStretchLastSection(true);
	vlay->addWidget(table_);

	auto btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	vlay->addWidget(btn_box);
	connect(btn_box, &QDialogButtonBox::accepted, this, &AttachProcessList::accept);
	connect(btn_box, &QDialogButtonBox::rejected, this, &AttachProcessList::reject);

	refresh();
}
void AttachProcessList::refresh()
{
	int num = proc_listallpids(nullptr, 0);
	if (num < 0)
	{
		QMessageBox::warning(this, "错误", "获取进程数量失败");
		return;
	}
	std::vector<pid_t> pids(num);
	if (proc_listallpids(pids.data(), pids.size()) < 0)
	{
		QMessageBox::warning(this, "错误", "获取pid列表失败");
		return;
	}

	pids.erase(std::remove(pids.begin(), pids.end(), 0), pids.end());	//XXX: 为什么会有很多pid为0?

	table_->setRowCount(pids.size());
	for (std::size_t i = 0; i < pids.size(); ++i)
	{
		table_->setItem(i, 0, new QTableWidgetItem(QString::number(pids[i])));
		char path[1024];
		proc_pidpath(pids[i], path, 1024);
		table_->setItem(i, 1, new QTableWidgetItem(path));
	}
}
