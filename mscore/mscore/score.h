//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//  $Id: score.h,v 1.21 2006/04/12 14:58:10 wschweer Exp $
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

#ifndef __SCORE_H__
#define __SCORE_H__

/**
 \file
 Definition of Score class.
*/

#include "input.h"
#include "globals.h"
#include "style.h"
#include "durationtype.h"
#include "select.h"

class System;
class TextStyle;
class Page;
class SigList;
class PageFormat;
class ElementList;
class TempoList;
class Selection;
class Segment;
class Rest;
class Xml;
class Canvas;
class Viewer;
class Articulation;
class Note;
class Chord;
class ChordRest;
class Slur;
class Hairpin;
class Undo;
class EditTempo;
class Part;
class MidiFile;
class ScoreLayout;
class MidiTrack;
class BSymbol;
class KeySig;
class Volta;
class BBTrack;
class MidiEvent;
struct MNote;
class Excerpt;
class EventMap;
class Harmony;
struct Channel;
class Tuplet;
class Capella;
class CapVoice;
class TextC;
class Dynamic;
class Measure;
class MeasureBase;
class Staff;
class Part;
class Instrument;
class UndoStack;

struct SigEvent;
struct TEvent;

extern bool showRubberBand;

//
// MuseScore _state
//

enum {
      STATE_DISABLED   = 0,
      STATE_NORMAL     = 1,
      STATE_NOTE_ENTRY = 2,
      STATE_EDIT       = 4,
      STATE_PLAY       = 8
      };

//---------------------------------------------------------
//   MeasureBaseList
//---------------------------------------------------------

class MeasureBaseList {
      int _size;
      MeasureBase* _first;
      MeasureBase* _last;

      void push_back(MeasureBase* e);
      void push_front(MeasureBase* e);

   public:
      MeasureBaseList();
      MeasureBase* first() const { return _first; }
      MeasureBase* last()  const { return _last; }
      void clear()               { _first = _last = 0; }
      void add(MeasureBase*);
      void remove(MeasureBase*);
      void change(MeasureBase* o, MeasureBase* n);
      };

//---------------------------------------------------------
//   MidiMapping
//---------------------------------------------------------

struct MidiMapping {
      char port;
      char channel;
      Part* part;
      Channel* articulation;
      };

//---------------------------------------------------------
//   MidiInputEvent
//---------------------------------------------------------

struct MidiInputEvent {
      int pitch;
      bool chord;
      };

//---------------------------------------------------------
//   Position
//---------------------------------------------------------

struct Position {
      Measure* measure;
      int tick;
      int staffIdx;
      int line;
      QPointF pos;
      };

//---------------------------------------------------------
//   ImagePath
//---------------------------------------------------------

class ImagePath {
      QString _path;
      int _references;  // > 0 if image is used in score
      QBuffer _buffer;
      bool _loaded;     // true if buffer contains image data

   public:
      ImagePath(const QString& p);
      void dereference();
      void reference();
      const QString& path() const      { return _path;     }
      QBuffer& buffer()                { return _buffer;   }
      void setLoaded(bool val)         { _loaded = val;    }
      bool loaded() const              { return _loaded;   }
      void setPath(const QString& val) { _path = val;      }
      bool isUsed() const              { return _references > 0;  }
      };

//---------------------------------------------------------
//   Score
//---------------------------------------------------------

class Score : public QObject {
      Q_OBJECT

      UndoStack* _undo;

      QList<ImagePath*> imagePathList;

      int _magIdx;
      double _mag;
      double _xoff, _yoff;

      QQueue<MidiInputEvent> midiInputQueue;
      QList<MidiMapping> _midiMapping;
      MeasureBaseList _measures;          // here are the notes
      QList<Element*> _gel;               // global elements: Slur, SLine

      InputState _is;

      QList<Excerpt*> _excerpts;

      Style _style;
      QVector<TextStyle*> _textStyles;

      QFileInfo info;
      bool _created;          ///< file is never saved, has generated name

