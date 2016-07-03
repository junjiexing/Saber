#include "RegisterModel.h"

RegisterModel::RegisterModel(QWidget *parent)
    :QStandardItemModel(1, 2, parent)
{
    setHorizontalHeaderLabels(QStringList()<<"寄存器"<<"值");
    auto regGroup = new QStandardItem("通用寄存器");
    setItem(0, 0, regGroup);
    regGroup->setChild(0, 0, new QStandardItem("RAX"));
    m_rax = new QStandardItem("0");
    regGroup->setChild(0, 1, m_rax);

    regGroup->setChild(1, 0, new QStandardItem("RBX"));
    m_rbx = new QStandardItem("0");
    regGroup->setChild(1, 1, m_rbx);

    regGroup->setChild(2, 0, new QStandardItem("RCX"));
    m_rcx = new QStandardItem("0");
    regGroup->setChild(2, 1, m_rcx);

    regGroup->setChild(3, 0, new QStandardItem("RDX"));
    m_rdx = new QStandardItem("0");
    regGroup->setChild(3, 1, m_rdx);

    regGroup->setChild(4, 0, new QStandardItem("RDI"));
    m_rdi = new QStandardItem("0");
    regGroup->setChild(4, 1, m_rdi);

    regGroup->setChild(5, 0, new QStandardItem("RSI"));
    m_rsi = new QStandardItem("0");
    regGroup->setChild(5, 1, m_rsi);

    regGroup->setChild(6, 0, new QStandardItem("RBP"));
    m_rbp = new QStandardItem("0");
    regGroup->setChild(6, 1, m_rbp);

    regGroup->setChild(7, 0, new QStandardItem("RSP"));
    m_rsp = new QStandardItem("0");
    regGroup->setChild(7, 1, m_rsp);

    regGroup->setChild(8, 0, new QStandardItem("R8"));
    m_r8 = new QStandardItem("0");
    regGroup->setChild(8, 1, m_r8);

    regGroup->setChild(9, 0, new QStandardItem("R9"));
    m_r9 = new QStandardItem("0");
    regGroup->setChild(9, 1, m_r9);

    regGroup->setChild(10, 0, new QStandardItem("R10"));
    m_r10 = new QStandardItem("0");
    regGroup->setChild(10, 1, m_r10);

    regGroup->setChild(11, 0, new QStandardItem("R11"));
    m_r11 = new QStandardItem("0");
    regGroup->setChild(11, 1, m_r11);

    regGroup->setChild(12, 0, new QStandardItem("R12"));
    m_r12 = new QStandardItem("0");
    regGroup->setChild(12, 1, m_r12);

    regGroup->setChild(13, 0, new QStandardItem("R13"));
    m_r13 = new QStandardItem("0");
    regGroup->setChild(13, 1, m_r13);

    regGroup->setChild(14, 0, new QStandardItem("R14"));
    m_r14 = new QStandardItem("0");
    regGroup->setChild(14, 1, m_r14);

    regGroup->setChild(15, 0, new QStandardItem("R15"));
    m_r15 = new QStandardItem("0");
    regGroup->setChild(15, 1, m_r15);


    setItem(1, 0, new QStandardItem("RIP"));
    m_rip = new QStandardItem("0");
    setItem(1, 1, m_rip);


    setItem(2, 0, new QStandardItem("RFLAGS"));
    m_rflags = new QStandardItem("0");
    setItem(2, 1, m_rflags);

    regGroup = new QStandardItem("段寄存器");
    setItem(3, 0, regGroup);

    regGroup->setChild(0, 0, new QStandardItem("CS"));
    m_cs = new QStandardItem("0");
    regGroup->setChild(0, 1, m_cs);
    regGroup->setChild(1, 0, new QStandardItem("FS"));
    m_fs = new QStandardItem("0");
    regGroup->setChild(1, 1, m_fs);
    regGroup->setChild(2, 0, new QStandardItem("GS"));
    m_gs = new QStandardItem("0");
    regGroup->setChild(2, 1, m_gs);

}

