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

#ifndef __LINE_H__
#define __LINE_H__

#include "spanner.h"
#include "globals.h"

class SLine;
class System;
class Painter;

//---------------------------------------------------------
//   LineSegment
//    Virtual base class for segmented lines segments
//    (OttavaSegment, HairpinSegment, TrillSegment...)
//
//    This class describes one segment of an segmented
//    line object. Line objects can span multiple staves.
//    For every staff a segment is created.
//---------------------------------------------------------

class LineSegment : public SpannerSegment {
   protected:
      QPointF _p2;
      QPointF _userOff2;            // depends on spatium
      QRectF r1, r2;

      virtual void layout() {}

   public:
      LineSegment(Score* s);
      LineSegment(const LineSegment&);
      virtual LineSegment* clone() const = 0;
      virtual void draw(Painter*) const = 0;
      SLine* line() const                         { return (SLine*)parent(); }
      const QPointF& userOff2() const             { return _userOff2;       }
      void setUserOff2(const QPointF& o)          { _userOff2 = o;          }
      void setUserXoffset2(qreal x)               { _userOff2.setX(x);      }
      void setPos2(const QPointF& p)              { _p2 = p;                }
      QPointF pos2() const                        { return _p2 + _userOff2; }
      virtual QPointF canvasPos() const;

      friend class SLine;
      };

//---------------------------------------------------------
//   SLine
//    virtual base class for Ottava, Pedal, Hairpin,
//    Trill and TextLine
//---------------------------------------------------------

class SLine : public Spanner {
   protected:
      bool _diagonal;

   public:
      SLine(Score* s);
      SLine(const SLine&);

      virtual void layout();
      bool readProperties(XmlReader* node);
      virtual LineSegment* createLineSegment() = 0;
      void setLen(qreal l);
      virtual QRectF bbox() const;

      virtual QPointF tick2pos(int grip, System** system);

      virtual void read(XmlReader*);

      bool diagonal() const         { return _diagonal; }
      void setDiagonal(bool v)      { _diagonal = v;    }
      int tick()  const;
      int tick2() const;

      LineSegment* frontSegment() const { return (LineSegment*)spannerSegments().front(); }
      LineSegment* backSegment() const  { return (LineSegment*)spannerSegments().back();  }
      LineSegment* takeFirstSegment()   { return (LineSegment*)spannerSegments().takeFirst(); }
      LineSegment* takeLastSegment()    { return (LineSegment*)spannerSegments().takeLast(); }
      LineSegment* segmentAt(int n) const { return (LineSegment*)spannerSegments().at(n); }
      };

#endif

