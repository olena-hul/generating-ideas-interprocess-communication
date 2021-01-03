#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>
#include <fstream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void showIdeas();
private slots:
    void on_start_clicked();

    void on_pushButton_clicked();

    void on_best_clicked();

private:
    enum {EXECUTORS_COUNT=3};
    const LPCWSTR EXECUTOR_PATH = L"C:\\Users\\Olena\\Desktop\\study\\OS\\build-Executor-Desktop_Qt_5_15_1_MinGW_64_bit-Debug\\debug\\Executor.exe";
    Ui::MainWindow *ui;
    void createExecutors();
    void connectPipe();
};
#endif // MAINWINDOW_H