      // the following variables are reset on startCmd()
      //   modified during cmd processing and used in endCmd() to
      //   determine what to layout and what to repaint:

      QRectF refresh;
      bool updateAll;
      Measure* layoutStart;   ///< start a relayout at this measure
      bool layoutAll;         ///< do a complete relayout

      Qt::KeyboardModifiers keyState;

      QList<Viewer*> viewer;

      ScoreLayout* _layout;

      bool _showInvisible;
      bool _showFrames;

      EditTempo* editTempo;
      Element* _dragObject;
      QPointF _startDragPosition;

      QList<Part*> _parts;
      QList<Staff*> _staves;

      bool _printing;   ///< True if we are drawing to a printer
      bool _playlistDirty;
      bool _dirty;      ///< Score data was modified.
      bool _saved;      ///< True if project was already saved; only on first
                        ///< save a backup file will be created, subsequent
                        ///< saves will not overwrite the backup file.
      int _playPos;     ///< sequencer seek position

      bool _foundPlayPosAfterRepeats; ///< Temporary used during playback rendering
                                      ///< indicating if playPos after expanded repeats
                                      ///< has been calculated.

      int _fileDivision; ///< division of current loading *.msc file
      int _mscVersion;   ///< version of current loading *.msc file

      QString _movementNumber;       // some meta data; used for musicXML
      QString _movementTitle;
      QString _workNumber;
      QString _workTitle;
      QString _source;

      ChordRest* nextMeasure(ChordRest* element, bool selectBehavior = false);
      ChordRest* prevMeasure(ChordRest* element);
      void cmdSetBeamMode(int);
      void cmdFlipStemDirection();
      Note* getSelectedNote();
      void pageNext();
      void pagePrev();
      void pageTop();
      void pageEnd();
      Note* upAlt(Element*);
      Note* upAltCtrl(Note*) const;
      Note* downAlt(Element*);
      Note* downAltCtrl(Note*) const;
      ChordRest* upStaff(ChordRest* cr);
      ChordRest* downStaff(ChordRest* cr);
      void moveUp(Note*);
      void moveDown(Note*);

      void convertMidi(MidiFile*, int);
      void convertCapella(Capella* cap);
      int readCapVoice(CapVoice* cvoice, int staffIdx, int tick);
      void convertTrack(MidiTrack*);
      void convertTrack(BBTrack*, int);

      void move(const QString& cmd);
      void selectMove(const QString& cmd);

      void collectChord(EventMap*, Instrument*,
         int pitchOffset, Chord*, int tick, int gateTime);
      void collectMeasureEvents(EventMap*, Measure*, int staffIdx, int tickOffset);

      void padToggle(int n);
      void insertMeasures(int, int);

      void cmdAddPitch(int note, bool addFlag);
      Note* cmdAddPitch1(int pitch, bool addFlag);
      void cmdAddInterval(int);

      void printFile();
      void addLyrics();
      void addTempo();
      void addMetronome();

      MeasureBase* searchLabel(const QString& s, MeasureBase* start = 0);
      void undoSigInsertTime(int, int);
      void undoFixTicks();

      void cmdAddText(int style);
      void cmdAddChordName();
      void cmdAddChordName2();
      int processPendingNotes(QList<MNote*>* notes, int, int);
      void writeExcerpt(Excerpt*, Xml&);
      void cmdResetBeamMode();
      void connectSlurs();
      void checkSlurs();
      void checkTuplets();
      void tupletDialog();

      void cmdInsertClef(int type);
      void cmdCreateTuplet(ChordRest*, Tuplet*);
      void cmdExchangeVoice(int, int);
      void cmdPaste();

      void updateSelectedElements(SelState);
      void removeChordRest(ChordRest* cr, bool clearSegment);
      void cmdMove(Element* e, QPointF delta);

      void resetUserStretch();
      void toDefault();

   signals:
      void selectionChanged(int);
      void dirtyChanged(Score*);
      void stateChanged(int);
      void posChanged(int);

