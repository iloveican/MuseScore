//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: layout.cpp 3705 2010-11-14 10:48:36Z wschweer $
//
//  Copyright (C) 2002-2010 Werner Schweer and others
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

#include <fenv.h>

#include "page.h"
#include "m-al/sig.h"
#include "key.h"
#include "clef.h"
#include "score.h"
#include "globals.h"
#include "segment.h"
#include "text.h"
#include "staff.h"
#include "style.h"
#include "timesig.h"
#include "chord.h"
#include "note.h"
#include "slur.h"
#include "keysig.h"
#include "barline.h"
#include "repeat.h"
#include "box.h"
#include "system.h"
#include "part.h"
#include "utils.h"
#include "measure.h"
#include "preferences.h"
#include "volta.h"
#include "beam.h"
#include "tuplet.h"
#include "sym.h"

bool layoutDebug = false;

//---------------------------------------------------------
//   rebuildBspTree
//---------------------------------------------------------

void Score::rebuildBspTree()
      {
      foreach(Page* page, _pages)
            page->rebuildBspTree();
      }

//---------------------------------------------------------
//   clefOffset
//---------------------------------------------------------

int Score::clefOffset(int tick, Staff* staff) const
      {
      return clefTable[staff->clefList()->clef(tick)].yOffset;
      }

//---------------------------------------------------------
//   AcEl
//---------------------------------------------------------

struct AcEl {
      Note* note;
      qreal x;
      };

//---------------------------------------------------------
//   layoutChords1
//    only called from layout0
//    - calculate displaced note heads
//---------------------------------------------------------

void Score::layoutChords1(Segment* segment, int staffIdx)
      {
      Staff* staff = Score::staff(staffIdx);

      if (staff->part()->instr()->drumset() || staff->useTablature())
            return;

      int startTrack = staffIdx * VOICES;
      int endTrack   = startTrack + VOICES;
      int voices     = 0;
      QList<Note*> notes;
      for (int track = startTrack; track < endTrack; ++track) {
            Element* e = segment->element(track);
            if (e && (e->type() == CHORD)) {
                  ++voices;
                  notes.append(static_cast<Chord*>(e)->notes());
                  }
            }
      if (notes.isEmpty())
            return;

      int startIdx, endIdx, incIdx;

      if (notes[0]->chord()->up() || (voices > 1)) {
            startIdx = 0;
            incIdx   = 1;
            endIdx   = notes.size();
            for (int i = 0; i < endIdx-1; ++i) {
                  if ((notes[i]->line() == notes[i+1]->line())
                     && (notes[i]->track() != notes[i+1]->track())
                     && (!notes[i]->chord()->up() && notes[i+1]->chord()->up())
                     ) {
                        Note* n = notes[i];
                        notes[i] = notes[i+1];
                        notes[i+1] = n;
                        }
                  }
            }
      else {
            startIdx = notes.size() - 1;
            incIdx   = -1;
            endIdx   = -1;
            }

      bool moveLeft = false;
      int ll        = 1000;      // line distance to previous note head
      bool isLeft   = notes[startIdx]->chord()->up();
      int move1     = notes[startIdx]->chord()->staffMove();
      bool mirror   = false;
      int lastHead  = -1;

      for (int idx = startIdx; idx != endIdx; idx += incIdx) {
            Note* note   = notes[idx];
            Chord* chord = note->chord();
            int move     = chord->staffMove();
            int line     = note->line();
            int ticks    = chord->ticks();
            int head     = note->noteHead();      // symbol number or note head

            bool conflict = (qAbs(ll - line) < 2) && (move1 == move);
            bool sameHead = (ll == line) && (head == lastHead);
            if ((chord->up() != isLeft) || conflict)
                  isLeft = !isLeft;
            bool nmirror  = (chord->up() != isLeft) && !sameHead;

            note->setHidden(false);
            chord->rxpos() = 0.0;

            if (conflict && (nmirror == mirror) && idx) {
                  if (sameHead) {
                        Note* pnote = notes[idx-1];
                        if (note->userOff().isNull() && pnote->userOff().isNull()) {
                              if (ticks > pnote->chord()->ticks()) {
                                    pnote->setHidden(true);
                                    // TODO: pnote->setAccidentalType(ACC_NONE);
                                    note->setHidden(false);
                                    }
                              else {
                                    // TODO: note->setAccidentalType(ACC_NONE);
                                    note->setHidden(true);
                                    }
                              }
                        else
                              note->setHidden(false);
                        }
                  else {
                        if ((line > ll) || !chord->up()) {
                              note->chord()->rxpos() = note->headWidth() - note->point(styleS(ST_stemWidth));
                              note->rxpos() = 0.0;
                              }
                        else {
                              notes[idx-incIdx]->chord()->rxpos() = note->headWidth() - note->point(styleS(ST_stemWidth));
                              note->rxpos() = 0.0;
                              }
                        moveLeft = true;
                        }
                  }
            if (note->userMirror() == DH_AUTO) {
                  mirror = nmirror;
                  }
            else {
                  mirror = note->chord()->up();
                  if (note->userMirror() == DH_LEFT)
                        mirror = !mirror;
                  }
            note->setMirror(mirror);
            if (mirror)                   //??
                  moveLeft = true;

            move1    = move;
            ll       = line;
            lastHead = head;
            }

      //---------------------------------------------------
      //    layout accidentals
      //    find column for dots
      //---------------------------------------------------

      QList<AcEl> aclist;

      qreal dotPosX  = 0.0;
      int nNotes = notes.size();
      qreal headWidth = notes[0]->headWidth();
      for (int i = nNotes-1; i >= 0; --i) {
            Note* note     = notes[i];
            Accidental* ac = note->accidental();
            if (ac) {
                  ac->setMag(note->mag());
                  ac->layout();
                  AcEl acel;
                  acel.note = note;
                  acel.x    = 0.0;
                  aclist.append(acel);
                  }
            qreal xx = note->pos().x() + headWidth;
            if (xx > dotPosX)
                  dotPosX = xx;
            }
      for (int track = startTrack; track < endTrack; ++track) {
            Element* e = segment->element(track);
            if (e && e->type() == CHORD)
                  static_cast<Chord*>(e)->setDotPosX(dotPosX - e->pos().x());
            }

      int nAcc = aclist.size();
      if (nAcc == 0)
            return;
      qreal pd  = point(styleS(ST_accidentalDistance));
      qreal pnd = point(styleS(ST_accidentalNoteDistance));
      //
      // layout top accidental
      //
      Note* note      = aclist[0].note;
      Accidental* acc = note->accidental();
      aclist[0].x     = -pnd * acc->mag() - acc->width() - acc->bbox().x();

      //
      // layout bottom accidental
      //
      if (nAcc > 1) {
            note = aclist[nAcc-1].note;
            acc  = note->accidental();
            int l1 = aclist[0].note->line();
            int l2 = note->line();

            int st1   = aclist[0].note->accidental()->subtype();
            int st2   = acc->subtype();
            int ldiff = st1 == ACC_FLAT ? 4 : 5;

            if (qAbs(l1-l2) > ldiff) {
                  aclist[nAcc-1].x = -pnd * acc->mag() - acc->width() - acc->bbox().x();
                  }
            else {
                  if ((st1 == ACC_FLAT) && (st2 == ACC_FLAT) && (qAbs(l1-l2) > 2))
                        aclist[nAcc-1].x = aclist[0].x - acc->width() * .5;
                  else
                        aclist[nAcc-1].x = aclist[0].x - acc->width();
                  }
            }

      //
      // layout middle accidentals
      //
      if (nAcc > 2) {
            int n = nAcc - 1;
            for (int i = 1; i < n; ++i) {
                  note = aclist[i].note;
                  acc  = note->accidental();
                  int l1 = aclist[i-1].note->line();
                  int l2 = note->line();
                  int l3 = aclist[n].note->line();
                  qreal x = 0.0;

                  int st1 = aclist[i-1].note->accidental()->subtype();
                  int st2 = acc->subtype();

                  int ldiff = st1 == ACC_FLAT ? 4 : 5;
                  if (qAbs(l1-l2) <= ldiff) {   // overlap accidental above
                        if ((st1 == ACC_FLAT) && (st2 == ACC_FLAT) && (qAbs(l1-l2) > 2))
                              x = aclist[i-1].x + acc->width() * .5;    // undercut flats
                        else
                              x = aclist[i-1].x;
                        }

                  ldiff = acc->subtype() == ACC_FLAT ? 4 : 5;
                  if (qAbs(l2-l3) <= ldiff) {       // overlap accidental below
                        if (aclist[n].x < x)
                              x = aclist[n].x;
                        }
                  if (x == 0.0 || x > acc->width())
                        x = -pnd * acc->mag() - acc->bbox().x();
                  else
                        x -= pd * acc->mag();   // accidental distance
                  aclist[i].x = x - acc->width() - acc->bbox().x();
                  }
            }

      foreach(const AcEl e, aclist) {
            Note* note = e.note;
            qreal x   = e.x;
            if (moveLeft) {
                  Chord* chord = note->chord();
                  if (((note->mirror() && chord->up()) || (!note->mirror() && !chord->up())))
                        x -= note->headWidth();
                  }
            note->accidental()->setPos(x, 0);
            }
      }

