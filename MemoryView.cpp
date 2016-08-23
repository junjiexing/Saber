//
// Created by System Administrator on 16/8/22.
//

#include "MemoryView.h"
#include "EventDispatcher.h"
#include "DebugCore.h"
#include "global.h"

#include <QtWidgets>

MemoryView::MemoryView(QWidget *parent)
	: QAbstractScrollArea(parent)
{
	m_fontWidth = fontMetrics().width("X");
	m_fontHeight = fontMetrics().height();
	setQwordMode(false);

	m_ctxMenu = new QMenu(this);
	m_ctxMenu->addAction("转到地址", [this]
	{
		QDialog dlg;
		dlg.setWindowTitle("转到地址");
		auto vlay = new QVBoxLayout(&dlg);
		auto edit = new QLineEdit(&dlg);
		vlay->addWidget(edit);
		edit->setValidator(new QRegExpValidator(QRegExp("[0-9a-fA-F]{1,16}"), edit));

		auto btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
		vlay->addWidget(btnBox);

		connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
		connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);


		if (dlg.exec() != QDialog::Accepted)
		{
			return;
		}

		setAddress(edit->text().toULongLong(nullptr, 16));
	});

	m_ctxMenu->addAction("编辑", [this]
	{

	});

	QObject::connect(EventDispatcher::instance(), &EventDispatcher::setDebugCore, this, &MemoryView::setDebugCore);
	QObject::connect(EventDispatcher::instance(), &EventDispatcher::breakpointChanged, this, &MemoryView::updateContent);
}

void MemoryView::updateContent()
{
	viewport()->update();
}

void MemoryView::setDebugCore(std::shared_ptr<DebugCore> debugCore)
{
	m_debugCore = debugCore;
}

void MemoryView::setAddress(uint64_t address)
{
	m_currentAddress = address;
	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		return;
	}

	if (address < m_regionStart
		|| address >= (m_regionStart + m_regionSize))
	{
		if (!debugCore->findRegion(address, m_regionStart, m_regionSize))
		{
			log("findRegion failed in MemoryView::setAddress", LogType::Error);
			return;
		}

		reCalcLayout();
	}

	verticalScrollBar()->setValue((address - m_regionStart) / (m_qwordModel? 8: 16));
}
void MemoryView::paintEvent(QPaintEvent *event)
{
	QAbstractScrollArea::paintEvent(event);

	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		return;
	}

	QPainter p(viewport());
	int lineBytes = m_qwordModel? 8: 16;
	uint8_t buffer[16];
	uint64_t start = m_regionStart + verticalScrollBar()->value() * lineBytes;
	for (int y = 0; y < viewport()->height(); y += m_fontHeight, start += lineBytes)
	{
		if (start >= (m_regionStart + m_regionSize))
		{
			break;
		}
		//TODO: 处理读取失败
		if (!debugCore->readMemory(start, buffer, lineBytes))
		{
			std::memset(buffer, '?', lineBytes);
		}

		QRect br;
		int x = 0;
		if (start == m_currentAddress)
		{
			p.setPen(Qt::red);
		}
		p.drawText(x, y, viewport()->width() - x, m_fontHeight, Qt::AlignLeft | Qt::AlignTop, QString("%1").arg(start, 16, 16, QChar('0')), &br);
		p.setPen(Qt::black);
		x += br.width() + m_fontWidth;
		p.drawLine(x, y, x, y + m_fontHeight);
		x += m_fontWidth;
		if (m_qwordModel)
		{
			uint64_t val = *(reinterpret_cast<uint64_t*>(buffer));
			if (start == m_hilightStart)
			{
				p.fillRect(x, y, m_fontWidth * 16, m_fontHeight, Qt::lightGray);
			}
			p.drawText(x, y, viewport()->width() - x, m_fontHeight, Qt::AlignLeft | Qt::AlignTop, QString("%1").arg(val, 16, 16, QChar('0')), &br);
		}
		else
		{
			for (int i = 0; i < 16; ++i)
			{
				if (start + i == m_hilightStart)
				{
					p.fillRect(x, y, m_fontWidth * 2, m_fontHeight, Qt::lightGray);
				}
				p.drawText(x, y, viewport()->width() - x, m_fontHeight, Qt::AlignLeft | Qt::AlignTop, QString("%1").arg(buffer[i], 2, 16, QChar('0')), &br);
				x += br.width() + m_fontWidth;
			}
			p.drawLine(x, y, x, y + m_fontHeight);
			x += m_fontWidth;


			for (int j = 0; j < 16; ++j)
			{
				if (buffer[j] < 0x20 || buffer[j] > 0x7e)
				{
					buffer[j] = '.';
				}
			}
			p.drawText(x, y, viewport()->width() - x, m_fontHeight, Qt::AlignLeft | Qt::AlignTop, QString(QByteArray((char*)buffer, 16)), &br);
		}
	}


}

void MemoryView::reCalcLayout()
{
	verticalScrollBar()->setMaximum(m_regionSize/(m_qwordModel? 8: 16));
	viewport()->update();
}

void MemoryView::setQwordMode(bool on)
{
	m_qwordModel = on;
	reCalcLayout();
}
void MemoryView::mousePressEvent(QMouseEvent *event)
{
	QAbstractScrollArea::mousePressEvent(event);

	int line_num = event->y() / m_fontHeight + verticalScrollBar()->value();
	if (m_qwordModel)
	{
		m_hilightStart = m_regionStart + line_num * 8;
	}
	else if (event->x() < m_fontWidth * (16 + 1))
	{
		m_hilightStart = m_regionStart + line_num * 16;
	}
	else if (event->x() < m_fontWidth * (16 + 1 + 1 + 3 * 16))
	{
		m_hilightStart = m_regionStart + line_num * 16 + (event->x() - (16 + 2) * m_fontWidth) / (m_fontWidth * 3);
	}

	viewport()->update();
}

void MemoryView::contextMenuEvent(QContextMenuEvent *event)
{
	QAbstractScrollArea::contextMenuEvent(event);

	m_ctxMenu->exec(event->globalPos());
}

MemoryView::~MemoryView()
{

}

StackView::StackView(QWidget *parent)
	: MemoryView(parent)
{
	setQwordMode(true);
	QObject::connect(EventDispatcher::instance(), &EventDispatcher::setStackAddress, this, &MemoryView::setAddress);

}
void StackView::updateContent()
{
	auto debugCore = m_debugCore.lock();
	if (!debugCore)
	{
		return;
	}

	setAddress(debugCore->stackAddr());
}
