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

#ifndef __SCANVAS_H__
#define __SCANVAS_H__

class Rest;
class Element;
class Page;
class Xml;
class Note;
class Lasso;
class ShadowNote;
class Navigator;
class Cursor;
class Segment;
class Measure;
class System;
class Score;
class TextB;

//---------------------------------------------------------
//   CommandEvent
//---------------------------------------------------------

struct CommandEvent : public QEvent
      {
      QString value;
      CommandEvent(const QString& c)
         : QEvent(QEvent::Type(QEvent::User+1)), value(c) {}
      };

//---------------------------------------------------------
//   ScoreView
//---------------------------------------------------------

class ScoreView : public QWidget {
      Q_OBJECT

      enum States { NORMAL, DRAG, DRAG_OBJECT, EDIT, DRAG_EDIT, LASSO,
            NOTE_ENTRY, MAG, PLAY, SEARCH, STATES
            };

      Score* _score;

      // the next elements are used during dragMove to give some visual
      // feedback:
      //    dropTarget:       if valid, the element is drawn in a different color
      //                      to mark it as a valid drop target
      //    dropRectangle:    if valid, the rectangle is filled with a
      //                      color to visualize a valid drop area
      //    dropAnchor:       if valid the line is drawn from the current
      //                      cursor position to the current anchor point
      // Note:
      //    only one of the elements is active during drag

      const Element* dropTarget;    ///< current drop target during dragMove
      QRectF dropRectangle;         ///< current drop rectangle during dragMove
      QLineF dropAnchor;            ///< line to current anchor point during dragMove

      // in text edit mode text is framed
      TextB* _editText;

      QMatrix _matrix, imatrix;
      int _magIdx;


      QStateMachine* sm;
      QState* states[STATES];

      QFocusFrame* focusFrame;
      Navigator* navigator;
      int level;

      bool dragScoreViewState;
      bool draggedScoreView;
      Element* dragElement;   // current moved drag&drop element
      Element* dragObject;    // current canvas element

      QPointF dragOffset;
      bool mousePressed;

      // editing mode
      int curGrip;
      QRectF grip[4];         // edit "grips"
      int grips;              // number of used grips

      QPointF startMove;

      //--input state:
      Cursor* cursor;
      ShadowNote* shadowNote;

      Lasso* lasso;           ///< temporarily drawn lasso selection
      QRectF _lassoRect;

      QColor _bgColor;
      QColor _fgColor;
      QPixmap* bgPixmap;
      QPixmap* fgPixmap;

      Element* origEditObject;
      Element* editObject;          ///< Valid in edit mode
      QPointF _startDragPosition;
      int textUndoLevel;
      System* dragSystem;           ///< Valid if DRAG_STAFF.
      int dragStaff;
      //============================================

      virtual void paintEvent(QPaintEvent*);
      void paint(const QRect&, QPainter&);

      void objectPopup(const QPoint&, Element*);
      void measurePopup(const QPoint&, Measure*);

      void saveChord(Xml&);

      virtual void resizeEvent(QResizeEvent*);
      virtual void wheelEvent(QWheelEvent*);
      virtual void dragEnterEvent(QDragEnterEvent*);
      virtual void dragLeaveEvent(QDragLeaveEvent*);
      virtual void dragMoveEvent(QDragMoveEvent*);
      virtual void dropEvent(QDropEvent*);
      virtual void focusInEvent(QFocusEvent*);
      virtual void focusOutEvent(QFocusEvent*);

      void contextItem(Element*);

      void lassoSelect();
      Note* searchTieNote(Note* note);

      void setShadowNote(const QPointF&);
      void drawElements(QPainter& p,const QList<const Element*>& el);
      bool dragTimeAnchorElement(const QPointF& pos);
      void dragSymbol(const QPointF& pos);
      bool dragMeasureAnchorElement(const QPointF& pos);
      bool dragAboveMeasure(const QPointF& pos);
      bool dragAboveSystem(const QPointF& pos);
      void updateGrips();
      const QList<const Element*> elementsAt(const QPointF&);
      void lyricsTab(bool back, bool end);
      void lyricsReturn();
      void lyricsEndEdit();
      void lyricsUpDown(bool up, bool end);
      void lyricsMinus();
      void lyricsUnderscore();
      void harmonyEndEdit();
      void chordTab(bool back);
      void cmdAddPitch(int note, bool addFlag);
      void cmdAddChordName();
      void cmdAddText(int style);

