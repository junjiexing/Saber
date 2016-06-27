#include "MainWidget.h"
#include <QtWidgets>

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
    addAction("file.open", menu->addAction("打开", this, [this]{onFileOpen();}));
    addAction("file.close", menu->addAction("关闭", this, []{}));
    addAction("file.exit", menu->addAction("退出", this, []{}));
    menuBar()->addMenu(menu);

    menu = new QMenu("编辑",this);
    addAction("edit.copy", menu->addAction("复制", this, []{}));
    menuBar()->addMenu(menu);

    menu = new QMenu("视图",this);
    addAction("view.disasmView", menu->addAction("反汇编窗口", this, [this]
    {
        static int index = 0;
        addDockWidget(Flex::FileView,QString("反汇编-%1").arg(++index),Flex::M,0,center);
    }));
    addAction("view.memoryView", menu->addAction("内存窗口", this, [this]{activeOrAddDockWidget(Flex::ToolView,"内存",Flex::B0,0,center);}));
    addAction("view.registerView", menu->addAction("寄存器编窗口", this, [this]{activeOrAddDockWidget(Flex::ToolView,"寄存器",Flex::B0,0,center);}));
    addAction("view.callstackView", menu->addAction("调用堆栈窗口", this, [this]{activeOrAddDockWidget(Flex::ToolView,"调用堆栈",Flex::B0,0,center);}));
    addAction("view.memoryMapView", menu->addAction("内存映射窗口窗口", this, [this]{activeOrAddDockWidget(Flex::ToolView,"内存映射",Flex::B0,0,center);}));
    addAction("view.watchView", menu->addAction("监视窗口", this, []{}));
    addAction("view.breakpointView", menu->addAction("断点窗口", this, []{}));
    addAction("view.outputView", menu->addAction("输出窗口", this, [this]{activeOrAddDockWidget(Flex::ToolView,"输出",Flex::B0,0,center);}));
    menuBar()->addMenu(menu);

    menu = new QMenu("调试",this);
    addAction("debug.run", menu->addAction("运行", this, []{}));
    addAction("debug.stepOver", menu->addAction("单步步过", this, []{}));
    addAction("debug.stepIn", menu->addAction("单步步入", this, []{}));
    menuBar()->addMenu(menu);

    menu = new QMenu("工具",this);
    addAction("tools.option", menu->addAction("选项", this, []{}));
    menuBar()->addMenu(menu);

    menu = new QMenu("窗口",this);
    addAction("window.saveLayout", menu->addAction("保存布局", this, &MainWidget::saveLayout));
    addAction("window.loadLayout", menu->addAction("加载布局", this, &MainWidget::loadLayout));
    menuBar()->addMenu(menu);

    menu = new QMenu("帮助",this);
    addAction("help.about", menu->addAction("关于", this, []{}));
    menuBar()->addMenu(menu);

    //创建工具栏
    const QString qss =
R"-(QToolBar {
    background: #CFD6E5;
    spacing: 3px;
    border:1px solid #CFD6E5;
}

QToolButton:hover  {
    border: 2px solid #E5C365;
    background-color: #FDF4BF;
}

QToolButton  {
    border: 2px solid #CFD6E5;
    background-color: #CFD6E5;
})-";
    auto tb = addToolBar("文件");
    tb->setStyleSheet(qss);
    tb->addAction(getAction("file.open"));
    tb->addAction(getAction("file.close"));

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

    tb = addToolBar("帮助");
    tb->setStyleSheet(qss);
    tb->addAction(getAction("help.about"));


    //初始化model
    m_memoryMapModel = new QStandardItemModel(0, 3, this);
    QStringList header;
    header<<"起始地址"<<"大小"<<"权限";
    m_memoryMapModel->setHorizontalHeaderLabels(header);

    m_outputEdit = new QTextEdit;
    m_outputEdit->hide();
//    m_outputEdit->setReadOnly(true);
    m_outputEdit->append("Hello");
}

MainWidget::~MainWidget()
{

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

void MainWidget::onOutputMessage(const QString &msg, MessageType type)
{
    QString str;
    switch (type)
    {
    case MessageType::Info:
        str = "Info: ";
        break;
    case MessageType::Warning:
        str = "Warn: ";
        break;
    case MessageType::Error:
        str = "Erro: ";
        break;
    }

    str += msg;

    m_outputEdit->append(str);
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
    if (widget->windowTitle() == "内存映射")
    {
        auto table = new QTableView(widget);
        table->setModel(m_memoryMapModel);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        widget->attachWidget(table);
    }
    else if (widget->windowTitle() == "输出")
    {
        m_outputEdit->show();
        widget->attachWidget(m_outputEdit);
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

    m_debugCore = new DebugCore;
    connect(m_debugCore, &DebugCore::memoryMapRefreshed,[this](std::vector<MemoryRegion>& regions)
    {
        m_memoryMapModel->setRowCount(regions.size());
        for (unsigned i = 0; i < regions.size(); ++i)
        {
            m_memoryMapModel->setItem(i, 0, new QStandardItem(QString("%1").arg(regions[i].start, 0, 16)));
            m_memoryMapModel->setItem(i, 1, new QStandardItem(QString("%1").arg(regions[i].size, 0, 16)));
            m_memoryMapModel->setItem(i, 2, new QStandardItem(QString("%1").arg(regions[i].info.protection, 0, 16)));
        }
    });
    connect(m_debugCore, SIGNAL(outputMessage(QString,MessageType)), this,
            SLOT(onOutputMessage(QString,MessageType)),Qt::BlockingQueuedConnection);
    m_debugCore->refreshMemoryMap();
    m_debugCore->debugNew(path, args);
}
