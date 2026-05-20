#pragma once

#include <QDialog>

class QTextEdit;

namespace gmkv::gui {

class LogWindow final : public QDialog
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget* parent = nullptr);
    void applyLocalization();

private:
    void refresh();
    void save();

    QTextEdit* m_logText = nullptr;
};

}
