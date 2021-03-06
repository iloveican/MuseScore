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

#ifndef __CHORD_H__
#define __CHORD_H__

/**
 \file
 Definition of classes Chord, HelpLine, NoteList and Stem.
*/

#include "globals.h"
#include "chordrest.h"

class Note;
class Hook;
class Arpeggio;
class Tremolo;
class Chord;
class Glissando;
class NoteEvent;
class Painter;

enum TremoloChordType { TremoloSingle, TremoloFirstNote, TremoloSecondNote };

//---------------------------------------------------------
//   Stem
//    Notenhals
//---------------------------------------------------------

/**
 Graphic representation of a note stem.
*/

class Stem : public Element {
      qreal _len;
      Spatium _userLen;

   public:
      Stem(Score*);
      Stem &operator=(const Stem&);

      virtual Stem* clone() const      { return new Stem(*this); }
      virtual ElementType type() const { return STEM; }
      virtual void draw(Painter*) const;
      void setLen(qreal v)            { _len = v; }
      qreal stemLen() const           { return _len + point(_userLen); }
      virtual QRectF bbox() const;

      virtual void read(XmlReader* e);
      Spatium userLen() const         { return _userLen; }
      virtual void setVisible(bool f);
      Chord* chord() const            { return (Chord*)parent(); }
      };

//---------------------------------------------------------
//   StemSlash
//    used for grace notes of type acciaccatura
//---------------------------------------------------------

class StemSlash : public Element {
      QLineF line;

   public:
      StemSlash(Score*);
      StemSlash &operator=(const Stem&);

      void setLine(const QLineF& l);

      virtual StemSlash* clone() const { return new StemSlash(*this); }
      virtual ElementType type() const { return STEM_SLASH; }
      virtual void draw(Painter*) const;
      };

//---------------------------------------------------------
///   Graphic representation of a ledger line.
///
///    parent:     Chord
///    x-origin:   Chord
///    y-origin:   SStaff
//---------------------------------------------------------

class LedgerLine : public Line {

   public:
      LedgerLine(Score*);
      LedgerLine &operator=(const LedgerLine&);
      virtual LedgerLine* clone() const { return new LedgerLine(*this); }
      virtual ElementType type() const  { return LEDGER_LINE; }
      virtual QPointF canvasPos() const;      ///< position in canvas coordinates
      Chord* chord() const { return (Chord*)parent(); }
      virtual void layout();
      };

//---------------------------------------------------------
//   Chord
//---------------------------------------------------------

/**
 Graphic representation of a chord.

 Single notes are handled as degenerated chords.
*/

class Chord : public ChordRest {
      QList<Note*> _notes;
      QList<LedgerLine*> _ledgerLines;

      Stem*      _stem;
      Hook*      _hook;
      StemSlash* _stemSlash;
      Direction  _stemDirection;
      Arpeggio*  _arpeggio;
      Tremolo*   _tremolo;
      TremoloChordType _tremoloChordType;
      Glissando* _glissando;
      QList<NoteEvent*> _playEvents;
      ElementList _el;        ///< chordline

      NoteType   _noteType;         ///< mark grace notes: acciaccatura and appoggiatura
      bool       _noStem;
      qreal     _dotPosX;

      virtual qreal upPos()   const;
      virtual qreal downPos() const;
      virtual qreal centerX() const;
      void addLedgerLine(qreal x, int staffIdx, int line, int extend);
      void addLedgerLines(qreal x, int move);

   public:
      Chord(Score* s = 0);
      Chord(const Chord&);
      ~Chord();
      Chord &operator=(const Chord&);

      virtual Chord* clone() const     { return new Chord(*this); }

      virtual void setScore(Score* s);
      virtual ElementType type() const { return CHORD; }

      void read(XmlReader*, const QList<Tuplet*>&, const QList<Slur*>&);
      virtual void read(XmlReader*);
      virtual void setSelected(bool f);

      virtual QRectF bbox() const;
      void setStemDirection(Direction d)     { _stemDirection = d; }
      Direction stemDirection() const        { return _stemDirection; }

      QList<LedgerLine*>* ledgerLines()      { return &_ledgerLines; }

      virtual void layoutStem1();
      virtual void layoutStem();
      void layoutArpeggio2();

      QList<Note*>& notes()                  { return _notes; }
      const QList<Note*>& notes() const      { return _notes; }
      Note* upNote() const                   { return _notes.back(); }
      Note* downNote() const                 { return _notes.front(); }
      Note* findNote(int pitch) const;

      Stem* stem() const                     { return _stem; }
      void setStem(Stem* s);
      Arpeggio* arpeggio() const             { return _arpeggio;  }
      Tremolo* tremolo() const               { return _tremolo;   }
      void setTremolo(Tremolo* t)            { _tremolo = t;      }
      Glissando* glissando() const           { return _glissando; }
      StemSlash* stemSlash() const           { return _stemSlash; }

      virtual QPointF stemPos(bool, bool) const;

      Hook* hook() const                     { return _hook; }
      void setHook(Hook* f);

      virtual void add(Element*);
      virtual void remove(Element*);

      Note* selectedNote() const;
      virtual void layout();
      void layout2();

      virtual int upLine() const;
      virtual int downLine() const;
      void readNote(XmlReader* node, const QList<Tuplet*>&, const QList<Slur*>&);

      NoteType noteType() const         { return _noteType; }
      void setNoteType(NoteType t)      { _noteType = t; }

      virtual void scanElements(void* data, void (*func)(void*, Element*));

      virtual void setTrack(int val);

      void computeUp();
      qreal dotPosX() const              { return _dotPosX; }
      void setDotPosX(qreal val)         { _dotPosX = val;  }
      bool noStem() const                 { return _noStem;  }
      void setNoStem(bool val)            { _noStem = val;   }
      virtual void setMag(qreal val);
      void pitchChanged();
      void renderPlayback();
      QList<NoteEvent*>& playEvents()                { return _playEvents; }
      const QList<NoteEvent*>& playEvents() const    { return _playEvents; }
      void setPlayEvents(const QList<NoteEvent*>& v) { _playEvents = v;    }
      TremoloChordType tremoloChordType() const      { return _tremoloChordType; }
      void setTremoloChordType(TremoloChordType t)   { _tremoloChordType = t; }

      ElementList& el()                { return _el; }
      const ElementList& el() const    { return _el; }
      };

#endif