//-------------------------------------------------------------------
//    layoutStage1
//    - compute note head lines and accidentals
//    - mark multi measure rest breaks if in multi measure rest mode
//-------------------------------------------------------------------

void Score::layoutStage1()
      {
      int idx = 0;
      for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
            ++idx;
            m->layoutStage1();
            MeasureBase* mb = m->prev();
            if (mb && mb->type() == MEASURE) {
                  Measure* prev = static_cast<Measure*>(mb);
                  if (prev->endBarLineType() != NORMAL_BAR && prev->endBarLineType() != BROKEN_BAR)
                        m->setBreakMMRest(true);
                  }
            }
      }

//---------------------------------------------------------
//   layoutStage2
//    auto - beamer
//---------------------------------------------------------

void Score::layoutStage2()
      {
      int tracks = nstaves() * VOICES;

      QList<Beam*> usedBeams;
      foreach(Beam* beam, _beams)
            beam->clear();

      for (int track = 0; track < tracks; ++track) {
            ChordRest* a1    = 0;      // start of (potential) beam
            Beam* beam       = 0;
            Measure* measure = 0;

            BeamMode bm = BEAM_AUTO;
            for (Segment* segment = firstSegment(); segment; segment = segment->next1()) {
                  Element* e = segment->element(track);
                  if (((segment->subtype() != SegChordRest) && (segment->subtype() != SegGrace))
                     || e == 0 || !e->isChordRest())
                        continue;
                  ChordRest* cr = static_cast<ChordRest*>(e);
                  bm            = cr->beamMode();
                  if (cr->measure() != measure) {
                        if (measure && (bm != BEAM_MID)) {
                              if (beam) {
                                    beam->layout1();
                                    beam = 0;
                                    }
                              else if (a1) {
                                    a1->setBeam(0);
                                    a1->layoutStem1();
                                    a1 = 0;
                                    }
                              }
                        measure = cr->measure();
                        if (bm != BEAM_MID) {
                              a1      = 0;
                              beam    = 0;
                              }
                        }
                  if (segment->subtype() == SegGrace) {
                        Segment* nseg = segment->next();
                        if (nseg
                           && nseg->subtype() == SegGrace
                           && nseg->element(track)
                           && static_cast<ChordRest*>(nseg->element(track))->durationType().hooks())
                              {
                              Beam* b = cr->beam();
                              if (b == 0) {
                                    b = new Beam(this);
                                    b->setTrack(track);
                                    b->setGenerated(true);
                                    add(b);
                                    }
                              b->add(cr);
                              Segment* s = nseg;
                              for (;;) {
                                    nseg = s;
                                    ChordRest* cr = static_cast<ChordRest*>(nseg->element(track));
                                    b->add(cr);
                                    s = nseg->next();
                                    if (!s || (s->subtype() != SegGrace) || !s->element(track))
                                          break;
                                    }
                              b->layout1();
                              segment = nseg;
                              }
                        else {
                              cr->setBeam(0);
                              cr->layoutStem1();
                              }
                        continue;
                        }
                  int len = cr->durationType().ticks();

                  if ((len >= AL::division) || (bm == BEAM_NO)) {
                        if (beam) {
                              beam->layout1();
                              beam = 0;
                              }
                        if (a1) {
                              a1->setBeam(0);
                              a1->layoutStem1();
                              a1 = 0;
                              }
                        cr->setBeam(0);
                        cr->layoutStem1();
                        continue;
                        }
                  bool beamEnd = false;
                  if (beam) {
                        ChordRest* le = beam->elements().back();
                        if (((bm != BEAM_MID) && (le->tuplet() != cr->tuplet())) || (bm == BEAM_BEGIN)) {
                              beamEnd = true;
                              }
                        else if (bm != BEAM_MID) {
                              if (endBeam(measure->timesig(), cr, cr->tick() - measure->tick()))
                                    beamEnd = true;
                              }
                        if (beamEnd) {
                              beam->layout1();
                              beam = 0;
                              }
                        else {
                              beam->add(cr);
                              cr = 0;

                              // is cr the last beam element?
                              if (bm == BEAM_END) {
                                    beam->layout1();
                                    beam = 0;
                                    }
                              }
                        }
                  if (cr && cr->tuplet() && (cr->tuplet()->elements().back() == cr)) {
                        if (beam) {
                              beam->layout1();
                              beam = 0;

                              cr->setBeam(0);
                              cr->layoutStem1();
                              }
                        else if (a1) {
                              beam = a1->beam();
                              if (beam == 0 || usedBeams.contains(beam)) {
                                    beam = new Beam(this);
                                    beam->setTrack(track);
                                    beam->setGenerated(true);
                                    add(beam);
                                    }
                              usedBeams.prepend(beam);
                              beam->add(a1);
                              beam->add(cr);
                              a1 = 0;
                              beam->layout1();
                              beam = 0;
                              }
                        else {
                              cr->setBeam(0);
                              cr->layoutStem1();
                              }
                        }
                  else if (cr) {
                        if (a1 == 0)
                              a1 = cr;
                        else {
                              if (bm != BEAM_MID
                                   &&
                                   (endBeam(measure->timesig(), cr, cr->tick() - measure->tick())
                                   || bm == BEAM_BEGIN
                                   || (a1->segment()->subtype() != cr->segment()->subtype())
                                   )
                                 ) {
                                    a1->setBeam(0);
                                    a1->layoutStem1();      //?
                                    a1 = cr;
                                    }
                              else {
                                    beam = a1->beam();
                                    if (beam == 0 || usedBeams.contains(beam)) {
                                          beam = new Beam(this);
                                          beam->setGenerated(true);
                                          beam->setTrack(track);
                                          add(beam);
                                          }
                                    usedBeams.append(beam);
                                    beam->add(a1);
                                    beam->add(cr);
                                    a1 = 0;
                                    }
                              }
                        }
                  }
            if (beam)
                  beam->layout1();
            else if (a1) {
                  a1->setBeam(0);
                  a1->layoutStem1();
                  }
            }
      foreach (Beam* beam, _beams) {
            if (beam->elements().isEmpty()) {
                  remove(beam);
                  delete beam;
                  }
            }
      }

