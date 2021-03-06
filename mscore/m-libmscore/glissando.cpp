//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: glissando.cpp 3537 2010-10-01 10:52:51Z wschweer $
//
//  Copyright (C) 2008 Werner Schweer and others
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

#include "glissando.h"
#include "chord.h"
#include "segment.h"
#include "note.h"
#include "style.h"
#include "score.h"
#include "sym.h"
#include "m-al/xml.h"
#include "painter.h"
#include "font.h"

//---------------------------------------------------------
//   Glissando
//---------------------------------------------------------

Glissando::Glissando(Score* s)
  : Element(s)
      {
      setFlags(ELEMENT_MOVABLE | ELEMENT_SELECTABLE);
      _text = "gliss.";
      _showText = true;
      qreal _spatium = spatium();
      setSize(QSizeF(_spatium * 2, _spatium * 4));    // for use in palettes
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void Glissando::layout()
      {
      Chord* chord = static_cast<Chord*>(parent());
      if (chord == 0)
            return;
      Note* anchor2   = chord->upNote();
      Segment* s = chord->segment();
      s = s->prev1();
      while (s) {
            if ((s->subtype() == SegChordRest || s->subtype() == SegGrace) && s->element(track()))
                  break;
            s = s->prev1();
            }
      if (s == 0) {
            printf("no segment for first note of glissando found\n");
            return;
            }
      ChordRest* cr = static_cast<ChordRest*>(s->element(track()));
      if (cr == 0 || cr->type() != CHORD) {
            printf("no first note for glissando found, track %d\n", track());
            return;
            }
      Note* anchor1 = static_cast<Chord*>(cr)->upNote();

      setPos(0.0, 0.0);

      QPointF cp1    = anchor1->canvasPos();
      QPointF cp2    = anchor2->canvasPos();

      // construct line from notehead to notehead
      qreal x1 = (anchor1->headWidth()) - (cp2.x() - cp1.x());
      qreal y1 = anchor1->pos().y();
      qreal x2 = anchor2->pos().x();
      qreal y2 = anchor2->pos().y();
      QLineF fullLine(x1, y1, x2, y2);

      // shorten line on each side by offsets
      qreal xo = spatium() * .5;
      qreal yo = xo;   // spatium() * .5;
      QPointF p1 = fullLine.pointAt(xo / fullLine.length());
      QPointF p2 = fullLine.pointAt(1 - (yo / fullLine.length()));

      line = QLineF(p1, p2);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Glissando::read(XmlReader* r)
      {
      _showText = false;
      while (r->readElement()) {
            if (r->readString("text", &_text)) {
                  _showText = true;
                  }
            else if (!Element::readProperties(r))
                  r->unknown();
            }
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Glissando::draw(Painter* p) const
      {
      qreal _spatium = spatium();

      p->setLineCap(Qt::RoundCap);
      p->setPenWidth(_spatium * .15);

      qreal w = line.dx();
      qreal h = line.dy();

      qreal l = sqrt(w * w + h * h);
      p->translate(line.p1());
      qreal wi = asin(-h / l) * 180.0 / M_PI;
      p->rotate(-wi);

      if (subtype() == 0) {
            p->drawLine(0.0, 0.0, l, 0.0);
            }
      else if (subtype() == 1) {
            qreal mags = magS();
            QRectF b = symbols[score()->symIdx()][trillelementSym].bbox(mags);
            qreal w  = symbols[score()->symIdx()][trillelementSym].width(mags);
            int n    = lrint(l / w);
            symbols[score()->symIdx()][trillelementSym].draw(p, mags, 0.0, b.height()*.5, n);
            }
      if (_showText) {
            const TextStyle& st = score()->textStyle(TEXT_STYLE_GLISSANDO);
            Font f = st.fontPx(_spatium);
            QRectF r = FontMetricsF(f).boundingRect(_text);
            if (r.width() < l) {
                  Font f = st.fontPx(_spatium);
                  qreal x = (l - r.width()) * .5;
                  p->drawText(f, QPointF(x, -_spatium * .5), _text);
                  }
            }
      }

//---------------------------------------------------------
//   space
//---------------------------------------------------------

Space Glissando::space() const
      {
      return Space(0.0, spatium() * 2.0);
      }

//---------------------------------------------------------
//   bbox
//---------------------------------------------------------

QRectF Glissando::bbox() const
      {
      return QRectF(line.p1(), line.p2()).normalized();
      }

//---------------------------------------------------------
//   setSize
//    used for palette
//---------------------------------------------------------

void Glissando::setSize(const QSizeF& s)
      {
      line = QLineF(0.0, s.height(), s.width(), 0.0);
      }

