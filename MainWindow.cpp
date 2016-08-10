#include "MainWindow.h"
#include "Log.h"
#include "DisasmView.h"
#include "EventDispatcher.h"
#include "global.h"

#include <QtDockWidget.h>
#include <QtFlexWidget.h>
#include <QtFlexManager.h>

#include "DebugCore.h"


MainWidget::MainWidget(QWidget *parent)
		: QMainWindow(parent)
{
	setObjectName("MainWindow");
	qApp->setProperty("window", QVariant::fromValue<QObject*>(this));

	connect(FlexManager::instance(), &FlexManager::dockWidgetCreated, this, &MainWidget::onDockEidgetCreated);

	center = FlexManager::instance()->createFlexWidget(Flex::HybridView, this, Flex::widgetFlags(), "RootFlex");
	setCentralWidget(center);

	//创建菜单
	auto menu = new QMenu("文件",this);
	addAction("file.open", menu->addAction(QIcon(":/icon/Resources/open.png"), "打开", [this]
	{
		onFileOpen();
	}, QKeySequence::New));
	addAction("file.exit", menu->addAction(QIcon(":/icon/Resources/close.png"), "退出", []
	{

	}, QKeySequence::Close));
	menuBar()->addMenu(menu);

	menu = new QMenu("编辑",this);
	addAction("edit.copy", menu->addAction(QIcon(":/icon/Resources/copy.png"), "复制", []{}, QKeySequence::Copy));
	menuBar()->addMenu(menu);

	menu = new QMenu("视图",this);
	addAction("view.disasmView", menu->addAction(QIcon(":/icon/Resources/disasm_view.png"), "反汇编窗口", [this]
	{
		activeOrAddDockWidget(Flex::FileView,"反汇编",Flex::M,0,center);
	}, QKeySequence(Qt::ALT + Qt::Key_D)));
	addAction("view.memoryView", menu->addAction(QIcon(":/icon/Resources/memory_view.png"),"内存窗口", [this]
	{
		activeOrAddDockWidget(Flex::ToolView,"内存",Flex::B0,0,center);
	}, QKeySequence(Qt::ALT + Qt::Key_M)));
	addAction("view.registerView", menu->addAction(QIcon(":/icon/Resources/register_view.png"), "寄存器窗口", [this]
	{
		activeOrAddDockWidget(Flex::ToolView,"寄存器",Flex::B0,0,center);
	}, QKeySequence(Qt::ALT + Qt::Key_R)));
	addAction("view.callstackView", menu->addAction("调用堆栈窗口", [this]
	{
		activeOrAddDockWidget(Flex::ToolView,"调用堆栈",Flex::B0,0,center);
	}, QKeySequence(Qt::ALT + Qt::Key_C)));
	addAction("view.memoryMapView", menu->addAction("内存映射窗口窗口", [this]{activeOrAddDockWidget(Flex::ToolView,"内存映射",Flex::B0,0,center);}));
	addAction("view.watchView", menu->addAction("监视窗口", []{}));
	addAction("view.breakpointView", menu->addAction(QIcon(":/icon/Resources/breakpoint_enabled.png"), "断点窗口", []
	{

	}, QKeySequence(Qt::ALT + Qt::Key_B)));
	addAction("view.outputView", menu->addAction(QIcon(":/icon/Resources/log_view.png"), "输出窗口", [this]
	{
		activeOrAddDockWidget(Flex::ToolView,"输出",Flex::B0,0,center);
	}, Qt::ALT + Qt::Key_L));
	menuBar()->addMenu(menu);

	menu = new QMenu("调试",this);
	addAction("debug.run", menu->addAction(QIcon(":/icon/Resources/run.png"), "运行", [this]
	{
		if (m_debugCore)
		{
			m_debugCore->continueDebug();
		}
		else
		{
			QMessageBox::warning(this, "错误", "请先选择要调试的程序");
		}
	}, QKeySequence(Qt::Key_F9)));
	addAction("debug.stop", menu->addAction(QIcon(":/icon/Resources/stop.png"), "停止", [this]
	{
		if (m_debugCore)
			m_debugCore->stop();
	}, QKeySequence(Qt::Key_F8)));
	addAction("debug.restart", menu->addAction(QIcon(":/icon/Resources/restart.png"), "重新启动", []
	{

	}, QKeySequence(Qt::CTRL + Qt::Key_F2)));
	addAction("debug.addBreakpoint", menu->addAction(QIcon(":/icon/Resources/breakpoint_enabled.png"), "添加/删除断点", [this]
	{
		if (!m_debugCore)
		{
			QMessageBox::information(this, "提示", "请先加载要调试的目标程序");
			return;
		}

		auto bp = m_debugCore->findBreakpoint(g_highlightAddress);
		if (bp)
		{
			if (!m_debugCore->removeBreakpoint(g_highlightAddress))
			{
				QMessageBox::warning(this, "错误", QString("移除断点 0x%1 失败").arg(g_highlightAddress, 0, 16));
			}
		}
		else if (!m_debugCore->addBreakpoint(g_highlightAddress))
		{
			QMessageBox::warning(this, "错误", QString("在 0x%1 处添加断点失败").arg(g_highlightAddress, 0, 16));
		}
		emit EventDispatcher::instance()->refreshDisasmView();
	}, QKeySequence(Qt::Key_F2)));
	addAction("debug.stepOver", menu->addAction(QIcon(":/icon/Resources/step_over.png"), "单步步过", []
	{

	}, QKeySequence(Qt::Key_F8)));
	addAction("debug.stepIn", menu->addAction(QIcon(":/icon/Resources/step_into.png"), "单步步入", [this]
	{
		if (m_debugCore)
		{
		  m_debugCore->stepIn();
		}
		else
		{
		  QMessageBox::warning(this, "错误", "请先选择要调试的程序");
		}
	}, QKeySequence(Qt::Key_F7)));
	addAction("debug.runToReturn", menu->addAction(QIcon(":/icon/Resources/run_to_ret.png"), "运行到返回", []
	{

	}, QKeySequence(Qt::CTRL + Qt::Key_F7)));
	addAction("debug.runToCursor", menu->addAction(QIcon(":/icon/Resources/run_to_cursor.png"), "运行到光标处", []
	{

	}, QKeySequence(Qt::Key_F4)));
	menuBar()->addMenu(menu);

	menu = new QMenu("工具",this);
	addAction("tools.option", menu->addAction(QIcon(":/icon/Resources/option.png"), "选项", []{}, QKeySequence(Qt::ALT + Qt::Key_O)));
	menuBar()->addMenu(menu);

	menu = new QMenu("窗口",this);
	addAction("window.saveLayout", menu->addAction("保存布局", this, &MainWidget::saveLayout));
	addAction("window.loadLayout", menu->addAction("加载布局", this, &MainWidget::loadLayout));
	menuBar()->addMenu(menu);

	menu = new QMenu("帮助",this);
	addAction("help.about", menu->addAction("关于", []{}));
	menuBar()->addMenu(menu);

	//创建工具栏
	const QString qss =
		R"-(QToolBar {
			background: #CFD6E5;
			spacing: 3px;
			border:1px solid #CFD6E5;
		}

		QToolButton:hover {
			border: 2px solid #E5C365;
			background-color: #FDF4BF;
		}

		QToolButton {
			border: 2px solid #CFD6E5;
			background-color: #CFD6E5;
			height: 1em;
			width: 1em;
		})-";
	auto tb = addToolBar("文件");
	tb->setStyleSheet(qss);
	tb->addAction(getAction("file.open"));
	tb->addAction(getAction("file.exit"));

	tb = addToolBar("编辑");
	tb->setStyleSheet(qss);
	tb->addAction(getAction("edit.copy"));

	tb = addToolBar("视图");
	tb->setStyleSheet(qss);
	tb->addAction(getAction("view.disasmView"));

	tb = addToolBar("调试");
	tb->setStyleSheet(qss);
	tb->addAction(getAction("debug.run"));
	tb->addAction(getAction("debug.stepOver"));
	tb->addAction(getAction("debug.stepIn"));


	tb = addToolBar("工具");
	tb->setStyleSheet(qss);
	tb->addAction(getAction("tools.option"));


	//初始化model
	m_memoryMapModel = new QStandardItemModel(0, 3, this);
	m_memoryMapModel->setHorizontalHeaderLabels(QStringList()<<"起始地址"<<"大小"<<"权限");

	m_registerModel = new RegisterModel(this);

	m_logModel = new QStandardItemModel(this);
    m_logModel->setHorizontalHeaderLabels(QStringList() << "级别" << "信息");
    setLogFunc([this](QString const& msg, LogType type)
    {
        std::lock_guard<std::mutex> _(m_logMtx);
        m_logModel->appendRow(
                QList<QStandardItem*>()
                <<(new QStandardItem(logTypeToString(type)))
                <<(new QStandardItem(msg)));
    });
}