//---------------------------------------------------------
//   layoutStage3
//---------------------------------------------------------

void Score::layoutStage3()
      {
      for (int staffIdx = 0; staffIdx < nstaves(); ++staffIdx) {
            for (Segment* segment = firstSegment(); segment; segment = segment->next1()) {
                  if ((segment->subtype() == SegChordRest) || (segment->subtype() == SegGrace)) {
                        layoutChords1(segment, staffIdx);
                        }
                  }
            }
      }

//---------------------------------------------------------
//   layout
//    - measures are akkumulated into systems
//    - systems are akkumulated into pages
//   already existent systems and pages are reused
//---------------------------------------------------------

void Score::doLayout()
      {
      _symIdx = _style.valueSt(ST_MusicalSymbolFont) == "Gonville" ? 1 : 0;

      fixTicks();
      updateVelo();

      int nstaves = _staves.size();
      for (int staffIdx = 0; staffIdx < nstaves; ++staffIdx) {
            int track = staffIdx * VOICES;
            Staff* st = _staves[staffIdx];
            KeySig* key1 = 0;
            for (Measure* m = firstMeasure(); m; m = m->nextMeasure()) {
                  for (Segment* s = m->first(); s; s = s->next()) {
                        if (!s->element(track))
                              continue;
                        Element* e = s->element(track);
                        if (e->generated())
                              continue;
                        if (s->subtype() == SegClef) {
                              Clef* clef = static_cast<Clef*>(e);
                              st->setClef(s->tick(), clef->clefType());
                              }
                        else if (s->subtype() == SegKeySig) {
                              KeySig* ks = static_cast<KeySig*>(e);
                              int naturals = key1 ? key1->keySigEvent().accidentalType() : 0;
                              ks->setOldSig(naturals);
                              st->setKey(s->tick(), ks->keySigEvent());
                              key1 = ks;
                              }
                        }
                  if (m->sectionBreak())
                        key1 = 0;
                  }
            }

      if (_staves.isEmpty() || first() == 0) {
            // score is empty
            foreach(Page* page, _pages)
                  delete page;
            _pages.clear();

            Page* page = addPage();
            page->layout();
            page->setNo(0);
            page->rebuildBspTree();
            return;
            }

      layoutStage1();   // compute note head lines and accidentals
      layoutStage2();   // beam notes, finally decide if chord is up/down
      layoutStage3();   // compute note head horizontal positions

      //-----------------------------------------------------------------------
      //    layout measures into systems and pages
      //-----------------------------------------------------------------------

      curMeasure  = first();
      curSystem   = 0;
      firstSystem = true;
      for (curPage = 0; curMeasure; curPage++) {
            getCurPage();
            MeasureBase* om = curMeasure;
            if (!layoutPage())
                  break;
            if (curMeasure == om) {
                  printf("empty page?\n");
                  break;
                  }
            }

      //---------------------------------------------------
      //   place Spanner & beams
      //---------------------------------------------------

      foreach(Beam* beam, _beams)
            beam->layout();

      int tracks = nstaves * VOICES;
      for (int track = 0; track < tracks; ++track) {
            for (Segment* segment = firstSegment(); segment; segment = segment->next1()) {
                  Element* e = segment->element(track);
                  if (e && e->isChordRest()) {
                        ChordRest* cr = static_cast<ChordRest*>(e);
                        cr->layoutArticulations();    // DEBUG
                        if (cr->type() == CHORD) {
                              Chord* c = static_cast<Chord*>(cr);
                              if (!c->beam())
                                    c->layoutStem();
                              c->layoutArpeggio2();
                              foreach(Note* n, c->notes()) {
                                    Tie* tie = n->tieFor();
                                    if (tie)
                                          tie->layout();
                                    }
                              }
                        }
                  else if (e && e->type() == BAR_LINE)
                        e->layout();
                  if (track == tracks-1) {
                        foreach(Spanner* s, segment->spannerFor())
                              s->layout();
                        foreach(Element* e, segment->annotations())
                              e->layout();
                        }
                  }
            }

      for (Measure* m = firstMeasure(); m; m = m->nextMeasure())
            m->layout2();

      //---------------------------------------------------
      //    remove remaining pages and systems
      //---------------------------------------------------

      int n = _pages.size() - curPage;
      for (int i = 0; i < n; ++i) {
            Page* page = _pages.takeLast();
            delete page;
            }
      n = _systems.size() - curSystem;
      for (int i = 0; i < n; ++i) {
            System* system = _systems.takeLast();
            delete system;
            }
      rebuildBspTree();
      }

