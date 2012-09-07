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

#ifndef RACE_CLIENT_H
#define RACE_CLIENT_H

#include <QObject>
#include <qabstractsocket.h>

class Network;
class QTcpSocket;

class RaceClient : public QObject
{
    Q_OBJECT
public:
    enum ClientState {
        Connecting,
        GettingRaceList,
        ConnectingToRace,
        Racing,
        Disconnected,
        Error
    };

    RaceClient(QString host, int port, QObject *parent = 0);
    virtual ~RaceClient();

    void setNetWeights();
    QString host();

signals:
    void stateChanged(RaceClient::ClientState state);

public slots:
    void disconnect();

private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError error);
    void readFromServer();

private:
    void round();
    void createNet(bool train);

    QTcpSocket *m_socket;
    Network *m_network;

    QString m_host;
    int m_port;

    QString m_selectedRace;

    ClientState m_state;
};

#endif // RACE_CLIENT_H
