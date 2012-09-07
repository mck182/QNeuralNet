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


#include "neuron.h"
#include <math.h>

#include <QDebug>

NeuronLink::NeuronLink(Neuron *input, Neuron *output, float inputValue, float weight)
{
    m_inputNeuron = input;
    m_outputNeuron = output;

    m_inputValue = inputValue;
    m_weight = weight;
}

NeuronLink::~NeuronLink()
{
}

//---------------------------------------------------------------------

Neuron::Neuron()
{
}

Neuron::~Neuron()
{
    for (int i = 0; i < m_inputs.size(); i++) {
        delete m_inputs[i];
    }
}

void Neuron::addInputLink(Neuron *poutn)
{
    NeuronLink *l = new NeuronLink(this, poutn);
    m_inputs.append(l);
    if (poutn) {
        poutn->m_outputs.append(l);
    }
}

void Neuron::addBias()
{
    m_inputs.append(new NeuronLink(this));
}

void Neuron::fire()
{
    m_output = 0.0f;

    //neuron output
    for (int i = 0; i < m_inputs.size(); i++) {
        m_output += m_inputs[i]->m_inputValue * m_inputs[i]->m_weight;
    }

    //sigmoid
    m_output = 1.0f / (1.0f + exp(float((-1.0f) * m_output)));

    for (int i = 0; i < m_outputs.size(); i++) {
        m_outputs[i]->m_inputValue = m_output;
    }
}

void Neuron::inputFire()
{
    //input layer normalization
    m_output = m_inputs[0]->m_inputValue * m_inputs[0]->m_weight;

    //sigmoid
//     m_output = 1.0f / (1.0f + exp(float((-1.0f) * m_output)));

//     qDebug() << m_output << "Input:" << m_inputs[0]->m_inputValue << "Weight" << m_inputs[0]->m_weight;

    for (int i = 0; i < m_outputs.size(); i++) {
        m_outputs[i]->m_inputValue = m_output;
    }
}