//---------------------------------------------------------
//   processSystemHeader
//    add generated timesig keysig and clef
//---------------------------------------------------------

void Score::processSystemHeader(Measure* m, bool isFirstSystem)
      {
      int tick = m->tick();
      int i = 0;
      foreach (Staff* staff, _staves) {
            if (!m->system()->staff(i)->show()) {
                  ++i;
                  continue;
                  }

            KeySig* hasKeysig = 0;
            Clef*   hasClef   = 0;
            int strack        = i * VOICES;

            // we assume that keysigs and clefs are only in the first
            // track of a segment

            const KeySigEvent& keyIdx = staff->keymap()->key(tick);

            for (Segment* seg = m->first(); seg; seg = seg->next()) {
                  // search only up to the first ChordRest
                  if (seg->subtype() == SegChordRest)
                        break;
                  Element* el = seg->element(strack);
                  if (!el)
                        continue;
                  switch (el->type()) {
                        case KEYSIG:
                              hasKeysig = static_cast<KeySig*>(el);
                              hasKeysig->changeKeySigEvent(keyIdx);
                              hasKeysig->setMag(staff->mag());
                              break;
                        case CLEF:
                              hasClef = static_cast<Clef*>(el);
                              hasClef->setMag(staff->mag());
                              hasClef->setSmall(false);
                              break;
                        case TIMESIG:
                              el->setMag(staff->mag());
                              break;
                        default:
                              break;
                        }
                  }
            bool needKeysig = keyIdx.isValid()
               /* && keyIdx.accidentalType() != 0 */
               && (isFirstSystem || styleB(ST_genKeysig))
               ;
            if (staff->useTablature())
                  needKeysig = false;
            if (needKeysig && !hasKeysig) {
                  //
                  // create missing key signature
                  //
                  KeySig* ks = keySigFactory(keyIdx);
                  KeySigEvent oKeySigBefore = staff->key(tick-1);
                  // if signature is not custom or prev. signature has same accid. as
                  // this one, reset naturals
                  if (!ks->isCustom() && oKeySigBefore.accidentalType() == ks->keySignature())
                        ks->setOldSig(0);
                  ks->setTrack(i * VOICES);
                  ks->setGenerated(true);
                  ks->setMag(staff->mag());
                  Segment* seg = m->getSegment(ks, tick);
                  seg->add(ks);
                  m->setDirty();
                  }
            else if (!needKeysig && hasKeysig) {
                  int track = hasKeysig->track();
                  Segment* seg = hasKeysig->segment();
                  seg->setElement(track, 0);    // TODO: delete element
                  m->setDirty();
                  }
            bool needClef = isFirstSystem || styleB(ST_genClef);
            if (needClef) {
                  ClefType idx = staff->clefList()->clef(tick);
                  if (!hasClef) {
                        //
                        // create missing clef
                        //
                        hasClef = new Clef(this);
                        hasClef->setTrack(i * VOICES);
                        hasClef->setGenerated(true);
                        hasClef->setSmall(false);
                        hasClef->setMag(staff->mag());
                        Segment* s = m->getSegment(hasClef, tick);
                        s->add(hasClef);
                        m->setDirty();
                        }
                  hasClef->setClefType(idx);
                  }
            else {
                  if (hasClef) {
                        int track = hasClef->track();
                        Segment* seg = hasClef->segment();
                        seg->setElement(track, 0);    // TODO: delete element
                        m->setDirty();
                        }
                  }
            ++i;
            }
      }

//---------------------------------------------------------
//   getNextSystem
//---------------------------------------------------------

System* Score::getNextSystem(bool isFirstSystem, bool isVbox)
      {
      System* system;
      if (curSystem >= _systems.size()) {
            system = new System(this);
            _systems.append(system);
            }
      else {
            system = _systems[curSystem];
            system->clear();   // remove measures from system
            }
      system->setFirstSystem(isFirstSystem);
      system->setVbox(isVbox);
      if (!isVbox) {
            int nstaves = Score::nstaves();
            for (int i = system->staves()->size(); i < nstaves; ++i)
                  system->insertStaff(i);
            int dn = system->staves()->size() - nstaves;
            for (int i = 0; i < dn; ++i)
                  system->removeStaff(system->staves()->size()-1);
            }
      return system;
      }

//---------------------------------------------------------
//   getCurPage
//---------------------------------------------------------

void Score::getCurPage()
      {
      Page* page = curPage >= _pages.size() ? addPage() : _pages[curPage];
      page->setNo(curPage);
      page->layout();
      }

//---------------------------------------------------------
//   layoutPage
//    return true, if next page must be relayouted
//---------------------------------------------------------