   public:
      int curTick;            // for read optimizations
      int curTrack;

      TextC* rights;

      int _pageOffset;              ///< Offset for page numbers.

      SigList*   sigmap;
      TempoList* tempomap;

      //---------------------------------------------------
      //    state information
      //---------------------------------------------------

      int _state;
      int _prevState;               ///< state before playback

      Selection* sel;
      Element* origEditObject;
      Element* editObject;          ///< Valid in edit mode
      QString oldInstrumentName;    ///< valid while editing instrument names

      System* dragSystem;           ///< Valid if DRAG_STAFF.
      int dragStaff;

      void cmdAppendMeasures(int);
      void cmdInsertMeasures(int);
      bool noStaves() const         { return _staves.empty(); }
      void insertPart(Part*, int);
      void removePart(Part*);
      void insertStaff(Staff*, int);
      void cmdRemoveStaff(int staffIdx);
      void removeStaff(Staff*);
      void addMeasure(MeasureBase*);
      void removeMeasure(MeasureBase*);
      void appendMeasures(int, int);
      void readStaff(QDomElement);

      void cmdTuplet(int);
      void cmdInsertPart(Part*, int);
      void cmdRemovePart(Part*);
      void cmdReplaceElements(Measure* sm, Measure* dm, int srcStaff, int dstStaff);
      void cmdAddSlur();
      void cmdAddSlur(Note* note);
      void cmdAddTie();
      void cmdAddHairpin(bool);
      void cmdAddStretch(double);
      void transpose();

      void cmdEnterRest();
      void cmdEnterRest(Duration::DurationType d);
      void updateEntryMode();

      Rest* addRest(int tick, int len, int track);
      void lyricsEndEdit();
      void harmonyEndEdit();

   public slots:
      void setClean(bool val);
      void setDirty(bool val = true) { setClean(!val); }

   public:
      Score(const Style&);
      ~Score();

      Score* clone();

      void clear();
      void write(Xml&, bool autosave);
      bool read(QDomElement);

      QList<Staff*>& staves()                { return _staves; }
      Q_INVOKABLE int nstaves() const        { return _staves.size(); }

      int staffIdx(const Part*) const;
      int staffIdx(const Staff* staff) const { return _staves.indexOf((Staff*)staff, 0); }
      Staff* staff(int n) const              { return _staves.value(n); }

      Part* part(int staffIdx);

      MeasureBase* pos2measure(const QPointF&, int* tick, int* staffIdx, int* pitch,
         Segment**, QPointF* offset) const;
      Measure* pos2measure2(const QPointF&, int* tick, int* staffIdx, int* pitch, Segment**) const;
      Measure* pos2measure3(const QPointF& p, int* tick) const;

      void addViewer(Viewer* v);

      void undoChangeSig(int tick, const SigEvent& o, const SigEvent& n);
      void undoChangeKeySig(Staff* staff, int tick, int o, int n);
      void undoChangeTempo(int tick, const TEvent& o, const TEvent& n);
      void undoChangeKey(Staff* staff, int tick, int o, int n);
      void undoChangeClef(Staff* staff, int tick, int o, int n);
      void undoAddElement(Element* element);
      void undoRemoveElement(Element* element);
      void undoChangeMeasureLen(Measure* m, int oldTicks, int newTicks);
      void undoChangeElement(Element* oldElement, Element* newElement);
      void undoInsertTime(int tick, int len);
      void undoChangeRepeatFlags(Measure*, int);
      void undoChangeVoltaEnding(Volta* volta, const QList<int>& l);
      void undoChangeVoltaText(Volta* volta, const QString& s);
      void undoChangeChordRestSize(ChordRest* cr, bool small);
      void undoChangeChordNoStem(Chord* cr, bool noStem);
      void undoChangeChordRestSpace(ChordRest* cr, Spatium l, Spatium t);
      void undoChangeSubtype(Element* element, int st);
      void undoChangePitch(Note* note, int pitch, int tpc, int userAccidental);
      void spellNotelist(QList<Note*>& notes);
      void undoChangeTpc(Note* note, int tpc);
      void undoChangeBeamMode(ChordRest* cr, BeamMode mode);
      void undoChangeChordRestLen(ChordRest* cr, int len);
      void undoChangeEndBarLineType(Measure*, int);
      void undoChangeBarLineSpan(Staff*, int);
      void undoChangeUserOffset(Element* e, const QPointF& offset);
      void undoChangeVelocity(Dynamic* e, int velocity);
      void undoChangeCopyright(const QString&);
      void undoTransposeHarmony(Harmony*, int);
      void undoExchangeVoice(Measure* measure, int val1, int val2, int staff1, int staff2);
      void undoRemovePart(Part* part, int idx);
      void undoInsertPart(Part* part, int idx);
      void undoRemoveStaff(Staff* staff, int idx);
      void undoInsertStaff(Staff* staff, int idx);
      void undoInsertMeasure(MeasureBase*);
      void undoChangeAccidental(Note*, int);
      void undoToggleInvisible(Element*);
      void undoMove(Element* e, const QPointF& pt);
      void undoChangeBracketSpan(Staff* staff, int column, int span);

