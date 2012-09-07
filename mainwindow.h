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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QGraphicsScene>

#include "race-client.h"

class RaceClient;
class Network;
namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void loadFile(QString filename);

public slots:
    void train();               ///Trains the net with data loaded from file
    void run();                 ///Gets net output for the loaded run set
    void reset();               ///Resets everything
    void openFileDialog();      ///Opens file dialog
    void netParamsChanged();    ///Slot for updating net params
    void connectToServer();     ///Connects to the race server
    void recognizeNumber();     ///Runs the net on the bitmap raster
    void paintNumber();         ///Paints the selected number on the bitmap raster

    void handleRaceStateChange(RaceClient::ClientState state);  ///Slot for race client state changes
    void toggleDisplaySegment(int row, int column);             ///Controls the bitmap raster

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;

    int m_iteration;                        ///Current iteration (increases after the whole train.set has been processed)
    QList<QStringList> m_inputsList;        ///Names of inputs
    QList<int> m_neuronsInLayersList;       ///Number of neurons in layers
    QList<QString> m_outputsList;           ///Names of the outputs
    QList<QList<float> > m_trainingSet;     ///The training set from the file
    QList<QList<float> > m_normTrainingSet; ///Normalized traning set
    QList<QList<float> > m_expectedOutputs; ///Expected net outputs read from the file

    QList<QList<float> > m_runSet;          ///Run set loaded from file
    QList<QList<float> > m_normRunSet;      ///Normalized run set

    QWeakPointer<Network> m_network;        ///The net itself

    RaceClient *m_raceClient;               ///Race client

    bool m_netForRecognition;               ///True if the net was trained for number recognition
};

#endif // MAINWINDOW_H