MainWidget::~MainWidget()
{
    setLogFunc(nullptr);
}

void MainWidget::addAction(const std::string& key, QAction* action)
{
	m_actions.emplace(key, action);
}

QAction *MainWidget::getAction(const std::string& key)
{
	auto it = m_actions.find(key);
	return (it == m_actions.end()? nullptr: it->second);
}

bool MainWidget::removeAction(const std::string& key)
{
	auto it = m_actions.find(key);
	if (it == m_actions.end())
	{
		return false;
	}

	m_actions.erase(it);
	return true;
}

DockWidget *MainWidget::findDockWidget(const QString& name)
{
	return FlexManager::instance()->dockWidget(name);
}


DockWidget *MainWidget::addDockWidget(Flex::ViewMode mode,
									  const QString& name, Flex::DockArea area, int siteIndex,
									  FlexWidget* parent)
{
	DockWidget* dockWidget = FlexManager::instance()->createDockWidget(mode, parent, Flex::widgetFlags(), name);
	dockWidget->setViewMode(mode);
	dockWidget->setWindowTitle(name);

	if (parent)
	{
		parent->addDockWidget(dockWidget,area,siteIndex);
	}
	return dockWidget;
}

DockWidget *MainWidget::activeOrAddDockWidget(
		Flex::ViewMode mode, const QString& name,
		Flex::DockArea area, int siteIndex, FlexWidget* parent)
{
	auto widget = findDockWidget(name);
	if (widget)
	{
		widget->activate();
		return widget;
	}

	return addDockWidget(mode, name, area, siteIndex, parent);
}

