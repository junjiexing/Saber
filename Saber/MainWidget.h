
#pragma once

#include <QMainWindow>
#include <QtWidgets>

#include <string>
#include <map>
#include <memory>

#include <QtFlexManager.h>

class DebugCore;

class MainWidget : public QMainWindow
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = 0);
    ~MainWidget();

    void addAction(const std::string& key, QAction* action);
    QAction* getAction(const std::string& key);
    bool removeAction(const std::string& key);

    DockWidget* findDockWidget(const QString& name);
private:
    DockWidget* addDockWidget(Flex::ViewMode mode, const QString& name, Flex::DockArea area, int siteIndex, FlexWidget* parent);
    DockWidget* activeOrAddDockWidget(Flex::ViewMode mode, const QString& name, Flex::DockArea area, int siteIndex, FlexWidget* parent);
    void saveLayout();
    void loadLayout();

    void onDockEidgetCreated(DockWidget* widget);

    void onFileOpen();

private:
    std::map<std::string,QAction*> m_actions;

    FlexWidget* center;

    std::shared_ptr<DebugCore> m_debugCore;

    QStandardItemModel* m_memoryMapModel;
};

