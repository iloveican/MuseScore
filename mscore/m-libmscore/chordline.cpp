//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id:$
//
//  Copyright (C) 2010 Werner Schweer and others
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

#include "chordline.h"
#include "m-al/xml.h"
#include "chord.h"
#include "measure.h"
#include "system.h"
#include "note.h"
#include "painter.h"

//---------------------------------------------------------
//   ChordLine
//---------------------------------------------------------

ChordLine::ChordLine(Score* s)
   : Element(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      modified = false;
      }

ChordLine::ChordLine(const ChordLine& cl)
   : Element(cl)
      {
//      path     = cl.path;
      modified = cl.modified;
      }

//---------------------------------------------------------
//   setSubtype
//---------------------------------------------------------

void ChordLine::setSubtype(int st)
      {
      qreal x2, y2;
      switch(st) {
            case 0:
                  break;
            case 1:                 // fall
                  x2 = 2;
                  y2 = 2;
                  break;
            default:
            case 2:                 // doit
                  x2 = 2;
                  y2 = -2;
                  break;
            }
      if (st) {
//            path = QPainterPath();
//            path.cubicTo(x2/2, 0.0, x2, y2/2, x2, y2);
            }
      Element::setSubtype(st);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void ChordLine::layout()
      {
      qreal _spatium = spatium();
      if (parent()) {
            Note* note = chord()->upNote();
            QPointF p(note->pos());
            setPos(p.x() + note->headWidth() + _spatium * .2, p.y());
            }
      else
            setPos(0.0, 0.0);
//      QRectF r(path.boundingRect());
//      setbbox(QRectF(r.x() * _spatium, r.y() * _spatium, r.width() * _spatium, r.height() * _spatium));
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void ChordLine::read(XmlReader*)
      {
#if 0
      path = QPainterPath();
      while (r->readElement()) {
            if (r->tag() == "Path") {
                  path = QPainterPath();
                  QPointF curveTo;
                  QPointF p1;
                  int state;
                  while (r->readElement()) {
                        QString tag(ee.tagName());
                        if (tag == "Element") {
                              int type = ee.attribute("type").toInt();
                              qreal x = ee.attribute("x").toDouble();
                              qreal y = ee.attribute("y").toDouble();
                              switch(QPainterPath::ElementType(type)) {
                                    case QPainterPath::MoveToElement:
                                          path.moveTo(x, y);
                                          break;
                                    case QPainterPath::LineToElement:
                                          path.lineTo(x, y);
                                          break;
                                    case QPainterPath::CurveToElement:
                                          curveTo.rx() = x;
                                          curveTo.ry() = y;
                                          state = 1;
                                          break;
                                    case QPainterPath::CurveToDataElement:
                                          if (state == 1) {
                                                p1.rx() = x;
                                                p1.ry() = y;
                                                state = 2;
                                                }
                                          else if (state == 2) {
                                                path.cubicTo(curveTo, p1, QPointF(x, y));
                                                }
                                          break;
                                    }
                              }
                        else
                              AL::domError(ee);
                        }
                  modified = true;
                  setSubtype(0);
                  }
            else if (!Element::readProperties(r))
                  r->unknown();
            }
#endif
      }

//---------------------------------------------------------
//   Symbol::draw
//---------------------------------------------------------

void ChordLine::draw(Painter* p) const
      {
      qreal _spatium = spatium();
      p->scale(_spatium, _spatium);
      qreal lw = 0.15;
      p->setLineCap(Qt::RoundCap);
      p->setLineJoin(Qt::RoundJoin);
      p->setPenWidth(lw);
//TODO      p->setBrush(Qt::NoBrush);
//      p->drawPath(path);
      }