void RegisterModel::setRax(uint64_t val)
{
    m_rax->setText(QString::number(val, 16));
}

void RegisterModel::setRbx(uint64_t val)
{
    m_rbx->setText(QString::number(val, 16));
}

void RegisterModel::setRcx(uint64_t val)
{
    m_rcx->setText(QString::number(val, 16));
}

void RegisterModel::setRdx(uint64_t val)
{
    m_rdx->setText(QString::number(val, 16));
}

void RegisterModel::setRdi(uint64_t val)
{
    m_rdi->setText(QString::number(val, 16));
}

void RegisterModel::setRsi(uint64_t val)
{
    m_rsi->setText(QString::number(val, 16));
}

void RegisterModel::setRbp(uint64_t val)
{
    m_rbp->setText(QString::number(val, 16));
}

void RegisterModel::setRsp(uint64_t val)
{
    m_rsp->setText(QString::number(val, 16));
}

void RegisterModel::setR8(uint64_t val)
{
    m_r8->setText(QString::number(val, 16));
}

void RegisterModel::setR9(uint64_t val)
{
    m_r9->setText(QString::number(val, 16));
}

void RegisterModel::setR10(uint64_t val)
{
    m_r10->setText(QString::number(val, 16));
}

void RegisterModel::setR11(uint64_t val)
{
    m_r11->setText(QString::number(val, 16));
}

void RegisterModel::setR12(uint64_t val)
{
    m_r12->setText(QString::number(val, 16));
}

void RegisterModel::setR13(uint64_t val)
{
    m_r13->setText(QString::number(val, 16));
}

void RegisterModel::setR14(uint64_t val)
{
    m_r14->setText(QString::number(val, 16));
}

void RegisterModel::setR15(uint64_t val)
{
    m_r15->setText(QString::number(val, 16));
}

void RegisterModel::setRip(uint64_t val)
{
    m_rip->setText(QString::number(val, 16));
}

void RegisterModel::setRflags(uint64_t val)
{
    m_rflags->setText(QString::number(val, 16));
}

void RegisterModel::setCs(uint64_t val)
{
    m_cs->setText(QString::number(val, 16));
}

void RegisterModel::setFs(uint64_t val)
{
    m_fs->setText(QString::number(val, 16));
}

void RegisterModel::setGs(uint64_t val)
{
    m_gs->setText(QString::number(val, 16));
}

void RegisterModel::setRegister(const Register regs)
{
    m_rax->setText(QString::number(regs.rax, 16));
    m_rbx->setText(QString::number(regs.rbx, 16));
    m_rcx->setText(QString::number(regs.rcx, 16));
    m_rdx->setText(QString::number(regs.rdx, 16));
    m_rdi->setText(QString::number(regs.rdi, 16));
    m_rsi->setText(QString::number(regs.rsi, 16));
    m_rbp->setText(QString::number(regs.rbp, 16));
    m_rsp->setText(QString::number(regs.rsp, 16));
    m_r8->setText(QString::number(regs.r8, 16));
    m_r9->setText(QString::number(regs.r9, 16));
    m_r10->setText(QString::number(regs.r10, 16));
    m_r11->setText(QString::number(regs.r11, 16));
    m_r12->setText(QString::number(regs.r12, 16));
    m_r13->setText(QString::number(regs.r13, 16));
    m_r14->setText(QString::number(regs.r14, 16));
    m_r15->setText(QString::number(regs.r15, 16));

    m_rip->setText(QString::number(regs.rip, 16));

    m_rflags->setText(QString::number(regs.rflags, 16));

    m_cs->setText(QString::number(regs.cs, 16));
    m_fs->setText(QString::number(regs.fs, 16));
    m_gs->setText(QString::number(regs.gs, 16));
}
