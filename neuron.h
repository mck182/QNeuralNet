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

#ifndef NEURON_H
#define NEURON_H
#include <QList>

class Neuron;

class NeuronLink {
    friend class Neuron;
public:
    /**
     * Neuron link constructor;
     * @param input - the input neuron in the link
     * @param output - the output neuron in the link; the output layer has no outputs
     * @param inputValue - initial value on the input
     * @param weight - initial weight of the link
     */
    NeuronLink(Neuron *input, Neuron *output = 0, float inputValue = 1.0f, float weight = 0.0f);
    ~NeuronLink();

    void setWeight(float weight) {m_weight = weight;}       ///Sets the weight for the link
    float weight() {return m_weight;}                       ///Gets the current weight

    float inputValue() {return m_inputValue;}               ///Value on the link's input
    void setInputValue(float iv) {m_inputValue = iv;}       ///Sets the value on the link's input

    float prevWeight() {return m_prevWeight;}               ///Previous link weight
    void setPrevWeight(float w) { m_prevWeight = w;}        ///Sets previous link weight

    Neuron *inputNeuron() {return m_inputNeuron;}           ///The neuron connected on the input

private:
    Neuron *m_inputNeuron;
    Neuron *m_outputNeuron;

    float m_iadd;
    float m_prevWeight;

    float m_weight;
    float m_inputValue;
};

//----------------------------------------------------------------------

class Neuron
{

public:
    Neuron();
    virtual ~Neuron();

    void addInputLink(Neuron *poutn = 0);   ///Adds input link to the neuron
    void addBias();                         ///Adds bias
    void inputFire();                       ///Processes the numbers on input, only for input layer
    void fire();                            ///Processes inputs and sets the result to outputs
    float output() {return m_output;}       ///Neuron's output

    void setDelta(float delta) {m_delta = delta;}   ///Delta setter
    float delta() {return m_delta;}                 ///Neuron's delta

    QList<NeuronLink *> m_inputs;           ///Neuron's connected inputs
    QList<NeuronLink *> m_outputs;          ///Neuron's connected outputs

private:
    float m_delta;
    float m_output;
};

#endif // NEURON_H
