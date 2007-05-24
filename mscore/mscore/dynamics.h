//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: dynamics.h,v 1.11 2006/03/13 21:35:58 wschweer Exp $
//
//  Copyright (C) 2002-2006 Werner Schweer (ws@seh.de)
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

#ifndef __DYNAMICS_H__
#define __DYNAMICS_H__

#include "text.h"

struct Dyn {
      int textStyle;
      int velocity;           ///< associated midi velocity (0-127, -1 = none)
      const QString str;
      const char* tag;

      Dyn(int style, int velo, const char* t, const QString& s)
         : textStyle(style), velocity(velo), str(s), tag(t) {}
      };

//---------------------------------------------------------
//   Dynamic
//---------------------------------------------------------

class Dynamic : public Text {

   public:
      Dynamic(Score*);
      Dynamic(Score*, int val);
      Dynamic(Score*, const QString&);
      Dynamic(const Dynamic&);
      virtual Dynamic* clone() const   { return new Dynamic(*this); }
      virtual ElementType type() const { return DYNAMIC; }

      virtual void setSubtype(int val);
      virtual void setSubtype(const QString&);
      virtual const QString subtypeName() const;

      virtual bool isMovable() const { return true; }
      virtual void endDrag();

      virtual void write(Xml& xml) const;
      virtual void read(QDomElement);
      virtual void layout(ScoreLayout*);
      };

#endif
