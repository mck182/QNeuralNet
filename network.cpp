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

#include "network.h"
#include "neuron.h"

#include <time.h>
#include <math.h>

#include <QList>
#include <QDebug>

Layer::Layer(int neuronCount)
{
    m_neuronCount = neuronCount;
    for (int i = 0; i < neuronCount; i++) {
        m_neurons.append(new Neuron());
    }

}

Layer::~Layer()
{
    for (int i = 0; i < m_neurons.size(); i++) {
        delete m_neurons[i];
    }
}


Neuron* Layer::neuron(int i)
{
    if (i < m_neurons.size()) {
        return m_neurons[i];
    }

    return 0;
}

//-----------------------------------------------------------------

Network::Network(QList<int> neuronsInLayers, float alpha, float learningRate, float maxError)
{

    m_alpha = alpha;
    m_learningRate = learningRate;
    m_maxError = maxError;
    m_trained = false;

    m_layersCount = neuronsInLayers.size();

    for (int i = 0; i < m_layersCount; i++) {
        qDebug() << "Creating new layer with" << neuronsInLayers[i] << "neurons";
        m_layers.append(new Layer(neuronsInLayers[i]));
    }

    srand((unsigned int)time(0));

    init();

    //randomize weights
    for (int i = 1; i < m_layersCount; i++) {
        for (int j = 0; j < m_layers[i]->neuronCount(); j++) {
            for (int k = 0; k < m_layers[i]->neuron(j)->m_inputs.size(); k++) {
                m_layers[i]->neuron(j)->m_inputs[k]->setWeight((float)rand() / RAND_MAX);
            }
        }
    }
}


Network::~Network()
{

}

void Network::init()
{
    Layer *currentLayer;
    Layer *previousLayer;
    Neuron *neuron;

    int layer = 0;

    //input layer
    currentLayer = m_layers[layer++];

    for (int n = 0; n < currentLayer->neuronCount(); n++) {
        neuron = currentLayer->neuron(n);
        //add one input link, this is the input itself
        neuron->addInputLink();

        neuron->m_inputs[0]->setWeight(1.0f);
    }

    //hidden layer
    for (int i = 0; i < m_layersCount - 2; i++) {
        previousLayer = currentLayer;
        currentLayer = m_layers[layer++];

        for (int n = 0; n < currentLayer->neuronCount(); n++) {
            neuron = currentLayer->neuron(n);
            neuron->addBias();

            for (int m = 0; m < previousLayer->neuronCount(); m++) {
                neuron->addInputLink(previousLayer->neuron(m));
            }
        }
    }

    //output layer
    previousLayer = currentLayer;
    currentLayer = m_layers[layer++];

    for (int n = 0; n < currentLayer->neuronCount(); n++) {
        neuron = currentLayer->neuron(n);
        neuron->addBias();

        for (int m = 0; m < previousLayer->neuronCount(); m++) {
            neuron->addInputLink(previousLayer->neuron(m));
        }
    }
}

void Network::backpropagate(const QList<float> &outputs)
{
    float delta, dw, output;

    //output layer delta
    for (int n = 0; n < m_layers.last()->neuronCount(); n++) {
        output = m_layers.last()->neuron(n)->output();
        m_layers.last()->neuron(n)->setDelta(output  * (1.0f - output) * (outputs[n] - output));
    }

    //hidden layer delta
    for (int l = m_layersCount - 2; l > 0; l--) {
        for (int n = 0; n < m_layers[l]->neuronCount(); n++) {
            delta = 0.0f;

            for (int i = 0; i < m_layers[l]->neuron(n)->m_outputs.size(); i++) {
                delta += m_layers[l]->neuron(n)->m_outputs[i]->weight() * m_layers[l]->neuron(n)->m_outputs[i]->inputNeuron()->delta();
            }

            output = m_layers[l]->neuron(n)->output();
            m_layers[l]->neuron(n)->setDelta(output * (1.0f - output) * delta);
        }
    }

    //correct weights
    for (int l = 1; l < m_layersCount; l++) {
        for (int n = 0; n < m_layers[l]->neuronCount(); n++) {
            for (int i = 0; i < m_layers[l]->neuron(n)->m_inputs.size(); i++) {
                dw = m_learningRate * m_layers[l]->neuron(n)->m_inputs[i]->inputValue() * m_layers[l]->neuron(n)->delta();
                dw += m_alpha * m_layers[l]->neuron(n)->m_inputs[i]->prevWeight();

                m_layers[l]->neuron(n)->m_inputs[i]->setPrevWeight(dw);
                m_layers[l]->neuron(n)->m_inputs[i]->setWeight(m_layers[l]->neuron(n)->m_inputs[i]->weight() + dw);
            }
        }
    }
}

