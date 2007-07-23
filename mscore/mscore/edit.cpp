//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id: edit.cpp,v 1.85 2006/04/12 14:58:10 wschweer Exp $
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

#include "canvas.h"
#include "note.h"
#include "rest.h"
#include "chord.h"
#include "key.h"
#include "sig.h"
#include "clef.h"
#include "padstate.h"
#include "score.h"
#include "slur.h"
#include "hairpin.h"
#include "segment.h"
#include "staff.h"
#include "part.h"
#include "layout.h"
#include "timesig.h"
#include "page.h"
#include "barline.h"
#include "tuplet.h"
#include "seq.h"
#include "mscore.h"
#include "lyrics.h"
#include "image.h"
#include "keysig.h"

//---------------------------------------------------------
//   selectNoteMessage
//---------------------------------------------------------

static void selectNoteMessage()
      {
      QMessageBox::critical(0,
               QMessageBox::tr("MuseScore:"),
               QMessageBox::tr("please select a single note and retry operation\n"),
               QMessageBox::Ok, QMessageBox::NoButton);
      }

static void selectNoteRestMessage()
      {
      QMessageBox::critical(0,
               QMessageBox::tr("MuseScore:"),
               QMessageBox::tr("please select a single note or rest and retry operation\n"),
               QMessageBox::Ok, QMessageBox::NoButton);
      }

static void selectNoteSlurMessage()
      {
      QMessageBox::critical(0,
               QMessageBox::tr("MuseScore:"),
               QMessageBox::tr("please select a single note or slur and retry operation\n"),
               QMessageBox::Ok, QMessageBox::NoButton);
      }

//---------------------------------------------------------
//   getSelectedNote
//---------------------------------------------------------

Note* Score::getSelectedNote()
      {
      Element* el = sel->element();
      if (el) {
            if (el->type() == NOTE)
                  return (Note*)el;
            }
      selectNoteMessage();
      return 0;
      }

//---------------------------------------------------------
//   getSelectedChordRest
//---------------------------------------------------------

ChordRest* Score::getSelectedChordRest()
      {
      Element* el = sel->element();
      if (el) {
            if (el->type() == NOTE)
                  return ((Note*)el)->chord();
            else if (el->type() == REST)
                  return (Rest*)el;
            }
      selectNoteRestMessage();
      return 0;
      }

//---------------------------------------------------------
//   pos
//---------------------------------------------------------

int Score::pos()
      {
      Element* el = sel->element();
      if (el && (el->type() == REST || el->type() == NOTE)) {
            if (el->type() == NOTE)
                  el = el->parent();
            return el->tick();
            }
      return -1;
      }

//---------------------------------------------------------
//   changeRest
//---------------------------------------------------------

void Score::changeRest(Rest* rest, int /*tick*/, int len)
      {
      rest->setTickLen(len);
      }

//---------------------------------------------------------
//   setRest
//---------------------------------------------------------

Rest* Score::setRest(int tick, int len, Staff* staffp, int voice, Measure* measure)
      {
      Rest* rest = 0;
      if (len / (division*4)) {
            int nlen =  len % (division*4);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division*4);
            }
      else if (len / (division*2)) {
            int nlen =  len % (division*2);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division*2);
            }
      else if (len / division) {
            int nlen =  len % (division);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division);
            }
      else if (len / (division/2)) {
            int nlen = len % (division/2);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division/2);
            }
      else if (len / (division/4)) {
            int nlen = len % (division/4);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division/4);
            }
      else if (len / (division/8)) {
            int nlen = len % (division/8);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division/8);
            }
      else if (len / (division/16)) {
            int nlen = len % (division/16);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division/16);
            }
      else if (len / (division/32)) {
            int nlen = len % (division/32);
            if (nlen) {
                  setRest(tick, nlen, staffp, voice, measure);
                  tick += nlen;
                  }
            rest = new Rest(this, tick, division/32);
            }
      if (rest) {
            rest->setVoice(voice);
            rest->setStaff(staffp);
            Segment::SegmentType st = Segment::segmentType(rest->type());
            Segment* seg = measure->findSegment(st, tick);
            if (seg == 0) {
                  seg = measure->createSegment(st, tick);
                  undoAddElement(seg);
                  }
            rest->setParent(seg);
            cmdAdd(rest);
            }
      return rest;
      }

//---------------------------------------------------------
//   addNote
//---------------------------------------------------------

Note* Score::addNote(Chord* chord, int pitch)
      {
      Note* note = new Note(this);
      note->setPitch(pitch);
      note->setParent(chord);
      cmdAdd(note);
      spell(note);
      return note;
      }

//---------------------------------------------------------
//   addKeySig
//---------------------------------------------------------