bool Score::layoutPage()
      {
      Page* page = _pages[curPage];
      const qreal slb = point(styleS(ST_staffLowerBorder));
      const qreal sub = point(styleS(ST_staffUpperBorder));

      // usable width of page:
      qreal w  = page->loWidth() - page->lm() - page->rm();
      qreal x  = page->lm();
      qreal ey = page->loHeight() - page->bm() - slb;

      page->clear();
      qreal y = page->tm();

      int rows = 0;
      bool firstSystemOnPage = true;

      while (curMeasure) {
            qreal h;
            QList<System*> sl;
            ElementType t = curMeasure->type();
            if (t == VBOX || t == TBOX || t == FBOX) {
                  System* system = getNextSystem(false, true);

                  foreach(SysStaff* ss, *system->staves())
                        delete ss;
                  system->staves()->clear();

                  system->setWidth(w);
                  VBox* vbox = static_cast<VBox*>(curMeasure);
                  vbox->setParent(system);
                  vbox->layout();
                  h = vbox->height();

                  // put at least one system on page
                  if (((y + h) > ey) && !firstSystemOnPage)
                        break;

                  system->setPos(x, y);
                  system->setHeight(h);
                  system->setPageBreak(vbox->pageBreak());
                  sl.append(system);

                  system->measures().push_back(vbox);
                  page->appendSystem(system);

                  curMeasure = curMeasure->next();
                  ++curSystem;
                  y += h + point(styleS(ST_frameSystemDistance));
                  if (y > ey) {
                        ++rows;
                        break;
                        }
                  }
            else {
                  if (firstSystemOnPage)
                        y += sub;
                  int cs          = curSystem;
                  MeasureBase* cm = curMeasure;
                  sl = layoutSystemRow(x, y, w, firstSystem, &h);
                  if (sl.isEmpty()) {
                        qFatal("layoutSystemRow returns zero systems\n");
                        abort();
                        }
                  qreal moveY = 0.0;
                  if (!page->systems()->isEmpty()) {
                        System* ps = page->systems()->back();
                        qreal b1;
                        if (ps->staves()->isEmpty())
                              b1 = 0.0;
                        else
                              b1 = ps->distanceDown(ps->staves()->size() - 1).val() * _spatium;
                        qreal b2  = 0.0;
                        foreach(System* s, sl) {
                              if (s->distanceUp(0).val() * _spatium > b2)
                                    b2 = s->distanceUp(0).val() * _spatium;
                              }
                        if (b2 > b1)
                              moveY = b2 - b1;
                        }

                  // a page contains at least one system
                  if (rows && (y + h + moveY > ey)) {
                        // system does not fit on page: rollback
                        curMeasure = cm;
                        curSystem  = cs;
                        break;
                        }
                  if (moveY > 0.0) {
                        y += moveY;
                        foreach(System* s, sl) {
                              s->rypos() = y;
                              }
                        }

                  foreach (System* system, sl) {
                        page->appendSystem(system);
                        system->rypos() = y;
                        }
                  firstSystem       = false;
                  firstSystemOnPage = false;
                  y += h;
                  }
            ++rows;
            if (sl.back()->pageBreak())
                  break;
            }

      //-----------------------------------------------------------------------
      // if remaining y space on page is greater (pageHeight*pageFillLimit)
      // then increase system distance to fill page
      //-----------------------------------------------------------------------

      qreal restHeight = ey - y;
      qreal ph         = page->loHeight() - page->bm() - page->tm() - slb - sub;

      if ((rows <= 1) || (restHeight > (ph * (1.0 - styleD(ST_pageFillLimit)))))
            return true;

      qreal systemDistance = point(styleS(ST_systemDistance));
      qreal extraDist      = (restHeight + systemDistance) / (rows - 1);
      y = 0;
      int n = page->systems()->size();
      for (int i = 0; i < n;) {
            System* system = page->systems()->at(i);
            qreal yy = system->pos().y();
            system->move(0, y);
            ++i;
            if (i >= n)
                  break;
            System* nsystem = page->systems()->at(i);
            if (nsystem->pos().y() != yy)
                  y += extraDist;               // next system row
            }
      return true;
      }

//---------------------------------------------------------
//   skipEmptyMeasures
//    search for empty measures; return number if empty
//    measures in sequence
//---------------------------------------------------------

Measure* Score::skipEmptyMeasures(Measure* m, System* system)
      {
      Measure* sm = m;
      int n       = 0;
      while (m->isEmpty()) {
            MeasureBase* mb = m->next();
            if (m->breakMultiMeasureRest() && n)
                  break;
            ++n;
            if (!mb || (mb->type() != MEASURE))
                  break;
            m = static_cast<Measure*>(mb);
            }
      m = sm;
      if (n >= styleI(ST_minEmptyMeasures)) {
            m->setMultiMeasure(n);  // first measure is presented as multi measure rest
            m->setSystem(system);
            for (int i = 1; i < n; ++i) {
                  m = static_cast<Measure*>(m->next());
                  m->setMultiMeasure(-1);
                  m->setSystem(system);
                  }
            }
      else
            m->setMultiMeasure(0);
      return m;
      }

//---------------------------------------------------------
//   layoutSystem1
//    return true on line break
//---------------------------------------------------------