      Note* setNote(int tick, int track, int pitch, int len, int headGroup = 0,
         Direction stemDirection = AUTO);
      void changeCRlen(ChordRest* cr, int len);
      int makeGap(int tick, int track, int len);
      int makeGap1(int tick, int staff, int len);
      void cloneCR(ChordRest* cr, int tick, int len, int track);

      Element* setTupletChordRest(ChordRest* cr, int pitch, int len);

      void setGraceNote(Chord*,  int pitch, NoteType type, int len);
      int clefOffset(int tick, Staff*) const;
      Rest* setRest(int tick, int len, int track);
      bool setRest(int tick, int track, int len, bool useDots);
      Canvas* canvas() const;

      void select(Element* obj, SelectType, int staff);
      void deselect(Element* obj);

      void searchSelectedElements();

      bool needLayout() const;

      void upDown(bool up, bool octave);
      Element* searchNote(int tick, int track) const;

      // undo/redo ops
      void endUndoRedo(Undo*);
      void addArticulation(int);
      void addAccidental(int);
      void addAccidental(Note* oNote, int prefix);

      void addElement(Element*);
      void removeElement(Element*);

      void addTimeSig(int tick, int keySigSubtype);

      void cmdAdd1(Element* e, const QPointF& pos, const QPointF& dragOffset);
      void cmdAddBSymbol(BSymbol*, const QPointF&, const QPointF&);

      Note* addNote(Chord*, int pitch);

      void deleteItem(Element*);
      void cmdDeleteSelection();
      void toggleInvisible(Element* obj);

      void changeRest(Rest* rest, int tick, int len);

      void putNote(const QPointF& pos, bool replace);
      void setPadState();
      void setPadState(Element* obj);

      void startCmd();        // start undoable command
      void endCmd();          // end undoable command
      void end();             // layout & update canvas

      void cmdAdd(Element*);
      void cmdRemove(Element*);

      void setUpdateAll()              { updateAll = true; }
      void setLayoutAll(bool val)      { layoutAll = val;  }
      void setLayoutStart(Measure* m)  { layoutStart = m;  }
      void addRefresh(const QRectF& r) { refresh |= r;     }

      void chordTab(bool back);
      void lyricsTab(bool back, bool end);
      void lyricsReturn();
      void lyricsMinus();
      void lyricsUnderscore();
      void changeLineSegment(bool);

      void startEdit(Element* element);
      void endEdit();

      void startDrag(Element*);
      void drag(const QPointF&);
      void endDrag();

      void changeVoice(int);
      void setNoteEntry(bool on);

