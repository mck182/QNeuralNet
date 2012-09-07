/*
    Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

    Based on C++ implementation by Chesnokov Yuriy, Copyright (C) 2008

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "network.h"
#include "neuron.h"
#include "race-client.h"

#include <QGraphicsEllipseItem>
#include <QDebug>
#include <QPointF>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QInputDialog>
#include <QTimer>

#include <QList>

#include <math.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_netForRecognition = false;
    qDebug() << "Ready";

    //connect UI stuff
    connect(ui->trainButton, SIGNAL(clicked()),
            this, SLOT(train()));

    connect(ui->openFileButton, SIGNAL(clicked()),
            this, SLOT(openFileDialog()));

    connect(ui->runButton, SIGNAL(clicked()),
            this, SLOT(run()));

    connect(ui->lrEdit, SIGNAL(editingFinished()),
            this, SLOT(netParamsChanged()));

    connect(ui->momentumEdit, SIGNAL(editingFinished()),
            this, SLOT(netParamsChanged()));

    connect(ui->maxErrorEdit, SIGNAL(editingFinished()),
            this, SLOT(netParamsChanged()));

    connect(ui->connectButton, SIGNAL(clicked()),
            this, SLOT(connectToServer()));

    connect(ui->resetButton, SIGNAL(clicked()),
            this, SLOT(reset()));

    connect(ui->display, SIGNAL(cellClicked(int,int)),
            this, SLOT(toggleDisplaySegment(int,int)));

    connect(ui->recognizeButton, SIGNAL(clicked(bool)),
            this, SLOT(recognizeNumber()));

    connect(ui->oneButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->twoButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->threeButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->fourButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->fiveButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->sixButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->sevenButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->eightButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->nineButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    connect(ui->zeroButton, SIGNAL(clicked(bool)),
            this, SLOT(paintNumber()));

    ui->runButton->setDisabled(true);
    ui->trainButton->setDisabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::train()
{
    int i = 0;
    QList<float> *netOutputs;
    bool trained = false;
        while (!trained && i < ui->iterationsEdit->text().toInt()) {
            qDebug() << "Epoch" << i;
            trained = m_network.data()->trainSet(m_normTrainingSet, m_expectedOutputs);
            i++;
            ui->iterationsCountEdit->setText(QString::number(i));

            if (i % 2 == 0) {
                for (int k = 0; k < m_normTrainingSet.size(); k++) {
                    netOutputs = m_network.data()->run(m_normTrainingSet.at(k));
                    QString nout;
                    for (int j = 0; j < netOutputs->size(); j++) {
                        nout.append(QString::number(netOutputs->at(j)));
                        if (j != netOutputs->size() - 1) {
                            nout.append("; ");
                        }
                    }
                    ui->trainTableWidget->setItem(k, 2, new QTableWidgetItem(nout));
                    QTimer::singleShot(0, ui->trainTableWidget, SLOT(resizeColumnsToContents()));
                }
            }
        }
        qDebug() << "Trained after" << i;
        ui->trainFirstLabel->hide();
        ui->runButton->setEnabled(true);
}

void MainWindow::reset()
{
    m_iteration = 0;
    m_trainingSet.clear();
    m_normTrainingSet.clear();
    m_expectedOutputs.clear();
    m_runSet.clear();
    m_normRunSet.clear();
    m_inputsList.clear();
    if (!m_network.isNull()) {
         delete m_network.data();
//     m_network = 0;
    }
    m_neuronsInLayersList.clear();
    m_outputsList.clear();

    ui->iterationsCountEdit->setText(QString("0"));
    ui->trainFirstLabel->show();
    ui->runButton->setDisabled(true);
    ui->trainButton->setDisabled(true);
    ui->trainTableWidget->clearContents();
    ui->runSetTableWidget->clearContents();
    ui->trainTableWidget->setRowCount(0);
    ui->runSetTableWidget->setRowCount(0);
}

void MainWindow::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"), QString(), tr("Text files (*.txt)"));

    loadFile(fileName);
}

void MainWindow::loadFile(QString filename)
{
    reset();

    QString lineString;
    bool layers = false;
    bool inputs = false;
    bool neuronsInLayers = false;
    bool outputs = false;
    bool lr = false;
    bool momentum = false;
    bool trainingSet = false;
    bool runSet = false;

    bool trainCountRead = false;
    bool runCountRead = false;

    int inputsRead = 0;
    int trainRead = 0;
    int trainToRead = 999;
    int runRead = 0;
    int runToRead = 999;

    int layersCount = 0;

    int inputNumber = 0;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();

        qDebug() << "Read line:" << line;

        if(line.contains("#") || line.isEmpty() || line == "\n")
                continue;

        if(layers == false) {
            QString sline(line.trimmed());
            layersCount = sline.toInt();

            //add input layer
            layersCount++;

            qDebug() << "Layers Count:" << layersCount;
            ui->layerCountEdit->setText(QString::number(layersCount));
            layers = true;
            continue;
        }

        if(layers && inputs == false)
        {
            qDebug() << "* Inputs while";
            QString sline(line.trimmed());
            inputNumber = sline.toInt();

            qDebug() << "Inputs to read:" << inputNumber;

            while(inputsRead < inputNumber)
            {
                line = file.readLine();
                if(line.contains("#") || line.isEmpty() || line == "\n")
                        continue;

                qDebug() << "Appending line with inputs";

                m_inputsList.append(QString(line.simplified()).split(" "));
                inputsRead++;
            }

            inputs = true;
            continue;
        }

        if(layers && inputs && neuronsInLayers == false)
        {
            qDebug() << "* Neurons in layers while";

//             line = file.readLine();

            if(line.contains("#") || line.isEmpty() || line == "\n")
                continue;

            qDebug() << "Read in Neurons in layers while:" << line;

            foreach(const QString &n, QString(line.simplified()).split(" ")) {
                m_neuronsInLayersList.append(n.toInt());
            }
            //ui->layerCountEdit->setText(QString::number(m_neuronsInLayersList.size()));
            neuronsInLayers = true;
//             continue;
        }

        if(layers && inputs && neuronsInLayers && outputs == false)
        {
            qDebug() << "* Outputs while";

            int outputsRead = 0;
            int outputNumber = m_neuronsInLayersList.last();

            while(outputsRead < outputNumber)
            {
                line = file.readLine();
                if(line.contains("#") || line.isEmpty() || line == "\n")
                    continue;

                qDebug() << "Outputs read:" << line;
                m_outputsList.append(QString(line.simplified()));
                outputsRead++;
            }

            outputs = true;
//             continue;
        }

        if(layers && inputs && neuronsInLayers && outputs && lr == false)
        {
            qDebug() << "* LR while";

            while(lr == false)
            {
                line = file.readLine();

                qDebug() << "Read in LR while:" << line;

                if(line.contains("#") || line.isEmpty() || line == "\n")
                        continue;

                ui->lrEdit->setText(QString(line.simplified()));
                lr = true;
            }
        }

        if(layers && inputs && neuronsInLayers && outputs && lr && momentum == false)
        {
            qDebug() << "* Momentum while";

            while(momentum == false)
            {
                line = file.readLine();

                qDebug() << "Read in momentum while:" << line;

                if(line.contains("#") || line.isEmpty() || line == "\n")
                    continue;

                ui->momentumEdit->setText(QString(line.simplified()));
                momentum = true;
            }
        }

        if(layers && inputs && neuronsInLayers && outputs && lr && momentum && trainingSet == false)
        {
            qDebug() << "* TS while";
//             QString sline(line.trimmed());
//             trainToRead = sline.toInt();

            while(trainRead < trainToRead)
            {
                line = file.readLine();
                line = line.trimmed();

                qDebug() << "Read in TS while:" << line;

                if(line.contains("#") || line.isEmpty() || line == "\n")
                        continue;

                if(trainCountRead == false)
                {
                    trainToRead = line.toInt();
                    qDebug() << "Train data to read:" << trainToRead;
                    trainCountRead = true;
                    continue;
                }

                QString sline(line.simplified());
                QStringList numbers = sline.split(" ");
                qDebug() << numbers.size() << numbers;

                QList<float> l;

                for(int j = 0; j < numbers.size() - m_outputsList.size(); j++)
                {
                    l.append(numbers.at(j).toFloat());
                }

                m_trainingSet.append(l);


                QList<float> o;

                for(int j = numbers.size() - m_outputsList.size(); j < numbers.size(); j++) {
                    o.append(numbers.at(j).toFloat());
                }

                m_expectedOutputs.append(o);

                trainRead++;
            }

            trainingSet = true;
        }

        if(layers && inputs && neuronsInLayers && outputs && lr && momentum && trainingSet && runSet == false)
        {
            while(runRead < runToRead)
            {
                line = file.readLine();
                line = line.trimmed();

                qDebug() << "Read in RS while:" << line;

                if(line.contains("#") || line.isEmpty() || line == "\n")
                        continue;

                if(runCountRead == false)
                {
                    runToRead = line.toInt();
                    qDebug() << "Run data to read:" << runToRead;
                    runCountRead = true;
                    continue;
                }

                QString sline(line);
                QStringList numbers = sline.split(" ");

                QList<float> l;

                for(int j = 0; j < numbers.size(); j++)
                {
                    l.append(numbers.at(j).toFloat());
                }

                m_runSet.append(l);


                runRead++;
            }

            runSet = true;
        }

    }

    m_neuronsInLayersList.prepend(m_inputsList.size());

    //normalize training set
    float x;
    QList<float> n;

    for (int i = 0; i < m_trainingSet.size(); i++) {
        n.clear();
        for (int j = 0; j < m_inputsList.size(); j++) {
            x = (m_trainingSet.at(i).at(j) - m_inputsList.at(j).at(1).toFloat()) / (m_inputsList.at(j).at(2).toFloat() - m_inputsList.at(j).at(1).toFloat());
            n.append(x);
        }
        m_normTrainingSet.append(n);
    }

    //normalize run set
    QList<float> r;

    for (int i = 0; i < m_runSet.size(); i++) {
        r.clear();
        for (int j = 0; j < m_inputsList.size(); j++) {
            x = (m_runSet.at(i).at(j) - m_inputsList.at(j).at(1).toFloat()) / (m_inputsList.at(j).at(2).toFloat() - m_inputsList.at(j).at(1).toFloat());
            r.append(x);
        }
        m_normRunSet.append(r);
    }

    qDebug() << "##### Summary #####";
    qDebug() << "Inputs list:" << m_inputsList;
    qDebug() << "Layers:" << layersCount;
    qDebug() << "Neurons in layer:" << m_neuronsInLayersList;
    qDebug() << "Outputs list:" << m_outputsList;
    qDebug() << "Training set:" << m_trainingSet;
    qDebug() << "Norm. training set:" << m_normTrainingSet;
    qDebug() << "Expected outputs:" << m_expectedOutputs;
    qDebug() << "Run set:" << m_runSet;
    qDebug() << "Norm. run set:" << m_normRunSet;
    qDebug() << "Learning:" << ui->lrEdit->text();
    qDebug() << "Momentum:" << ui->momentumEdit->text();

    m_network = new Network(m_neuronsInLayersList, ui->momentumEdit->text().toFloat(), ui->lrEdit->text().toFloat(), ui->maxErrorEdit->text().toFloat());
    m_netForRecognition = false;

    for (int i = 0; i < m_trainingSet.size(); i++) {
        QString tset, eout;
        for (int j = 0; j < m_trainingSet.at(i).size(); j++) {
            tset.append(QString::number(m_trainingSet.at(i).at(j)));
            if (j != m_trainingSet.at(i).size() - 1) {
                tset.append("; ");
            }
        }
        for (int j = 0; j < m_expectedOutputs.at(i).size(); j++) {
            eout.append(QString::number(m_expectedOutputs.at(i).at(j)));
            if (j != m_expectedOutputs.at(i).size() - 1) {
                eout.append("; ");
            }
        }
        ui->trainTableWidget->setRowCount(ui->trainTableWidget->rowCount() + 1);
        ui->trainTableWidget->setItem(i, 0, new QTableWidgetItem(tset));
        ui->trainTableWidget->setItem(i, 1, new QTableWidgetItem(eout));
    }

    for (int i = 0; i < m_runSet.size(); i++) {
        QString rset;
        for (int j = 0; j < m_runSet.at(i).size(); j++) {
            rset.append(QString::number(m_runSet.at(i).at(j)));
            if (j != m_runSet.at(i).size() - 1) {
                rset.append("; ");
            }
        }

        ui->runSetTableWidget->setRowCount(ui->runSetTableWidget->rowCount() + 1);
        ui->runSetTableWidget->setItem(i, 0, new QTableWidgetItem(rset));
    }

    ui->trainButton->setEnabled(true);
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::run()
{
    QList<float> *result;
    for (int i = 0; i < m_normRunSet.size(); i++) {
        result = m_network.data()->run(m_normRunSet.at(i));
        qDebug() << "Net output for" << m_normRunSet.at(i) << ":" << *result;
        QString rout;
        for (int j = 0; j < result->size(); j++) {
            rout.append(QString::number(result->at(j)));
            if (j != result->size() - 1) {
                rout.append("; ");
            }
        }

        ui->runSetTableWidget->setItem(i, 1, new QTableWidgetItem(rout));
    }

//     QMessageBox::information(this, "Running set completed", perceptronOutput, QMessageBox::Ok);
}

void MainWindow::netParamsChanged()
{
    if (!m_network.isNull()) {
        m_network.data()->setLearningRate(ui->lrEdit->text().toFloat());
        m_network.data()->setMomentum(ui->momentumEdit->text().toFloat());
        m_network.data()->setMaxError(ui->maxErrorEdit->text().toFloat());
    }
}

void MainWindow::connectToServer()
{
    bool ok;
    QString text = QInputDialog::getText(this,
                                         tr("Enter server:port"),
                                         tr("Connect to:"),
                                         QLineEdit::Normal,
                                         QString("127.0.0.1:9477"), &ok);

    if (ok) {
        QStringList s = text.split(":");
        m_raceClient = new RaceClient(s.at(0), s.at(1).toInt(), this);
        connect(m_raceClient, SIGNAL(stateChanged(RaceClient::ClientState)),
                this, SLOT(handleRaceStateChange(RaceClient::ClientState)));
//         ui->tabWidget->setCurrentIndex(2);
    }
}

void MainWindow::handleRaceStateChange(RaceClient::ClientState state)
{
//     qDebug() << "Got state change" << state;
    switch (state) {
        case RaceClient::Connecting:
            ui->raceStateLabel->setText("Connecting...");
            break;
        case RaceClient::ConnectingToRace:
            ui->raceStateLabel->setText("Connecting to race...");
            ui->serverHostLabel->setText(m_raceClient->host());
            break;
        case RaceClient::GettingRaceList:
            ui->raceStateLabel->setText("Getting race list...");
            ui->serverHostLabel->setText(m_raceClient->host());
            break;
        case RaceClient::Racing:
            ui->raceStateLabel->setText("Race in progress");
            break;
        case RaceClient::Disconnected:
            ui->raceStateLabel->setText("Disconnected");
            ui->serverHostLabel->setText("Not connected");
            break;
        case RaceClient::Error:
            ui->raceStateLabel->setText("Error");
            ui->serverHostLabel->setText("Not connected");
            break;
    }
}

void MainWindow::toggleDisplaySegment(int row, int column)
{
    if (ui->display->item(row, column)->backgroundColor() == Qt::black) {
        ui->display->item(row, column)->setBackgroundColor(Qt::white);
    } else {
        ui->display->item(row, column)->setBackgroundColor(Qt::black);
    }
}

void MainWindow::recognizeNumber()
{
    if (!m_netForRecognition) {
        if (!m_network.isNull()) {
            delete m_network.data();
        }

        //init network with 4 layers, alpha=0.8, learningRate=0.1 and maxError=0.05
        m_network = new Network(QList<int>() << 25 << 18 << 8 << 4, 0.8f, 0.1f, 0.05f);
        m_netForRecognition = true;
    }

    bool train = false;
    if (train) {
        QList<QList<float> > trainSet;
        //prepare the train set
        trainSet << (QList<float>() << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0)

                 << (QList<float>() << 0 << 0 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 0 << 1
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 1 << 0 << 0
                                    << 0 << 1 << 1 << 1 << 1)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 0 << 0
                                    << 0 << 1 << 1 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 0 << 0
                                    << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0
                                    << 0 << 0 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0)

                 << (QList<float>() << 0 << 1 << 1 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 0 << 1 << 0
                                    << 0 << 1 << 1 << 1 << 0);

                QList<QList<float> > outputs;
                //expected outputs for the trainset
                outputs << (QList<float>() << 0 << 0 << 0 << 1 )
                        << (QList<float>() << 0 << 0 << 1 << 0 )
                        << (QList<float>() << 0 << 0 << 1 << 1 )
                        << (QList<float>() << 0 << 1 << 0 << 0 )
                        << (QList<float>() << 0 << 1 << 0 << 1 )
                        << (QList<float>() << 0 << 1 << 1 << 0 )
                        << (QList<float>() << 0 << 1 << 1 << 1 )
                        << (QList<float>() << 1 << 0 << 0 << 0 )
                        << (QList<float>() << 1 << 0 << 0 << 1 )
                        << (QList<float>() << 0 << 0 << 0 << 0);

        bool trained = false;
        int i = 0;

        while (!trained && i < 1000000) {
                qDebug() << "Epoch" << i;
            trained = m_network.data()->trainSet(trainSet, outputs);
            i++;
        }
        qDebug() << "Trained after" << i;

        m_network.data()->printWeights();

    }
    else if (!m_network.data()->isTrained()) {
        QList<QList<QList<float> > > weights;
        //these are already learnt weights,
        //we can plug them in directly
        weights << (QList<QList<float> >() //Layer 1
        << (QList<float>() //======= Neuron 0
        << -0.638249
        << 5.1979e+34
        << -1.20019
        << -1.6819
        << -1.09797
        << 0.924777
        << 0.657582
        << 1.43451
        << 0.858619
        << -1.06778
        << 5.1979e+34
        << 0.690148
        << 1.82082
        << 8.44057
        << -0.887892
        << 0.268565
        << 8.39364
        << 0.963709
        << 7.91213
        << -1.00071
        << 0.0830466
        << 0.295485
        << 8.28963
        << 1.14708
        << -0.124517
        << 0.304544
        )
        << (QList<float>() //======= Neuron 1
        << 0.29967
        << 0.870814
        << 0.765024
        << 0.679504
        << 0.069199
        << 0.240901
        << 0.110031
        << 0.726196
        << 0.539977
        << 0.126177
        << 0.650974
        << 0.197559
        << 0.0849223
        << 0.765929
        << 5.1979e+34
        << 0.879934
        << 0.456076
        << 0.846467
        << 0.699583
        << 0.68227
        << 0.115032
        << 0.593228
        << 0.293917
        << 0.527159
        << 0.706633
        << 0.376964
        )
        << (QList<float>() //======= Neuron 2
        << 0.459642
        << 0.139617
        << 0.141882
        << 0.944169
        << 0.0812357
        << 7.97222
        << 0.683023
        << 0.0548349
        << -0.180621
        << 0.29584
        << 0.450087
        << 0.261754
        << 0.0199526
        << 0.792234
        << 0.0250054
        << 0.129392
        << 0.187624
        << 0.551229
        << 8.39532
        << 0.0685427
        << 0.352788
        << 0.351397
        << 0.314208
        << 0.0884204
        << -0.329294
        << 0.392975
        )
        << (QList<float>() //======= Neuron 3
        << 0.627829
        << 0.327585
        << 0.952842
        << 0.402285
        << 0.686778
        << 0.742779
        << 0.49185
        << 0.864678
        << 0.500939
        << 0.903072
        << 0.352692
        << 0.238011
        << 0.0900838
        << 0.502744
        << 0.972462
        << 0.595284
        << 0.762793
        << 0.46233
        << 0.588605
        << 0.129226
        << 0.598043
        << 0.772971
        << 0.640006
        << 0.5098
        << 0.186489
        << 0.979626
        )
        << (QList<float>() //======= Neuron 4
        << 0.814555
        << 0.482391
        << 0.0466217
        << 0.918806
        << 0.845161
        << 0.67434
        << 0.206014
        << 0.776563
        << -0.0222101
        << 0.872025
        << 0.53864
        << 0.518424
        << 0.722334
        << 0.0767335
        << 0.424393
        << 0.136975
        << 0.331279
        << 0.600645
        << 0.638374
        << 0.290789
        << 0.194916
        << 0.400807
        << 0.795931
        << 0.786033
        << 0.521326
        << 0.388687
        )
        << (QList<float>() //======= Neuron 5
        << 0.413598
        << 0.175111
        << 0.811081
        << 0.654055
        << 0.0115422
        << 0.726331
        << 0.240075
        << 0.147409
        << 0.570754
        << 0.0102338
        << 0.818822
        << 0.810774
        << 0.868903
        << 0.870152
        << 0.58166
        << 0.449223
        << 0.399556
        << 0.512367
        << 0.506754
        << 0.750066
        << 0.645971
        << 0.87377
        << 0.412466
        << 0.242805
        << 0.0553436
        << 0.612802
        )
        << (QList<float>() //======= Neuron 6
        << 0.569417
        << 0.985444
        << 0.356942
        << 0.194374
        << 0.258291
        << 0.982039
        << 0.411436
        << 0.182654
        << 0.701165
        << 0.464978
        << 0.976098
        << 0.979798
        << 0.642279
        << 0.540673
        << -0.0202861
        << 0.597694
        << 0.397084
        << -0.030039
        << 0.473526
        << 0.00808778
        << 0.455406
        << 0.878382
        << 0.595643
        << 0.966126
        << 0.616998
        << 0.268087
        )
        << (QList<float>() //======= Neuron 7
        << 0.753884
        << 0.185996
        << 0.645607
        << -0.00577245
        << 0.716753
        << 0.242165
        << 0.0523934
        << 0.058172
        << 0.452261
        << 0.445495
        << 0.0655982
        << 0.889925
        << 0.638352
        << 1.01554
        << 0.334988
        << 0.662814
        << 0.964899
        << 0.129958
        << 0.0693398
        << 0.121968
        << 0.793669
        << 0.646209
        << -0.110862
        << 0.0952327
        << 0.646846
        << 0.341836
        )
        << (QList<float>() //======= Neuron 8
        << 0.100053
        << 0.392045
        << 0.495312
        << 0.879876
        << 0.61502
        << 0.391291
        << 0.0692459
        << 0.213325
        << 0.42099
        << 0.839641
        << 0.476256
        << 0.510634
        << 0.115516
        << 0.932432
        << 0.886265
        << 0.4125
        << 0.833385
        << 0.606765
        << 0.408887
        << 0.224176
        << 0.280755
        << 0.362501
        << 0.489256
        << 0.536698
        << 0.371874
        << 0.287155
        )
        << (QList<float>() //======= Neuron 9
        << 0.170275
        << 0.488962
        << 0.613596
        << 1.05904
        << 0.00491128
        << 0.699241
        << 0.332762
        << 0.48596
        << 0.518941
        << 0.951626
        << 0.914193
        << 0.651736
        << 0.147743
        << 0.378435
        << 0.550325
        << 0.68597
        << 0.890478
        << 0.704469
        << 0.622018
        << 0.828893
        << 0.135139
        << 0.462814
        << 0.48916
        << 0.576806
        << 0.743847
        << 0.71844
        )
        << (QList<float>() //======= Neuron 10
        << -0.200217
        << 0.23124
        << -3.28007
        << 0.35858
        << -0.588327
        << 0.431818
        << 0.806845
        << -3.66991
        << 1.17144
        << 8.70239
        << 2.39643
        << 0.705297
        << -0.477115
        << 1.47038
        << -0.391965
        << 0.269451
        << 0.98894
        << 1.9322
        << 2.29101
        << -2.19258
        << 0.61046
        << 0.539773
        << 2.22004
        << 2.19271
        << -0.72845
        << 2.04387
        )
        << (QList<float>() //======= Neuron 11
        << 0.676969
        << 0.812025
        << 0.988102
        << 0.522151
        << 0.512143
        << 0.83022
        << 0.682983
        << 0.748511
        << 0.073214
        << 0.155559
        << 0.221455
        << 0.954948
        << 0.156676
        << 0.597879
        << 0.746748
        << 0.000279446
        << 0.303257
        << 0.0925613
        << 0.333456
        << -0.0153699
        << 0.389778
        << 0.326424
        << 0.934926
        << 0.0427942
        << 0.845148
        << 0.537651
        )
        << (QList<float>() //======= Neuron 12
        << 0.501227
        << 0.137979
        << 0.783623
        << 0.909891
        << 0.462751
        << 0.48429
        << 0.757801
        << 0.487276
        << 0.888869
        << 0.2144
        << 0.273183
        << 0.619016
        << 0.0548286
        << 0.467659
        << 0.731538
        << 0.31471
        << 0.408466
        << 0.0510212
        << 0.880433
        << 0.135566
        << 0.0549525
        << 0.215928
        << 0.276269
        << 0.36743
        << 0.145076
        << 0.658816
        )
        << (QList<float>() //======= Neuron 13
        << 0.0116832
        << 0.153915
        << 0.604098
        << 0.845789
        << -0.0115643
        << 0.308974
        << 0.727722
        << 0.20564
        << -0.225678
        << -0.471452
        << 0.860859
        << 0.0125502
        << -0.00463327
        << 0.118323
        << -0.39898
        << 0.0485463
        << 0.516539
        << 0.700111
        << 0.401385
        << -0.281075
        << 0.712222
        << 0.91053
        << 0.495492
        << 0.74
        << -0.615617
        << 0.334743
        )
        << (QList<float>() //======= Neuron 14
        << 0.474491
        << 0.392752
        << 1.49001
        << -1.50425
        << -0.282762
        << 0.546569
        << 0.217412
        << 3.8008
        << -1.54673
        << 0.272263
        << 0.777552
        << 0.864033
        << -1.68972
        << -0.891027
        << -0.234574
        << 0.351716
        << 7.89021
        << 7.21943
        << 0.903861
        << -0.322442
        << 0.923649
        << 0.765779
        << -2.53046
        << -2.20075
        << -0.275142
        << 1.46239
        )
        << (QList<float>() //======= Neuron 15
        << -1.1465
        << 0.471745
        << -1.57734
        << -2.98888
        << -0.575554
        << 0.27388
        << 0.820471
        << 7.65122
        << 2.23622
        << 4.07006
        << -1.08973
        << 0.554789
        << 1.55879
        << 3.77034
        << -1.01493
        << 0.841116
        << 0.281638
        << -1.52002
        << -1.15223
        << 0.301965
        << 0.209756
        << 0.608977
        << -2.17572
        << -2.00542
        << -1.17533
        << -1.15865
        )
        << (QList<float>() //======= Neuron 16
        << 0.195776
        << 0.403355
        << 0.766659
        << 0.403445
        << 0.182523
        << 0.974314
        << 0.787944
        << 0.550817
        << 0.340515
        << 0.697159
        << 0.822887
        << 0.222505
        << 0.37848
        << 0.273933
        << 0.0977766
        << 0.710982
        << 0.810627
        << 0.827038
        << 0.0330758
        << 0.345249
        << 0.686108
        << 0.410226
        << 0.105523
        << 0.874096
        << 0.794886
        << 0.25077
        )
        << (QList<float>() //======= Neuron 17
        << 0.437263
        << 0.112545
        << 0.822089
        << 0.785233
        << 0.248118
        << 0.0300051
        << 0.224622
        << 0.921445
        << 0.312252
        << 0.391278
        << 0.877626
        << 0.134149
        << 0.0501969
        << 0.348447
        << 0.75238
        << 0.999187
        << 0.56183
        << 0.246947
        << 0.195374
        << 0.671379
        << 0.969781
        << 0.0656514
        << 0.483887
        << 0.0486363
        << 0.267307
        << 0.160138
        )
        )
        << (QList<QList<float> >() //Layer 2
        << (QList<float>() //======= Neuron 0
        << 0.178451
        << -3.976
        << -0.205545
        << 0.326222
        << 0.490684
        << 0.308852
        << -0.0362052
        << 0.164522
        << -0.193196
        << 0.40153
        << 0.357726
        << 8.67246
        << 0.426068
        << -0.25771
        << -0.299917
        << -4.15451
        << -8.9028
        << -0.249595
        << -0.339952
        )
        << (QList<float>() //======= Neuron 1
        << 1.18769
        << -7.96305
        << 0.787468
        << 0.0298932
        << 0.561004
        << 0.37888
        << 0.262285
        << 0.292738
        << 0.987077
        << 0.437383
        << 0.982195
        << 0.378854
        << 0.904507
        << 0.474314
        << 0.271511
        << -6.74124
        << 5.50723
        << 0.671667
        << 0.702404
        )
        << (QList<float>() //======= Neuron 2
        << 0.747726
        << 0.186832
        << 0.297231
        << 0.410272
        << 0.864299
        << 0.0306146
        << 0.418939
        << -0.0146307
        << 0.701318
        << 0.561328
        << 0.185698
        << 0.711099
        << 0.533675
        << 0.386463
        << 0.280103
        << 0.793565
        << 0.845675
        << 0.54373
        << -0.0362906
        )
        << (QList<float>() //======= Neuron 3
        << -0.747092
        << 5.8468
        << -0.848339
        << -0.943121
        << -0.769602
        << -1.03691
        << -0.487673
        << -0.314098
        << -0.755436
        << -0.412112
        << -0.898585
        << 8.00953
        << -0.653425
        << -0.715515
        << -1.28258
        << -2.67403
        << 4.37444
        << -0.821575
        << -0.722394
        )
        << (QList<float>() //======= Neuron 4
        << 0.435513
        << -0.066507
        << 0.0852716
        << 0.647979
        << 0.731186
        << 0.672742
        << 0.115318
        << 0.072359
        << 0.491632
        << 0.0126811
        << 0.666747
        << 0.515901
        << 0.428924
        << 0.0550359
        << 0.75176
        << 0.761755
        << 0.432166
        << -0.0494573
        << 0.34277
        )
        << (QList<float>() //======= Neuron 5
        << 0.0837435
        << 0.292227
        << 0.227575
        << 0.401492
        << 0.142086
        << 0.834204
        << 0.957018
        << 0.345003
        << 0.907822
        << 0.432286
        << 0.634957
        << 0.455256
        << 0.947885
        << 0.682272
        << 0.620158
        << 0.706003
        << 0.496918
        << 0.376103
        << 0.906114
        )
        << (QList<float>() //======= Neuron 6
        << 0.654446
        << 0.950988
        << 0.00950564
        << 0.402675
        << 0.547573
        << 0.526604
        << 0.546417
        << 0.457126
        << 0.327198
        << 0.0927709
        << 0.492343
        << 0.756157
        << 0.181816
        << 0.791764
        << 0.987501
        << 0.588576
        << 0.939666
        << 0.831041
        << 0.553025
        )
        << (QList<float>() //======= Neuron 7
        << 0.281296
        << 0.73561
        << 0.981949
        << 0.921995
        << 0.197693
        << 0.935818
        << 0.610236
        << 0.825053
        << 0.647473
        << 0.11292
        << 0.207159
        << 0.560004
        << 0.767486
        << 0.162667
        << 0.569349
        << 0.174505
        << 0.708094
        << 0.0970896
        << 0.719656
        )
        )
        << (QList<QList<float> >() //Layer 3
        << (QList<float>() //======= Neuron 0
        << -1.77379
        << -13.5457
        << 4.46566
        << -1.25398
        << 10.2485
        << -1.84167
        << -1.48271
        << -1.7586
        << -1.34868
        )
        << (QList<float>() //======= Neuron 1
        << 0.880317
        << -3.24585
        << 3.10604
        << 1.16153
        << -12.6381
        << 0.600392
        << 1.10201
        << 1.44197
        << 0.559829
        )
        << (QList<float>() //======= Neuron 2
        << 0.415434
        << 15.9083
        << -6.32204
        << 0.530543
        << -11.0359
        << 0.537199
        << 0.315458
        << 0.359619
        << 1.05985
        )
        << (QList<float>() //======= Neuron 3
        << -2.53008
        << 2.14123
        << 18.1963
        << -2.83274
        << 1.06212
        << -2.51476
        << -1.98554
        << -2.1052
        << -2.24029
        )
        )
        ;

        m_network.data()->setNetWeights(weights);
    }

    QList<float> runset;

    for (int i = 0; i < ui->display->rowCount(); i++) {
        for (int j = 0; j < ui->display->columnCount(); j++) {
            runset.append(ui->display->item(i,j)->backgroundColor() == Qt::black ? 1 : 0);
        }
    }

    qDebug() << runset;

    QList<float> *netOutputs = m_network.data()->run(runset);
    QString realNetOut;

    unsigned int n = 0;
    for (int i = 0; i < netOutputs->size(); i++) {
        n <<= 1;
        if (round(netOutputs->at(i)) == 1) {
            n += 1;
        }

        realNetOut.append(QString::number(round(netOutputs->at(i)))).append(" (").append(QString::number(netOutputs->at(i))).append(")");
        if (i != netOutputs->size() -1) {
            realNetOut.append(";\n");
        }
    }

    delete netOutputs;

    qDebug() << "Recognized number:" << n;

    ui->recognizedNrLabel->setText(QString::number(n));
    ui->netOutLabel->setText(realNetOut);
}

void MainWindow::paintNumber()
{
    //clear the raster
    for (int i = 0; i < ui->display->rowCount(); i++) {
        for (int j = 0; j < ui->display->columnCount(); j++) {
            ui->display->item(i,j)->setBackgroundColor(Qt::white);
        }
    }

    QPushButton *button = qobject_cast<QPushButton*>(sender());

    switch(button->text().toInt()) {
        case 1:
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,2)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 2:
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(1,4)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,4)->setBackgroundColor(Qt::black);
            break;
        case 3:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,2)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 4:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,2)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 5:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,2)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 6:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,2)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,1)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 7:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 8:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,2)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,1)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 9:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,2)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
        case 0:
            ui->display->item(0,1)->setBackgroundColor(Qt::black);
            ui->display->item(0,2)->setBackgroundColor(Qt::black);
            ui->display->item(0,3)->setBackgroundColor(Qt::black);
            ui->display->item(1,1)->setBackgroundColor(Qt::black);
            ui->display->item(1,3)->setBackgroundColor(Qt::black);
            ui->display->item(2,1)->setBackgroundColor(Qt::black);
            ui->display->item(2,3)->setBackgroundColor(Qt::black);
            ui->display->item(3,1)->setBackgroundColor(Qt::black);
            ui->display->item(3,3)->setBackgroundColor(Qt::black);
            ui->display->item(4,1)->setBackgroundColor(Qt::black);
            ui->display->item(4,2)->setBackgroundColor(Qt::black);
            ui->display->item(4,3)->setBackgroundColor(Qt::black);
            break;
    }
}