bool Score::layoutSystem1(qreal& minWidth, qreal w, bool isFirstSystem)
      {
      System* system = getNextSystem(isFirstSystem, false);

      qreal xo = 0;
      if (curMeasure->type() == HBOX)
            xo = point(static_cast<Box*>(curMeasure)->boxWidth());

//      system->setInstrumentNames();
      system->layout(xo);

      minWidth            = system->leftMargin();
      qreal systemWidth  = w;

      bool continueFlag   = false;

      int nstaves = Score::nstaves();
      bool isFirstMeasure = true;

      for (; curMeasure;) {
            MeasureBase* nextMeasure;
            if (curMeasure->type() == MEASURE) {
                  Measure* m = static_cast<Measure*>(curMeasure);
                  if (styleB(ST_createMultiMeasureRests)) {
                        nextMeasure = skipEmptyMeasures(m, system)->next();
                        }
                  else {
                        m->setMultiMeasure(0);
                        nextMeasure = curMeasure->next();
                        }
                  }
            else
                  nextMeasure = curMeasure->next();

            System* oldSystem = curMeasure->system();
            curMeasure->setSystem(system);
            qreal ww      = 0.0;
            qreal stretch = 0.0;

            if (curMeasure->type() == HBOX) {
                  ww = point(static_cast<Box*>(curMeasure)->boxWidth());
                  if (!isFirstMeasure) {
                        // try to put another system on current row
                        // if not a line break
                        continueFlag = !curMeasure->lineBreak();
                        }
                  }
            else if (curMeasure->type() == MEASURE) {
                  Measure* m = static_cast<Measure*>(curMeasure);
                  if (isFirstMeasure)
                        processSystemHeader(m, isFirstSystem);

                  //
                  // remove generated elements
                  //    assume: generated elements are only living in voice 0
                  //    TODO: check if removed elements can be deleted
                  //
                  for (Segment* seg = m->first(); seg; seg = seg->next()) {
                        if (seg->subtype() == SegEndBarLine)
                              continue;
                        for (int staffIdx = 0;  staffIdx < nstaves; ++staffIdx) {
                              int track = staffIdx * VOICES;
                              Element* el = seg->element(track);
                              if (el == 0)
                                    continue;
                              if (el->generated()) {
                                    if (!isFirstMeasure || (seg->subtype() == SegTimeSigAnnounce))
                                          seg->setElement(track, 0);
                                    }
                              qreal staffMag = staff(staffIdx)->mag();
                              if (el->type() == CLEF) {
                                    Clef* clef = static_cast<Clef*>(el);
                                    clef->setSmall(!isFirstMeasure || (seg != m->first()));
                                    clef->setMag(staffMag);
                                    }
                              else if (el->type() == KEYSIG || el->type() == TIMESIG)
                                    el->setMag(staffMag);
                              }
                        }

                  m->createEndBarLines();

                  m->layoutX(1.0);
                  ww      = m->layoutWidth().stretchable;
                  stretch = m->userStretch() * styleD(ST_measureSpacing);

                  ww *= stretch;
                  if (ww < point(styleS(ST_minMeasureWidth)))
                        ww = point(styleS(ST_minMeasureWidth));
                  isFirstMeasure = false;
                  }

            // collect at least one measure
            if ((minWidth + ww > systemWidth) && !system->measures().isEmpty()) {
                  curMeasure->setSystem(oldSystem);
                  break;
                  }

            minWidth += ww;
            system->measures().append(curMeasure);
            ElementType nt = curMeasure->next() ? curMeasure->next()->type() : INVALID;
            int n = styleI(ST_FixMeasureNumbers);
            if ((n && system->measures().size() >= n)
               || continueFlag || curMeasure->pageBreak()
               || curMeasure->lineBreak()
               || (nt == VBOX || nt == TBOX || nt == FBOX)) {
                  system->setPageBreak(curMeasure->pageBreak());
                  curMeasure = nextMeasure;
                  break;
                  }
            else
                  curMeasure = nextMeasure;
            }
      system->setInstrumentNames();

      //
      //    hide empty staves
      //
      bool showChanged = false;
      int staves = system->staves()->size();
      int staffIdx = 0;
      foreach (Part* p, _parts) {
            int nstaves   = p->nstaves();
            bool hidePart = false;

            if (styleB(ST_hideEmptyStaves) && (staves > 1)) {
                  hidePart = true;
                  for (int i = staffIdx; i < staffIdx + nstaves; ++i) {
                        foreach(MeasureBase* m, system->measures()) {
                              if (m->type() != MEASURE)
                                    continue;
                              if (!((Measure*)m)->isMeasureRest(i)) {
                                    hidePart = false;
                                    break;
                                    }
                              }
                        }
                  }

            for (int i = staffIdx; i < staffIdx + nstaves; ++i) {
                  SysStaff* s  = system->staff(i);
                  Staff* staff = Score::staff(i);
                  bool oldShow = s->show();
                  s->setShow(hidePart ? false : staff->show());
                  if (oldShow != s->show()) {
                        showChanged = true;
                        }
                  }
            staffIdx += nstaves;
            }
#if 0 // DEBUG: endless recursion can happen if number of measures change
      // relayout if stave's show status has changed
      if (showChanged) {
            minWidth = 0;
            curMeasure = firstMeasure;
            bool val = layoutSystem1(minWidth, w, isFirstSystem);
            return val;
            }
#endif
      return continueFlag && curMeasure;
      }

//---------------------------------------------------------
//   layoutSystemRow
//    x, y  position of row on page
//---------------------------------------------------------