      void colorItem(Element*);
      void adjustCanvasPosition(Element* el, bool playBack);
      Element* dragObject() const    { return _dragObject; }
      void setDragObject(Element* e) { _dragObject = e; }
      void midiNoteReceived(int pitch, bool);
      const QList<Part*>* parts() const  { return &_parts; }
      void appendPart(Part* p);
      void updateStaffIndex();
      void sortStaves(QList<int>& dst);
      bool read(QString name);

      void setSpatium(double v);

      bool showInvisible() const { return _showInvisible; }
      bool showFrames() const { return _showFrames; }
      void setShowInvisible(bool v);
      void setShowFrames(bool v);

      bool loadMsc(QString name);
      bool loadCompressedMsc(QString name);
      void importMusicXml(const QString&);
      void importCompressedMusicXml(const QString&);
      bool importMidi(const QString& name);
      bool importMuseData(const QString& name);
      bool importLilypond(const QString& name);
      bool importBB(const QString& name);
      bool importCapella(const QString& name);
      void saveFile(QFileInfo& info, bool autosave);
      void saveFile(QIODevice* f, bool autosave);
      void saveCompressedFile(QFileInfo&, bool autosave);
      void saveCompressedFile(QIODevice*, QFileInfo&, bool autosave);
      bool saveAs(bool saveCopy = false);

      void print(QPrinter* printer);
      bool saveXml(const QString& name);
      bool saveMxl(const QString& name);
      bool saveMidi(const QString& name);
      bool savePsPdf(const QString& saveName, QPrinter::OutputFormat format);
      bool savePng(const QString& name);
      bool saveSvg(const QString& name);
      bool saveLilypond(const QString& name);

      ChordRest* getSelectedChordRest() const;
      Element* getSelectedElement() const { return sel->element(); }
      Selection* selection() const        { return sel; }
      void setSelection(Selection* s)     { sel = s;    }

      int pos();
      Measure* tick2measure(int tick) const;
      MeasureBase* tick2measureBase(int tick) const;
      Segment* tick2segment(int tick) const;
      void fixTicks();
      PageFormat* pageFormat() const;
      ScoreLayout* layout() const { return _layout; }

      void addArticulation(Element*, Articulation* atr);

      bool playlistDirty();
      void changeTimeSig(int tick, int st);

      void cmd(const QString&);
      int fileDivision(int t) const { return (t * division + _fileDivision/2) / _fileDivision; }
      bool saveFile(bool autosave);
      void adjustTime(int tick, MeasureBase*);

      QString filePath() const    { return info.filePath(); }
      QFileInfo* fileInfo()       { return &info; }

      QString name() const           { return info.completeBaseName(); }
      void setName(const QString& s) { info.setFile(s); }

      bool isSavable() const;
      bool dirty() const        { return _dirty;    }
      void setCreated(bool val) { _created = val;   }
      bool created() const      { return _created;  }
      bool saved() const        { return _saved;    }
      void setSaved(bool v)     { _saved = v;       }
      bool printing() const     { return _printing; }

      bool pos2TickAnchor(const QPointF&, int staffIdx, int* tick, QPointF* anchor) const;
      void spell();
      void spell(Note*);
      int nextSeg(int tick, int track);
      int nextSeg1(int tick, int& track);
      int prevSeg1(int tick, int& track);

      const Style& style() const              { return _style;                   }
      void setStyle(const Style& s)           { _style = s;                      }
      StyleVal style(STYLE_TYPE type) const   { return _style[type];             }
      Spatium styleS(STYLE_TYPE type) const   { return _style[type].toSpatium(); }
      bool styleB(STYLE_TYPE type) const      { return _style[type].toBool();    }
      double styleD(STYLE_TYPE type) const    { return _style[type].toDouble();  }
      int styleI(STYLE_TYPE type) const       { return _style[type].toInt();     }
      void setStyle(STYLE_TYPE t, StyleVal v) { _style[t] = v;                   }

      void insertTime(int tick, int len);
      void cmdRemoveTime(int tick, int len);
      QList<Viewer*> getViewer()              { return viewer;      }
      int playPos() const                     { return _playPos;    }
      void setPlayPos(int val)                { _playPos = val;     }

