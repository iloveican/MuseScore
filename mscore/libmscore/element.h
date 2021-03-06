//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id$
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __ELEMENT_H__
#define __ELEMENT_H__

#include "xml.h"
#include "mscore.h"

/**
 \file
 Definition of classes Element, ElementList, StaffLines.
*/

#include "spatium.h"

class Xml;
class Measure;
class Staff;
class Score;
class Sym;
class MuseScoreView;
class Segment;
class TextStyle;
class Element;
class Painter;

//---------------------------------------------------------
//   ElementFlag
//---------------------------------------------------------

enum ElementFlag {
      ELEMENT_SYSTEM_FLAG = 0x1,
      ELEMENT_DROP_TARGET = 0x2,
      ELEMENT_SELECTABLE  = 0x4,
      ELEMENT_MOVABLE     = 0x8,
      ELEMENT_SEGMENT     = 0x10,
      ELEMENT_HAS_TAG     = 0x20
      };

typedef QFlags<ElementFlag> ElementFlags;
Q_DECLARE_OPERATORS_FOR_FLAGS(ElementFlags)

//---------------------------------------------------------
///   \brief Unit of horizontal measure
//---------------------------------------------------------

class Space {
      qreal _lw;       // space needed to the left
      qreal _rw;       // space needed to the right

   public:
      Space() : _lw(0.0), _rw(0.0)  {}
      Space(qreal a, qreal b) : _lw(a), _rw(b) {}
      qreal lw() const             { return _lw; }
      qreal rw() const             { return _rw; }
      qreal& rLw()                 { return _lw; }
      qreal& rRw()                 { return _rw; }
      void setLw(qreal e)          { _lw = e; }
      void setRw(qreal m)          { _rw = m; }
      inline void max(const Space& s);
      };

//---------------------------------------------------------
//   LinkedElements
//---------------------------------------------------------

class LinkedElements : public QList<Element*> {
      static int _linkId;
      int _lid;                     // unique id for every linked list

   public:
      LinkedElements();
      LinkedElements(int id);
      void setLid(int val);
      int lid() const                         { return _lid;                }
      };

//---------------------------------------------------------
//   DropData
//---------------------------------------------------------

struct DropData {
      MuseScoreView* view;
      QPointF pos;
      QPointF dragOffset;
      Element* element;
      Qt::KeyboardModifiers modifiers;
      };

//---------------------------------------------------------
//   EditData
//    used in editDrag
//---------------------------------------------------------

struct EditData {
      MuseScoreView* view;
      int curGrip;
      QPointF delta;
      QPointF pos;
      bool hRaster;
      bool vRaster;
      };

//------------------------------------------------------------------------
//   Element Properties
//    accessible through
//    virtual QVariant Element::getProperty(int propertyId)
//    virtual void Element::setProperty(int propertyId, const QVariant&)
//------------------------------------------------------------------------

enum {
      P_SUBTYPE,
      P_COLOR,
      P_VISIBLE,
      P_SMALL,
      P_SHOW_COURTESY,
      P_LINE_TYPE,
      P_TPC,
      P_STEM_DIRECTION,
      P_DIRECTION,
      P_ARTICULATION_ANCHOR
      };


//---------------------------------------------------------
///   \brief base class of score layout elements
///
///   The Element class is the virtual base class of all
///   score layout elements.
///
///   More details: TBD
//---------------------------------------------------------

class Element {
      Q_DECLARE_TR_FUNCTIONS(Element)

      LinkedElements* _links;
      Element* _parent;

      bool _selected:1;           ///< set if element is selected
      bool _generated:1;          ///< automatically generated Element
      bool _visible:1;            ///< visibility attribute

      mutable ElementFlags _flags;

      int _subtype;
      int _track;                 ///< staffIdx * VOICES + voice
                                  ///< -1 if this is a system element
      QColor _color;
      qreal _mag;                 ///< standard magnification (derived value)

      QPointF _pos;               ///< Reference position, relative to _parent.
      QPointF _userOff;           ///< offset from normal layout position:
                                  ///< user dragged object this amount.
      QPointF _readPos;

      mutable QRectF _bbox;       ///< Bounding box relative to _pos + _userOff
                                  ///< valid after call to layout()
      uint _tag;                  ///< tag bitmask

   protected:
      Score* _score;

