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

#ifndef __ACCIDENTAL_H__
#define __ACCIDENTAL_H__

/**
 \file
 Definition of class Accidental
*/

#include "element.h"
#include "globals.h"

class Note;
class Painter;

//---------------------------------------------------------
//   SymElement
//---------------------------------------------------------

struct SymElement {
      int sym;
      qreal x;
      SymElement(int _sym, qreal _x) : sym(_sym), x(_x) {}
      };

//---------------------------------------------------------
//   AccidentalRole
//---------------------------------------------------------

enum AccidentalRole {
      ACC_AUTO,               // layout created accidental
      ACC_USER                // user created accidental
      };

//---------------------------------------------------------
//   Accidental
//---------------------------------------------------------

class Accidental : public Element {
      QList<SymElement> el;
      bool _hasBracket;
      AccidentalRole _role;

   public:
      Accidental(Score* s);
      virtual Accidental* clone() const     { return new Accidental(*this); }
      virtual ElementType type() const      { return ACCIDENTAL; }
      virtual const QString subtypeName() const;
      const char* subtypeUserName() const;
      virtual void setSubtype(const QString& s);
      void setSubtype(AccidentalType t)     { Element::setSubtype(int(t)); }
      AccidentalType accidentalType() const { return AccidentalType(subtype()); }
      virtual void layout();
      virtual void draw(Painter*) const;

      int symbol();
      Note* note() const                  { return (Note*)parent(); }
      bool hasBracket() const             { return _hasBracket;     }
      void setHasBracket(bool val)        { _hasBracket = val;      }
      AccidentalRole role() const         { return _role;           }
      void setRole(AccidentalRole r)      { _role = r;              }

      virtual void read(XmlReader*);

      static int subtype2value(AccidentalType);             // return effective pitch offset
      static const char* subtype2name(AccidentalType);      // return effective pitch offset
      static AccidentalType value2subtype(int);
      static AccidentalType name2subtype(const QString&);
      };

//---------------------------------------------------------
//   AccidentalBracket
//    used as icon in palette
//---------------------------------------------------------

class AccidentalBracket : public Compound {
   public:
      AccidentalBracket(Score*);
      virtual AccidentalBracket* clone() const { return new AccidentalBracket(*this); }
      virtual ElementType type() const         { return ACCIDENTAL_BRACKET; }
      virtual void setSubtype(int v);
      };

#endif

