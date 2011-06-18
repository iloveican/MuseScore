//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2009 Werner Schweer and others
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

#include "splitstaff.h"
#include "libmscore/score.h"
#include "libmscore/staff.h"
#include "libmscore/clef.h"
#include "libmscore/measure.h"
#include "libmscore/part.h"
#include "libmscore/note.h"
#include "libmscore/chord.h"
#include "libmscore/bracket.h"
#include "libmscore/system.h"
#include "seq.h"
#include "libmscore/slur.h"
#include "libmscore/segment.h"

//---------------------------------------------------------
//   SplitStaff
//---------------------------------------------------------

SplitStaff::SplitStaff(QWidget* parent)
   : QDialog(parent)
      {
      setupUi(this);
      splitPoint->setValue(60);
      }

//---------------------------------------------------------
//   splitStaff
//---------------------------------------------------------

struct SNote {
      int tick;
      int pitch;
      Fraction fraction;
      Note* note;
      };