Element* Score::addKeySig(KeySig* sig, const QPointF& pos)
      {
printf("Score::addKeySig===\n");
      int tick;
      if (!pos2measure(pos, &tick, 0, 0, 0, 0)) {
            delete sig;
            return 0;
            }
      (*sig->staff()->keymap())[tick] = sig->subtype();

      if (tick != 0) {
            sig->setTick(tick);
            printf("not impl.: keys signature at tick != 0\n");
            delete sig;
            return 0;
            }
      else {
#if 0 //TODOx
            delete sig;
            Page* page = pages->front();
            System* system = page->systems()->front();
            for (int i = 0; i < nstaves(); ++i)
                  system->addKeysig(i, 0);
#endif
            sig = 0;
            }
      layout();
      return sig;
      }

//---------------------------------------------------------
//   removeClef
//---------------------------------------------------------

void Score::removeClef(Clef* clef)
      {
      int tick = clef->tick();
      if (tick == 0)    // cannot remove first clef
            return;
      int staffIdx = clef->staffIdx();
      ClefList* ct = staff(staffIdx)->clef();
      int oidx  = ct->clef(tick-1);  // clef state before clef

      //---------------------------------------------
      //    remove unnessesary clef symbols
      //---------------------------------------------

      for (Measure* m = _layout->first(); m; m = m->next()) {
            if ((m->tick()+m->tickLen()) < tick)
                  continue;
            for (Segment* segment = m->first(); segment; segment = segment->next()) {
                  if (segment->subtype() != Segment::SegClef)
                        continue;
                  int startTrack = staffIdx * VOICES;
                  int endTrack   = startTrack + VOICES;
                  for (int track = startTrack; track < endTrack; ++track) {
                        Element* e = segment->element(track);
                        if (e == 0 || e->type() != CLEF)
                              continue;
                        int etick = e->tick();
                        if (etick <= tick)
                              continue;
                        int cidx = ((Clef*)e)->subtype();
                        if (cidx == oidx) {
                              // this clef is can be removed:
                              undoOp(UndoOp::RemoveElement, e);
                              segment->setElement(track, 0);
                              }
                        break;
                        }
                  }
            }

      removeElement(clef);
      layout();
      }

//---------------------------------------------------------
//   addClef
//---------------------------------------------------------

Element* Score::addClef(Clef* clef)
      {
      int tick     = clef->tick();
      int idx      = clef->subtype();
      int staffIdx = clef->staffIdx();
      ClefList* ct = staff(staffIdx)->clef();
      if (ct->clef(tick) == idx)    // no effect
            return 0;

      //---------------------------------------------
      //    remove unnessesary clef symbols
      //---------------------------------------------

      for (Measure* m = _layout->first(); m; m = m->next()) {
            if ((m->tick()+m->tickLen()) < tick)
                  continue;
            for (Segment* segment = m->first(); segment; segment = segment->next()) {
                  if (segment->subtype() != Segment::SegClef)
                        continue;
                  int startTrack = staffIdx * VOICES;
                  int endTrack   = startTrack + VOICES;
                  for (int track = startTrack; track < endTrack; ++track) {
                        Element* e = segment->element(track);
                        if (e == 0 || e->type() != CLEF)
                              continue;
                        int etick = e->tick();
                        if (etick < tick)
                              continue;
                        iClefEvent ic = ct->find(etick);
                        if (ic == ct->end() || (ic->first == tick)) {
                              undoOp(UndoOp::RemoveElement, e);
                              segment->setElement(track, 0);
                              // printf("remove Clef at %d\n", tick);
                              }
                        int cidx = ((Clef*)e)->subtype();
                        if (etick > tick &&  idx == cidx) {
                              undoOp(UndoOp::RemoveElement, e);
                              segment->setElement(track, 0);
                              // printf("remove Clef at %d\n", tick);
                              break;
                              }
                        }
                  }
            }
      Measure* measure = tick2measure(tick);
      if (!measure) {
            printf("measure for tick %d not found!\n", tick);
            return 0;
            }
      Segment::SegmentType st = Segment::segmentType(CLEF);
      Segment* seg = measure->findSegment(st, tick);
      if (seg == 0) {
            seg = measure->createSegment(st, tick);
            undoAddElement(seg);
            }
      clef->setParent(seg);
      addElement(clef);
      layout();
      return clef;
      }

//---------------------------------------------------------
//   addBar
//---------------------------------------------------------

void Score::addBar(BarLine* barLine, Measure* measure)
      {
      if (measure == 0) {
            printf("can only put barLine in measure\n");
            delete barLine;
            return;
            }
      for (int staffIdx = 0; staffIdx < nstaves(); ++staffIdx) {
            BarLine* obarLine = measure->barLine(staffIdx);
            if (obarLine) {
                  Staff* staffp = staff(staffIdx);
                  BarLine* nbarLine = new BarLine(barLine->score());
                  nbarLine->setParent(barLine->parent());
                  nbarLine->setStaff(staffp);
                  nbarLine->setSubtype(barLine->subtype());
                  cmdRemove(obarLine);
                  cmdAdd(nbarLine);
                  }
            }
      delete barLine;
      }

//---------------------------------------------------------
//   changeTimeSig
//
// change time signature at tick into subtype st for all staves
// in response to gui command (drop timesig on measure or timesig)
//
// note: use subtupe instead of z/n to enable use of "c" and "c|" symbols
//
// FIXME: undo/redo does not work
//---------------------------------------------------------

