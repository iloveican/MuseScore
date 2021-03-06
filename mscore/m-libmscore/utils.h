//=============================================================================
//  MuseScore
//  Music Score Editor/Player
//  $Id:$
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __UTILS_H__
#define __UTILS_H__

#include <QtCore/QPointF>

#include "globals.h"
#include "interval.h"

//---------------------------------------------------------
//   cycles
//---------------------------------------------------------

static inline unsigned long long cycles()
      {
      unsigned long long rv;
      __asm__ __volatile__("rdtsc" : "=A" (rv));
      return rv;
      }

class Measure;
class Segment;
class System;
class Element;

extern int getStaff(System* system, const QPointF& p);
extern int pitchKeyAdjust(int note, int key);
extern int line2pitch(int line, int clef, int key);
extern int y2pitch(qreal y, int clef, qreal spatium);
extern int quantizeLen(int, int);
extern qreal curTime();
extern void selectNoteMessage();
extern void selectNoteRestMessage();
extern void selectNoteSlurMessage();
extern void selectStavesMessage();
extern QString pitch2string(int v);
extern void transposeInterval(int pitch, int tpc, int* rpitch, int* rtpc,
   Interval, bool useDoubleSharpsFlats);
extern int transposeTpc(int tpc, Interval interval, bool useDoubleSharpsFlats);

extern Interval intervalList[26];
extern int searchInterval(int steps, int semitones);
extern int chromatic2diatonic(int val);

int diatonicUpDown(int /*clef*/, int pitch, int steps);

extern int version();
extern int majorVersion();
extern int minorVersion();
extern int updateVersion();

extern Segment* nextSeg1(Segment* s, int& track);
extern Segment* prevSeg1(Segment* seg, int& track);

#endif