QList<System*> Score::layoutSystemRow(qreal x, qreal y, qreal rowWidth,
   bool isFirstSystem, qreal* h)
      {
      bool raggedRight = layoutDebug;

      *h = 0.0;
      QList<System*> sl;

      qreal ww = rowWidth;
      qreal minWidth;
      for (bool a = true; a;) {
            a = layoutSystem1(minWidth, ww, isFirstSystem);
            sl.append(_systems[curSystem]);
            ++curSystem;
            ww -= minWidth;
            }

      //
      // dont stretch last system row, if minWidth is <= lastSystemFillLimit
      //
      if (curMeasure == 0 && ((minWidth / rowWidth) <= styleD(ST_lastSystemFillLimit)))
            raggedRight = true;

      //-------------------------------------------------------
      //    Round II
      //    stretch measures
      //    "nm" measures fit on this line of score
      //-------------------------------------------------------

      bool needRelayout = false;

      foreach(System* system, sl) {
            //
            //    add cautionary time/key signatures if needed
            //

            if (system->measures().isEmpty()) {
                  qFatal("system %p is empty\n", system);
                  abort();
                  }
            Measure* m = system->lastMeasure();
            bool hasCourtesyKeysig = false;
            Measure* nm = m ? m->nextMeasure() : 0;
            Segment* s;

            if (m && nm && !m->sectionBreak()) {
                  int tick        = m->tick() + m->ticks();
                  Fraction sig2   = m->timesig();
                  Fraction sig1   = nm->timesig();

                  // locate a time sig. in the next measure and, if found,
                  // check if it has cout. sig. turned off
                  TimeSig* ts;
                  Segment* tss = nm->findSegment(SegTimeSig, tick);
                  bool showCourtesySig = true;              // assume this time time change has court. sig turned on
                  if (tss) {
                        ts = static_cast<TimeSig*>(tss->element(0));
                        if (ts && !ts->showCourtesySig())
                              showCourtesySig = false;     // this key change has court. sig turned off
                        }

                  // if due, create a new courtesy time signature for each staff
                  if (styleB(ST_genCourtesyTimesig) && !sig1.identical(sig2) && showCourtesySig) {
                        s  = m->getSegment(SegTimeSigAnnounce, tick);
                        int nstaves = Score::nstaves();
                        for (int track = 0; track < nstaves * VOICES; track += VOICES) {
                              if (s->element(track))
                                    continue;
                              ts = new TimeSig(this, sig2);
                              tss = nm->findSegment(SegTimeSig, tick);
                              if (tss) {
                                    TimeSig* nts = (TimeSig*)tss->element(0);
                                    if (nts)
                                          ts->setSubtype(nts->subtype());
                                    }
                              ts->setTrack(track);
                              ts->setGenerated(true);
                              ts->setMag(ts->staff()->mag());
                              s->add(ts);
                              needRelayout = true;
                              }
                        }
                  // courtesy key signatures
                  if (styleB(ST_genCourtesyKeysig)) {
                        int n = _staves.size();
                        for (int staffIdx = 0; staffIdx < n; ++staffIdx) {
                              Staff* staff = _staves[staffIdx];
                              KeySigEvent key1 = staff->key(tick - 1);
                              KeySigEvent key2 = staff->key(tick);

                              // locate a key sig. in next measure and, if found,
                              // check if it has court. sig turned off
                              s = nm->findSegment(SegKeySig, tick);
                              showCourtesySig = true;	// assume this key change has court. sig turned on
                              if (s) {
                                    KeySig* ks = static_cast<KeySig*>(s->element(staffIdx*VOICES));
                                    if (ks && !ks->showCourtesySig())
                                          showCourtesySig = false;     // this key change has court. sig turned off
                                    }

                              if (key1 != key2 && showCourtesySig) {
                                    hasCourtesyKeysig = true;
                                    s  = m->getSegment(SegKeySigAnnounce, tick);
                                    int track = staffIdx * VOICES;
                                    if (!s->element(track)) {
                                          KeySig* ks = new KeySig(this);
                                          ks->setSig(key1.accidentalType(), key2.accidentalType());
                                          ks->setTrack(track);
                                          ks->setGenerated(true);
                                          ks->setMag(staff->mag());
                                          s->add(ks);
                                          needRelayout = true;
                                          }
                                    // change bar line to qreal bar line
                                    m->setEndBarLineType(DOUBLE_BAR, true);
                                    }
                              }
                        }
                  // courtesy clefs
                  if (styleB(ST_genCourtesyClef)) {
                        Clef* c;
                        int n = _staves.size();
                        for (int staffIdx = 0; staffIdx < n; ++staffIdx) {
                              Staff* staff = _staves[staffIdx];
                              ClefType c1 = staff->clef(tick - 1);
                              ClefType c2 = staff->clef(tick);
                              if (c1 != c2) {
                                    // locate a clef in next measure and, if found,
                                    // check if it has court. sig turned off
                                    s = nm->findSegment(SegClef, tick);
                                    showCourtesySig = true;	// assume this clef change has court. sig turned on
                                    if (s) {
                                          c = static_cast<Clef*>(s->element(staffIdx*VOICES));
                                          if (c && !c->showCourtesyClef())
                                                continue;   // this key change has court. sig turned off
                                          }

                                    s  = m->getSegment(SegClef, tick);
                                    int track = staffIdx * VOICES;
                                    if (!s->element(track)) {
                                          c = new Clef(this);
                                          c->setClefType(c2);
                                          c->setTrack(track);
                                          c->setGenerated(true);
                                          c->setSmall(true);
                                          c->setMag(staff->mag());
                                          s->add(c);
                                          needRelayout = true;
                                          }
                                    }
                              }
                        }
                  }

            const QList<MeasureBase*>& ml = system->measures();
            int n                         = ml.size();
            while (n > 0) {
                  if (ml[n-1]->type() == MEASURE)
                        break;
                  --n;
                  }

            //
            //    compute repeat bar lines
            //
            bool firstMeasure = true;
            MeasureBase* lmb = ml.back();
            if (lmb->type() == MEASURE) {
                  if (static_cast<Measure*>(lmb)->multiMeasure() > 0) {
                        for (;;lmb = lmb->next()) {
                              if (lmb->next() == 0)
                                    break;
                              if ((lmb->next()->type() == MEASURE) && ((Measure*)(lmb->next()))->multiMeasure() >= 0)
                                    break;
                              }
                        }
                  }
            for (MeasureBase* mb = ml.front(); mb; mb = mb->next()) {
                  if (mb->type() != MEASURE) {
                        if (mb == lmb)
                              break;
                        continue;
                        }
                  Measure* m = static_cast<Measure*>(mb);
                  // first measure repeat?
                  bool fmr = firstMeasure && (m->repeatFlags() & RepeatStart);

                  if (mb == ml.back()) {       // last measure in system?
                        //
                        // if last bar has a courtesy key signature,
                        // create a qreal bar line as end bar line
                        //
                        BarLineType bl = hasCourtesyKeysig ? DOUBLE_BAR : NORMAL_BAR;

                        if (m->repeatFlags() & RepeatEnd)
                              m->setEndBarLineType(END_REPEAT, true);
                        else if (m->endBarLineGenerated())
                              m->setEndBarLineType(bl, true);
                        needRelayout |= m->setStartRepeatBarLine(fmr);
                        }
                  else {
                        MeasureBase* mb = m->next();
                        while (mb && mb->type() != MEASURE && (mb != ml.back()))
                              mb = mb->next();

                        Measure* nm = 0;
                        if (mb && mb->type() == MEASURE)
                              nm = static_cast<Measure*>(mb);

                        needRelayout |= m->setStartRepeatBarLine(fmr);
                        if (m->repeatFlags() & RepeatEnd) {
                              if (nm && (nm->repeatFlags() & RepeatStart))
                                    m->setEndBarLineType(END_START_REPEAT, true);
                              else
                                    m->setEndBarLineType(END_REPEAT, true);
                              }
                        else if (nm && (nm->repeatFlags() & RepeatStart))
                              m->setEndBarLineType(START_REPEAT, true);
                        else if (m->endBarLineGenerated())
                              m->setEndBarLineType(NORMAL_BAR, true);
                        }
                  needRelayout |= m->createEndBarLines();
                  firstMeasure = false;
                  if (mb == lmb)
                        break;
                  }

            foreach (MeasureBase* mb, ml) {
                  if (mb->type() != MEASURE)
                        continue;
                  Measure* m = static_cast<Measure*>(mb);
                  int nn = m->multiMeasure() - 1;
                  if (nn > 0) {
                        // skip to last rest measure of multi measure rest
                        Measure* mm = m;
                        for (int k = 0; k < nn; ++k)
                              mm = mm->nextMeasure();
                        if (mm) {
                              m->setMmEndBarLineType(mm->endBarLineType());
                              needRelayout |= m->createEndBarLines();
                              }
                        }
                  }
            }

      minWidth           = 0.0;
      qreal totalWeight = 0.0;

      foreach(System* system, sl) {
            foreach (MeasureBase* mb, system->measures()) {
                  if (mb->type() == HBOX)
                        minWidth += point(((Box*)mb)->boxWidth());
                  else {
                        Measure* m = (Measure*)mb;
                        if (needRelayout)
                              m->layoutX(1.0);
                        minWidth    += m->layoutWidth().stretchable;
                        totalWeight += m->ticks() * m->userStretch();
                        }
                  }
            minWidth += system->leftMargin();
            }

      qreal rest = (raggedRight ? 0.0 : rowWidth - minWidth) / totalWeight;
      qreal xx   = 0.0;

      foreach(System* system, sl) {
            QPointF pos;

            bool firstMeasure = true;
            foreach(MeasureBase* mb, system->measures()) {
                  qreal ww = 0.0;
                  if (mb->type() == MEASURE) {
                        if (firstMeasure) {
                              pos.rx() += system->leftMargin();
                              firstMeasure = false;
                              }
                        mb->setPos(pos);
                        Measure* m    = static_cast<Measure*>(mb);
                        if (styleB(ST_FixMeasureWidth)) {
                              ww = rowWidth / system->measures().size();
                              }
                        else {
                              qreal weight = m->ticks() * m->userStretch();
                              ww            = m->layoutWidth().stretchable + rest * weight;
                              }
                        m->layout(ww);
                        }
                  else if (mb->type() == HBOX) {
                        mb->setPos(pos);
                        ww = point(static_cast<Box*>(mb)->boxWidth());
                        mb->layout();
                        }
                  pos.rx() += ww;
                  }
            system->setPos(xx + x, y);
            qreal w = pos.x();
            system->setWidth(w);
            system->layout2();
            foreach(MeasureBase* mb, system->measures()) {
                  if (mb->type() == HBOX)
                        mb->setHeight(system->height());
                  }
            xx += w;
            qreal hh = system->height() + point(system->staves()->back()->distanceDown());
            if (hh > *h)
                  *h = hh;
            }
      return sl;
      }

