//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: tuplet.cpp 3552 2010-10-05 14:56:20Z wschweer $
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

#include "tuplet.h"
#include "score.h"
#include "chord.h"
#include "note.h"
#include "m-al/xml.h"
#include "preferences.h"
#include "style.h"
#include "text.h"
#include "element.h"
#include "utils.h"
#include "measure.h"
#include "painter.h"

//---------------------------------------------------------
//   Tuplet
//---------------------------------------------------------

Tuplet::Tuplet(Score* s)
  : DurationElement(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      _numberType   = SHOW_NUMBER;
      _bracketType  = AUTO_BRACKET;
      _number       = 0;
      _hasBracket   = false;
      _userModified = false;
      _isUp         = true;
      _direction    = AUTO;
      }

//---------------------------------------------------------
//   ~Tuplet
//---------------------------------------------------------

Tuplet::~Tuplet()
      {
      //
      // delete all references
      //
      foreach(DurationElement* e, _elements)
            e->setTuplet(0);
      }

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void Tuplet::setSelected(bool f)
      {
      Element::setSelected(f);
      if (_number)
            _number->setSelected(f);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Tuplet::layout()
      {
      if (_elements.empty()) {
//            printf("Tuplet::layout(): tuplet is empty\n");
            return;
            }
      qreal _spatium = spatium();
      if (_numberType != NO_TEXT) {
            if (_number == 0) {
                  _number = new Text(score());
                  _number->setSubtype(TEXT_TUPLET);
                  _number->setTextStyle(TEXT_STYLE_TUPLET);
                  _number->setParent(this);
                  }
            if (_numberType == SHOW_NUMBER)
                  _number->setText(QString("%1").arg(_ratio.numerator()));
            else
                  _number->setText(QString("%1:%2").arg(_ratio.numerator()).arg(_ratio.denominator()));
            }
      else {
            if (_number) {
                  if (_number->selected())
                        score()->deselect(_number);
                  delete _number;
                  _number = 0;
                  }
            }

      if (_elements.empty()) {
//            printf("layout: not tuplet members\n");
            return;
            }

      //
      // find out main direction
      //
      if (_direction == AUTO) {
            int up = 1;
            foreach(const DurationElement* e, _elements) {
                  if (e->type() == CHORD) {
                        const Chord* c = static_cast<const Chord*>(e);
                        if (c->stemDirection() != AUTO)
                              up += c->stemDirection() == UP ? 1000 : -1000;
                        else
                              up += c->up() ? 1 : -1;
                        }
                  else if (e->type() == TUPLET) {
                        // TODO
                        }
                  }
            _isUp = up > 0;
            }
      else
            _isUp = _direction == UP;

      //
      // set all elements to main direction
      //
      bool tupletContainsRest = false;
      foreach(DurationElement* e, _elements) {
            if (e->type() == REST)
                  tupletContainsRest = true;
            }

      const DurationElement* cr1 = _elements.front();
      while (cr1->type() == TUPLET) {
            const Tuplet* t = static_cast<const Tuplet*>(cr1);
            if (t->elements().empty())
                  break;
            cr1 = t->elements().front();
            }
      const DurationElement* cr2 = _elements.back();
      while (cr2->type() == TUPLET) {
            const Tuplet* t = static_cast<const Tuplet*>(cr2);
            if (t->elements().empty())
                  break;
            cr2 = t->elements().back();
            }

      //
      //   shall we draw a bracket?
      //
      if (cr1->beam() && !tupletContainsRest) {
            if (_bracketType == AUTO_BRACKET)
                  _hasBracket = false;
            else
                  _hasBracket = _bracketType == SHOW_BRACKET;
            }
      else
            _hasBracket = _bracketType != SHOW_NO_BRACKET;


      //
      //    calculate bracket start and end point p1 p2
      //
      qreal headDistance = _spatium * .75;
      if (_isUp) {
            p1       = cr1->abbox().topLeft();
            p1.ry() -= headDistance;
            p2       = cr2->abbox().topRight();
            p2.ry() -= headDistance;

            if (cr1->type() == CHORD) {
                  const Chord* chord1 = static_cast<const Chord*>(cr1);
                  Stem* stem = chord1->stem();

                  if (stem && chord1->up()) {
                        p1.setY(stem->abbox().y());
                        if (chord1->beam())
                              p1.setX(stem->abbox().x());
                        }
                  else if ((cr2->type() == CHORD) && stem && !chord1->up()) {
                        const Chord* chord2 = static_cast<const Chord*>(cr2);
                        Stem* stem2 = chord2->stem();
                        if (stem2) {
                              int l1 = chord1->upNote()->line();
                              int l2 = chord2->upNote()->line();
                              p1.ry() = stem2->abbox().top() + _spatium * .5 * (l1 - l2);
                              }
                        }
                  }

            if (cr2->type() == CHORD) {
                  const Chord* chord2 = static_cast<const Chord*>(cr2);
                  Stem* stem = chord2->stem();
                  if (stem && chord2->up()) {
                        p2.setY(stem->abbox().top());
                        if (chord2->beam())
                              p2.setX(stem->abbox().x());
                        }
                  else if ((cr1->type() == CHORD) && stem && !chord2->up()) {
                        const Chord* chord1 = static_cast<const Chord*>(cr1);
                        int l1 = chord1->upNote()->line();
                        int l2 = chord2->upNote()->line();
                        p2.ry() = p1.ry() + _spatium * .5 * (l2 - l1);
                        }
                  }
            //
            // special case: one of the bracket endpoints is
            // a rest
            //
            if (cr1->type() != CHORD && cr2->type() == CHORD) {
                  if (p2.y() < p1.y())
                        p1.setY(p2.y());
                  else
                        p2.setY(p1.y());
                  }
            else if (cr1->type() == CHORD && cr2->type() != CHORD) {
                  if (p1.y() < p2.y())
                        p2.setY(p1.y());
                  else
                        p1.setY(p2.y());
                  }

            // check for collisions

            int n = _elements.size();
            if (n >= 3) {
                  qreal d = (p2.y() - p1.y())/(p2.x() - p1.x());
                  for (int i = 1; i < (n-1); ++i) {
                        Element* e = _elements[i];
                        if (e->type() == CHORD) {
                              const Chord* chord = static_cast<const Chord*>(e);
                              const Stem* stem = chord->stem();
                              if (stem) {
                                    QRectF r(chord->up() ? stem->abbox() : chord->abbox());
                                    qreal y3 = r.top();
                                    qreal x3 = r.x() + r.width() * .5;
                                    qreal y0 = p1.y() + (x3 - p1.x()) * d;
                                    qreal c  = y0 - y3;
                                    if (c > 0) {
                                          p1.ry() -= c;
                                          p2.ry() -= c;
                                          }
                                    }
                              }
                        }
                  }
            }
      else {
            p1 = cr1->abbox().bottomLeft();
            p1.ry() += headDistance;

            if (cr1->type() == CHORD) {
                  const Chord* chord1 = static_cast<const Chord*>(cr1);
                  Stem* stem = chord1->stem();
                  if (stem && !chord1->up()) {
                        p1.setY(stem->abbox().bottom());
                        if (chord1->beam())
                              p1.setX(stem->abbox().x());
                        }
                  else if ((cr2->type() == CHORD) && stem && chord1->up()) {
                        const Chord* chord2 = static_cast<const Chord*>(cr2);
                        Stem* stem2 = chord2->stem();
                        if (stem2) {
                              int l1 = chord1->upNote()->line();
                              int l2 = chord2->upNote()->line();
                              p1.ry() = stem2->abbox().bottom() + _spatium * .5 * (l1 - l2);
                              }
                        }
                  }

            p2 = cr2->abbox().bottomRight();
            p2.ry() += headDistance;

            if (cr2->type() == CHORD) {
                  const Chord* chord2 = static_cast<const Chord*>(cr2);
                  Stem* stem = chord2->stem();
                  if (stem && !chord2->up()) {
                        if (chord2->beam())
                              p2.setX(stem->abbox().x());
                        p2.setY(stem->abbox().bottom());
                        }
                  else if ((cr1->type() == CHORD) && stem && chord2->up()) {
                        const Chord* chord1 = static_cast<const Chord*>(cr1);
                        int l1 = chord1->upNote()->line();
                        int l2 = chord2->upNote()->line();
                        p2.ry() = p1.ry() + _spatium * .5 * (l2 - l1);
                        }
                  }
            if (cr1->type() != CHORD && cr2->type() == CHORD) {
                  if (p2.y() > p1.y())
                        p1.setY(p2.y());
                  else
                        p2.setY(p1.y());
                  }
            else if (cr1->type() == CHORD && cr2->type() != CHORD) {
                  if (p1.y() > p2.y())
                        p2.setY(p1.y());
                  else
                        p1.setY(p2.y());
                  }

            // check for collisions

            int n = _elements.size();
            if (n >= 3) {
                  qreal d  = (p2.y() - p1.y())/(p2.x() - p1.x());
                  for (int i = 1; i < (n-1); ++i) {
                        Element* e = _elements[i];
                        if (e->type() == CHORD) {
                              const Chord* chord = static_cast<const Chord*>(e);
                              const Stem* stem = chord->stem();
                              if (stem) {
                                    QRectF r(chord->up() ? chord->abbox() : stem->abbox());
                                    qreal y3 = r.bottom();
                                    qreal x3 = r.x() + r.width() * .5;
                                    qreal y0 = p1.y() + (x3 - p1.x()) * d;
                                    qreal c  = y0 - y3;
                                    if (c < 0) {
                                          p1.ry() -= c;
                                          p2.ry() -= c;
                                          }
                                    }
                              }
                        }
                  }
            }

      qreal l1 = _spatium;          // bracket tip height
      qreal l2 = _spatium * .5;     // bracket distance to note

      setPos(0.0, 0.0);
      QPointF mp(parent()->canvasPos());
      p1 -= mp;
      p2 -= mp;

      p1 += _p1;
      p2 += _p2;

      // center number
      qreal x3 = 0.0, y3 = 0.0;
      qreal numberWidth = 0.0;
      if (_number) {
            _number->layout();
            x3 = p1.x() + (p2.x() - p1.x()) * .5;

            y3 = p1.y() + (p2.y() - p1.y()) * .5
               - _number->bbox().height() * .5
               - (l1 + l2) * (_isUp ? 1.0 : -1.0);

            numberWidth = _number->bbox().width();
            _number->setPos(QPointF(x3 - numberWidth * .5, y3) - ipos());
            }

      if (_hasBracket) {
            qreal slope = (p2.y() - p1.y()) / (p2.x() - p1.x());

            if (_isUp) {
                  if (_number) {
                        bracketL[0] = QPointF(p1.x(), p1.y() - l2);
                        bracketL[1] = QPointF(p1.x(), p1.y() - l1 - l2);
                        qreal x     = x3 - numberWidth * .5 - _spatium * .5;
                        qreal y     = p1.y() + (x - p1.x()) * slope;
                        bracketL[2] = QPointF(x,   y - l1 - l2);

                        x           = x3 + numberWidth * .5 + _spatium * .5;
                        y           = p1.y() + (x - p1.x()) * slope;
                        bracketR[0] = QPointF(x,   y - l1 - l2);
                        bracketR[1] = QPointF(p2.x(), p2.y() - l1 - l2);
                        bracketR[2] = QPointF(p2.x(), p2.y() - l2);
                        }
                  else {
                        bracketL[0] = QPointF(p1.x(), p1.y() - l2);
                        bracketL[1] = QPointF(p1.x(), p1.y() - l1 - l2);
                        bracketL[2] = QPointF(p2.x(), p2.y() - l1 - l2);
                        bracketL[3] = QPointF(p2.x(), p2.y() - l2);
                        }
                  }
            else {
                  if (_number) {
                        bracketL[0] = QPointF(p1.x(), p1.y() + l2);
                        bracketL[1] = QPointF(p1.x(), p1.y() + l1 + l2);
                        qreal x     = x3 - numberWidth * .5 - _spatium * .5;
                        qreal y     = p1.y() + (x - p1.x()) * slope;
                        bracketL[2] = QPointF(x,   y + l1 + l2);

                        x           = x3 + numberWidth * .5 + _spatium * .5;
                        y           = p1.y() + (x - p1.x()) * slope;
                        bracketR[0] = QPointF(x,   y + l1 + l2);
                        bracketR[1] = QPointF(p2.x(), p2.y() + l1 + l2);
                        bracketR[2] = QPointF(p2.x(), p2.y() + l2);
                        }
                  else {
                        bracketL[0] = QPointF(p1.x(), p1.y() + l2);
                        bracketL[1] = QPointF(p1.x(), p1.y() + l1 + l2);
                        bracketL[2] = QPointF(p2.x(), p2.y() + l1 + l2);
                        bracketL[3] = QPointF(p2.x(), p2.y() + l2);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   bbox
//---------------------------------------------------------

QRectF Tuplet::bbox() const
      {
      QRectF r;
      if (_number) {
            r |= _number->bbox().translated(_number->pos());
            if (_hasBracket) {
                  QRectF b;
                  b.setCoords(bracketL[1].x(), bracketL[1].y(), bracketR[2].x(), bracketR[2].y());
                  r |= b;
                  }
            }
      else if (_hasBracket) {
            QRectF b;
            b.setCoords(bracketL[1].x(), bracketL[1].y(), bracketL[3].x(), bracketL[3].y());
            r |= b;
            }
      return r;
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Tuplet::draw(Painter* p) const
      {
      if (_number) {
            p->save();
            p->translate(_number->pos());
            _number->draw(p);
            p->restore();
            }
      if (_hasBracket) {
            p->setPenWidth(spatium() * .1);
#if 0
            if (!_number)
                  p->drawPolyline(bracketL, 4);
            else {
                  p->drawPolyline(bracketL, 3);
                  p->drawPolyline(bracketR, 3);
                  }
#endif
            }
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Tuplet::read(XmlReader* r, const QList<Tuplet*>& tuplets, const QList<Slur*>&)
      {
      _id    = 0;
      while (r->readAttribute()) {
            if (r->tag() == "id")
                  _id = r->intValue();
            }

      while (r->readElement()) {
            MString8 tag = r->tag();
            QString val;
            int i;

            if (r->readInt("numberType", &_numberType))
                  ;
            else if (r->readInt("bracketType", &_bracketType))
                  ;
            else if (r->readString("baseNote", &val))
                  _baseLen = TimeDuration(val);
            else if (r->readInt("normalNotes", &i))
                  _ratio.setDenominator(i);
            else if (r->readInt("actualNotes", &i))
                  _ratio.setNumerator(i);
            else if (tag == "Number") {
                  _number = new Text(score());
                  _number->setParent(this);
                  _number->read(r);
                  _number->setSubtype(TEXT_TUPLET);   // override read
                  _number->setTextStyle(TEXT_STYLE_TUPLET);
                  }
            else if (r->readString("direction", &val)) {
                  if (val == "up")
                        _direction = UP;
                  else if (val == "down")
                        _direction = DOWN;
                  else
                        _direction = AUTO;
                  }
            else if (r->readPoint("p1", &_p1))
                  _userModified = true;
            else if (r->readPoint("p2", &_p2))
                  _userModified = true;
            else if (r->readInt("Tuplet", &i)) {
                  foreach(Tuplet* t, tuplets) {
                        if (t->id() == i) {
                              setTuplet(t);
                              break;
                              }
                        }
                  if (tuplet() == 0)
                        printf("Tuplet id %d not found\n", i);
                  }
            else if (!Element::readProperties(r))
                  r->unknown();
            }
      Fraction f(_ratio.denominator(), _baseLen.fraction().denominator());
      setDuration(f);
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Tuplet::add(Element* e)
      {
      switch(e->type()) {
            case TEXT:
                  _number = static_cast<Text*>(e);
                  break;
            case CHORD:
            case REST:
            case TUPLET:
                  {
                  int i;
                  for (i = 0; i < _elements.size(); ++i) {
                        DurationElement* de = static_cast<DurationElement*>(e);
                        if (_elements[i]->tick() > de->tick()) {
                              _elements.insert(i, de);
                              break;
                              }
                        }
                  if (i == _elements.size())
                        _elements.append(static_cast<DurationElement*>(e));
                  }

                  // the tick position of a tuplet is the tick position of its
                  // first element:
                  setTick(_elements.front()->tick());
                  break;
            default:
//                  printf("Tuplet::add() unknown element\n");
                  break;
            }
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Tuplet::remove(Element* e)
      {
      switch(e->type()) {
            case TEXT:
                  if (e == _number)
                        _number = 0;
                  break;
            case CHORD:
            case REST:
            case TUPLET:
                  if (!_elements.removeOne(static_cast<DurationElement*>(e))) {
//                        printf("Tuplet::remove: cannot find element\n");
//                        printf("  elements %d\n", _elements.size());
                        }
                  break;
            default:
//                  printf("Tuplet::remove: unknown element\n");
                  break;
            }
      }

//---------------------------------------------------------
//   setTrack
//---------------------------------------------------------

void Tuplet::setTrack(int val)
      {
      Element::setTrack(val);
      }

