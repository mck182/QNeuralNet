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

#include "race-client.h"
#include "network.h"

#include <QTcpSocket>
#include <QStringList>
#include <qinputdialog.h>

RaceClient::RaceClient(QString host, int port, QObject *parent)
{
    m_host = host;
    m_port = port;

    m_socket = new QTcpSocket(this);
    m_state = RaceClient::Connecting;
    createNet(false);
    emit stateChanged(m_state);

    connect(m_socket, SIGNAL(readyRead()),
            this, SLOT(readFromServer()));

    connect(m_socket, SIGNAL(connected()),
            this, SLOT(onConnected()));

    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));
}

RaceClient::~RaceClient()
{

}

void RaceClient::onConnected()
{
    if (m_socket->isOpen()) {
        switch(m_state) {
            case RaceClient::Connecting:
                qDebug() << "Socket opened";
                m_socket->write("racelist\n\n");
                m_state = RaceClient::GettingRaceList;
                emit stateChanged(m_state);
                break;
            case RaceClient::ConnectingToRace:
                m_socket->write(QByteArray("driver\n"));
                m_socket->write(m_selectedRace.toStdString().c_str());
                m_socket->write(QByteArray("driver:Martyman\n"));
                m_socket->write(QByteArray("color:FF0000\n\n"));
                break;
        }
    }
}

void RaceClient::readFromServer()
{
    while (m_socket->canReadLine()) {
        QByteArray response = m_socket->readLine();
        if (response.contains("ok")) {
            QByteArray data = m_socket->readAll();
            switch(m_state) {
                case RaceClient::GettingRaceList:
                    m_selectedRace = QInputDialog::getItem(0, "Select a race", "Available races on the server", QString(data.trimmed()).split("\n"), 0, false);
                    m_selectedRace.prepend("race:");
                    m_selectedRace.append("\n");
                    m_state = RaceClient::ConnectingToRace;
                    emit stateChanged(m_state);
                    m_socket->connectToHost(m_host, m_port);
                    break;
                case RaceClient::ConnectingToRace:
                    break;
            }
        } else if (response.contains("round")) {
            m_state = RaceClient::Racing;
            emit stateChanged(m_state);
            round();
        }

    }
}

void RaceClient::onError(QAbstractSocket::SocketError error)
{
    qDebug() << "Got an error:" << error;

    if (error == QAbstractSocket::RemoteHostClosedError) {
        m_state = RaceClient::Disconnected;
    } else {
        m_state = RaceClient::Error;
    }

    emit stateChanged(m_state);
}

QString RaceClient::host()
{
    return m_host;
}

void RaceClient::disconnect()
{
    m_socket->close();
    m_state = RaceClient::Disconnected;
}

void RaceClient::round()
{
    float distance = 0;  // vzdalenost od cary <0,1>
    float angle = 0;     // uhel k care <0,1>
    float speed = 0;     // rychlost auta <0,1>
    float distance2 = 0; // vzdalenost od cary za sekundu <0,1>

    QByteArray line = m_socket->readLine(1024);
    while (line.length() > 0) {
        QStringList data = QString(line).split(":");
        if (data[0] == QLatin1String("distance")) {
            distance = data[1].toFloat();
        } else if (data[0] == QLatin1String("angle")) {
            angle = data[1].toFloat();
        } else if (data[0] == QLatin1String("speed")) {
            speed = data[1].toFloat();
        } else if (data[0] == QLatin1String("distance2")) {
            distance2 = data[1].toFloat();
        } else {
            //qDebug() << "Got garbage data";
        }

        line = m_socket->readLine(1024);
    }

    QList<float> *o = m_network->run(QList<float>() << angle << distance << distance2);

    qDebug() << "Net output for" << angle << distance << distance2 << "is" << *o;

    QString wheelString = QString("wheel:%1\n").arg(o->at(0));
    QString accString = QString("acc:%1\n").arg(o->at(1));
    // odpoved serveru
    m_socket->write(QByteArray("ok\n"));
    m_socket->write(QByteArray(accString.toAscii()));
    m_socket->write(QByteArray(wheelString.toAscii()));
    m_socket->write("\n");

    delete o;
}