void MainWidget::saveLayout()
{
	QSettings settings("MacBook","Saber");

	auto bytes = FlexManager::instance()->save();
	settings.setValue("Default",bytes);
}

void MainWidget::loadLayout()
{
	QSettings settings("MacBook","Saber");
	QByteArray content = settings.value("Default").toByteArray();
	if (content.isEmpty())
	{
		return;
	}

	QMap<QString, QWidget*> parents;
	parents[objectName()] = this;

	FlexManager::instance()->load(content,parents);
}

void MainWidget::onDockEidgetCreated(DockWidget *widget)
{
    auto const& title = widget->windowTitle();
	if (title == "内存映射")
	{
		auto table = new QTableView(widget);
		table->setModel(m_memoryMapModel);
		table->setSelectionBehavior(QAbstractItemView::SelectRows);
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		widget->attachWidget(table);
	}
	else if (title == "输出")
	{
        auto log_view = new QTableView(this);
        log_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        log_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        log_view->horizontalHeader()->setStretchLastSection(true);
        log_view->setModel(m_logModel);
        widget->attachWidget(log_view);
	}
	else if (title == "寄存器")
	{
		auto view = new QTreeView(widget);
		view->setModel(m_registerModel);
		widget->attachWidget(view);
	}
    else if (title == "反汇编")
    {
        auto view = new DisasmView(widget);
		QObject::connect(EventDispatcher::instance(), &EventDispatcher::setDebugCore, view, &DisasmView::setDebugCore);
		QObject::connect(EventDispatcher::instance(), &EventDispatcher::setDisasmAddress, view, &DisasmView::gotoAddress);
		QObject::connect(EventDispatcher::instance(), &EventDispatcher::refreshDisasmView, view, &DisasmView::onRefresh);
		view->setDebugCore(m_debugCore);
		view->gotoAddress(g_highlightAddress);
        widget->attachWidget(view);
    }
	else
	{
		widget->attachWidget(new QTextEdit(widget->windowTitle(), widget));
	}
}

void MainWidget::onFileOpen()
{
	//TODO:
	QString path,args;
	{
		QDialog dlg(this);
		dlg.setWindowTitle("打开文件");

		QGridLayout* glay = new QGridLayout;
		QLineEdit* pathEdit = new QLineEdit(&dlg);
		QPushButton* browseBtn = new QPushButton("...");
		glay->addWidget(new QLabel("程序：", &dlg), 0, 0);
		glay->addWidget(pathEdit, 0, 1);
		glay->addWidget(browseBtn, 0, 2);
		QLineEdit* argsEdit = new QLineEdit(&dlg);
		glay->addWidget(new QLabel("参数：", &dlg), 1, 0);
		glay->addWidget(argsEdit, 1, 1);
		QDialogButtonBox* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
		glay->addWidget(btnBox, 2, 0, -1, -1);
		dlg.setLayout(glay);

		connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
		connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
		connect(browseBtn, &QPushButton::clicked,[pathEdit,&dlg]
		{
			pathEdit->setText(QFileDialog::getOpenFileName(&dlg,"打开",QDir::currentPath()));
		});

		dlg.resize(500, 150);
		if (dlg.exec() != QDialog::Accepted)
		{
			return;
		}

		path = pathEdit->text();
		args = argsEdit->text();
	}

	m_debugCore = std::make_shared<DebugCore>();
    emit EventDispatcher::instance()->setDebugCore(m_debugCore);
	connect(EventDispatcher::instance(), &EventDispatcher::showMemoryMap, [this](std::vector<MemoryRegion> regions)
	{
		m_memoryMapModel->setRowCount(regions.size());
		for (unsigned i = 0; i < regions.size(); ++i)
		{
			m_memoryMapModel->setItem(i, 0, new QStandardItem(QString("%1").arg(regions[i].start, 0, 16)));
			m_memoryMapModel->setItem(i, 1, new QStandardItem(QString("%1").arg(regions[i].size, 0, 16)));
			m_memoryMapModel->setItem(i, 2, new QStandardItem(QString("%1").arg(regions[i].info.protection, 0, 16)));
		}
	});
	connect(EventDispatcher::instance(), &EventDispatcher::showRegisters, m_registerModel, &RegisterModel::setRegister);

	m_debugCore->debugNew(path, args);
}


