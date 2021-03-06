//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: breath.cpp 3266 2010-07-09 07:29:03Z wschweer $
//
//  Copyright (C) 2002-2007 Werner Schweer and others
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

#include "breath.h"
#include "sym.h"
#include "system.h"
#include "segment.h"
#include "measure.h"
#include "score.h"
#include "m-al/xml.h"
#include "painter.h"

int Breath::symList[Breath::breathSymbols] = {
      rcommaSym,
      lcommaSym,
      caesuraCurvedSym,
      caesuraStraight
      };

//---------------------------------------------------------
//   Breath
//---------------------------------------------------------

Breath::Breath(Score* s)
  : Element(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Breath::layout()
      {
      _bbox = symbols[score()->symIdx()][symList[subtype()]].bbox(mag());
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Breath::read(XmlReader* r)
      {
      while(r->readElement()) {
            if (!Element::readProperties(r))
                  r->unknown();
            }
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Breath::draw(Painter* p) const
      {
      symbols[score()->symIdx()][symList[subtype()]].draw(p, mag());
      }

//---------------------------------------------------------
//   space
//---------------------------------------------------------

Space Breath::space() const
      {
      return Space(0.0, spatium() * 1.5);
      }

//---------------------------------------------------------
//   canvasPos
//---------------------------------------------------------

QPointF Breath::canvasPos() const
      {
      if (parent() == 0)
            return pos();
      qreal xp = x();
      for (Element* e = parent(); e; e = e->parent())
            xp += e->x();
      System* system = segment()->measure()->system();
      qreal yp = y();
      if (system)
            yp += system->staff(staffIdx())->y() + system->y();
      return QPointF(xp, yp);
      }