void Score::changeTimeSig(int tick, int timeSigSubtype)
      {
      int oz, on;
      sigmap->timesig(tick, oz, on);

      int z, n;
      TimeSig::getSig(timeSigSubtype, &n, &z);
      if (oz == z && on == n) {
            // search for time signature symbol
            Segment* ts = 0;
            for (Measure* m = _layout->first(); m; m = m->next()) {
                  for (Segment* segment = m->first(); segment; segment = segment->next()) {
                        if (segment->subtype() != Segment::SegTimeSig)
                              continue;
                        int etick = segment->tick();
                        if (etick == tick) {
                              ts = segment;
                              break;
                              }
                        }
                  if (ts)
                        break;
                  }
            if (ts) {
                  // modify time signature symbol
                  for (int staffIdx = 0; staffIdx < nstaves(); ++staffIdx) {
                        int track    = staffIdx * VOICES;
                        TimeSig* sig = (TimeSig*)ts->element(track);
                        TimeSig* nsig = new TimeSig(this, timeSigSubtype);
                        nsig->setParent(sig->parent());
                        nsig->setTick(sig->tick());
                        nsig->setStaff(sig->staff());
                        undoOp(UndoOp::RemoveElement, sig);
                        removeElement(sig);
                        undoAddElement(nsig);
                        }
                  layout();
                  }
            else {
                  // add symbol
                  addTimeSig(tick, timeSigSubtype);
                  }
            return;
            }

      sigmap->add(tick, z, n);

// printf("Score::changeTimeSig tick %d z %d n %d\n",
//         tick, z, n);

      //---------------------------------------------
      // remove unnessesary timesig symbols
      //---------------------------------------------

      for (Measure* m = _layout->first(); m; m = m->next()) {
again:
            for (Segment* segment = m->first(); segment; segment = segment->next()) {
                  if (segment->subtype() != Segment::SegTimeSig)
                        continue;
                  int etick = segment->tick();
                  if (etick >= tick) {
                        iSigEvent ic = sigmap->find(etick);
                        if (etick == tick
                            || ic == sigmap->end()
                            || (ic->first == tick && ic->second.nominator != z && ic->second.denominator == n)) {
                              m->remove(segment);
                              goto again;
                              }
                        else if (etick > tick)
                              break;
                        }
                  }
            }

      //---------------------------------------------
      // modify measures
      //---------------------------------------------

      int mtick = 0;
      int staves = nstaves();
      for (Measure* m = _layout->first(); m; m = m->next()) {
            int tickdiff = mtick - m->tick();
            if (tickdiff) {
                  // printf("move %d ticks\n", tickdiff);
                  m->moveTicks(tickdiff);
                  }
            int mtickLen = sigmap->ticksMeasure(mtick);
            int lendiff  = mtickLen - m->tickLen();
            m->setTickLen(mtickLen);

            mtick += mtickLen;
            if (lendiff == 0)
                  continue;

            for (int staffIdx = 0; staffIdx < staves; ++staffIdx) {
                  Staff* staffp = staff(staffIdx);
                  int rests  = 0;
                  Rest* rest = 0;
                  for (Segment* segment = m->first(); segment; segment = segment->next()) {
                        for (int rstaff = 0; rstaff < VOICES; ++rstaff) {
                              Element* e = segment->element(staffIdx * VOICES + rstaff);
                              if (e && e->type() == REST) {
                                    ++rests;
                                    rest = (Rest*)e;
                                    }
                              }
                        }
                  // printf("rests = %d\n", rests);
                  if (rests == 1) {
                        // printf("rest set ticklen %d\n", rest->tickLen() + lendiff);
                        rest->setTickLen(rest->tickLen() + lendiff);
                        }
                  else {
                        if (lendiff < 0) {
                              printf("shrinking of measure not supported yet\n");
                              continue;
                              }
                        else {
                              // add rest to measure
                              int rtick = mtick - lendiff;
                              rest = new Rest(this, rtick, lendiff);
                              rest->setStaff(staffp);
                              // printf("add rest\n");
                              m->add(rest);
                              }
                        }
                  }
            }
      addTimeSig(tick, timeSigSubtype);
      }

//---------------------------------------------------------
//   addTimeSig
//---------------------------------------------------------

void Score::addTimeSig(int tick, int timeSigSubtype)
      {
      Measure* measure = tick2measure(tick);
      foreach(Staff* staff, _staves) {
            TimeSig* nsig = new TimeSig(this, timeSigSubtype);
            nsig->setStaff(staff);
            nsig->setTick(tick);
            Segment::SegmentType st = Segment::segmentType(TIMESIG);
            Segment* seg = measure->findSegment(st, tick);
            if (seg == 0) {
                  seg = measure->createSegment(st, tick);
                  undoAddElement(seg);
                  }
            nsig->setParent(seg);
            undoAddElement(nsig);
            }
      layout();
      }

//---------------------------------------------------------
//   putNote
//    mouse click in state NOTE_ENTRY
//---------------------------------------------------------

