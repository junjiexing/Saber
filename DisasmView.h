#ifndef DISASMVIEW_H
#define DISASMVIEW_H

#include <QAbstractScrollArea>

#include <vector>

class DisasmView : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit DisasmView(QWidget *parent = 0);

signals:

public slots:
    void gotoAddress(uint64_t address);
    void setRegion(uint64_t address);
    void analysis();
    // QWidget interface
protected:
    void paintEvent(QPaintEvent *e) override;

private:
    uint64_t m_regionStart;
    uint64_t m_regionSize;

    std::vector<uint64_t> m_insnStart;

    uint64_t m_hilightLine;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // DISASMVIEW_H
