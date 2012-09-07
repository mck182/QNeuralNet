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


#ifndef NETWORK_H
#define NETWORK_H

#include <QString>
#include <QList>
#include <QObject>

class Neuron;

class Layer {

public:
    /**
     * Represents one layer in the network
     * @param neuronCount - how many neurons are in this layer
     */
    Layer(int neuronCount);
    ~Layer();

    int neuronCount() {return m_neuronCount;}   ///Returns number of neurons in the layer
    Neuron* neuron(int i);                      ///Returns the ith neuron in the layer

private:
    int m_neuronCount;          ///The number of neurons in the layer
    QList<Neuron*> m_neurons;   ///The neurons in the layer
};

//------------------------------------------------------------

class Network : public QObject
{
    Q_OBJECT
public:
    /**
     * The net constructor
     * @param neuronsInLayers - list of how many neurons are in layers,
     *                          eg. {5, 4, 3} means 3 layers with 5 neurons in the input layer
     *                          4 neurons in the hidden layer and 3 output neurons
     * @param alpha - the 'momentum', aka the influence from the previous step
     * @param learningRate - the learning rate of the net
     * @param maxError - max allowed error for one output
     */
    Network(QList<int> neuronsInLayers, float alpha = 0.1f, float learningRate = 0.4f, float maxError = 0.1f);
    virtual ~Network();

    ///Constructs the links between neurons
    void init();

    ///Trains the net
    ///@param inputs - the training set
    ///@param outputs - the desired output
    bool train(const QList<float> &inputs, const QList<float> &outputs);

    ///This is convenience function used when there are more than one train sets
    ///@param allInputs - list of all training sets
    ///@param allOutputs - list of all desired outputs
    bool trainSet(const QList<QList<float> > &allInputs, const QList<QList<float> > &allOutputs);

    ///Runs the net with the given input, returns list of outputs
    QList<float>* run(const QList<float> &inputs);

    ///The backpropagation
    ///@param outputs - the desired net outputs
    void backpropagate(const QList<float> &outputs);

    ///Returns the whole net output
    QList<float>* output();

    ///Returns number of layers in the net, including input and output
    int layersCount() {return m_layersCount;};

    ///Returns ith layer of the net
    Layer *layer(int i);

    ///Utility function to set already trained weights
    void setNetWeights(const QList<QList<QList<float> > > &weights);    //structure: layer-neuron-input weights

    ///Utility function to print the current net weights in a format, which can be plugged directly in the net (copy&paste into code)
    void printWeights();

    ///True if the net was trained, false otherwise
    bool isTrained() {return m_trained;};

public slots:
    void setLearningRate(float learningRate);   ///Sets net's learning rate
    void setMomentum(float momentum);           ///Sets net's momentum
    void setMaxError(float maxError);           ///Sets net's max error per output

private:
    bool m_trained;             ///True if the net is trained, false otherwise
    int m_layersCount;          ///number of layers in network, including input & output layers
    QList<Layer *> m_layers;    ///list of layers

    float m_learningRate;       ///Learning rate
    float m_alpha;              ///Momentum
    float m_maxError;           ///Max error per output
};

#endif // NETWORK_H