void Score::putNote(const QPointF& pos, bool addToChord)
      {
      int tick, pitch;
      Staff* staff = 0;
      Measure* m = pos2measure(pos, &tick, &staff, &pitch, 0, 0);

      if (m == 0 || pitch < 0 || pitch > 127) {
            printf("cannot put note/rest at this position, bad pitch %d!\n", pitch);
            return;
            }

      int len   = padState.tickLen;
      int voice = padState.voice;
      int track = staff->idx() * VOICES + voice;

      if (addToChord) {
            ChordRest* el = 0;
            for (Segment* segment = m->first(); segment; segment = segment->next()) {
                  if (segment->subtype() != Segment::SegChordRest)
                        continue;
                  Element* ie  = segment->element(track);
                  if (!ie)
                        continue;
                  el = (ChordRest*)ie;
                  if (el->tick() >= tick)
                        break;
                  }
            if (!el) {
                  printf("putNote: chord/rest not found\n");
                  return;
                  }
            if (el->tuplet())
                  len = el->tuplet()->noteLen();

            if (el->type() == CHORD) {
                  Note* note = addNote((Chord*)el, pitch);
                  select(note, 0, 0);
                  if (seq && mscore->playEnabled()) {
                        Staff* staff = note->staff();
                        seq->startNote(staff->midiChannel(), note->pitch(), 60);
                        }
                  }
            else {
                  setNote(tick, staff, voice, pitch, len);
                  }
            }
      else {
            // replace chord
            if (padState.rest)
                  setRest(tick, staff, voice, len);
            else
                  setNote(tick, staff, voice, pitch, len);
            }
      cis->staff     = _staves.indexOf(staff);
      cis->voice     = voice;
      padState.pitch = pitch;
      cis->pos       = tick + len;

      moveCursor();
      }

//---------------------------------------------------------
//   modifyElement
//---------------------------------------------------------

void Canvas::modifyElement(Element* el)
      {
      if (el == 0) {
            printf("modifyElement: el==0\n");
            delete el;
            return;
            }
      Score* cs = el->score();
      if (cs->sel->state() != SEL_SINGLE) {
            printf("modifyElement: cs->sel->state() != SEL_SINGLE\n");
            delete el;
            return;
            }
      Element* e = cs->sel->element();
      Chord* chord;
      if (e->type() == CHORD)
            chord = (Chord*) e;
      else if (e->type() == NOTE)
            chord = ((Note*)e)->chord();
      else {
            printf("modifyElement: no note/Chord selected:\n  ");
            e->dump();
            delete el;
            return;
            }
      switch (el->type()) {
            case ATTRIBUTE:
                  chord->add((NoteAttribute*)el);
                  break;
            default:
                  printf("modifyElement: %s not ATTRIBUTE\n", el->name());
                  delete el;
                  return;
            }
      cs->layout();
      }

//---------------------------------------------------------
//   addSlur
//    'S' typed on keyboard
//---------------------------------------------------------

Slur* Score::cmdAddSlur()
      {
      Element* e = sel->element();
      if (!e || e->type() != NOTE) {
            printf("no note selected\n");
            return 0;
            }
      Note* note    = (Note*)(e);
      Chord* chord  = note->chord();
      Staff* staff  = chord->staff();
      int voice     = chord->voice();

      int track = chord->staffIdx() * VOICES + voice;
      int tick2 = nextSeg(chord->tick(), track);

      if (tick2 == 0) {
            printf("cannot create slur: at end\n");
            return 0;
            }
      Slur* slur = new Slur(this);
      slur->setStaff(staff);
      slur->setStart(chord->tick(), track);
      slur->setEnd(tick2, track);
      slur->setParent(chord->measure());
      cmdAdd(slur);
// select slur segment
//      slur->setSelected(true);
//      select(slur, 0, 0);
//      refresh |= slur->abbox();

      return slur;
      }

//---------------------------------------------------------
//   addTie
//    shift+'S' typed on keyboard
//---------------------------------------------------------

void Score::cmdAddTie()
      {
      Element* e = sel->element();
      if (!e || e->type() != NOTE) {
            printf("no note selected\n");
            return;
            }

      Note* note    = (Note*)(e);
      Chord* chord  = note->chord();
      Staff* staff  = chord->staff();
      ChordRest* el = nextChordRest(chord);
      if (el == 0 || el->type() != CHORD) {
            printf("at end\n");
            return;
            }
      NoteList* nl = ((Chord*)el)->noteList();
      Note* note2 = 0;
      for (iNote i = nl->begin(); i != nl->end(); ++i) {
            if (i->second->pitch() == note->pitch()) {
                  note2 = i->second;
                  break;
                  }
            }
      if (note2 == 0) {
            printf("1: next note for tie not found\n");
            return;
            }

      Tie* tie = new Tie(this);
      tie->setStartNote(note);
      tie->setEndNote(note);
      tie->setStaff(staff);
      note->setTieFor(tie);
      _layout->connectTies();
      layout();
      select(tie, 0, 0);
      }

