//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: layout.cpp,v 1.61 2006/09/08 19:37:08 lvinken Exp $
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

#include "page.h"
#include "sig.h"
#include "key.h"
#include "clef.h"
#include "score.h"
#include "globals.h"
#include "segment.h"
#include "text.h"
#include "staff.h"
#include "style.h"
#include "layout.h"
#include "timesig.h"
#include "canvas.h"

//---------------------------------------------------------
//   ScoreLayout
//---------------------------------------------------------

ScoreLayout::ScoreLayout()
      {
      _spatium     = ::_spatium;
      _pageFormat  = 0;
      _systems     = new SystemList;
      _pages       = new PageList;
      _paintDevice = 0;
      }

ScoreLayout::~ScoreLayout()
      {
      if (_pageFormat)
            delete _pageFormat;
      if (_systems)
            delete _systems;
      if (_pages)
            delete _pages;
      }

//---------------------------------------------------------
//   setScore
//---------------------------------------------------------

void ScoreLayout::setScore(Score* s)
      {
      _score = s;
      _spatium = ::_spatium;
      _systems->clear();
      _pages->clear();
      _needLayout = false;
      if (_pageFormat)
            delete _pageFormat;
      _pageFormat = new PageFormat;
      _paintDevice = _score->canvas();
      }

//---------------------------------------------------------
//   searchNote
//    search for note or rest before or at tick position tick
//    in staff
//---------------------------------------------------------

