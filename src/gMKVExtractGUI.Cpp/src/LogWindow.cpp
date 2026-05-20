#include "gmkvextractgui/LogWindow.h"

#include "gmkvextractgui/UiLocalization.h"
#include "gmkvtoolnix/Log.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

namespace gmkv::gui {

LogWindow::LogWindow(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Log"));
    setWindowTitleKey(this, QStringLiteral("UI.LogForm.Title"));
    resize(720, 420);

    auto* layout = new QVBoxLayout(this);
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    layout->addWidget(m_logText, 1);

    auto* buttons = new QHBoxLayout();
    auto* refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
    setTextKey(refreshButton, QStringLiteral("UI.LogForm.Actions.Refresh"));
    auto* clearButton = new QPushButton(QStringLiteral("Clear"), this);
    setTextKey(clearButton, QStringLiteral("UI.LogForm.Actions.ClearLog"));
    auto* copyButton = new QPushButton(QStringLiteral("Copy"), this);
    setTextKey(copyButton, QStringLiteral("UI.LogForm.Actions.CopySelection"));
    auto* saveButton = new QPushButton(QStringLiteral("Save..."), this);
    setTextKey(saveButton, QStringLiteral("UI.LogForm.Actions.Save"));
    auto* closeButton = new QPushButton(QStringLiteral("Close"), this);
    setTextKey(closeButton, QStringLiteral("UI.LogForm.Actions.Close"));
    buttons->addWidget(refreshButton);
    buttons->addWidget(clearButton);
    buttons->addWidget(copyButton);
    buttons->addWidget(saveButton);
    buttons->addStretch(1);
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);

    connect(refreshButton, &QPushButton::clicked, this, &LogWindow::refresh);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        gmkv::Logger::clear();
        refresh();
    });
    connect(copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_logText->toPlainText());
    });
    connect(saveButton, &QPushButton::clicked, this, &LogWindow::save);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(&gmkv::Logger::instance(), &gmkv::Logger::logLineAdded, this, [this]() {
        refresh();
    });

    refresh();
    applyLocalization();
}

void LogWindow::applyLocalization()
{
    gmkv::gui::applyLocalization(this);
}

void LogWindow::refresh()
{
    m_logText->setPlainText(gmkv::Logger::logText());
    m_logText->moveCursor(QTextCursor::End);
}

void LogWindow::save()
{
    const QString filename = QFileDialog::getSaveFileName(this, loc(QStringLiteral("UI.LogForm.Actions.Save")), QString(), QStringLiteral("Text Files (*.txt);;All Files (*)"));
    if (filename.isEmpty()) {
        return;
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        file.write(m_logText->toPlainText().toUtf8());
    }
}

}