//---------------------------------------------------------
//   cmdAddHairpin
//    'H' typed on keyboard
//---------------------------------------------------------

void Score::cmdAddHairpin(bool decrescendo)
      {
      Element* el = sel->element();
      if (!el) {
            printf("selState != single\n");
            return;
            }
      if (el->type() == NOTE)
            el = el->parent();
      int tick1 = el->tick();
      int tick2 = tick1 + 4 * division;
      Hairpin* gabel = new Hairpin(this);
      gabel->setTick(tick1);
      gabel->setTick2(tick2);
      gabel->setSubtype(decrescendo ? 1 : 0);

      if (!cmdAddHairpin(gabel, el->pos()))
            delete gabel;
      else
            select(gabel, 0, 0);
      }

//---------------------------------------------------------
//   cmdSetBeamMode
//---------------------------------------------------------

void Score::cmdSetBeamMode(int mode)
      {
      ChordRest* cr = getSelectedChordRest();
      if (cr == 0)
            return;
      cr->setBeamMode(BeamMode(mode));
      layout();
      }

//---------------------------------------------------------
//   cmdFlipStemDirection
//---------------------------------------------------------

void Score::cmdFlipStemDirection()
      {
      Element* el = sel->element();
      if (el && el->type() == NOTE) {
            Chord* chord = ((Note*)el)->chord();

            chord->setStemDirection(chord->isUp() ? DOWN : UP);
            undoOp(UndoOp::FlipStemDirection, chord);
            }
      else if (el && el->type() == SLUR_SEGMENT) {
            SlurTie* slur = ((SlurSegment*) el)->slurTie();
            slur->setSlurDirection(slur->isUp() ? DOWN : UP);
            undoOp(UndoOp::FlipSlurDirection, slur);
            }
      else {
            selectNoteSlurMessage();
            return;
            }
      layout();
      }

//---------------------------------------------------------
//   cmdAddBSymbol
//---------------------------------------------------------

void Score::cmdAddBSymbol(BSymbol* s, const QPointF& pos, const QPointF& off)
      {
      s->setSelected(false);

      if (s->anchor() == ANCHOR_STAFF) {
            Staff* staff = 0;
            int pitch, tick;
            QPointF offset;
            Segment* segment;

            Measure* measure = pos2measure(pos, &tick, &staff, &pitch, &segment, &offset);
            if (measure == 0) {
                  printf("addSymbol: cannot put symbol here: no measure\n");
                  delete s;
                  return;
                  }
            offset -= off;
            s->setPos(segment->x(), 0.0);
            s->setUserOff(offset / _spatium);
            s->setTick(segment->tick());
            s->setStaff(staff);
            s->setParent(measure);
            }
      else if (s->anchor() == ANCHOR_PAGE) {
            bool foundPage = false;
            for (ciPage ip = _layout->pages()->begin(); ip != _layout->pages()->end(); ++ip) {
                  const Page* page = *ip;
                  if (page->contains(pos)) {
                        QList<System*>* sl = page->systems();
                        if (sl->isEmpty()) {
                              printf("addSymbol: cannot put symbol here: no system on page\n");
                              delete s;
                              return;
                              }
                        System* system = sl->front();
                        Measure* m = system->measures().front();
                        if (m == 0) {
                              printf("addSymbol: cannot put symbol here: no measure in system\n");
                              delete s;
                              return;
                              }
                        s->setPos(0.0, 0.0);
                        s->setUserOff((pos - page->pos() - off) / _spatium);
                        s->setStaff(0);
                        s->setParent(m);
                        foundPage = true;
                        break;
                        }
                  }
            if (!foundPage) {
                  printf("addSymbol: cannot put symbol here: no page\n");
                  delete s;
                  return;
                  }
            }
      else {
            printf("Anchor type not implemented\n");
            delete s;
            return;
            }

      undoAddElement(s);
      addRefresh(s->abbox());
      select(s, 0, 0);
      }

//---------------------------------------------------------
//   cmdAddHairpin
//---------------------------------------------------------

Element* Score::cmdAddHairpin(Hairpin* pin, const QPointF& pos)
      {
      Staff* staff = 0;
      int pitch, tick1;
      QPointF offset;
      Segment* segment;
      Measure* measure = pos2measure(pos, &tick1, &staff, &pitch, &segment, &offset);
      if (measure == 0) {
            printf("addHairpin: cannot put object here\n");
            delete pin;
            return 0;
            }
      pin->setStaff(staff);
      pin->setParent(measure);
      cmdAdd(pin);
      return pin;
      }

//---------------------------------------------------------
//   addSlur
//---------------------------------------------------------

Element* Score::addSlur(Slur* /*slur*/, const QPointF& /*pos*/)
      {
      printf("addSlur 2\n");
#if 0
      int staff=-1, pitch, tick1;
      QPointF offset;
      Segment* segment;
      Measure* measure = cs->pos2measure(pos, &tick1, &staff, &pitch, &segment, &offset);
      if (measure == 0) {
            printf("addSLur: cannot put object here\n");
            delete slur;
            return 0;
            }
      slur->setTick(tick1);
      slur->setTick2(tick2);
      slur->setStaffIdx(staff);
      measure->add(slur);
      return slur;
#endif
      return 0;
      }

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