void RaceClient::createNet(bool train)
{
    m_network = new Network(QList<int>() << 3 << 12 << 8 << 6 << 2);

    if (train) {

        QList<QList<float> > inputs;
        QList<QList<float> > outputs;

        //training set
        inputs << (QList<float>() << 0.1 << 0.9 << 0.6)
        << (QList<float>() << 1 << 1 << 1)
        << (QList<float>() << 1 << 0 << 0.3)
        << (QList<float>() << 0.1 << 0.2 << 0)
        << (QList<float>() << 0.5 << 0.4 << 0.2)
        << (QList<float>() << 0.5 << 0.7 << 1)
        << (QList<float>() << 0.5 << 0.5 << 0.5);

        outputs << (QList<float>() << 0.7 << 0.8)
                << (QList<float>() << 0.1 << 0.8)
                <<    (QList<float>() << 0.3 << 0.8)
                <<    (QList<float>() << 0.9 << 0.9)
                <<    (QList<float>() << 0.7 << 0.7)
                <<    (QList<float>() << 0.2 << 0.7)
                <<    (QList<float>() << 0.5 << 1);

        bool trained = false;
        int i = 0;
        while (!trained && i < 100000) {
            qDebug() << "Epoch" << i;
            trained = m_network->trainSet(inputs, outputs);
            i++;
        }

        qDebug() << "Net trained";
    } else {
        setNetWeights();
    }

    m_socket->connectToHost(m_host, m_port);

}

