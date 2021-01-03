#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<windows.h>
#include<string>
#include<QDebug>
#include <QTimer>
using namespace std;

const LPCSTR PIPE = R"(\\.\Pipe\myPipe)";
QString output = "";
bool flag = true;
QTimer *timer;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow(){
    delete ui;
}

const int LENGTH = 10;
string ideasString;

string chooseIdea(){
    int index = rand()%ideasString.length();
    int start, end;
    while(ideasString[index]!='\n' && index>0){
        index--;
    }
    start = ++index;
    while(ideasString[index]!='\n' && index<ideasString.length()){
        index++;
    }
    end = index;
    return ideasString.substr(start,  end - start);

}


void MainWindow::on_pushButton_clicked(){
    if(WaitNamedPipeA(PIPE, NMPWAIT_WAIT_FOREVER) == FALSE){
        ui->statusbar->showMessage("not connect");
    }
    pipeHandle = CreateFileA(PIPE, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    srand(time(nullptr));
    timer = new QTimer;
    timer->start(1000);

    connect(timer, SIGNAL(timeout()), this, SLOT(generateIdea()));
}

void MainWindow::generateIdea(){
    char lengthBuffer[LENGTH] = {0};
    char *buffer = nullptr;
    DWORD dwWritten, dwRead;

    string sizeString;
    string idea;
    int size;
 output+="Generated idea:";
 output+="\n";

    if (pipeHandle != INVALID_HANDLE_VALUE) {
        if (ReadFile(pipeHandle, lengthBuffer, LENGTH, &dwRead, nullptr) != FALSE) {
            lengthBuffer[dwRead] = '\0';
            qDebug() << QString::fromStdString(lengthBuffer) << " size got from server" << endl;

            // cast char array sizeString to int
            sizeString = string(lengthBuffer);
            size = stoi(sizeString);


            if(size == 0){
                ui->statusbar->showMessage("generating ended");
                timer->stop();
                vote();
                return;
            }
            // reallocate memory for main buffer
            delete[] buffer;
            buffer = new char[size];
            Sleep(10);

            if (ReadFile(pipeHandle, buffer, size, &dwRead, nullptr) != FALSE) {
                ideasString = string(buffer);
                delete[] buffer;
                buffer = nullptr;
            }
            idea = chooseIdea();
            output += QString::fromStdString(idea);
            size = idea.size() + 1;
            string sizeString = to_string(size);
            for (int i = 0; i < LENGTH; i++) {
                if (i < size - 1) {
                    lengthBuffer[i] = sizeString[i];
                } else {
                    lengthBuffer[i] = 0;
                }
            }
            WriteFile(pipeHandle, lengthBuffer, LENGTH, nullptr, nullptr);
            qDebug() << QString::fromStdString(lengthBuffer) << " sent to server" << endl;
            Sleep(10);

            buffer = new char[size];
            // copy idea to buffer
            for (int i = 0; i < size - 1; i++) {
                buffer[i] = idea[i];
            }

            buffer[size - 1] = '\0';

            // send the second request containing buffer data
            WriteFile(pipeHandle, buffer, size, &dwWritten, nullptr);
            qDebug() << QString::fromStdString(idea) << " sent to server" << endl;
        };
        output+="\n";
        ui->textEdit->setText(output);

     }
}

void MainWindow::vote(){
    ui->textEdit->append("Voting started:");
    char buffer[1];
    DWORD dwWritten, dwRead;
    if (pipeHandle != INVALID_HANDLE_VALUE) {
        for(int i = 0;i<3;i++){
            if (ReadFile(pipeHandle, buffer, 1, &dwRead, nullptr) != FALSE) {
                char numberOfIdeas = QString::number(buffer[0]).toInt();
                int randIdea = rand()%numberOfIdeas;

                if(buffer[0] == 2){
                    ui->textEdit->append("The process voted for idea, chosen by process - ");
                    ui->textEdit->append(QString::number(randIdea + 1));
                } else {
                    ui->textEdit->append("The process voted for idea number");
                    ui->textEdit->append(QString::number(randIdea));
                }
                buffer[0] = randIdea;
                WriteFile(pipeHandle, buffer, 1, nullptr, nullptr);
                Sleep(10);
            }
        }
    }
}