void Score::deleteItem(Element* el)
      {
      switch(el->type()) {
            case SLUR_SEGMENT:
                  {
                  SlurTie* s = ((SlurSegment*)el)->slurTie();
                  s->parent()->remove(s);
                  undoOp(UndoOp::RemoveElement, s);
                  updateAll = true;
                  }
                  break;

            case TEXT:
                  if (el->subtype() == TEXT_INSTRUMENT_LONG) {
                        el->staff()->part()->setLongName(QString());
                        _layout->setInstrumentNames();
                        layout();
                        break;
                        }
                  if (el->subtype() == TEXT_INSTRUMENT_SHORT) {
                        el->staff()->part()->setShortName(QString());
                        _layout->setInstrumentNames();
                        layout();
                        break;
                        }

            case SYMBOL:
            case COMPOUND:
            case DYNAMIC:
            case SLUR:
            case LYRICS:
            case ATTRIBUTE:
            case BRACKET:
            case VOLTA:
            case LAYOUT_BREAK:
            case CLEF:
            case KEYSIG:
            case IMAGE:
                  cmdRemove(el);
                  updateAll = true;
                  break;

            case OTTAVA_SEGMENT:
            case HAIRPIN_SEGMENT:
            case TRILL_SEGMENT:
            case PEDAL_SEGMENT:
                  {
                  SLine* l = (SLine*)el->parent();
                  foreach(LineSegment* seg, l->lineSegments())
                        undoRemoveElement(seg);
                  undoRemoveElement(l);
                  }
                  break;
            case NOTE:
                  {
                  Chord* chord = (Chord*)(el->parent());
                  int notes = chord->noteList()->size();
                  if (notes > 1) {
                        chord->remove((Note*) el);
                        undoOp(UndoOp::RemoveElement, el);
                        break;
                        }
                  // else fall through
                  el = chord;
                  }

            case CHORD:
                  {
                  Chord* chord = (Chord*) el;
                  undoRemoveElement(chord);
                  if (el->voice() == 0) {
                        //
                        // voice 0 chords are always replaced by rests
                        //
                        Rest* rest   = new Rest(this, chord->tick(), chord->tickLen());
                        rest->setStaff(el->staff());
                        rest->setParent(chord->parent());
                        rest->setVoice(el->voice());
                        undoAddElement(rest);
                        }
                  }
                  break;

            case REST:
                  //
                  // only allow for voices != 0
                  //    e.g. voice 0 rests cannot be removed
                  //
printf("delete rest voice %d\n", el->voice());
                  if (el->voice() != 0)
                        undoRemoveElement(el);
                  break;

            case MEASURE:
                  {
                  Measure* m = (Measure*)el;
                  undoOp(UndoOp::RemoveMeasure, m);
                  removeMeasure(m->tick());
                  }
                  break;

            case ACCIDENTAL:
                  addAccidental((Note*)(el->parent()), ACC_NONE);
                  break;

            default:
                  printf("delete %s: not implemented\n", el->name());
            }
      }

//---------------------------------------------------------
//   cmdDeleteItem
//    called from object popup
//---------------------------------------------------------

void Score::cmdDeleteItem(Element* el)
      {
      deleteItem(el);
      select(0, 0, 0);
      layout();
      }

//---------------------------------------------------------
//   cmdDeleteSelection
//---------------------------------------------------------

void Score::cmdDeleteSelection()
      {
      if (sel->state() == SEL_SYSTEM) {
            Measure* is = tick2measure(sel->tickStart);
            if (is->next()) {
                  Measure* ie = tick2measure(sel->tickEnd);
                  if (ie) {
                        do {
                              ie = ie->prev();
                              deleteItem(ie);
                              } while (ie != is);
                        }
                  }
            else
                  deleteItem(is);
            }
      else {
            // deleteItem modifies sel->elements() list,
            // so we need a local copy:
            foreach(Element* e, *sel->elements()) {
                  e->setSelected(false);  // in case item is not deleted
                  if (e->type() == SLUR_SEGMENT)
                        e = ((SlurSegment*)e)->slurTie();
                  deleteItem(e);
                  }
            }
      sel->elements()->clear();
      layout();
      }

//---------------------------------------------------------
//   lyricsTab
//---------------------------------------------------------

