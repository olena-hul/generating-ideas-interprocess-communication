#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <windows.h>
#include <fstream>
#include <vector>
#include <QTimer>
#include <QFile>
const LPCSTR PIPE = R"(\\.\Pipe\myPipe)";
const std::string FILE_PATH = R"(C:\Users\Olena\Desktop\study\OS\Supervisor\ideas.txt)";
const int BUFFER_SIZE = 256;
const int LENGTH = 10;
HANDLE pipeHandle;
DWORD threadId;

std::vector<std::string> chosenIdeas;
std::vector<std::string> ideasFromFirstProcess, ideasFromSecondProcess;
std::vector<int> ideasFromThirdProcess;
std::vector<HANDLE> threadHandles;
std::vector<DWORD> threadIds;


MainWindow* mainWindow;

bool flag = true;
int connected = 0;

DWORD WINAPI instanceThread(LPVOID param);
DWORD WINAPI changeFlag(LPVOID param);
void getIdeas();
DWORD WINAPI votingThread(LPVOID param);

HANDLE mutex;
std::string ideasString;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mainWindow = this;
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_start_clicked()
{
    CreateThread(
                NULL,
                0,
                changeFlag,
                nullptr,
                0,
                nullptr
                );
    createExecutors();
    mutex = CreateMutexA(nullptr, FALSE, "mutex");
    getIdeas();
    while (connected != 3){
        connectPipe();
    }
    HANDLE th[3];
    for(int i = 0; i < 3; i++){
        th[i] = threadHandles[i];
    }
    WaitForMultipleObjects(3, th, FALSE, INFINITE);
    showIdeas();
}

void MainWindow::connectPipe(){
    pipeHandle = CreateNamedPipeA(
                PIPE,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                BUFFER_SIZE,
                BUFFER_SIZE,
                NMPWAIT_WAIT_FOREVER,
                nullptr
                );
    if(pipeHandle != INVALID_HANDLE_VALUE){
        qDebug() << "CAN CONNECT";
        if(ConnectNamedPipe(pipeHandle, nullptr) != FALSE){
            qDebug() << "CONNECTED";
            connected++;
            threadHandles.push_back(CreateThread(
                        NULL,
                        0,
                        instanceThread,
                        (LPVOID) pipeHandle,
                        0,
                        &threadId
                        ));
        }
    }
}

void MainWindow::createExecutors(){
    _PROCESS_INFORMATION *pid = new _PROCESS_INFORMATION[EXECUTORS_COUNT];
    HANDLE *threadHandles = new HANDLE [EXECUTORS_COUNT];
    for(int i = 0; i < EXECUTORS_COUNT; i++){
        STARTUPINFO si;
        PROCESS_INFORMATION pInfo;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        ZeroMemory(&pInfo, sizeof(PROCESS_INFORMATION));

        CreateProcess(EXECUTOR_PATH,
                      nullptr,
                      nullptr,
                      nullptr,
                      FALSE,
                      CREATE_SUSPENDED,
                      nullptr,
                      nullptr,
                      &si,
                      &pInfo);
       pid[i] = pInfo;
       threadHandles[i] = pid[i].hThread;
    }

    for(int i = 0; i < EXECUTORS_COUNT; i++){
         ResumeThread(pid[i].hThread);
    }
}

DWORD WINAPI changeFlag(LPVOID param){
    Sleep(20000);
    flag = false;
    qDebug() << "generation ended" << endl;
    return 0;
}

