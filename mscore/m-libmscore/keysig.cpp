//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: keysig.cpp 3583 2010-10-14 09:59:28Z wschweer $
//
//  Copyright (C) 2002-2010 Werner Schweer and others
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

#include "sym.h"
#include "staff.h"
#include "clef.h"
#include "keysig.h"
#include "system.h"
#include "segment.h"
#include "measure.h"
#include "score.h"
#include "m-al/xml.h"
#include "painter.h"

const char* keyNames[15] = {
      QT_TRANSLATE_NOOP("MuseScore", "G major, E minor"),
      QT_TRANSLATE_NOOP("MuseScore", "Cb major, Ab minor"),
      QT_TRANSLATE_NOOP("MuseScore", "D major, B minor"),
      QT_TRANSLATE_NOOP("MuseScore", "Gb major, Eb minor"),
      QT_TRANSLATE_NOOP("MuseScore", "A major, F# minor"),
      QT_TRANSLATE_NOOP("MuseScore", "Db major, Bb minor"),
      QT_TRANSLATE_NOOP("MuseScore", "E major, C# minor"),
      QT_TRANSLATE_NOOP("MuseScore", "Ab major, F minor"),
      QT_TRANSLATE_NOOP("MuseScore", "B major, G# minor"),
      QT_TRANSLATE_NOOP("MuseScore", "Eb major, C minor"),
      QT_TRANSLATE_NOOP("MuseScore", "F# major, D# minor"),
      QT_TRANSLATE_NOOP("MuseScore", "Bb major, G minor"),
      QT_TRANSLATE_NOOP("MuseScore", "C# major, A# minor"),
      QT_TRANSLATE_NOOP("MuseScore", "F major,  D minor"),
      QT_TRANSLATE_NOOP("MuseScore", "C major, A minor")
      };

//---------------------------------------------------------
//   KeySig
//---------------------------------------------------------

KeySig::KeySig(Score* s)
  : Element(s)
      {
      _showCourtesySig = true;
	_showNaturals = true;
      }

KeySig::KeySig(const KeySig& k)
   : Element(k)
      {
	_showCourtesySig = k.showCourtesySig();
	_showNaturals = k.showNaturals();
	foreach(KeySym* ks, k.keySymbols)
            keySymbols.append(new KeySym(*ks));
      _sig = k._sig;
      }

//---------------------------------------------------------
//   canvasPos
//---------------------------------------------------------

QPointF KeySig::canvasPos() const
      {
      if (parent() == 0)
            return pos();
      qreal xp = x();
      for (Element* e = parent(); e; e = e->parent())
            xp += e->x();
      qreal yp = y();
      System* system = segment()->measure()->system();
      if (system)
            yp += system->staff(staffIdx())->y() + system->y();
      return QPointF(xp, yp);
      }

//---------------------------------------------------------
//   setCustom
//---------------------------------------------------------