void Score::lyricsTab(bool back)
      {
      Lyrics* lyrics   = (Lyrics*)editObject;
      Segment* segment = (Segment*)(lyrics->parent());
      int staff        = lyrics->staffIdx();

      // search next chord
      if (back) {
            while ((segment = segment->prev1())) {
                  if (segment->subtype() == Segment::SegChordRest)
                        break;
                  }
            }
      else {
            while ((segment = segment->next1())) {
                  if (segment->subtype() == Segment::SegChordRest)
                        break;
                  }
            }
      if (segment == 0)
            return;

      canvas()->setState(Canvas::NORMAL);
      Lyrics* oldLyrics = lyrics;
      switch(oldLyrics->syllabic()) {
            case Lyrics::SINGLE:
            case Lyrics::END:
                  break;
            case Lyrics::BEGIN:
                  oldLyrics->setSyllabic(Lyrics::SINGLE);
                  break;
            case Lyrics::MIDDLE:
                  oldLyrics->setSyllabic(Lyrics::END);
                  break;
            }
      endCmd(true);

      startCmd();
      LyricsList* ll = segment->lyricsList(staff);
      lyrics = ll->value(oldLyrics->no());
      if (!lyrics)
            lyrics = new Lyrics(this);
      else {
            lyrics->parent()->remove(lyrics);
            undoOp(UndoOp::RemoveElement, lyrics);
            }

      switch(lyrics->syllabic()) {
            case Lyrics::SINGLE:
            case Lyrics::BEGIN:
                  break;
            case Lyrics::END:
                  lyrics->setSyllabic(Lyrics::SINGLE);
                  break;
            case Lyrics::MIDDLE:
                  lyrics->setSyllabic(Lyrics::BEGIN);
                  break;
            }

      lyrics->setTick(segment->tick());
      lyrics->setStaff(oldLyrics->staff());
      lyrics->setParent(segment);
      lyrics->setNo(oldLyrics->no());
      undoAddElement(lyrics);

      select(lyrics, 0, 0);
      canvas()->startEdit(lyrics);
      ((Lyrics*)editObject)->moveCursorToEnd();

      layout();
      }

//---------------------------------------------------------
//   lyricsMinus
//---------------------------------------------------------

void Score::lyricsMinus()
      {
      Lyrics* lyrics   = (Lyrics*)editObject;
      Segment* segment = (Segment*)(lyrics->parent());
      int staff        = lyrics->staffIdx();
      int track        = staff * VOICES;

      canvas()->setState(Canvas::NORMAL);
      endCmd(true);

      // search next chord
      while ((segment = segment->next1())) {
            Element* el = segment->element(track);
            if (el &&  el->type() == CHORD)
                  break;
            }
      if (segment == 0) {
            return;
            }

      startCmd();

      Lyrics* oldLyrics = lyrics;

      LyricsList* ll = segment->lyricsList(staff);
      lyrics = ll->value(oldLyrics->no());
      if (!lyrics)
            lyrics = new Lyrics(this);

      switch(oldLyrics->syllabic()) {
            case Lyrics::SINGLE:
                  oldLyrics->setSyllabic(Lyrics::BEGIN);
                  break;
            case Lyrics::BEGIN:
            case Lyrics::MIDDLE:
                  break;
            case Lyrics::END:
                  oldLyrics->setSyllabic(Lyrics::MIDDLE);
                  break;
            }
      lyrics->setSyllabic(Lyrics::END);

      lyrics->setTick(segment->tick());
      lyrics->setStaff(oldLyrics->staff());
      lyrics->setParent(segment);
      lyrics->setNo(oldLyrics->no());
      undoAddElement(lyrics);

      select(lyrics, 0, 0);
      canvas()->startEdit(lyrics);
      ((Lyrics*)editObject)->moveCursorToEnd();

      layout();
      }

//---------------------------------------------------------
//   lyricsReturn
//---------------------------------------------------------

void Score::lyricsReturn()
      {
      Lyrics* lyrics   = (Lyrics*)editObject;
      Segment* segment = (Segment*)(lyrics->parent());

      canvas()->setState(Canvas::NORMAL);
      endCmd(true);

      startCmd();

      Lyrics* oldLyrics = lyrics;

      lyrics = new Lyrics(this);
      lyrics->setTick(segment->tick());
      lyrics->setStaff(oldLyrics->staff());
      lyrics->setParent(segment);
      lyrics->setNo(oldLyrics->no() + 1);
      undoAddElement(lyrics);
      select(lyrics, 0, 0);
      canvas()->startEdit(lyrics);

      layout();
      }

//---------------------------------------------------------
//   addLyrics
//    called from Keyboard Accelerator & menue
//---------------------------------------------------------

void Score::addLyrics()
      {
      Note* e = getSelectedNote();
      if (e == 0) {
            endCmd(true);
            return;
            }

      Chord* chord     = e->chord();
      Segment* segment = chord->segment();
      int tick         = chord->tick();
      int staff        = chord->staffIdx();

      Lyrics* lyrics;
      LyricsList* ll = segment->lyricsList(staff);
      int no = 0;
      if (ll)
            no = ll->size();
      lyrics = new Lyrics(this);
      lyrics->setTick(tick);
      lyrics->setStaff(chord->staff());
      lyrics->setParent(segment);
      lyrics->setNo(no);
      undoAddElement(lyrics);
      select(lyrics, 0, 0);
      canvas()->startEdit(lyrics);
      layout();
      }
#if 0
//---------------------------------------------------------
//   addExpression
//---------------------------------------------------------

