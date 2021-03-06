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
 
#ifndef SIMPLE_INPAINT_POLAR3_H
#define SIMPLE_INPAINT_POLAR3_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Very simple, but no so bad on real cases!
// Interpolate phases also, to avoid phases issues
// See: https://cradpdf.drdc-rddc.gc.ca/PDFS/unc341/p811241_A1b.pdf
// and: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
//
class SimpleInpaintPolar3
{
 public:
    SimpleInpaintPolar3(bool processHoriz, bool processVert);
    virtual ~SimpleInpaintPolar3();

    void Process(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 int width, int height);
    
 protected:
    // Magns
    void ProcessMagnsHoriz(WDL_TypedBuf<BL_FLOAT> *magns, int width, int height);
    void ProcessMagnsVert(WDL_TypedBuf<BL_FLOAT> *magns, int width, int height);

    // "both", magns + phases
    void ProcessBothDir(WDL_TypedBuf<BL_FLOAT> *magns,
                        WDL_TypedBuf<BL_FLOAT> *phases,
                        int width, int height);
    
    // Phases, horizontal
    //
    
    //
    void ProcessPhasesHorizLToR(WDL_TypedBuf<BL_FLOAT> *phases,
                                int width, int height);

    // TEST
    // From right to left
    void ProcessPhasesHorizRToL(WDL_TypedBuf<BL_FLOAT> *phases,
                                int width, int height);

    // Combined, L -> R or R -> L (choose the best)
    void ProcessPhasesHorizCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    WDL_TypedBuf<BL_FLOAT> *phases,
                                    int width, int height);

    // Method 3: take the bigger magn to find the direction
    // for each line => very good!
    void CombinePhaseHorizBiggestMagn(const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                      const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                      const WDL_TypedBuf<BL_FLOAT> &magns,
                                      WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                      int width, int height);

    // Phases, vertical
    //

    // Very good
    void ProcessPhasesVertDToU(WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height);

    
    void ProcessPhasesVertUToD(WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height);

    // Combined, D -> U or U -> D (choose the best)
    void ProcessPhasesVertCombined(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   WDL_TypedBuf<BL_FLOAT> *phases,
                                   int width, int height);
    
    void CombinePhaseVertBiggestMagn(const WDL_TypedBuf<BL_FLOAT> &phasesD,
                                     const WDL_TypedBuf<BL_FLOAT> &phasesU,
                                     const WDL_TypedBuf<BL_FLOAT> &magns,
                                     WDL_TypedBuf<BL_FLOAT> *phasesResult,
                                     int width, int height);
    
    // 1 line, all the frequencies
    static void DBG_DumpPhaseCol(const char *fileName, int timeIndex,
                                 WDL_TypedBuf<BL_FLOAT> *phases,
                                 int width, int height);

    // 1 line, all the times
    static void DBG_DumpPhaseLine(const char *fileName, int freqIndex,
                                  WDL_TypedBuf<BL_FLOAT> *phases,
                                  int width, int height);
    
    static void DBG_DumpSignal(const char *fileName,
                               WDL_TypedBuf<BL_FLOAT> *magns,
                               WDL_TypedBuf<BL_FLOAT> *phases,
                               int width, int height,
                               int index);
    
    //
    bool mProcessHoriz;
    bool mProcessVert;
};

#endif