void RaceClient::setNetWeights()
{
    QList<QList<QList<float> > > weights;

    weights << (QList<QList<float> >()
//     ===== Layer 1
//     ======= Neuron 0
                        << (QList<float>()
                            << -2.80654
                            << 2.6221
                            << 0.686521
                            << 2.73229)
//     ======= Neuron 1
                        << (QList<float>()
                            << 1.52913
                            << -0.401705
                            << 0.729855
                            << 1.00387)
//     ======= Neuron 2
                        << (QList<float>()
                            << 1.37795
                            << 0.41187
                            << 1.0693
                            << 1.1912)
//     ======= Neuron 3
                        << (QList<float>()
                            << 0.764249
                            << 0.557123
                            << 0.313394
                            << 0.102989)
//     ======= Neuron 4
                        << (QList<float>()
                            << -1.27588
                            << 1.1445
                            << 0.453066
                            << 1.50387)
//     ======= Neuron 5
                        << (QList<float>()
                            << 0.603464
                            << 0.959547
                            << 0.696308
                            << 0.30212)
//     ======= Neuron 6
                        << (QList<float>()
                            << 0.139704
                            << 1.04926
                            << 0.29343
                            << 0.95763)
//     ======= Neuron 7
                        << (QList<float>()
                            << -1.87375
                            << 1.24232
                            << 0.733455
                            << 2.07801)
//     ======= Neuron 8
                        << (QList<float>()
                            << 0.34052
                            << -0.124197
                            << 0.788133
                            << -0.629414)
//     ======= Neuron 9
                        << (QList<float>()
                            << 0.653954
                            << 0.369395
                            << 0.713707
                            << 0.46016)
//     ======= Neuron 10
                        << (QList<float>()
                            << -2.11382
                            << 3.082
                            << 0.937743
                            << 1.37253)
//     ======= Neuron 11
                        << (QList<float>()
                            << -1.18778
                            << 1.89674
                            << 0.44782
                            << 1.18591))
//     ===== Layer 2
    << (QList<QList<float> >()
//     ======= Neuron 0
                        << (QList<float>()
                            << -2.94006
                            << 3.83473
                            << -1.29491
                            << -0.746784
                            << -0.312694
                            << 1.23489
                            << -0.417823
                            << 0.23746
                            << 2.24859
                            << -0.688965
                            << -0.387917
                            << 2.48656
                            << 1.64651)
//     ======= Neuron 1
                        << (QList<float>()
                            << -0.0219092
                            << 0.164956
                            << 0.461906
                            << 0.360542
                            << 0.0720558
                            << 0.532757
                            << 0.890795
                            << 0.594805
                            << 0.236693
                            << 0.408418
                            << 0.84492
                            << 0.957969
                            << 0.932418)
//     ======= Neuron 2
                        << (QList<float>()
                            << 0.142848
                            << 0.273678
                            << 0.935818
                            << 0.362713
                            << 0.689645
                            << 0.0781531
                            << 0.0645733
                            << 1.04479
                            << 0.769604
                            << 0.754205
                            << 0.0857862
                            << 0.836722
                            << 1.04404)
//     ======= Neuron 3
                        << (QList<float>()
                            << 0.956791
                            << 0.221858
                            << 0.319237
                            << 0.472062
                            << 0.739699
                            << 0.437306
                            << 0.664464
                            << 0.312462
                            << 0.906242
                            << 0.833445
                            << 0.898055
                            << 0.889692
                            << 0.504316)
//     ======= Neuron 4
                        << (QList<float>()
                            << -2.37014
                            << 2.86908
                            << -0.982659
                            << -0.907981
                            << -0.384325
                            << 1.44081
                            << 0.16658
                            << 0.296929
                            << 1.81609
                            << -0.778137
                            << -0.25221
                            << 2.81579
                            << 1.21461)
//     ======= Neuron 5
                        << (QList<float>()
                            << 1.0591
                            << 0.47352
                            << 0.0991782
                            << 0.835397
                            << 0.511194
                            << 0.994554
                            << 1.0444
                            << 0.798919
                            << 0.440338
                            << 0.75677
                            << 0.227248
                            << 0.0832977
                            << 0.0358181)
//     ======= Neuron 6
                        << (QList<float>()
                            << 0.16909
                            << 0.887237
                            << 0.950576
                            << 1.02503
                            << 0.420289
                            << 0.082084
                            << 0.379941
                            << 0.840538
                            << 0.236663
                            << 0.722959
                            << 0.319696
                            << 0.642275
                            << 0.921782)
//     ======= Neuron 7
                        << (QList<float>()
                            << 0.0252198
                            << 0.647771
                            << 0.133369
                            << 0.791418
                            << 0.663594
                            << 0.120179
                            << 0.251052
                            << 0.708189
                            << 0.896071
                            << 0.705581
                            << 0.668502
                            << 0.899673
                            << 0.466589))
//     ===== Layer 3
    << (QList<QList<float> >()
//     ======= Neuron 0
                        << (QList<float>()
                            << -0.826627
                            << 0.253279
                            << -0.2592
                            << -0.754851
                            << -0.307781
                            << 0.255632
                            << 0.100251
                            << -0.421601
                            << -0.238259)
//     ======= Neuron 1
                        << (QList<float>()
                            << -0.526917
                            << -0.105443
                            << 0.129265
                            << -0.669048
                            << -0.150946
                            << 0.0762235
                            << -0.457076
                            << -0.516051
                            << -0.317358)
//     ======= Neuron 2
                        << (QList<float>()
                            << -1.35824
                            << 5.08353
                            << -0.931582
                            << -1.32773
                            << -1.08178
                            << 3.72575
                            << -1.21924
                            << -1.29344
                            << -0.915256)
//     ======= Neuron 3
                        << (QList<float>()
                            << -0.599011
                            << 0.248906
                            << -0.210551
                            << -0.10985
                            << -0.684605
                            << -0.619109
                            << -0.503677
                            << -0.544118
                            << -0.0326856)
//     ======= Neuron 4
                        << (QList<float>()
                            << 0.685106
                            << -4.29453
                            << -0.428321
                            << 0.00478665
                            << 0.146901
                            << -4.53776
                            << 0.153054
                            << 0.168064
                            << -0.246553)
//     ======= Neuron 5
                        << (QList<float>()
                            << -0.930157
                            << 4.26979
                            << -0.239643
                            << -0.15718
                            << -0.608218
                            << 3.31867
                            << -0.672438
                            << -0.328713
                            << -0.663078))
//     ===== Layer 4
    << (QList<QList<float> >()
//     ======= Neuron 0
                        << (QList<float>()
                            << 0.995229
                            << -0.40378
                            << 0.0845297
                            << -3.50284
                            << 0.213906
                            << 2.08495
                            << -0.479429)
//     ======= Neuron 1
                        << (QList<float>()
                            << -0.171872
                            << 0.132028
                            << 0.00207806
                            << -3.05037
                            << 0.322874
                            << 3.8503
                            << 3.22125));

    m_network->setNetWeights(weights);
}