void Score::addExpression()
      {
      printf("Not impl.: addExpression()\n");
      endCmd(true);
      }

//---------------------------------------------------------
//   addTechnik
//---------------------------------------------------------

void Score::addTechnik()
      {
      printf("Not impl.: addTechnik()\n");
      endCmd(true);
      }
#endif
//---------------------------------------------------------
//   cmdTuplet
//---------------------------------------------------------

void Score::cmdTuplet(int n)
      {
      Note* note = getSelectedNote();
      if (note == 0)
            return;
      Chord* chord = note->chord();

      int normalNotes=2, actualNotes=3;
      switch (n) {
            case 2:
                  // normalNotes = 3;
                  // actualNotes = 2;
                  printf("duole not implemented\n");
                  return;
            case 3:
                  normalNotes = 2;
                  actualNotes = 3;
                  break;
            case 5:
                  normalNotes = 4;
                  actualNotes = 5;
                  break;
            }
      int baseLen = chord->tickLen() / normalNotes;
      if (chord->tickLen() % normalNotes) {
            printf("cannot handle tuplet (rest %d)\n", chord->tickLen() % normalNotes);
            return;
            }

      //---------------------------------------------------
      //    - remove rest/note
      //    - replace with note + (actualNotes-1) rests
      //    - add 2 rests of 1/d2 duration as placeholder
      //---------------------------------------------------

      int voice    = chord->voice();
      Staff* staff = chord->staff();
      int staffIdx = chord->staffIdx();
      int track    = staffIdx * VOICES + voice;
      int tick     = chord->tick();
      int pitch    = note->pitch();

      Segment* segment = chord->segment();
      segment->setElement(track, 0);
      undoOp(UndoOp::RemoveElement, chord);

      Tuplet* tuplet = new Tuplet(this);
      tuplet->setNormalNotes(normalNotes);
      tuplet->setActualNotes(actualNotes);
      tuplet->setBaseLen(baseLen);
      tuplet->setStaff(staff);
      Measure* measure = chord->measure();
      tuplet->setParent(measure);
      undoAddElement(tuplet);

      int ticks = baseLen * normalNotes / actualNotes;

      note = new Note(this);
      note->setPitch(pitch);
      note->setStaff(staff);
      chord = new Chord(this);
      chord->setTick(tick);
      chord->setTuplet(tuplet);
      tuplet->add(chord);
      chord->setVoice(voice);
      chord->setStaff(staff);
      chord->add(note);
      chord->setTickLen(ticks);
      Segment::SegmentType st = Segment::segmentType(chord->type());
      Segment* seg = measure->findSegment(st, tick);
      if (seg == 0) {
            seg = measure->createSegment(st, tick);
            undoAddElement(seg);
            }
      chord->setParent(seg);
      undoAddElement(chord);
//      measure->layoutNoteHeads(staffIdx);

      for (int i = 0; i < (actualNotes-1); ++i) {
            tick += ticks;
            Rest* rest = new Rest(this);
            rest->setTick(tick);
            rest->setTuplet(tuplet);
            tuplet->add(rest);
            rest->setVoice(voice);
            rest->setStaff(staff);
            rest->setTickLen(ticks);
            Segment::SegmentType st = Segment::segmentType(rest->type());
            Segment* seg = measure->findSegment(st, tick);
            if (seg == 0) {
                  seg = measure->createSegment(st, tick);
                  undoAddElement(seg);
                  }
            rest->setParent(seg);
            undoAddElement(rest);
            }
      layout();
      }

//---------------------------------------------------------
//   changeVoice
//---------------------------------------------------------

void Score::changeVoice(int voice)
      {
      padState.voice = voice;
      if (cis->voice != voice) {
            cis->voice = voice;
            layout();
            // moveCursor();
            }
      }

//---------------------------------------------------------
//   colorItem
//---------------------------------------------------------

void Score::colorItem(Element* element)
      {
      QColor sc(element->color());
      QColor c = QColorDialog::getColor(sc);
      if (!c.isValid())
            return;

      foreach(Element* e, *sel->elements()) {
            if (e->color() != c) {
                  QColor color = e->color();
                  e->setColor(c);
                  undoOp(UndoOp::ChangeColor, e, color);
                  refresh |= e->abbox();
                  }
            }
      sel->deselectAll(this);
      }

//---------------------------------------------------------
//   pageBreak
//---------------------------------------------------------

void Score::pageBreak()
      {
      if (sel->state() != SEL_STAFF && sel->state() != SEL_SYSTEM) {
            printf("no system selected!\n");
            return;
            }
      Measure* m = tick2measure(sel->tickStart);
      m->setPageBreak(!m->pageBreak());
      layout();
      }

//---------------------------------------------------------
//   systemBreak
//---------------------------------------------------------

void Score::systemBreak()
      {
      if (sel->state() != SEL_STAFF && sel->state() != SEL_SYSTEM) {
            printf("no system selected!\n");
            return;
            }
      Measure* m = tick2measure(sel->tickStart);
      m->setLineBreak(!m->lineBreak());
      layout();
      }