      int _mxmlOff;               ///< MusicXML offset in ticks.
                                  ///< Note: interacts with userXoffset.

      QPointF _startDragPosition;   ///< used during drag


   public:
      Element(Score* s = 0);
      Element(const Element&);
      virtual ~Element();
      Element &operator=(const Element&);
      virtual Element* clone() const = 0;
      virtual Element* linkedClone();
      QList<Element*> linkList() const;

      void linkTo(Element*);
      int lid() const                         { return _links ? _links->lid() : 0; }
      LinkedElements* links() const           { return _links;      }
      void setLinks(LinkedElements* le)       { _links = le;        }

      Score* score() const                    { return _score;      }
      virtual void setScore(Score* s)         { _score = s;         }
      Element* parent() const                 { return _parent;     }
      void setParent(Element* e)              { _parent = e;        }

      qreal spatium() const;

      bool selected() const                   { return _selected;   }
      virtual void setSelected(bool f)        { _selected = f;      }

      bool visible() const                    { return _visible;    }
      virtual void setVisible(bool f)         { _visible = f;       }
      bool generated() const                  { return _generated;  }
      void setGenerated(bool val)             { _generated = val;   }

      const QPointF& ipos() const             { return _pos;                    }
      virtual QPointF pos() const             { return _pos + _userOff;         }
      virtual qreal x() const                 { return _pos.x() + _userOff.x(); }
      virtual qreal y() const                 { return _pos.y() + _userOff.y(); }
      void setPos(qreal x, qreal y);
      void setPos(const QPointF& p)           { setPos(p.x(), p.y());           }
      void movePos(const QPointF& p)          { _pos += p;               }
      qreal& rxpos()                          { return _pos.rx();        }
      qreal& rypos()                          { return _pos.ry();        }
      virtual void move(qreal xd, qreal yd)   { _pos += QPointF(xd, yd); }
      virtual void move(const QPointF& s)     { _pos += s;               }

      virtual QPointF pagePos() const;          ///< position in page coordinates
      virtual QPointF canvasPos() const;        ///< position in canvas coordinates
      qreal pageX() const;

      const QPointF& userOff() const          { return _userOff;  }
      void setUserOff(const QPointF& o)       { _userOff = o;     }
      void setUserXoffset(qreal v)            { _userOff.setX(v); }
      void setUserYoffset(qreal v)            { _userOff.setY(v); }
      int mxmlOff() const                     { return _mxmlOff;  }
      void setMxmlOff(int o)                  { _mxmlOff = o;     }

      QPointF readPos() const                 { return _readPos;   }
      void setReadPos(const QPointF& p)       { _readPos = p;      }
      void adjustReadPos();

      virtual QRectF bbox() const             { return _bbox;              }
      virtual qreal height() const            { return bbox().height();    }
      virtual void setHeight(qreal v)         { _bbox.setHeight(v);        }
      virtual qreal width() const             { return bbox().width();     }
      virtual void setWidth(qreal v)          { _bbox.setWidth(v);         }
      QRectF abbox() const                        { return bbox().translated(pagePos());   }
      QRectF pageBoundingRect() const             { return bbox().translated(pagePos());   }
      QRectF canvasBoundingRect() const           { return bbox().translated(canvasPos()); }
      virtual void setbbox(const QRectF& r) const { _bbox = r;           }
      virtual void addbbox(const QRectF& r) const { _bbox |= r;          }
      virtual bool contains(const QPointF& p) const;
      bool intersects(const QRectF& r) const;
      virtual QPainterPath shape() const;
      virtual qreal baseLine() const          { return -height();       }

      virtual ElementType type() const = 0;
      int subtype() const                     { return _subtype;        }
      virtual void setSubtype(int val)        { _subtype = val;         }
      bool isChordRest() const                { return type() == REST || type() == CHORD;   }
      bool isDurationElement() const          { return isChordRest() || (type() == TUPLET); }
      bool isSLine() const {
            return type() == HAIRPIN || type() == OTTAVA || type() == PEDAL
               || type() == TRILL || type() == VOLTA || type() == TEXTLINE;
            }

      virtual void draw(Painter*) const {}

      void writeProperties(Xml& xml, const Element* proto = 0) const;
      bool readProperties(QDomElement);

      virtual void write(Xml&) const;
      virtual void read(QDomElement);

