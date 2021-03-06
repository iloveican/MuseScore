//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: pedal.cpp 3592 2010-10-18 17:24:18Z wschweer $
//
//  Copyright (C) 2002-2009 Werner Schweer and others
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

#include "pedal.h"
#include "textline.h"
#include "sym.h"

#include "score.h"

//---------------------------------------------------------
//   Pedal
//---------------------------------------------------------

Pedal::Pedal(Score* s)
   : TextLine(s)
      {
      setBeginSymbol(pedalPedSym);
      setBeginSymbolOffset(QPointF(0.0, -.2));

      setEndHook(true);
      setBeginHookHeight(Spatium(-1.2));
      setEndHookHeight(Spatium(-1.2));

      setYoff(8.0);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Pedal::read(XmlReader* e)
      {
      if (score()->mscVersion() >= 110) {
            setBeginSymbol(-1);
            setEndHook(false);
            }
      TextLine::read(e);
      }
