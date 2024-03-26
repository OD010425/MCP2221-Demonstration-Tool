#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

// Callback functions
public slots:
    void ChangeVrefCallback(int index);
    void TimerCallback();
    void ApplySettingsCallback();
    void DigitalOutCallback(int index);
    void ConnectSerialCallback();
    void SendSerial();
    void ReadSerialPort();
    void Mcp2221_ScanI2cBus();
    void ExitApp();
};
#endif // MAINWINDOW_H
