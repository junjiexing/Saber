#ifndef REGISTERMODEL_H
#define REGISTERMODEL_H

#include <QtWidgets>
#include "Common.h"

class RegisterModel : public QStandardItemModel
{
public:
    RegisterModel(QWidget* parent);

public slots:
    void setRegister(const Register reg);
private:
    QStandardItem* m_rax;
    QStandardItem* m_rbx;
    QStandardItem* m_rcx;
    QStandardItem* m_rdx;
    QStandardItem* m_rdi;
    QStandardItem* m_rsi;
    QStandardItem* m_rbp;
    QStandardItem* m_rsp;
    QStandardItem* m_r8;
    QStandardItem* m_r9;
    QStandardItem* m_r10;
    QStandardItem* m_r11;
    QStandardItem* m_r12;
    QStandardItem* m_r13;
    QStandardItem* m_r14;
    QStandardItem* m_r15;

    QStandardItem* m_rip;

    QStandardItem* m_rflags;

    QStandardItem* m_cs;
    QStandardItem* m_fs;
    QStandardItem* m_gs;





};

#endif // REGISTERMODEL_H