Element* Score::searchNote(int tick, int staff) const
      {
      for (const Measure* measure = _layout->first(); measure; measure = measure->next()) {
            Element* ipe = 0;

            for (int track = staff * VOICES; track < (staff+1)*VOICES; ++track) {
                  for (Segment* segment = measure->first(); segment; segment = segment->next()) {
                        Element* ie  = segment->element(track);
                        if (!ie)
                              continue;
                        if (!ie->isChordRest())
                              continue;
                        if (ie->tick() == tick)
                              return ie;
                        if (ie->tick() >  tick)
                              return ipe ? ipe : ie;
                        ipe = ie;
                        }
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   clefOffset
//---------------------------------------------------------

int Score::clefOffset(int tick, int staffIdx) const
      {
      return clefTable[staff(staffIdx)->clef()->clef(tick)].yOffset;
      }

//---------------------------------------------------------
//   layout
//    - measures are akkumulated into systems
//    - systems are akkumulated into pages
//   already existent systems and pages are reused
//---------------------------------------------------------

void ScoreLayout::doLayout()
      {
      ::_spatium = _spatium;        // ??
      _needLayout = false;

// printf("do layout\n");

      int n = _score->nstaves();
      for (int i = 0; i < n; ++i) {
            for (Element* m = _measures.first(); m; m = m->next()) {
                  ((Measure*)m)->layoutNoteHeads(i);
                  }
            }

      Measure* im = (Measure*)(_measures.first());
      iPage    ip = _pages->begin();
      iSystem  is = _systems->begin();

      if (im == 0) {
            if (_pages->empty())
                  addPage();
            return;
            }

      //-----------------------------------------
      //    pass I:  process pages
      //-----------------------------------------

      while (im) {
            Page* page;
            if (ip == _pages->end()) {
                  page = addPage();
                  ip = _pages->end();
                  }
            else {
                  page = *ip;
                  ++ip;
                  }
            Measure* om = im;
            layoutPage(page, im, is);
            if (im == om) {
                  printf("empty page?\n");
                  break;
                  }
            }

      //---------------------------------------------------
      //   pass II:  place ties & slurs & hairpins
      //---------------------------------------------------

      for (Measure* m = first(); m; m = m->next()) {
            m->layout2();
            }

      //---------------------------------------------------
      //    remove remaining pages and systems
      //---------------------------------------------------

      for (iPage i = ip; i != _pages->end(); ++i)
            delete *i;
      _pages->erase(ip, _pages->end());

      for (iSystem i = is; i != _systems->end(); ++i)
            delete *i;
      _systems->erase(is, _systems->end());
      }

//---------------------------------------------------------
//   processSystemHeader
//    add generated timesig keysig and clef
//---------------------------------------------------------

void ScoreLayout::processSystemHeader(Measure* m)
      {
      int tick = m->tick();
      int nstaves = _score->nstaves();

      for (int i = 0; i < nstaves; ++i) {
            Staff* staff   = _score->staff(i);
            bool hasKeysig  = false;
            bool hasClef    = false;
            int strack      = i * VOICES;

            // we assume that keysigs and clefs are only in the first
            // track of a segment

            for (Segment* seg = m->first(); seg; seg = seg->next()) {
                  // search only up to the first ChordRest
                  if (seg->segmentType() == Segment::SegChordRest)
                        break;
                  Element* el = seg->element(strack);
                  if (!el)
                        continue;
                  switch (el->type()) {
                        case KEYSIG:
                              hasKeysig = true;
                              break;
                        case CLEF:
                              hasClef = true;
                              ((Clef*)el)->setSmall(false);
                              break;
                        default:
                              break;
                        }
                  }
            if (!hasKeysig) {
                  int idx = staff->keymap()->key(tick);
                  if (idx) {
                        KeySig* ks = new KeySig(_score);
                        ks->setStaff(staff);
                        ks->setTick(tick);
                        ks->setGenerated(true);
                        ks->setSubtype(idx);
                        Segment* seg = m->getSegment(ks);
                        seg->add(ks);
                        }
                  }
            if (!hasClef) {
                  int idx = staff->clef()->clef(tick);
                  Clef* cs = new Clef(_score, idx);
                  cs->setStaff(staff);
                  cs->setTick(tick);
                  cs->setGenerated(true);
                  Segment* s = m->getSegment(cs);
                  s->add(cs);
                  }
            }
      }

//---------------------------------------------------------
//   clearGenerated
//    remove generated elements form measure
//---------------------------------------------------------

void ScoreLayout::clearGenerated(Measure* m)
      {
      for (Segment* seg = m->first(); seg; seg = seg->next()) {
            QList<Element*>* el = seg->elist();
            for (QList<Element*>::iterator i = el->begin(); i != el->end(); ++i) {
                  Element* el = *i;
                  if (el && el->generated())
                        *i = 0;
                  }
            }
      }

//---------------------------------------------------------
//   addGenerated
//---------------------------------------------------------

void ScoreLayout::addGenerated(Measure*)
      {
/*      for (Segment* seg = m->first(); seg; seg = seg->next()) {
            int tick = seg->tick();

            QList<Element*>* el = seg->elist();
            for (QList<Element*>::iterator i = el->begin(); i != el->end(); ++i) {
                  Element* el = *i;
                  if (el && el->generated())
                        *i = 0;
                  }
            }
*/
      }

//---------------------------------------------------------
//   addMeasure
//---------------------------------------------------------

double Page::addMeasure(Measure* m, double y)
      {
      //---------------------------------------------------
      //    collect page elements from measure
      //---------------------------------------------------

      ElementList sel = *(m->pel());
      m->pel()->clear();
      bool textFound = false;
      for (iElement ie = sel.begin(); ie != sel.end(); ++ie) {
            Element* el = *ie;
            add(el);

            el->layout();
            if (el->type() == TEXT) {
                  Text* text = (Text*)el;
                  if (text->anchor() == ANCHOR_PAGE) {
                        // TODO: only collect top aligned page elements?
                        if (el->pos().y() > y)
                              y = el->pos().y();
                        textFound = true;
                        }
                  }
            }
      if (textFound)
            y += point(::style->staffUpperBorder);
      return y;
      }

//---------------------------------------------------------
//   layoutPage
//    return true, if next page must be relayouted
//---------------------------------------------------------

bool ScoreLayout::layoutPage(Page* page, Measure*& im, iSystem& is)
      {
      page->layout();

      // usable width of page:
      qreal w  = page->loWidth() - page->lm() - page->rm();
      qreal x  = page->lm();
      qreal ey = page->loHeight() - page->bm() - point(::style->staffLowerBorder);

      page->systems()->clear();
      page->pel()->clear();
      qreal y = page->addMeasure(im, page->tm());

      int systemNo = 0;
      while (im) {
            // get next system:
            System* system;
            if (is == _systems->end()) {
                  system = new System(_score);
                  _systems->push_back(system);
                  is = _systems->end();
                  }
            else {
                  system = *is++;
                  system->clear();   // remove measures from system
                  }

            layoutSystem(im, system, x, y, w);
            system->setParent(page);

            qreal h = system->bbox().height() + point(::style->systemDistance);
            if (y + h >= ey) {  // system does not fit on page
                  // rollback
                  im = system->measures()->front();
                  --is;
                  break;
                  }
            page->appendSystem(system);

            //  move system vertically to final position:

            double systemDistance;
            if (systemNo == 1)
                  systemDistance = point(style->staffUpperBorder);
            else
                  systemDistance = point(style->systemDistance);
            system->move(0.0, systemDistance);
            y += h;
            if (system->pageBreak())
                  break;
            }

      //-----------------------------------------------------------------------
      // if remaining y space on page is greater (pageHeight*pageFillLimit)
      // then insert space between staffs to fill page
      //-----------------------------------------------------------------------

      double restHeight = ey - y;
      double ph = page->height()
            - point(::style->staffLowerBorder + ::style->staffUpperBorder);

      if (restHeight > (ph * ::style->pageFillLimit))
            return true;

      SystemList* sl   = page->systems();
      int systems      = sl->size();
      double extraDist = restHeight / (systems-1);
      y                = 0;
      for (iSystem i = sl->begin(); i != sl->end(); ++i) {
            (*i)->move(0, y);
            y += extraDist;
            }
      return true;
      }

//---------------------------------------------------------
//   layoutSystem
//---------------------------------------------------------

System* ScoreLayout::layoutSystem(Measure*& im, System* system, qreal x, qreal y, qreal w)
      {
      for (int i = system->staves()->size(); i < _score->nstaves(); ++i)
            system->insertStaff(_score->staff(i), i);

      double systemOffset = system->layout(QPointF(x, y), w);

      //-------------------------------------------------------
      //    Round I
      //    find out how many measures fit in system
      //-------------------------------------------------------

      int nm              = 0;
      double systemWidth  = w - systemOffset;
      double minWidth     = 0;
      QList<double> mwList;
      double uStretch = 0.0;

      bool pageBreak = false;

      for (Measure* m = im; m; m = m->next()) {
            pageBreak = m->pageBreak();

            clearGenerated(m);
            m->setSystem(system);   // needed by m->layout()
            if (m == im) {
                  //
                  // special handling for first measure in a system:
                  // add generated clef and key signature
                  //
                  processSystemHeader(m);
                  }
            else {
                  addGenerated(m);  //DEBUG
                  //
                  // if this is not the first measure in a system
                  // switch all clefs to small size
                  //
                  int nstaves = _score->nstaves();
                  for (int i = 0; i < nstaves; ++i) {
                        int strack = i * VOICES;
                        for (Segment* seg = m->first(); seg; seg = seg->next()) {
                              Element* el = seg->element(strack);
                              if (el && el->type() == CLEF) {
                                    ((Clef*)el)->setSmall(true);
                                    break;
                                    }
                              }
                        }
                  }

            MeasureWidth mw = m->layoutX(1.0);
            double ww = mw.stretchable;

            mwList.push_back(ww);
            double stretch = m->userStretch() * ::style->measureSpacing;
            ww *= stretch;
            if (ww < point(::style->minMeasureWidth))
                  ww = point(::style->minMeasureWidth);

            if (minWidth + ww > systemWidth) {
                  // minimum is one measure
                  if (nm == 0) {
                        minWidth  += ww;
                        nm = 1;
                        printf("warning: system too small (%f+%f) > %f\n",
                           minWidth, ww, systemWidth);
                        // bad things are happening here
                        }
                  break;
                  }
            ++nm;
            minWidth  += ww;
            uStretch  += stretch;
            if (pageBreak || m->lineBreak())
                  break;
            }
      system->setPageBreak(pageBreak);

      //-------------------------------------------------------
      //    Round II
      //    stretch measures
      //    "nm" measures fit on this line of score
      //    "minWidth"   is the minimum width they use
      //    uStretch is the accumulated userStretch
      //-------------------------------------------------------

      minWidth = 0.0;
      double totalWeight = 0.0;
      Measure* itt = im;
      for (int i = 0; itt && (i < nm); ++i, itt = itt->next()) {
            minWidth    += mwList[i];
            totalWeight += itt->tickLen() * itt->userStretch();
            }

      double rest = layoutDebug ? 0.0 : systemWidth - minWidth;
      QPointF pos(systemOffset, 0);

      itt = im;
      for (int i = 0; itt && (i < nm); ++i, itt = itt->next()) {
            system->measures()->push_back(itt);
            itt->setPos(pos);
            double weight = itt->tickLen() * itt->userStretch();
            double ww     = mwList[i] + rest * weight / totalWeight;
            itt->layout(ww);
            pos.rx() += ww;
            }

      system->layout2();      // layout staff distances

      im = itt;
      return system;
      }

//---------------------------------------------------------
//   addPage
//---------------------------------------------------------

Page* ScoreLayout::addPage()
      {
      Page* page = new Page(this);
      page->setNo(_pages->size());
      _pages->push_back(page);
      _pages->update();
      return page;
      }

//---------------------------------------------------------
//   setPageFormat
//---------------------------------------------------------

void ScoreLayout::setPageFormat(const PageFormat& pf)
      {
      *_pageFormat = pf;
      }

//---------------------------------------------------------
//   ScoreLayout
//---------------------------------------------------------

ScoreLayout::ScoreLayout(const ScoreLayout& l)
      {
      _score      = l._score;
      _spatium    = l._spatium;
      _pageFormat = l._pageFormat;
      _measures   = l._measures;
      _pages      = new PageList(*(l._pages));
      _systems    = new SystemList(*(l._systems));
      _needLayout = l._needLayout;
      }


