//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __CHORDLINE_H__
#define __CHORDLINE_H__

#include "element.h"

class Chord;
class Painter;

//---------------------------------------------------------
//   ChordLine
//    bezier line attached to top note of a chord
//    implements fall, doit, plop, bend
//---------------------------------------------------------

class ChordLine : public Element {
      QPainterPath path;
      bool modified;

   public:
      ChordLine(Score*);
      ChordLine(const ChordLine&);

      virtual ChordLine* clone() const { return new ChordLine(*this); }
      virtual ElementType type() const { return CHORDLINE; }
      virtual void setSubtype(int);
      Chord* chord() const             { return (Chord*)(parent()); }

      virtual void read(QDomElement);
      virtual void write(Xml& xml) const;
      virtual void layout();
      virtual void draw(Painter*) const;

      virtual void editDrag(const EditData&);
      virtual void updateGrips(int*, QRectF*) const;
      };

#endif

