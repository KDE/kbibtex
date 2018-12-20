// krazy:excludeall=copyright,license
/*
Copyright (c) 2011, Andre Somers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Rathenau Instituut, Andre Somers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANDRE SOMERS AND/OR RATHENAU INSTITUTE BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
Found at  http://qt-project.org/wiki/Delay_action_to_wait_for_user_interaction
modified/simplified by Thomas Fischer
*/

#include "delayedexecutiontimer.h"

#include <QTimer>

DelayedExecutionTimer::DelayedExecutionTimer(int maximumDelay, int minimumDelay, QObject *parent):
    QObject(parent),
    m_isEnabled(true),
    m_minimumDelay(minimumDelay),
    m_maximumDelay(maximumDelay),
    m_minimumTimer(new QTimer(this)),
    m_maximumTimer(new QTimer(this))
{
    connect(m_minimumTimer, &QTimer::timeout, this, &DelayedExecutionTimer::timeout);
    connect(m_maximumTimer, &QTimer::timeout, this, &DelayedExecutionTimer::timeout);
}

DelayedExecutionTimer::DelayedExecutionTimer(QObject *parent):
    QObject(parent),
    m_isEnabled(true),
    m_minimumDelay(250),
    m_maximumDelay(1000),
    m_minimumTimer(new QTimer(this)),
    m_maximumTimer(new QTimer(this))
{
    connect(m_minimumTimer, &QTimer::timeout, this, &DelayedExecutionTimer::timeout);
    connect(m_maximumTimer, &QTimer::timeout, this, &DelayedExecutionTimer::timeout);
}

void DelayedExecutionTimer::timeout()
{
    m_minimumTimer->stop();
    m_maximumTimer->stop();
    emit triggered();
}

void DelayedExecutionTimer::trigger()
{
    if (!m_isEnabled) return; /// ignore trigger events if disabled

    if (!m_maximumTimer->isActive()) {
        m_maximumTimer->start(m_maximumDelay);
    }
    m_minimumTimer->stop();
    m_minimumTimer->start(m_minimumDelay);
}
