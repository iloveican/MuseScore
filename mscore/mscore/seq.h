//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: seq.h,v 1.20 2006/03/02 17:08:43 wschweer Exp $
//
//  Copyright (C) 2002-2008 Werner Schweer and others
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

#ifndef __SEQ_H__
#define __SEQ_H__

#include "event.h"
#include "driver.h"
#include "fifo.h"
#include "tempo.h"

class Synth;
class Note;
class QTimer;
class Score;
class Painter;
class Measure;
class Driver;
class Part;
struct Channel;

//---------------------------------------------------------
//   MidiPatch
//---------------------------------------------------------

struct MidiPatch {
      bool drum;
      signed char hbank, lbank, prog;
      const char* name;
      };

//---------------------------------------------------------
//   SeqMsg
//    message format for gui -> sequencer messages
//---------------------------------------------------------

enum { SEQ_NO_MESSAGE, SEQ_TEMPO_CHANGE, SEQ_PLAY, SEQ_SEEK };

struct SeqMsg {
      int id;
      int data;
      MidiOutEvent midiOutEvent;
      };

//---------------------------------------------------------
//   SeqMsgFifo
//---------------------------------------------------------

static const int SEQ_MSG_FIFO_SIZE = 256;

class SeqMsgFifo : public FifoBase {
      SeqMsg messages[SEQ_MSG_FIFO_SIZE];

   public:
      SeqMsgFifo();
      virtual ~SeqMsgFifo()     {}
      void enqueue(const SeqMsg&);        // put object on fifo
      SeqMsg dequeue();                   // remove object from fifo
      };

//---------------------------------------------------------
//   Seq
//    sequencer
//---------------------------------------------------------

class Seq : public QObject {
      Q_OBJECT

      Score* cs;
      bool running;                       // true if sequencer is available
      int state;                          // STOP, PLAY, START_PLAY
      bool playlistChanged;
      bool pauseState;

      SeqMsgFifo toSeq;

      Driver* driver;

      EventMap events;                    // playlist

      QList<NoteOn*> activeNotes;         // notes sounding
      double playTime;
      double startTime;

      EventMap::const_iterator playPos;   // moved in real time thread
      EventMap::const_iterator guiPos;    // moved in gui thread
      QList<Note*> markedNotes;           // notes marked as sounding

      int endTick;
      int curTick;
      int curUtick;

      float _volume;

      QTimer* heartBeatTimer;

      QList<Event*> eventList;

      void collectEvents();
      void collectMeasureEvents(Measure*, int staffIdx);

      void stopTransport();
      void startTransport();
      void setPos(int);
      void playEvent(const Event*);
      void playEvent(const MidiOutEvent&);
      void guiStop();
      void guiToSeq(const SeqMsg& msg);
      void startNote(Channel*, int, int);

   private slots:
      void seqMessage(int msg);
      void heartBeat();
      void selectionChanged(int);
      void midiInputReady();

   public slots:
      void setVolume(float);
      void setRelTempo(int);
      void seek(int);
      void stopNotes();

   signals:
      void started();
      void stopped();
      int toGui(int);

   public:
      enum { STOP, PLAY, START_PLAY };

      Seq();
      ~Seq();
      void start();
      void stop();
      void pause();
      void rewindStart();
      void seekEnd();
      void nextMeasure();
      void nextChord();
      void prevMeasure();
      void prevChord();

      bool loadSoundFont(const QString&);
      bool init();
      void exit();
      bool isRunning() const    { return running; }
      bool isPlaying() const    { return state == PLAY; }
      bool isStopped() const    { return state == STOP; }

      void processMessages();
      void process(unsigned, float*, float*, int stride);
      void processMidi();
      QList<QString> inputPorts();
      int sampleRate() const;
      int getEndTick() const    { return endTick;  }
      float volume() const      { return _volume;  }
      bool isRealtime() const   { return true;     }
      void sendMessage(SeqMsg&) const;
      void startNote(Channel*, int, int, int);
      void setController(int, int, int);
      void sendEvent(const MidiOutEvent&);
      void setScore(Score* s);
      void initInstruments();

      const MidiPatch* getPatchInfo(bool onlyDrums, const MidiPatch* p);
      Driver* getDriver()  { return driver; }
      int getCurTime();
      void getCurTick(int*, int*);
      };

extern Seq* seq;
extern void initSequencer();
extern bool initMidi();
#endif

