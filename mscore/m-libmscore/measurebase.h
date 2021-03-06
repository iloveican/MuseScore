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

#ifndef __MEASUREBASE_H__
#define __MEASUREBASE_H__

/**
 \file
 Definition of MeasureBase class.
*/

#include "element.h"

class Score;
class System;
class Measure;

//---------------------------------------------------------
//   MeasureWidth
//---------------------------------------------------------

/**
 result of layoutX().
*/

struct MeasureWidth {
      qreal stretchable;
      qreal nonStretchable;

      MeasureWidth() {}
      MeasureWidth(qreal a, qreal b) {
            stretchable = a;
            nonStretchable = b;
            }
      };

//---------------------------------------------------------
//   MeasureBase
//---------------------------------------------------------

/**
      Base class for Measure, HBox and VBox
*/

class MeasureBase : public Element {
      MeasureBase* _next;
      MeasureBase* _prev;
      int _tick;

   protected:
      MeasureWidth _mw;
      ElementList _el;        ///< Measure(/tick) relative -elements: with defined start time
                              ///< but outside the staff

      bool _dirty;
      bool _lineBreak;        ///< Forced line break
      bool _pageBreak;        ///< Forced page break
      bool _sectionBreak;
      qreal _pause;          ///< section break playback rest (sec)

   public:
      MeasureBase(Score* score);
      ~MeasureBase();
      MeasureBase(const MeasureBase&);
      virtual MeasureBase* clone() const = 0;

      MeasureBase* next() const              { return _next;   }
      void setNext(MeasureBase* e)           { _next = e;      }
      MeasureBase* prev() const              { return _prev;   }
      void setPrev(MeasureBase* e)           { _prev = e;      }

      Measure* nextMeasure() const;
      Measure* prevMeasure() const;

      virtual int ticks() const              { return 0;       }

      virtual void scanElements(void* data, void (*func)(void*, Element*));
      MeasureWidth& layoutWidth()            { return _mw;        }
      ElementList* el()                      { return &_el; }
      const ElementList* el() const          { return &_el; }
      System* system() const                 { return (System*)parent(); }
      void setSystem(System* s)              { setParent((Element*)s);   }

      bool lineBreak() const                 { return _lineBreak; }
      bool pageBreak() const                 { return _pageBreak; }
      bool sectionBreak() const              { return _sectionBreak; }
      void setLineBreak(bool v)              { _lineBreak = v;    }
      void setPageBreak(bool v)              { _pageBreak = v;    }
      void setSectionBreak(bool v)           { _sectionBreak = v; }

      virtual void moveTicks(int diff)       { setTick(tick() + diff); }

      virtual qreal distanceUp(int) const        { return 0.0; }
      virtual qreal distanceDown(int) const      { return 0.0; }
      virtual Spatium userDistanceUp(int) const   { return Spatium(0.0); }
      virtual Spatium userDistanceDown(int) const { return Spatium(0.0); }

      virtual void add(Element*);
      virtual void remove(Element*);
      void setDirty()                        { _dirty = true; }

      int tick() const                       { return _tick;         }
      void setTick(int t)                    { _tick = t;            }

      qreal pause() const                   { return _pause;        }
      void setPause(qreal v)                { _pause = v;           }
      };

#endif