DWORD WINAPI instanceThread(LPVOID param){
    char lengthBuffer[LENGTH] = {0}; // buffer with size 10 containing the length of main buffer
    char *buffer;
    HANDLE pipeHandle = (HANDLE) param;
    DWORD dwRead, dwWritten;
    std::string idea;
    while(true){
        WaitForSingleObject(mutex, INFINITE);

        if(!ideasString.empty()){
            // cast the size of ideasString from type int to string and write it to char array buffer
            if(!flag){
                int size = 0;
                std::string sizeString = std::to_string(size);
                for(int i = 0; i < LENGTH; i++){
                    lengthBuffer[i] = 0;
                }
                lengthBuffer[0] = '0';
                WriteFile(pipeHandle, lengthBuffer, LENGTH, nullptr, nullptr);
                qDebug() << lengthBuffer << " was written" << endl;
                ReleaseMutex(mutex);
                CreateThread(
                            NULL,
                            0,
                            votingThread,
                            (LPVOID) pipeHandle,
                            0,
                            &threadId
                            );
                threadIds.push_back(threadId);

                return 0;
            }

            int size = ideasString.size();

            std::string sizeString = std::to_string(size);
            for(int i = 0; i < sizeString.size(); i++){
                lengthBuffer[i] = sizeString[i];
            }
            // send the first request containing buffer size
            WriteFile(pipeHandle, lengthBuffer, LENGTH, nullptr, nullptr);
            qDebug() << lengthBuffer << " was written" << endl;
            Sleep(100);

            buffer = new char[size];
            // copy ideas string to buffer
            for(int i = 0; i < ideasString.size(); i++){
                buffer[i] = ideasString[i];
            }

            // send the second request containing buffer data
            WriteFile(pipeHandle, buffer, size, &dwWritten, nullptr);
            qDebug() << "ideas string was written" << endl;

            Sleep(100);

            if(ReadFile(pipeHandle, lengthBuffer, LENGTH, &dwRead, nullptr) != FALSE){
                lengthBuffer[dwRead] = '\0';
                qDebug() << lengthBuffer << " size got from client" << endl;

                // cast char array sizeString to int
                sizeString = std::string(lengthBuffer);
                size = stoi(sizeString);

                // reallocate memory for main buffer
                delete [] buffer;
                buffer = new char[size];

                Sleep(100);

                if(ReadFile(pipeHandle, buffer, size, &dwRead, nullptr) != FALSE){
                    idea = std::string(buffer);
                    qDebug() << QString::fromStdString(idea) << " got from client" << endl;
                    chosenIdeas.push_back(idea);
                    int pos = ideasString.find(idea);
                    ideasString.erase(pos, idea.size() + 1);
                    delete [] buffer;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        ReleaseMutex(mutex);
    }
    DisconnectNamedPipe(pipeHandle);
    return 0;
}

void getIdeas(){
    std::string idea;
    std::ifstream ideasFile(FILE_PATH);
    while (getline (ideasFile, idea)) {
        ideasString += idea + "\n";
    }
    ideasString[ideasString.size() - 1] = '\0';
    ideasFile.close();
}

void MainWindow::showIdeas(){
    ui->ideas->setText("Generated ideas:");
    ui->ideas->append("\n");
    for(int i = 0; i < chosenIdeas.size(); i++){
        ui->ideas->append(QString::fromStdString(chosenIdeas[i]));
    }
}

void MainWindow::on_pushButton_clicked()
{
    showIdeas();
}

DWORD WINAPI votingThread(LPVOID param){
    std::vector<int> ideasVector;
    char buffer[1];
    HANDLE pipeHandle = (HANDLE) param;
    DWORD dwRead, dwWritten;
    WaitForSingleObject(mutex, INFINITE);
    for(int i = 0;i<3;i++){
        buffer[0] = chosenIdeas.size();
        if(GetCurrentThreadId() == threadIds[2]){
            buffer[0] = 2;
        }
        WriteFile(pipeHandle, buffer, 1, &dwWritten, nullptr);
        Sleep(100);
        if(ReadFile(pipeHandle, buffer, 1, &dwRead, nullptr) != FALSE){
            int numberOfIdea = buffer[0];
            if(GetCurrentThreadId() == threadIds[0]){
                ideasFromFirstProcess.push_back(chosenIdeas[numberOfIdea]);
                chosenIdeas.erase(chosenIdeas.begin()+numberOfIdea);
            }else if(GetCurrentThreadId() == threadIds[1]){
                ideasFromSecondProcess.push_back(chosenIdeas[numberOfIdea]);
                chosenIdeas.erase(chosenIdeas.begin()+numberOfIdea);
            }else{
                ideasFromThirdProcess.push_back(numberOfIdea);
            }

        }
    }
    ReleaseMutex(mutex);

    return 0;
}

void MainWindow::on_best_clicked()
{

    std::vector<std::string> bestIdeas;
    for(int i = 0; i < 3; i++){
        if(ideasFromThirdProcess[i] == 0){
            bestIdeas.push_back(ideasFromFirstProcess[i]);
        } else {
            bestIdeas.push_back(ideasFromSecondProcess[i]);
        }
    }

    ui->ideas->append("\nBest generated ideas are:");

    for(int i = 0; i < 3; i++){
        ui->ideas->append(QString::fromStdString(bestIdeas[i]));
    }
    QString filename="C:\\Users\\Olena\\Desktop\\project.txt";
    QFile file( filename );
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream( &file );
        for(int i=0; i<3; i++){
        stream << QString::fromStdString(bestIdeas[i]) << endl;
    }}
}