   private slots:
      void moveCursor();
      void textUndoLevelAdded();
      void enterState();
      void exitState();

   public slots:
      void setViewRect(const QRectF&);
      void dataChanged(const QRectF&);

      void startEdit();
      void endEdit();
      void endStartEdit() { endEdit(); startEdit(); }

      void startDrag();
      void endDrag();

      void endDragEdit();

      void startNoteEntry();
      void endNoteEntry();

      void endLasso();
      void deselectAll();
      void adjustCanvasPosition(Element* el, bool playBack);
      void editCopy();
      void editPaste();

   public:
      ScoreView(QWidget* parent = 0);
      ~ScoreView();

      void startEdit(Element*, int startGrip);
      void startEdit(Element*);

      void moveCursor(Segment*, int staffIdx);
      void setCursorOn(bool);
      void setBackground(QPixmap*);
      void setBackground(const QColor&);
      void setForeground(QPixmap*);
      void setForeground(const QColor&);

      Page* addPage();
      void modifyElement(Element* obj);
      void setScore(Score* s);

      void setMag(qreal m);
      void showNavigator(bool visible);
      void redraw(const QRectF& r);
      void updateNavigator(bool layoutChanged) const;
      Element* elementAt(const QPointF& pp);
      Element* elementNear(const QPointF& pp);
      QRectF lassoRect() const { return _lassoRect; }
      void setLassoRect(const QRectF& r) { _lassoRect = r; }
      bool navigatorVisible() const;
      void cmd(const QAction* a);

      void drag(const QPointF&);
      void endUndoRedo();
      void zoom(int step, const QPoint& pos);
      void contextPopup(QMouseEvent* ev);
      void setOrigEditObject(Element* e) { origEditObject = e; }
      void editKey(QKeyEvent*);
      Element* getDragObject() const { return dragObject; }
      void dragScoreView(QMouseEvent* ev);
      void dragNoteEntry(QMouseEvent* ev);
      void noteEntryButton(QMouseEvent* ev);
      void doDragElement(QMouseEvent* ev);
      void doDragLasso(QMouseEvent* ev);
      void doDragEdit(QMouseEvent* ev);
      void select(QMouseEvent*);
      void mousePress(QMouseEvent* ev);
      bool testElementDragTransition(QMouseEvent* ev) const;
      bool editElementDragTransition(QMouseEvent* ev);
      bool editScoreViewDragTransition(QMouseEvent* e);
      void cmdAddSlur();
      void cmdAddSlur(Note* firstNote, Note* lastNote);
      bool noteEntryMode() const;
      void editInputTransition(QInputMethodEvent* ie);
      void onEditPasteTransition(QMouseEvent* ev);

      Score* score() const                      { return _score; }
      void setDropRectangle(const QRectF&);
      void setDropTarget(const Element*);
      void setDropAnchor(const QLineF&);
      const QMatrix& matrix() const              { return _matrix; }
      void setEditText(TextB* t)                 { _editText = t;      }
      TextB* editText() const                    { return _editText;   }
      qreal mag() const;
      int magIdx() const                         { return _magIdx; }
      void setMag(int idx, double mag);
      qreal xoffset() const;
      qreal yoffset() const;
      void setOffset(qreal x, qreal y);
      QSizeF fsize() const;
      void pageNext();
      void pagePrev();
      void pageTop();
      void pageEnd();
      QPointF toLogical(const QPoint& p) const { return imatrix.map(QPointF(p)); }
      void search(const QString& s);
      void postCmd(const char* cmd)   { sm->postEvent(new CommandEvent(cmd));  }
      void setFocusRect();
      };

extern int searchStaff(const Element* element);

#endif
