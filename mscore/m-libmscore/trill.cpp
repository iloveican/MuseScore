//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: trill.cpp 3592 2010-10-18 17:24:18Z wschweer $
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

#include <math.h>

#include "trill.h"
#include "style.h"
#include "system.h"
#include "measure.h"
#include "m-al/xml.h"
#include "utils.h"
#include "sym.h"
#include "score.h"
#include "accidental.h"
#include "segment.h"
#include "painter.h"

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TrillSegment::draw(Painter* p) const
      {
      qreal mag = magS();
      int idx    = score()->symIdx();
      qreal w2   = symbols[idx][trillelementSym].width(mag);
      QRectF b2(symbols[idx][trillelementSym].bbox(mag));

      if (spannerSegmentType() == SEGMENT_SINGLE || spannerSegmentType() == SEGMENT_BEGIN) {
            QRectF b1(symbols[idx][trillSym].bbox(mag));
            QRectF b2(symbols[idx][trillelementSym].bbox(mag));

            qreal x0   = -b1.x();
            qreal x1   = x0 + b1.width();
            qreal x2   = pos2().x();
            int n      = int(floor((x2-x1) / w2));

            symbols[idx][trillSym].draw(p, mag, x0, 0.0);
            symbols[idx][trillelementSym].draw(p, mag,  x1, b2.y() * .9, n);

            if (trill()->accidental()) {
                  p->save();
                  QPointF o(trill()->accidental()->canvasPos());
                  p->translate(o);
                  trill()->accidental()->draw(p);
                  p->restore();
                  }
            }
      else {
            qreal x1 = 0.0;
            qreal x2 = pos2().x();
            int n = int(floor((x2-x1) / w2));
            symbols[idx][trillelementSym].draw(p, mag,  x1, b2.y() * .9, n);
            }
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void TrillSegment::layout()
      {
      qreal mag = magS();
      int idx    = score()->symIdx();
      QRectF b1(symbols[idx][trillSym].bbox(mag));
      QRectF rr(b1.translated(-b1.x(), 0.0));
      rr |= QRectF(0.0, rr.y(), pos2().x(), rr.height());
      if (spannerSegmentType() == SEGMENT_SINGLE || spannerSegmentType() == SEGMENT_BEGIN) {
            if (trill()->accidental()) {
                  rr |= trill()->accidental()->bbox().translated(trill()->accidental()->pos());
                  }
            }
      setbbox(rr);
      }

//---------------------------------------------------------
//   Trill
//---------------------------------------------------------

Trill::Trill(Score* s)
  : SLine(s)
      {
      _accidental = 0;
      setLen(spatium() * 7);   // for use in palettes
      setYoff(-1.0);    // default position
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Trill::add(Element* e)
      {
      if (e->type() == ACCIDENTAL)
            _accidental = static_cast<Accidental*>(e);
      else
            SLine::add(e);
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Trill::remove(Element* e)
      {
      if (e->type() == ACCIDENTAL)
            _accidental = 0;
      else
            SLine::remove(e);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Trill::layout()
      {
      qreal _spatium = spatium();
      setPos(0.0, yoff() * _spatium);
      SLine::layout();

      //
      // special case:
      // if end segment is first chord/rest segment in measure,
      // shorten trill line so it ends at end of previous measure
      //
      Segment* seg1  = static_cast<Segment*>(startElement());
      Segment* seg2  = static_cast<Segment*>(endElement());
      if (seg2
         && (seg1->system() == seg2->system())
         && (spannerSegments().size() == 1)
         && (seg2->tick() == seg2->measure()->tick())
         ) {
            qreal x1   = seg2->canvasPos().x();
            Measure* m = seg2->measure()->prevMeasure();
            if (m) {
                  Segment* s2 = m->last();
                  qreal x2 = s2->canvasPos().x();
                  qreal dx = x1 - x2 + _spatium * .3;
                  TrillSegment* ls = static_cast<TrillSegment*>(frontSegment());
                  ls->setPos2(ls->pos2() + QPointF(-dx, 0.0));
                  ls->layout();
                  }
            }

      if (_accidental) {
            _accidental->setMag(.6);
            _accidental->layout();
            _accidental->setPos(_spatium*1.3, -2.2*_spatium);
            }
      }

//---------------------------------------------------------
//   createLineSegment
//---------------------------------------------------------

LineSegment* Trill::createLineSegment()
      {
      TrillSegment* seg = new TrillSegment(score());
      seg->setTrack(track());
      return seg;
      }

//---------------------------------------------------------
//   Trill::read
//---------------------------------------------------------

void Trill::read(XmlReader* r)
      {
      foreach(SpannerSegment* seg, spannerSegments())
            delete seg;
      spannerSegments().clear();
      int id = -1;
      while (r->readAttribute()) {
            if (r->tag() == "id")
                  id = r->intValue();
            }
      setId(id);
      while (r->readElement()) {
            if (r->tag() == "Accidental") {
                  _accidental = new Accidental(score());
                  _accidental->read(r);
                  }
            else if (!SLine::readProperties(r))
                  r->unknown();
            }
      }

