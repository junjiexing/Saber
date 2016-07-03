
#pragma once

#include <QMainWindow>
#include <QtWidgets>

#include <string>
#include <map>
#include <memory>

#include <QtFlexManager.h>

#include "Common.h"
#include "RegisterModel.h"

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

public slots:
    void onOutputMessage(const QString& msg, MessageType type);
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

    DebugCore* m_debugCore;

    QStandardItemModel* m_memoryMapModel;
    RegisterModel* m_registerModel;
    //TODO:实现一个text的model
    QTextEdit* m_outputEdit;
};

