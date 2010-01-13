//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id$
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

#ifndef __SELECT_H__
#define __SELECT_H__

#include "ui_select.h"

//---------------------------------------------------------
//   SelState
//---------------------------------------------------------

enum SelState {
      SEL_NONE,         // nothing is selected
      SEL_SINGLE,       // a single object is selected
      SEL_MULT,         // more than one object is selected
      SEL_STAFF,        // a range in one or more staffs is selected
      SEL_SYSTEM,       // a system range ("passage") is selected
      };

class Score;
class Page;
class System;
class ChordRest;
class Element;
class Segment;
class Note;
struct ElementPattern;

//---------------------------------------------------------
//   Selection
//---------------------------------------------------------

class Selection {
      Score* _score;
      SelState _state;
      QList<Element*> _el;          // valid in mode SEL_SINGLE and SEL_MULT
      Segment* _startSegment;
      Segment* _endSegment;         // next segment after selection
      Segment* _activeSegment;

      int _staffStart;              // valid if selState is SEL_STAFF
      int _staffEnd;                // valid if selState is SEL_STAFF
      int _activeTrack;

      QByteArray staffMimeData() const;

   public:
      Selection(Score*);
      Score* score() const             { return _score; }
      SelState state() const           { return _state; }
      void setState(SelState s)        { _state = s;    }

      void searchSelectedElements();
      const QList<Element*>& elements() const { return _el; }
      void clearElements()             { _el.clear(); }
      QList<Note*> noteList() const;
      void add(Element*);
      void append(Element* el)         { _el.append(el); }
      QRectF deselectAll(Score*);
      void remove(Element*);
      QRectF clear();
      Element* element() const;
      ChordRest* firstChordRest(int track = -1) const;
      ChordRest* lastChordRest(int track = -1) const;
      void update();
      void updateState();
      void dump();
      QString mimeType() const;
      QByteArray mimeData() const;

      Segment* startSegment() const     { return _startSegment; }
      Segment* endSegment() const       { return _endSegment;   }
      void setStartSegment(Segment* s)  { _startSegment = s; }
      void setEndSegment(Segment* s)    { _endSegment = s; }
      void setRange(Segment* a, Segment* b, int c, int d);
      Segment* activeSegment() const    { return _activeSegment; }
      void setActiveSegment(Segment* s) { _activeSegment = s; }
      ChordRest* activeCR() const;
      bool isStartActive() const;
      bool isEndActive() const;
      int tickStart() const;
      int tickEnd() const;
      int staffStart() const            { return _staffStart;  }
      int staffEnd() const              { return _staffEnd;    }
      int activeTrack() const           { return _activeTrack; }
      void setStaffStart(int v)         { _staffStart = v;  }
      void setStaffEnd(int v)           { _staffEnd = v;    }
      void setActiveTrack(int v)        { _activeTrack = v; }
      };

//---------------------------------------------------------
//   SelectDialog
//---------------------------------------------------------

class SelectDialog : public QDialog, Ui::SelectDialog {
      Q_OBJECT
      const Element* e;

   public:
      SelectDialog(const Element* e, QWidget* parent);
      void setPattern(ElementPattern* p);
      bool doReplace() const       { return replace->isChecked();       }
      bool doAdd() const           { return add->isChecked();           }
      bool doSubtract() const      { return subtract->isChecked();      }
      bool doFromSelection() const { return fromSelection->isChecked(); }
      };

#endif