bool Network::trainSet(const QList<QList<float> > &allInputs, const QList<QList<float> > &allOutputs)
{
    bool trained = true;

    for (int i = 0; i < allInputs.size(); i++) {
        if (!train(allInputs.at(i), allOutputs.at(i))) {
            trained = false;
        }
    }

    m_trained = trained;

    return trained;
}

bool Network::train(const QList< float >& inputs, const QList< float >& outputs)
{
    float error = 0.0f;

    QList<float> *netOutputs = run(inputs);

    for (int n = 0; n < m_layers.last()->neuronCount(); n++) {
        error = fabs(netOutputs->at(n) - (float)outputs.at(n));
        qDebug() << "Inputs:" << inputs << "Net output:" << netOutputs->at(n) << "Expected:" << outputs.at(n);
        if (error > m_maxError) {
             break;
        }
    }

    backpropagate(outputs);
    delete netOutputs;

    if (error > m_maxError) {
        return false;
    }

    return true;
}

QList<float>* Network::run(const QList<float> &inputs)
{
    //input layer
    for (int i = 0; i < m_layers[0]->neuronCount(); i++) {
        m_layers[0]->neuron(i)->m_inputs[0]->setInputValue(inputs[i]);
        m_layers[0]->neuron(i)->inputFire();
    }

    //hidden and output
    for (int l = 1; l < m_layersCount; l++) {
        for (int n = 0; n < m_layers[l]->neuronCount(); n++) {
            m_layers[l]->neuron(n)->fire();
        }
    }

    return output();
}

QList<float>* Network::output()
{
    QList<float> *netOutput = new QList<float>();

    for (int n = 0; n < m_layers.last()->neuronCount(); n++) {
        netOutput->append(m_layers.last()->neuron(n)->output());
    }

    return netOutput;
}

Layer* Network::layer(int i)
{
    return m_layers.at(i);
}

void Network::setLearningRate(float learningRate)
{
    m_learningRate = learningRate;
}

void Network::setMomentum(float momentum)
{
    m_alpha = momentum;
}

void Network::setMaxError(float maxError)
{
    m_maxError = maxError;
}

void Network::setNetWeights(const QList<QList<QList<float> > > &weights)
{
    for (int l = 1; l < m_layersCount; l++) {
        qDebug() << "===== Layer" << l;
        for (int n = 0; n < m_layers[l]->neuronCount(); n++) {
            qDebug() << "======= Neuron" << n << "(of" << m_layers[l]->neuronCount() << ")";
            for (int i = 0; i < m_layers[l]->neuron(n)->m_inputs.size(); i++) {
                qDebug() << weights.at(l-1).at(n).at(i);
                m_layers[l]->neuron(n)->m_inputs[i]->setWeight(weights.at(l-1).at(n).at(i));
            }
        }
    }

    //mark the net as trained
    m_trained = true;
}

void Network::printWeights()
{
    for (int l = 1; l < m_layersCount; l++) {
        qDebug() << "<< (QList<QList<float> >() //Layer" << l;
        for (int n = 0; n < m_layers[l]->neuronCount(); n++) {
            qDebug() << " << (QList<float>() //======= Neuron" << n;
            for (int i = 0; i < m_layers[l]->neuron(n)->m_inputs.size(); i++) {
                qDebug() << "<<" << m_layers[l]->neuron(n)->m_inputs.at(i)->weight();
            }
            qDebug() << ")";
        }
        qDebug() << ")";
    }
    qDebug() << ";";
}
