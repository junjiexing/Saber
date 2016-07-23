#ifndef REGISTERMODEL_H
#define REGISTERMODEL_H

#include <QtWidgets>
#include "Common.h"

class RegisterModel : public QStandardItemModel
{
public:
    RegisterModel(QWidget* parent);

public slots:
    void setRax(uint64_t val);
    void setRbx(uint64_t val);
    void setRcx(uint64_t val);
    void setRdx(uint64_t val);
    void setRdi(uint64_t val);
    void setRsi(uint64_t val);
    void setRbp(uint64_t val);
    void setRsp(uint64_t val);
    void setR8(uint64_t val);
    void setR9(uint64_t val);
    void setR10(uint64_t val);
    void setR11(uint64_t val);
    void setR12(uint64_t val);
    void setR13(uint64_t val);
    void setR14(uint64_t val);
    void setR15(uint64_t val);
    void setRip(uint64_t val);
    void setRflags(uint64_t val);
    void setCs(uint64_t val);
    void setFs(uint64_t val);
    void setGs(uint64_t val);

    void setRegister(const Register regs);
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