      bool noteEntryMode() const              { return _is.noteEntryMode; }
      int inputPos() const                    { return _is.pos();   }
      int inputTrack() const                  { return _is.track;   }
      InputState& inputState()                 { return _is;        }
      void setInputState(const InputState& st) { _is = st;          }
      void setInputTrack(int);

      TextStyle* textStyle(int idx) { return idx < 0 ? 0 : _textStyles[idx]; }
      const QVector<TextStyle*>& textStyles() const { return _textStyles; }
      void setTextStyles(const QVector<TextStyle*>&s);
      bool loadStyle(QFile* qf);
      void loadStyle();
      void saveStyle();
      void textStyleChanged(const QVector<TextStyle*>&s);

      void pasteStaff(QDomElement, int dstTick, int staffIdx);
      bool isVolta(int tick, int repeat) const;
      void toEList(EventMap* events, int tickOffset);
      void toEList(EventMap* events, bool expandRepeats, int tickOffset, int staffIdx);
      int mscVersion() const { return _mscVersion; }

      SigList*   getSigmap()  { return sigmap; }
      MeasureBase* appendMeasure(int type);
      void addLyrics(int tick, int staffIdx, const QString&);

      QList<Excerpt*>* excerpts() { return &_excerpts; }
      Score* createExcerpt(Excerpt*);
      MeasureBaseList* measures()  { return &_measures; }

      bool checkHasMeasures() const;

      QList<Element*>* gel()                  { return &_gel; }
      const QList<Element*>* gel() const      { return &_gel; }
      void setLayout(Measure* m);
      int midiPort(int idx) const;
      int midiChannel(int idx) const;
      QList<MidiMapping>* midiMapping()       { return &_midiMapping; }
      void rebuildMidiMapping();
      void updateChannel();
      void cmdTransposeStaff(int staffIdx, int offset);
      void cmdConcertPitchChanged(bool);
      TempoList* getTempomap() const { return tempomap; }

      QString movementNumber() const           { return _movementNumber; }
      QString movementTitle() const            { return _movementTitle;  }
      QString workNumber() const               { return _workNumber;     }
      QString workTitle() const                { return _workTitle;      }
      QString source() const                   { return _source;         }
      void setMovementNumber(const QString& s) { _movementNumber = s;    }
      void setMovementTitle(const QString& s)  { _movementTitle = s;     }
      void setWorkNumber(const QString& s)     { _workNumber = s;        }
      void setWorkTitle(const QString& s)      { _workTitle = s;         }
      void setSource(const QString& s)         { _source = s;            }

      void lassoSelect(const QRectF&);
      void lassoSelectEnd(const QRectF&);

      Page* searchPage(const QPointF&) const;
      bool getPosition(Position* pos, const QPointF&, bool divideSegment) const;

      void setState(int s);
      int state() const        { return _state; }
      void setPrevState(int s) { _prevState = s; }
      int prevState() const    { return _prevState; }

      void cmdDeleteTuplet(Tuplet*, bool replaceWithRest);

      int magIdx() const      { return _magIdx; }
      void setMagIdx(int val);
      double mag()            { return _mag; }
      void setMag(double d);
      double xoff() const     { return _xoff; }
      double yoff() const     { return _yoff; }
      void setXoff(double v)  { _xoff = v;    }
      void setYoff(double v)  { _yoff = v;    }

      void setPos(int tick);

      ImagePath* addImage(const QString&);      // add image to imagePathList
      void moveBracket(int staffIdx, int srcCol, int dstCol);
      Measure* getCreateMeasure(int tick);

      void adjustBracketsDel(int sidx, int eidx);
      void adjustBracketsIns(int sidx, int eidx);
      void renumberMeasures();
      UndoStack* undo() const { return _undo; }
      TextC* copyright() { return rights; }
      void setCopyright(const QString& s);
      void setCopyrightHtml(const QString& s);
      void endUndoRedo();
      };

extern Score* gscore;
extern void fixTicks();

#endif

