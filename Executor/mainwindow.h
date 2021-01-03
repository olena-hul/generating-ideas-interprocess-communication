#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void generateIdea();
private slots:
    void on_pushButton_clicked();

signals:
    void stopTimer();

private:
    Ui::MainWindow *ui;
    HANDLE pipeHandle;
    void vote();
};
#endif // MAINWINDOW_H
