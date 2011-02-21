//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: importmidi.cpp 2721 2010-02-15 19:41:28Z wschweer $
//
//  Copyright (C) 2002-2011 Werner Schweer and others
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

#include "importpdf.h"
#include "score.h"
#include "omr/omr.h"
#include "part.h"
#include "staff.h"
#include "measure.h"
#include "al/al.h"
#include "rest.h"
#include "omr/omrpage.h"
#include "segment.h"
#include "layoutbreak.h"
#include "page.h"
#include "clef.h"
#include "bracket.h"

//---------------------------------------------------------
//   importPdf
//---------------------------------------------------------

bool Score::importPdf(const QString& path)
      {
      _omr = new Omr(path, this);
      if (!_omr->readPdf()) {
            delete _omr;
            _omr = 0;
            return false;
            }
      _spatium = _omr->spatiumMM() * DPMM;
      style()->set(StyleVal(ST_pageFillLimit, 1.0));
      style()->set(StyleVal(ST_lastSystemFillLimit, 0.0));
      style()->set(StyleVal(ST_staffLowerBorder, 0.0));
      style()->set(StyleVal(ST_measureSpacing, 1.0));

      _pageFormat->evenLeftMargin   = 5.0 * DPMM / DPI;
      _pageFormat->evenRightMargin  = 5.0 * DPMM / DPI;
      _pageFormat->evenTopMargin    = 0;
      _pageFormat->evenBottomMargin = 0;
      _pageFormat->oddLeftMargin    = 5.0 * DPMM / DPI;
      _pageFormat->oddRightMargin   = 5.0 * DPMM / DPI;
      _pageFormat->oddTopMargin     = 0;
      _pageFormat->oddBottomMargin  = 0;

      style()->set(StyleVal(ST_systemDistance,   Spatium(_omr->systemDistance())));
      style()->set(StyleVal(ST_akkoladeDistance, Spatium(_omr->staffDistance())));

      Part* part   = new Part(this);
      Staff* staff = new Staff(this, part, 0);
      part->staves()->push_back(staff);
      staves().insert(0, staff);
      staff = new Staff(this, part, 1);
      part->staves()->push_back(staff);
      staves().insert(1, staff);
      part->staves()->front()->setBarLineSpan(part->nstaves());
      insertPart(part, 0);

      Duration d(Duration::V_MEASURE);
      Measure* measure;
      int tick = 0;
      foreach(const OmrPage* omrPage, _omr->pages()) {
            int nsystems = omrPage->systems().size();
            for (int k = 0; k < nsystems; ++k) {
                  const OmrSystem& omrSystem = omrPage->systems().at(k);
                  int numMeasures = omrSystem.barLines.size() - 1;
                  if (numMeasures < 1)
                        numMeasures = 1;
                  else if (numMeasures > 50)
                        numMeasures = 50;
                  for (int i = 0; i < numMeasures; ++i) {
                        measure = new Measure(this);
                        measure->setTick(tick);

		            Rest* rest = new Rest(this, d);
                        rest->setDuration(Fraction(4,4));
                        rest->setTrack(0);
                        Segment* s = measure->getSegment(SegChordRest, tick);
		            s->add(rest);
		            rest = new Rest(this, d);
                        rest->setDuration(Fraction(4,4));
                        rest->setTrack(4);
		            s->add(rest);

                        measures()->add(measure);
                        tick += AL::division * 4;
                        }
                  if (k < (nsystems-1)) {
                        LayoutBreak* b = new LayoutBreak(this);
                        b->setSubtype(LAYOUT_BREAK_LINE);
                        measure->add(b);
                        }
                  }
            LayoutBreak* b = new LayoutBreak(this);
            b->setSubtype(LAYOUT_BREAK_PAGE);
            measure->add(b);
            }

      //---create bracket

      _staves[0]->setBracket(0, BRACKET_AKKOLADE);
      _staves[0]->setBracketSpan(0, 2);

      //---create clefs

      measure = firstMeasure();
      Clef* clef = new Clef(this);
      clef->setClefType(CLEF_G);
      clef->setTrack(0);
      Segment* segment = measure->getSegment(SegClef, 0);
      segment->add(clef);

      clef = new Clef(this);
      clef->setClefType(CLEF_F);
      clef->setTrack(4);
      segment->add(clef);

      setShowOmr(true);
      _omr->page(0)->readHeader(this);
      rebuildMidiMapping();
      return true;
      }

