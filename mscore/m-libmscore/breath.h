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

#ifndef __BREATH_H__
#define __BREATH_H__

#include "element.h"

class Painter;

//---------------------------------------------------------
//   Breath
//    subtype() is index in symList
//---------------------------------------------------------

class Breath : public Element {
      static const int breathSymbols = 4;
      static int symList[breathSymbols];

      Segment* segment() const         { return (Segment*)parent(); }

   public:
      Breath(Score* s);
      virtual Breath* clone() const { return new Breath(*this); }
      virtual ElementType type() const { return BREATH; }
      virtual Space space() const;

      virtual void draw(Painter*) const;
      virtual void layout();
      virtual void read(XmlReader*);
      virtual QPointF canvasPos() const;      ///< position in canvas coordinates
      };

#endif