      virtual QRectF drag(const EditData&);
      virtual void endDrag()                  {}
      virtual QLineF dragAnchor() const       { return QLineF(); }

      virtual bool isEditable() const         { return !_generated; }
      virtual void startEdit(MuseScoreView*, const QPointF&) {}
      virtual bool edit(MuseScoreView*, int grip, int key, Qt::KeyboardModifiers, const QString& s);
      virtual void editDrag(const EditData&);
      virtual void endEditDrag()                               {}
      virtual void endEdit()                                   {}
      virtual void updateGrips(int* grips, QRectF*) const      { *grips = 0;       }
      virtual QPointF gripAnchor(int) const   { return QPointF(); }
      virtual void setGrip(int, const QPointF&);
      virtual QPointF getGrip(int) const;

      int track() const                       { return _track; }
      virtual void setTrack(int val)          { _track = val;  }

      virtual int z() const                   { return type() * 100; }  // stacking order

      int staffIdx() const                    { return _track / VOICES;         }
      int voice() const                       { return _track % VOICES;         }
      void setVoice(int v)                    { _track = (_track / VOICES) + v; }
      Staff* staff() const;

      virtual void add(Element*);
      virtual void remove(Element*);
      virtual void change(Element* o, Element* n);

      virtual void layout() {}
      virtual void spatiumChanged(qreal /*oldValue*/, qreal /*newValue*/);

      // debug functions
      virtual void dump() const;
      const char* name() const;
      virtual QString userName() const;
      void dumpQPointF(const char*) const;

      virtual Space space() const     { return Space(0.0, width()); }

      QColor color() const            { return _color; }
      QColor curColor() const;
      void setColor(const QColor& c)  { _color = c;    }
      static ElementType readType(QDomElement& node, QPointF*);

      virtual QByteArray mimeData(const QPointF&) const;
/**
 Return true if this element accepts a drop at canvas relative \a pos
 of given element \a type and \a subtype.

 Reimplemented by elements that accept drops. Used to change cursor shape while
 dragging to indicate drop targets.
*/
      virtual bool acceptDrop(MuseScoreView*, const QPointF&, int, int) const { return false; }

/**
 Handle a dropped element at canvas relative \a pos of given element
 \a type and \a subtype. Returns dropped element if any.

 Reimplemented by elements that accept drops.
*/
      virtual Element* drop(const DropData&) { return 0;}

/**
 Return a name for a \a subtype. Used for outputting xml data.
 Reimplemented by elements with subtype names.
 */
      virtual const QString subtypeName() const { return QString("%1").arg(_subtype); }

/**
 Set subtype by name
 Used for reading xml data.
 Reimplemented by elements with subtype names.
 */
      virtual void setSubtype(const QString& s) { setSubtype(s.toInt()); }

/**
 delivers mouseEvent to element in edit mode
 returns true if mouse event is accepted by element
 */
      virtual bool mousePress(const QPointF&, QMouseEvent*) { return false; }

      mutable int itemDiscovered;     ///< helper flag for bsp

      virtual QList<Prop> properties(Xml&, const Element* proto = 0) const;
      virtual void scanElements(void* data, void (*func)(void*, Element*), bool all=true);

      virtual void toDefault();

      qreal mag() const                        { return _mag;   }
      qreal magS() const;
      virtual void setMag(qreal val)           { _mag = val;    }

      bool isText() const {
              return type()  == TEXT
                || type() == LYRICS
                || type() == DYNAMIC
                || type() == HARMONY
                || type() == MARKER
                || type() == JUMP
                || type() == STAFF_TEXT
                || type() == INSTRUMENT_CHANGE
                || type() == TEMPO_TEXT;
            }
      qreal point(const Spatium sp) const { return sp.val() * spatium(); }

      //
      // check element for consistency; return false if element
      // is not valid
      //
      virtual bool check() const { return true; }

      QPointF startDragPosition() const           { return _startDragPosition; }
      void setStartDragPosition(const QPointF& v) { _startDragPosition = v; }

      static const char* name(ElementType type);
      static Element* create(ElementType type, Score*);
      static ElementType name2type(const QString&);
      static Element* name2Element(const QString&, Score*);

