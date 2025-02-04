/*
    SPDX-FileCopyrightText: 2022 Toni Schriber

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/******************************************************************************************************
* Angle calculations are based on position measurements of
* - Rotator angle in "Circular Angle (A)" mode (0 <> 359.99° CCW)
* - Camera offset angle & Camera position angle in "Position Angle (PA)" mode (180 <> -179.99° CCW)
* This leads to the following calculations:
* - Camera PA = calcCameraAngel(Rotator A)
* - Rotator A = calcRotatorAngle(Camera PA)
* - Camera offset PA = calcOffsetAngle(Rotator A, Camera PA)
*******************************************************************************************************/

#include "rotatorutils.h"
#include "Options.h"

#include "opticaltrainmanager.h"

#include <indicom.h>
#include <basedevice.h>
#include <cmath>

RotatorUtils * RotatorUtils::m_Instance = nullptr;

RotatorUtils * RotatorUtils::Instance()
{
    if (m_Instance)
        return m_Instance;

    m_Instance = new RotatorUtils();
    return m_Instance;
}

void RotatorUtils::release()
{
    delete (m_Instance);
    m_Instance = nullptr;
}

RotatorUtils::RotatorUtils() {}

RotatorUtils::~RotatorUtils() {}

void RotatorUtils::initRotatorUtils(const QString &train)
{
    m_Offset = Options::pAOffset();
    m_Mount = Ekos::OpticalTrainManager::Instance()->getMount(train);

    if (m_Mount)
    {
        connect(m_Mount, &ISD::Mount::pierSideChanged, this, [this] (ISD::Mount::PierSide Side)
        {
            m_flippedMount = (Side != m_CalPierside);
            emit changedPierside(Side);
        });
    }
}

double RotatorUtils::calcRotatorAngle(double PositionAngle)
{
    if (m_flippedMount)
    {
        PositionAngle += 180;
    }
    return KSUtils::range360(PositionAngle - m_Offset);
}

double RotatorUtils::calcCameraAngle(double RotatorAngle, bool flippedImage)
{
    double PositionAngle = 0;
    if (RotatorAngle > 180)
    {
        PositionAngle = (RotatorAngle - 360) + m_Offset;
    }
    else
    {
        PositionAngle = RotatorAngle + m_Offset;
    }
    if (!m_flippedMount != !flippedImage) // XOR
    {
        if (PositionAngle > 0)
        {
            PositionAngle -= 180;
        }
        else
        {
            PositionAngle += 180;
        }

    }
    return KSUtils::rangePA(PositionAngle);
}

double RotatorUtils::calcOffsetAngle(double RotatorAngle, double PositionAngle)
{
    double OffsetAngle = 0;
    if (RotatorAngle > 180)
    {
        OffsetAngle = PositionAngle - (RotatorAngle - 360);
    }
    else
    {
        OffsetAngle = PositionAngle - RotatorAngle;
    }
    if (m_flippedMount)
    {
        OffsetAngle -= 180;
    }
    return KSUtils::rangePA(OffsetAngle);
}

void RotatorUtils::updateOffset(double Angle)
{
    m_Offset = Angle;
    Options::setPAOffset(Angle);
}

ISD::Mount::PierSide RotatorUtils::getMountPierside()
{
    return(m_Mount->pierSide());
}

void RotatorUtils::setImagePierside(ISD::Mount::PierSide ImgPierside)
{
    m_ImgPierside = ImgPierside;
}

bool RotatorUtils::checkImageFlip()
{
    bool flipped = false;

    if (m_ImgPierside != ISD::Mount::PIER_UNKNOWN)
        if (!m_flippedMount != (m_ImgPierside == m_CalPierside)) // XOR
            flipped = true;
    return flipped;
}

double RotatorUtils::DiffPA(double diff)
{
    if (diff > 180)
        return (360 - diff);
    else
        return diff;
}
