
#pragma once

#include <QMainWindow>
#include <string>
#include <map>

#include <QtFlexManager.h>

class QAction;

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
    DockWidget* addDockWidget(Flex::ViewMode mode, const QString& name, Flex::DockArea area, int siteIndex, FlexWidget* parent, QWidget *content);
    DockWidget* activeOrAddDockWidget(Flex::ViewMode mode, const QString& name, Flex::DockArea area, int siteIndex, FlexWidget* parent, QWidget *content);
    void saveLayout();
    void loadLayout();


private:
    std::map<std::string,QAction*> m_actions;

    FlexWidget* center;
};