      void setFlag(ElementFlag f, bool v)  {
            if (v)
                  _flags |= f;
            else
                  _flags &= ~f;
            }
      bool flag(ElementFlag f) const   { return _flags & f; }
      void setFlags(ElementFlags f)    { _flags = f;    }
      ElementFlags flags() const       { return _flags; }
      bool systemFlag() const          { return flag(ELEMENT_SYSTEM_FLAG); }
      void setSystemFlag(bool f)       { setFlag(ELEMENT_SYSTEM_FLAG, f);  }
      bool selectable() const          { return flag(ELEMENT_SELECTABLE);  }
      void setSelectable(bool val)     { setFlag(ELEMENT_SELECTABLE, val); }
      bool dropTarget() const          { return flag(ELEMENT_DROP_TARGET); }
      void setDropTarget(bool v) const {
            if (v)
                  _flags |= ELEMENT_DROP_TARGET;
            else
                  _flags &= ~ELEMENT_DROP_TARGET;
            }
      virtual bool isMovable() const   { return flag(ELEMENT_MOVABLE);     }
      bool isSegment() const           { return flag(ELEMENT_SEGMENT);     }
      uint tag() const                 { return _tag;                      }
      void setTag(uint val)            { _tag = val;                       }
      virtual QVariant getProperty(int propertyId) const;
      virtual void setProperty(int propertyId, const QVariant&);
      };

//---------------------------------------------------------
//   ElementList
//---------------------------------------------------------

class ElementList : public QList<Element*> {
   public:
      ElementList() {}
      bool remove(Element*);
      void replace(Element* old, Element* n);
      void write(Xml&) const;
      void write(Xml&, const char* name) const;
      void read(QDomElement);
      };

typedef ElementList::iterator iElement;
typedef ElementList::const_iterator ciElement;

//---------------------------------------------------------
//   StaffLines
//---------------------------------------------------------

/**
 The StaffLines class is the graphic representation of a staff,
 it draws the horizontal staff lines.
*/

class StaffLines : public Element {
      qreal dist;
      qreal lw;
      int lines;

   public:
      StaffLines(Score*);
      virtual StaffLines* clone() const    { return new StaffLines(*this); }
      virtual ElementType type() const     { return STAFF_LINES; }
      virtual void layout();

      Measure* measure() const             { return (Measure*)parent(); }
      virtual void draw(Painter*) const;
      virtual QPointF pagePos() const;   ///< position in page coordinates
      qreal y1() const;
      qreal y2() const;
      };

//---------------------------------------------------------
//   Line
//---------------------------------------------------------

class Line : public Element {
      Spatium _width;
      Spatium _len;

   protected:
      bool vertical;

   public:
      Line(Score*);
      Line(Score*, bool vertical);
      Line &operator=(const Line&);

      virtual Line* clone() const { return new Line(*this); }
      virtual ElementType type() const { return LINE; }
      virtual void layout();

      virtual void draw(Painter*) const;
      void writeProperties(Xml& xml) const;
      bool readProperties(QDomElement);
      void dump() const;

      Spatium len()    const { return _len; }
      Spatium lineWidth()  const { return _width; }
      void setLen(Spatium);
      void setLineWidth(Spatium);
      };

//---------------------------------------------------------
//   Compound
//---------------------------------------------------------

class Compound : public Element {
      QList<Element*> elemente;

   protected:
      const QList<Element*>& getElemente() const { return elemente; }

   public:
      Compound(Score*);
      Compound(const Compound&);
      virtual ElementType type() const = 0;

      virtual void draw(Painter*) const;
      virtual void addElement(Element*, qreal x, qreal y);
      void clear();
      virtual void setSelected(bool f);
      virtual void setVisible(bool);
      virtual void layout();
      };

//---------------------------------------------------------
//   RubberBand
//---------------------------------------------------------

class RubberBand : public Element {
      QPointF _p1, _p2;

   public:
      RubberBand(Score* s) : Element(s) {}
      virtual RubberBand* clone() const { return new RubberBand(*this); }
      virtual ElementType type() const { return RUBBERBAND; }
      virtual void draw(Painter*) const;

      void set(const QPointF& p1, const QPointF& p2) { _p1 = p1; _p2 = p2; }
      QPointF p1() const { return _p1; }
      QPointF p2() const { return _p2; }
      };

extern bool elementLessThan(const Element* const, const Element* const);
extern void collectElements(void* data, Element* e);

#endif