void KeySig::setCustom(const QList<KeySym*>& symbols)
      {
      _sig.setCustomType(0);
      keySymbols = symbols;
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void KeySig::addLayout(int sym, qreal x, int line)
      {
      KeySym* ks = new KeySym;
      ks->sym    = sym;
      ks->spos   = QPointF(x, qreal(line) * .5);
      keySymbols.append(ks);
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void KeySig::layout()
      {
      qreal _spatium = spatium();
      _bbox           = QRectF(0, 0, 0, 0);

      if (isCustom()) {
            foreach(KeySym* ks, keySymbols) {
                  ks->pos = ks->spos * _spatium;
                  _bbox |= symbols[score()->symIdx()][ks->sym].bbox(magS()).translated(ks->pos);
                  }
            return;
            }

      foreach(KeySym* ks, keySymbols)
            delete ks;
      keySymbols.clear();

      int yoff = 0;
      int clef = 0;
      if (staff()) {
            clef = staff()->clefList()->clef(tick());
            yoff = clefTable[clef].yOffset;
            }

      char t1  = _sig.accidentalType();
      char t2  = _sig.naturalType();
      qreal xo = 0.0;

      int accidentals = 0, naturals = 0;
      switch(qAbs(t1)) {
            case 7: accidentals = 0x7f; break;
            case 6: accidentals = 0x3f; break;
            case 5: accidentals = 0x1f; break;
            case 4: accidentals = 0xf;  break;
            case 3: accidentals = 0x7;  break;
            case 2: accidentals = 0x3;  break;
            case 1: accidentals = 0x1;  break;
            case 0: accidentals = 0;    break;
            default:
                  printf("illegal t1 key %d (t2=%d)\n", t1, t2);
                  break;
            }
      switch(qAbs(t2)) {
            case 7: naturals = 0x7f; break;
            case 6: naturals = 0x3f; break;
            case 5: naturals = 0x1f; break;
            case 4: naturals = 0xf;  break;
            case 3: naturals = 0x7;  break;
            case 2: naturals = 0x3;  break;
            case 1: naturals = 0x1;  break;
            case 0: naturals = 0;    break;
            default:
                  printf("illegal t2 key %d (t1=%d)\n", t2, t1);
                  break;
            }

      xo = 0.0;
      int coffset = t2 < 0 ? 7 : 0;

      if (!((t1 > 0) ^ (t2 > 0)))
            naturals &= ~accidentals;

      if(_showNaturals) {
	      for (int i = 0; i < 7; ++i) {
                  if (naturals & (1 << i)) {
                        addLayout(naturalSym, xo, clefTable[clef].lines[i + coffset]);
				xo += 1.0;
				}
                  }
            }
      switch(t1) {
            case 7:  addLayout(sharpSym, xo + 6.0, clefTable[clef].lines[6]);
            case 6:  addLayout(sharpSym, xo + 5.0, clefTable[clef].lines[5]);
            case 5:  addLayout(sharpSym, xo + 4.0, clefTable[clef].lines[4]);
            case 4:  addLayout(sharpSym, xo + 3.0, clefTable[clef].lines[3]);
            case 3:  addLayout(sharpSym, xo + 2.0, clefTable[clef].lines[2]);
            case 2:  addLayout(sharpSym, xo + 1.0, clefTable[clef].lines[1]);
            case 1:  addLayout(sharpSym, xo + 0.0, clefTable[clef].lines[0]);
                     break;
            case -7: addLayout(flatSym, xo + 6.0, clefTable[clef].lines[13]);
            case -6: addLayout(flatSym, xo + 5.0, clefTable[clef].lines[12]);
            case -5: addLayout(flatSym, xo + 4.0, clefTable[clef].lines[11]);
            case -4: addLayout(flatSym, xo + 3.0, clefTable[clef].lines[10]);
            case -3: addLayout(flatSym, xo + 2.0, clefTable[clef].lines[9]);
            case -2: addLayout(flatSym, xo + 1.0, clefTable[clef].lines[8]);
            case -1: addLayout(flatSym, xo + 0.0, clefTable[clef].lines[7]);
            case 0:
                  break;
            default:
                  printf("illegal t1 key %d (t2=%d)\n", t1, t2);
                  break;
            }
      foreach(KeySym* ks, keySymbols) {
            ks->pos = ks->spos * _spatium;
            _bbox |= symbols[score()->symIdx()][ks->sym].bbox(magS()).translated(ks->pos);
            }
      }

//---------------------------------------------------------
//   set
//---------------------------------------------------------

void KeySig::draw(Painter* p) const
      {
      foreach(const KeySym* ks, keySymbols)
            symbols[score()->symIdx()][ks->sym].draw(p, magS(), ks->pos.x(), ks->pos.y());
      }

//---------------------------------------------------------
//   setSig
//---------------------------------------------------------

void KeySig::setSig(int old, int newSig)
      {
      KeySigEvent ks;
      ks.setNaturalType(old);
      ks.setAccidentalType(newSig);
      setKeySigEvent(ks);
      }

//---------------------------------------------------------
//   setOldSig
//---------------------------------------------------------

void KeySig::setOldSig(int old)
      {
      _sig.setNaturalType(old);
      }

//---------------------------------------------------------
//   space
//---------------------------------------------------------

Space KeySig::space() const
      {
      return Space(point(score()->styleS(ST_keysigLeftMargin)), width());
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void KeySig::read(XmlReader* r)
      {
      _sig = KeySigEvent();   // invalidate _sig

      while (r->readElement()) {
            MString8 tag = r->tag();
            int i;
            QString val;
            if (tag == "KeySym") {
                  KeySym* ks = new KeySym;
                  while (r->readElement()) {
                        tag = r->tag();
                        if (r->readInt("sym", &ks->sym))
                              ;
                        else if (r->readPoint("pos", &ks->spos))
                              ;
                        else
                              r->unknown();
                        }
                  keySymbols.append(ks);
                  }
            else if (r->readBool("showCourtesySig", &_showCourtesySig))
		      ;
            else if (r->readBool("showNaturals", &_showNaturals))
		      ;
            else if (r->readInt("accidental", &i))
                  _sig.setAccidentalType(i);
            else if (r->readInt("natural", &i))
                  _sig.setNaturalType(i);
            else if (r->readInt("custom", &i))
                  _sig.setCustomType(i);
            else if (!Element::readProperties(r))
                  r->unknown();
            }
      if (_sig.invalid() && subtype()) {
            _sig.initFromSubtype(subtype());     // for backward compatibility
            }
      }

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool KeySig::operator==(const KeySig& k) const
      {
      bool ct1 = customType() != 0;
      bool ct2 = k.customType() != 0;
      if (ct1 != ct2)
            return false;

      if (ct1) {
            int n = keySymbols.size();
            if (n != k.keySymbols.size())
                  return false;
            for (int i = 0; i < n; ++i) {
                  KeySym* ks1 = keySymbols[i];
                  KeySym* ks2 = k.keySymbols[i];
                  if (ks1->sym != ks2->sym)
                        return false;
                  if (ks1->spos != ks2->spos)
                        return false;
                  }
            return true;
            }
      return _sig == k._sig;
      }

//---------------------------------------------------------
//   changeKeySigEvent
//---------------------------------------------------------

void KeySig::changeKeySigEvent(const KeySigEvent& t)
      {
      if (_sig == t)
            return;
      if (t.custom()) {
            KeySig* ks = _score->customKeySig(t.customType());
            foreach(KeySym* k, keySymbols)
                  delete k;
            keySymbols.clear();
            foreach(KeySym* k, ks->keySymbols)
                  keySymbols.append(new KeySym(*k));
            }
      setKeySigEvent(t);
      }

//---------------------------------------------------------
//   tick
//---------------------------------------------------------

int KeySig::tick() const
      {
      return segment() ? segment()->tick() : 0;
      }

