#ifndef DIALOG_CONNECTION_METHOD_H
#define DIALOG_CONNECTION_METHOD_H

#include <QDialog>

namespace Ui {
class DialogConnectionMethod;
}

/*
 * DialogConnectionMethod
 *
 * Asks the user whether to use ANT+ or Bluetooth Low Energy (BTLE) as the
 * sensor connection method before a workout starts.
 */
class DialogConnectionMethod : public QDialog
{
    Q_OBJECT

public:
    enum ConnectionMethod { ANTPlus, BTLE };

    explicit DialogConnectionMethod(bool antAvailable, QWidget *parent = nullptr);
    ~DialogConnectionMethod();

    ConnectionMethod selectedMethod() const { return m_method; }

private slots:
    void accept() Q_DECL_OVERRIDE;

private:
    Ui::DialogConnectionMethod *ui;
    ConnectionMethod m_method = ANTPlus;
};

#endif // DIALOG_CONNECTION_METHOD_H