//---------------------------------------------------------
//   addPage
//---------------------------------------------------------

Page* Score::addPage()
      {
      Page* page = new Page(this);
      page->setNo(_pages.size());
      _pages.push_back(page);
      return page;
      }

//---------------------------------------------------------
//   setPageFormat
//---------------------------------------------------------

void Score::setPageFormat(const PageFormat& pf)
      {
      *_pageFormat = pf;
      }

//---------------------------------------------------------
//   setInstrumentNames
//---------------------------------------------------------

void Score::setInstrumentNames()
      {
      for (iSystem is = systems()->begin(); is != systems()->end(); ++is)
            (*is)->setInstrumentNames();
      }

//---------------------------------------------------------
//   searchTieNote
//---------------------------------------------------------

static Note* searchTieNote(Note* note, Segment* segment, int track)
      {
      int pitch = note->pitch();

      while ((segment = segment->next1())) {
            Element* element = segment->element(track);
            if (element == 0 || element->type() != CHORD)
                  continue;
            Note* n = static_cast<Chord*>(element)->findNote(pitch);
            if (n)
                  return n;
            }
      return 0;
      }

//---------------------------------------------------------
//   connectTies
///   Rebuild tie connections.
//---------------------------------------------------------

void Score::connectTies()
      {
      int tracks = nstaves() * VOICES;
      Measure* m = firstMeasure();
      if (!m)
            return;
      for (Segment* s = m->first(); s; s = s->next1()) {
            for (int i = 0; i < tracks; ++i) {
                  Element* el = s->element(i);
                  if (el == 0 || el->type() != CHORD)
                        continue;
                  foreach(Note* n, static_cast<Chord*>(el)->notes()) {
                        Tie* tie = n->tieFor();
                        if (!tie)
                              continue;
                        Note* nnote = searchTieNote(n, s, i);
                        if (nnote == 0) {
                              printf("next note at %d voice %d for tie not found; delete tie\n",
                                 s->tick(), i );
                              n->setTieFor(0);
                              delete tie;
                              }
                        else {
                              tie->setEndNote(nnote);
                              nnote->setTieBack(tie);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void Score::add(Element* el)
      {
      switch(el->type()) {
            case MEASURE:
            case HBOX:
            case VBOX:
            case TBOX:
            case FBOX:
                  measures()->add((MeasureBase*)el);
                  break;
            case BEAM:
                  {
                  Beam* b = static_cast<Beam*>(el);
                  _beams.append(b);
                  foreach(ChordRest* cr, b->elements())
                        cr->setBeam(b);
                  }
                  break;
            case SLUR:
                  break;
            default:
                  printf("Score::add() invalid element <%s>\n", el->name());
                  delete el;
                  break;
            }
      }

//---------------------------------------------------------
//   remove
//---------------------------------------------------------

void Score::remove(Element* el)
      {
      switch(el->type()) {
            case MEASURE:
            case HBOX:
            case VBOX:
            case TBOX:
            case FBOX:
                  measures()->remove(static_cast<MeasureBase*>(el));
                  break;
            case BEAM:
                  {
                  Beam* b = static_cast<Beam*>(el);
                  if (_beams.removeOne(b)) {
                        foreach(ChordRest* cr, b->elements())
                              cr->setBeam(0);
                        }
                  else
                        printf("Score::remove(): cannot find Beam\n");
                  }
                  break;
            case SLUR:
                  break;
            default:
                  printf("Score::remove(): invalid element %s\n", el->name());
                  break;
            }
      }

