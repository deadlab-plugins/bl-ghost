/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  CMAParamSmooth2.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include "CMAParamSmooth2.h"

CMAParamSmooth2::CMAParamSmooth2(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate)
{
    mSmoothers[0] = new CMAParamSmooth(smoothingTimeMs, samplingRate);
    mSmoothers[1] = new CMAParamSmooth(smoothingTimeMs, samplingRate);
}

CMAParamSmooth2::~CMAParamSmooth2()
{
    delete mSmoothers[0];
    delete mSmoothers[1];
}

void
CMAParamSmooth2::Reset(BL_FLOAT samplingRate)
{
    mSmoothers[0]->Reset(samplingRate, 0.0);
    mSmoothers[1]->Reset(samplingRate, 0.0);
}

void
CMAParamSmooth2::Reset(BL_FLOAT samplingRate, BL_FLOAT val)
{
    mSmoothers[0]->Reset(samplingRate, val);
    mSmoothers[1]->Reset(samplingRate, val);
}

void
CMAParamSmooth2::SetSmoothTimeMs(BL_FLOAT smoothingTimeMs)
{
    mSmoothers[0]->SetSmoothTimeMs(smoothingTimeMs);
    mSmoothers[1]->SetSmoothTimeMs(smoothingTimeMs);
}

BL_FLOAT
CMAParamSmooth2::Process(BL_FLOAT inVal)
{
    inVal = mSmoothers[0]->Process(inVal);
    BL_FLOAT result = mSmoothers[1]->Process(inVal);

    return result;
}
