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

void RegisterModel::setRegister(const Register reg)
{
    m_rax->setText(QString::number(reg.threadState.__rax, 16));
    m_rbx->setText(QString::number(reg.threadState.__rbx, 16));
    m_rcx->setText(QString::number(reg.threadState.__rcx, 16));
    m_rdx->setText(QString::number(reg.threadState.__rdx, 16));
    m_rdi->setText(QString::number(reg.threadState.__rdi, 16));
    m_rsi->setText(QString::number(reg.threadState.__rsi, 16));
    m_rbp->setText(QString::number(reg.threadState.__rbp, 16));
    m_rsp->setText(QString::number(reg.threadState.__rsp, 16));
    m_r8->setText(QString::number(reg.threadState.__r8, 16));
    m_r9->setText(QString::number(reg.threadState.__r9, 16));
    m_r10->setText(QString::number(reg.threadState.__r10, 16));
    m_r11->setText(QString::number(reg.threadState.__r11, 16));
    m_r12->setText(QString::number(reg.threadState.__r12, 16));
    m_r13->setText(QString::number(reg.threadState.__r13, 16));
    m_r14->setText(QString::number(reg.threadState.__r14, 16));
    m_r15->setText(QString::number(reg.threadState.__r15, 16));

    m_rip->setText(QString::number(reg.threadState.__rip, 16));

    m_rflags->setText(QString::number(reg.threadState.__rflags, 16));

    m_cs->setText(QString::number(reg.threadState.__cs, 16));
    m_fs->setText(QString::number(reg.threadState.__fs, 16));
    m_gs->setText(QString::number(reg.threadState.__gs, 16));





}
