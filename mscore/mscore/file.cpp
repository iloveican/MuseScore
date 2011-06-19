//=============================================================================
//  MuseScore
//  Linux Music Score Editor
//  $Id$
//
//  Copyright (C) 2002-2011 Werner Schweer et al.
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

/**
 File handling: loading and saving.
 */

#include "config.h"
#include "globals.h"
#include "musescore.h"
#include "scoreview.h"
#include "libmscore/xml.h"
#include "libmscore/element.h"
#include "libmscore/note.h"
#include "libmscore/rest.h"
#include "al/sig.h"
#include "libmscore/clef.h"
#include "libmscore/key.h"
#include "instrdialog.h"
#include "libmscore/score.h"
#include "libmscore/page.h"
#include "libmscore/dynamic.h"
#include "file.h"
#include "libmscore/style.h"
#include "al/tempo.h"
#include "libmscore/select.h"
#include "preferences.h"
#include "input.h"
#include "playpanel.h"
#include "libmscore/staff.h"
#include "libmscore/part.h"
#include "libmscore/utils.h"
#include "libmscore/barline.h"
#include "palette.h"
#include "symboldialog.h"
#include "libmscore/slur.h"
#include "libmscore/hairpin.h"
#include "libmscore/ottava.h"
#include "libmscore/textline.h"
#include "libmscore/pedal.h"
#include "libmscore/trill.h"
#include "libmscore/volta.h"
#include "newwizard.h"
#include "libmscore/timesig.h"
#include "libmscore/box.h"
#include "libmscore/excerpt.h"
#include "libmscore/system.h"
#include "libmscore/tuplet.h"
#include "libmscore/keysig.h"
#include "zarchive.h"
#include "magbox.h"
#include "libmscore/measure.h"
#include "libmscore/undo.h"
#include "libmscore/repeatlist.h"
#include "scoretab.h"
#include "libmscore/beam.h"
#include "libmscore/stafftype.h"
#include "seq.h"
#include "libmscore/revisions.h"
#include "libmscore/lyrics.h"
#include "libmscore/segment.h"
#include "libmscore/tempotext.h"
#include "libmscore/sym.h"
#include "painterqt.h"

#ifdef OMR
#include "omr/omr.h"
#include "omr/omrpage.h"
#endif

#include "diff/diff_match_patch.h"
#include "libmscore/chordlist.h"
#include "libmscore/mscore.h"

//---------------------------------------------------------
//   createDefaultFileName
//---------------------------------------------------------

static QString createDefaultFileName(QString fn)
      {
      //
      // special characters in filenames are a constant source
      // of trouble, this replaces some of them common in german:
      //
      fn = fn.replace(QChar(' '),  "_");
      fn = fn.replace(QChar('\n'), "_");
      fn = fn.replace(QChar(0xe4), "ae");
      fn = fn.replace(QChar(0xf6), "oe");
      fn = fn.replace(QChar(0xfc), "ue");
      fn = fn.replace(QChar(0xdf), "ss");
      fn = fn.replace(QChar(0xc4), "Ae");
      fn = fn.replace(QChar(0xd6), "Oe");
      fn = fn.replace(QChar(0xdc), "Ue");
      return fn;
      }

//---------------------------------------------------------
//   readScoreError
//---------------------------------------------------------

void MuseScore::readScoreError(int rv, const QString& name) const
      {
      if (rv == 1) {
            QMessageBox::critical(0,
               tr("MuseScore: Load error"),
               QString(tr("Cannot read file: %1 error: %2").arg(name).arg(strerror(errno)))
               );
            }
      else {
            QMessageBox::critical(0,
               tr("MuseScore: Load error"),
               tr("unsupported file extension")
               );
            }
      }

/**
 Load file \a name.
 Display message box with error if loading fails.
 Return true if OK and false on error.
 */

bool LoadFile::load(const QString& name)
      {
      if (name.isEmpty())
            return false;

      QFile fp(name);
      if (!fp.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(0,
               QWidget::tr("MuseScore: file not found:"),
               name,
               QString::null, QWidget::tr("Quit"), QString::null, 0, 1);
            return false;
            }
      if (!loader(&fp)) {
            QMessageBox::warning(0,
               QWidget::tr("MuseScore: load failed:"),
               error,
               QString::null, QWidget::tr("Quit"), QString::null, 0, 1);
            fp.close();
            return false;
            }
      fp.close();
      return true;
      }

//---------------------------------------------------------
//   checkDirty
//    if dirty, save score
//    return true on cancel
//---------------------------------------------------------

bool MuseScore::checkDirty(Score* s)
      {
      if (s->dirty()) {
            QMessageBox::StandardButton n = QMessageBox::warning(this, tr("MuseScore"),
               tr("Save changes to the score \"%1\"\n"
               "before closing?").arg(s->name()),
               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
               QMessageBox::Save);
            if (n == QMessageBox::Save) {
                  if (s->isSavable()) {
                        if (!s->saveFile(false))
                              return true;
                        }
                  else {
                        if (!s->saveAs(false))
                              return true;
                        }

                  }
            else if (n == QMessageBox::Cancel)
                  return true;
            }
      return false;
      }

//---------------------------------------------------------
//   loadFile
//---------------------------------------------------------

/**
 Create a modal file open dialog.
 If a file is selected, load it.
 Handles the GUI's file-open action.
 */

void MuseScore::loadFile()
      {
      QString fn = getOpenScoreName(
         lastOpenPath,
#ifdef OMR
         tr("All Supported Files (*.mscz *.mscx *.msc *.xml *.mxl *.mid *.midi *.kar *.md *.mgu *.MGU *.sgu *.SGU *.cap *.pdf *.ove *.scw *.bww *.GTP *.GP3 *.GP4);;")+
#else
         tr("All Supported Files (*.mscz *.mscx *.msc *.xml *.mxl *.mid *.midi *.kar *.md *.mgu *.MGU *.sgu *.SGU *.cap *.ove *.scw *.bww *.GTP *.GP3 *.GP4);;")+
#endif
         tr("MuseScore Files (*.mscz *.mscx *.msc);;")+
         tr("MusicXML Files (*.xml *.mxl);;")+
         tr("MIDI Files (*.mid *.midi *.kar);;")+
         tr("Muse Data Files (*.md);;")+
         tr("Capella Files (*.cap);;")+
         tr("BB Files <experimental> (*.mgu *.MGU *.sgu *.SGU);;")+
#ifdef OMR
         tr("PDF Files <experimental omr> (*.pdf);;")+
#endif
         tr("Overture / Score Writer Files <experimental> (*.ove *.scw);;")+
         tr("Bagpipe Music Writer Files <experimental> (*.bww);;")+
         tr("Guitar Pro (*.GTP *.GP3 *.GP4 *.GP5);;")+
         tr("All Files (*)")
         );
      if (fn.isEmpty())
            return;
      Score* score = new Score(MScore::defaultStyle());
      int rv = score->readScore(fn);
      if (rv == 0) {
            setCurrentScoreView(appendScore(score));
            lastOpenPath = score->fileInfo()->path();
            writeSessionFile(false);
            }
      else {
            readScoreError(rv, fn);
            delete score;
            }
      }

//---------------------------------------------------------
//   saveFile
//    return true on success
//---------------------------------------------------------

/**
 Save the current score.
 Handles the GUI's file-save action.
 */

void MuseScore::saveFile()
      {
      if (cs == 0)
            return;
      cs->setSyntiState();
      if (cs->created()) {
            QString selectedFilter;
            QString fn = cs->fileInfo()->baseName();
            Text* t = cs->getText(TEXT_TITLE);
            if (t)
                  fn = t->getText();
            QString name = createDefaultFileName(fn);
            QString f1 = tr("Compressed MuseScore File (*.mscz)");
            QString f2 = tr("MuseScore File (*.mscx)");

            QString fname   = QString("%1.mscz").arg(name);
            QString filter = f1 + ";;" + f2;
            fn = mscore->getSaveScoreName(
               tr("MuseScore: Save Score"),
               fname,
               filter,
               &selectedFilter
               );
            if (fn.isEmpty())
                  return;
            cs->fileInfo()->setFile(fn);
            updateRecentScores(cs);
            cs->setCreated(false);
            writeSessionFile(false);
            }
      if (!cs->saveFile(false)) {
            QMessageBox::critical(mscore, tr("MuseScore: Save File"), MScore::lastError);
            return;
            }
      setWindowTitle("MuseScore: " + cs->name());
      int idx = scoreList.indexOf(cs);
      tab1->setTabText(idx, cs->name());
      if (tab2)
            tab2->setTabText(idx, cs->name());
      QString tmp = cs->tmpName();
      if (!tmp.isEmpty()) {
            QFile f(tmp);
            if (!f.remove())
                  printf("cannot remove temporary file <%s>\n", qPrintable(f.fileName()));
            cs->setTmpName("");
            }
      writeSessionFile(false);
      }

//---------------------------------------------------------
//   createDefaultName
//---------------------------------------------------------

QString MuseScore::createDefaultName() const
      {
      QString name(tr("Untitled"));
      int n;
      for (n = 1; ; ++n) {
            bool nameExists = false;
            QString tmpName;
            if (n == 1)
                  tmpName = name;
            else
                  tmpName = QString("%1-%2").arg(name).arg(n);
            foreach(Score* s, scoreList) {
                  if (s->name() == tmpName) {
                        nameExists = true;
                        break;
                        }
                  }
            if (!nameExists) {
                  name = tmpName;
                  break;
                  }
            }
      return name;
      }

//---------------------------------------------------------
//   newFile
//    create new score
//---------------------------------------------------------

void MuseScore::newFile()
      {
      if (newWizard == 0)
            newWizard = new NewWizard(this);
      newWizard->restart();
      if (newWizard->exec() != QDialog::Accepted)
            return;
      int pickupTimesigZ, pickupTimesigN;
      int measures       = newWizard->measures();
      Fraction timesig   = newWizard->timesig();
      bool pickupMeasure = newWizard->pickupMeasure(&pickupTimesigZ, &pickupTimesigN);
      if (pickupMeasure)
            measures += 1;
      KeySigEvent ks     = newWizard->keysig();

      Score* score = new Score(MScore::defaultStyle());
      score->setCreated(true);

      //
      //  create score from template
      //
      if (newWizard->useTemplate()) {
            int rv = score->readScore(newWizard->templatePath());
            if (rv != 0) {
                  readScoreError(rv, newWizard->templatePath());
#if 0
                  QMessageBox::warning(0,
                     tr("MuseScore: failure"),
                     tr("Load template file ") + newWizard->templatePath() + tr(" failed"),
                     QString::null, QString::null, QString::null, 0, 1);
#endif
                  delete score;
                  return;
                  }
            score->fileInfo()->setFile(createDefaultName());

            int m = 0;
            for (Measure* mb = score->firstMeasure(); mb; mb = mb->nextMeasure()) {
                  if (mb->type() == MEASURE)
                        ++m;
                  }
            //
            // remove all notes & rests
            //
            score->deselectAll();
            for (Segment* s = score->firstMeasure()->first(); s;) {
                  Segment* ns = s->next1();
                  if (s->subtype() == SegChordRest && s->tick() == 0) {
                        int tracks = s->elist().size();
                        for (int track = 0; track < tracks; ++track) {
                              delete s->element(track);
                              s->setElement(track, 0);
                              }
                        }
                  else if (
                     (s->subtype() == SegChordRest)
                     || (s->subtype() == SegClef)
                     || (s->subtype() == SegKeySig)
                     || (s->subtype() == SegGrace)
                     || (s->subtype() == SegBreath)
                     ) {
                        s->measure()->remove(s);
                        delete s;
                        }
                  s = ns;
                  }
            }
      //
      //  create new score from scratch
      //
      else {
            score->fileInfo()->setFile(createDefaultName());
            newWizard->createInstruments(score);
            }
      if (!newWizard->title().isEmpty())
            score->fileInfo()->setFile(newWizard->title());
      Measure* pm = score->firstMeasure();
      for (int i = 0; i < measures; ++i) {
            Measure* m;
            if (pm) {
                  m  = pm;
                  pm = pm->nextMeasure();
                  }
            else {
                  m = new Measure(score);
                  score->measures()->add(m);
                  }
            m->setTimesig(timesig);
            m->setLen(timesig);
            if (pickupMeasure) {
                  if (i == 0) {
                        m->setIrregular(true);        // dont count pickup measure
                        m->setLen(Fraction(pickupTimesigZ, pickupTimesigN));
                        }
                  else if (i == (measures - 1)) {
                        // last measure is shorter
                        m->setLen(timesig - Fraction(pickupTimesigZ, pickupTimesigN));
                        }
                  }
            if (i == (measures - 1))
                  m->setEndBarLineType(END_BAR, false);
            else
                  m->setEndBarLineType(NORMAL_BAR, true);
            }

      int tick = 0;
      for (MeasureBase* mb = score->measures()->first(); mb; mb = mb->next()) {
            mb->setTick(tick);
            if (mb->type() != MEASURE)
                  continue;
            Measure* measure = static_cast<Measure*>(mb);
            int ticks = measure->ticks();
	      for (int staffIdx = 0; staffIdx < score->nstaves(); ++staffIdx) {
                  Staff* staff = score->staff(staffIdx);
                  if (tick == 0) {
                        if (!staff->useTablature()) {
                              TimeSig* ts = new TimeSig(score);
                              ts->setSig(timesig);
                              ts->setTrack(staffIdx * VOICES);
                              Segment* s = measure->getSegment(ts, 0);
                              s->add(ts);
                              }
                        Part* part = staff->part();
                        if (!part->instr()->useDrumset()) {
                              //
                              // transpose key
                              //
                              KeySigEvent nKey = ks;
                              if (part->instr()->transpose().chromatic && !newWizard->useTemplate()) {
                                    int diff = -part->instr()->transpose().chromatic;
                                    nKey.setAccidentalType(transposeKey(nKey.accidentalType(), diff));
                                    }
                              if (nKey.accidentalType()) {
                                    (*(staff->keymap()))[0] = nKey;
                                    KeySig* keysig = new KeySig(score);
                                    keysig->setTrack(staffIdx * VOICES);
                                    keysig->setKeySigEvent(nKey);
                                    Segment* s = measure->getSegment(keysig, 0);
                                    s->add(keysig);
                                    }
                              }
                        }
                  if (staff->primaryStaff()) {
                        if (measure->timesig() != measure->len()) {
                              int tick = measure->tick();
                              QList<Duration> dList = toDurationList(measure->len(), false);
                              if (!dList.isEmpty()) {
                                    foreach(Duration d, dList) {
		                              Rest* rest = new Rest(score, d);
                                          rest->setDuration(d.fraction());
            	                        rest->setTrack(staffIdx * VOICES);
	            	                  Segment* s = measure->getSegment(rest, tick);
		                              s->add(rest);
                                          tick += rest->actualTicks();
                                          }
                                    }
                              }
                        else {
		                  Rest* rest = new Rest(score, Duration(Duration::V_MEASURE));
                              rest->setDuration(measure->len());
            	            rest->setTrack(staffIdx * VOICES);
	            	      Segment* s = measure->getSegment(rest, tick);
		                  s->add(rest);
                              }
                        }
                  }
            tick += ticks;
            }
      score->fixTicks();
      //
      // ceate linked staves
      //
      foreach(Staff* staff, score->staves()) {
            if (!staff->linkedStaves())
                  continue;
            foreach(Staff* lstaff, staff->linkedStaves()->staves()) {
                  if (staff != lstaff) {
                        cloneStaff(staff, lstaff);
                        }
                  }
            }
      //
      // select first rest
      //
      Measure* m = score->firstMeasure();
      for (Segment* s = m->first(); s; s = s->next()) {
            if (s->subtype() == SegChordRest) {
                  if (s->element(0)) {
                        score->select(s->element(0), SELECT_SINGLE, 0);
                        break;
                        }
                  }
            }

      QString title     = newWizard->title();
      QString subtitle  = newWizard->subtitle();
      QString composer  = newWizard->composer();
      QString poet      = newWizard->poet();
      QString copyright = newWizard->copyright();

      if (!title.isEmpty() || !subtitle.isEmpty() || !composer.isEmpty() || !poet.isEmpty()) {
            MeasureBase* measure = score->measures()->first();
            if (measure->type() != VBOX) {
                  MeasureBase* nm = new VBox(score);
                  nm->setTick(0);
                  nm->setNext(measure);
                  score->measures()->add(nm);
                  measure = nm;
                  }
            if (!title.isEmpty()) {
                  Text* s = new Text(score);
                  s->setSubtype(TEXT_TITLE);
                  s->setTextStyle(TEXT_STYLE_TITLE);
                  s->setText(title);
                  measure->add(s);
                  score->setMetaTag("workTitle", title);
                  }
            if (!subtitle.isEmpty()) {
                  Text* s = new Text(score);
                  s->setSubtype(TEXT_SUBTITLE);
                  s->setTextStyle(TEXT_STYLE_SUBTITLE);
                  s->setText(subtitle);
                  measure->add(s);
                  }
            if (!composer.isEmpty()) {
                  Text* s = new Text(score);
                  s->setSubtype(TEXT_COMPOSER);
                  s->setTextStyle(TEXT_STYLE_COMPOSER);
                  s->setText(composer);
                  measure->add(s);
                  }
            if (!poet.isEmpty()) {
                  Text* s = new Text(score);
                  s->setSubtype(TEXT_POET);
                  s->setTextStyle(TEXT_STYLE_POET);
                  s->setText(poet);
                  measure->add(s);
                  }
            }
      if (newWizard->createTempo()) {
            double tempo = newWizard->tempo();
            TempoText* tt = new TempoText(score);

            QString s = symbols[0][note4Sym].toString();
            tt->setText(QString("%1 = %2").arg(s).arg(tempo));
            tt->setTempo(tempo/60.0);
            tt->setTrack(0);
            Segment* seg = score->firstMeasure()->first(SegChordRest);
            seg->add(tt);
            score->tempomap()->changeTempo(0, tempo);
            }
      if (!copyright.isEmpty())
            score->setMetaTag("copyright", copyright);

      score->rebuildMidiMapping();
      score->doLayout();
      setCurrentScoreView(appendScore(score));
      }

//---------------------------------------------------------
//   getOpenFileName
//---------------------------------------------------------

QString MuseScore::getOpenScoreName(QString& dir, const QString& filter)
      {
      if (preferences.nativeDialogs) {
            return QFileDialog::getOpenFileName(this,
               tr("MuseScore: Load Score"), dir, filter);
            }
      QFileInfo myScores(preferences.myScoresPath);
      if (myScores.isRelative())
            myScores.setFile(QDir::home(), preferences.myScoresPath);
      if (loadScoreDialog == 0) {
            loadScoreDialog = new QFileDialog(this);
            loadScoreDialog->setFileMode(QFileDialog::ExistingFile);
            loadScoreDialog->setOption(QFileDialog::DontUseNativeDialog, true);
            loadScoreDialog->setWindowTitle(tr("MuseScore: Load Score"));

            QSettings settings;
            loadScoreDialog->restoreState(settings.value("loadScoreDialog").toByteArray());
            loadScoreDialog->setAcceptMode(QFileDialog::AcceptOpen);
            }
      // setup side bar urls
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(myScores.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));
      urls.append(QUrl::fromLocalFile(mscoreGlobalShare+"/demos"));
      loadScoreDialog->setSidebarUrls(urls);

      loadScoreDialog->setNameFilter(filter);
      loadScoreDialog->setDirectory(dir);

      QStringList result;
      if (loadScoreDialog->exec()) {
            result = loadScoreDialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getSaveScoreName
//---------------------------------------------------------

QString MuseScore::getSaveScoreName(const QString& title,
   QString& name, const QString& filter, QString* selectedFilter)
      {
      if (preferences.nativeDialogs) {
            QString fn = QFileDialog::getSaveFileName(this,
               title,
               name,
               filter,
               selectedFilter
               );
            return fn;
            }

      QFileInfo myScores(preferences.myScoresPath);
      if (myScores.isRelative())
            myScores.setFile(QDir::home(), preferences.myScoresPath);
      if (saveScoreDialog == 0) {
            saveScoreDialog = new QFileDialog(this);
            QSettings settings;
            saveScoreDialog->restoreState(settings.value("saveScoreDialog").toByteArray());
            saveScoreDialog->setFileMode(QFileDialog::AnyFile);
            saveScoreDialog->setOption(QFileDialog::DontConfirmOverwrite, false);
            saveScoreDialog->setOption(QFileDialog::DontUseNativeDialog, true);
            saveScoreDialog->setAcceptMode(QFileDialog::AcceptSave);
            }
      // setup side bar urls
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(myScores.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));
      saveScoreDialog->setSidebarUrls(urls);

      saveScoreDialog->setWindowTitle(title);
      saveScoreDialog->setNameFilter(filter);
      // saveScoreDialog->setDirectory(name);
      saveScoreDialog->selectFile(name);
      QStringList result;
      connect(saveScoreDialog, SIGNAL(filterSelected(const QString&)),
         SLOT(saveScoreDialogFilterSelected(const QString&)));
      if (saveScoreDialog->exec()) {
            result = saveScoreDialog->selectedFiles();
            *selectedFilter = saveScoreDialog->selectedNameFilter();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   saveScoreDialogFilterSelected
//    update selected file name extensions, when filter
//    has changed
//---------------------------------------------------------

void MuseScore::saveScoreDialogFilterSelected(const QString& s)
      {
      QRegExp rx(QString(".+\\(\\*\\.(.+)\\)"));
      if (rx.exactMatch(s)) {
            QFileInfo fi(saveScoreDialog->selectedFiles().front());
            saveScoreDialog->selectFile(fi.baseName() + "." + rx.cap(1));
            }
      }

//---------------------------------------------------------
//   getStyleFilename
//---------------------------------------------------------

QString MuseScore::getStyleFilename(bool open)
      {
      QString currentPath = QDir::currentPath();
      if (preferences.nativeDialogs) {
            QString fn;
            if (open) {
                  fn = QFileDialog::getOpenFileName(
                     this, tr("MuseScore: Load Style"),
                     currentPath,
                     tr("MuseScore Styles (*.mss);;" "All Files (*)")
                     );
                  }
            else {
                  fn = QFileDialog::getSaveFileName(
                     this, tr("MuseScore: Save Style"),
                     currentPath,
                     tr("MuseScore Style File (*.mss)")
                     );
                  }
            return fn;
            }

      QFileInfo myStyles(preferences.myStylesPath);
      if (myStyles.isRelative())
            myStyles.setFile(QDir::home(), preferences.myStylesPath);
      QFileDialog* dialog;
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(myStyles.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));

      if (open) {
            if (loadStyleDialog == 0) {
                  loadStyleDialog = new QFileDialog(this);
                  loadStyleDialog->setFileMode(QFileDialog::ExistingFile);
                  loadStyleDialog->setOption(QFileDialog::DontUseNativeDialog, true);
                  loadStyleDialog->setWindowTitle(tr("MuseScore: Load Style"));
                  loadStyleDialog->setNameFilter(tr("MuseScore Style File (*.mss)"));
                  loadStyleDialog->setDirectory(currentPath);

                  QSettings settings;
                  loadStyleDialog->restoreState(settings.value("loadStyleDialog").toByteArray());
                  loadStyleDialog->setAcceptMode(QFileDialog::AcceptOpen);
                  }
            urls.append(QUrl::fromLocalFile(mscoreGlobalShare+"/styles"));
            dialog = loadStyleDialog;
            }
      else {
            if (saveStyleDialog == 0) {
                  saveStyleDialog = new QFileDialog(this);
                  saveStyleDialog->setAcceptMode(QFileDialog::AcceptSave);
                  saveStyleDialog->setFileMode(QFileDialog::AnyFile);
                  saveStyleDialog->setOption(QFileDialog::DontConfirmOverwrite, false);
                  saveStyleDialog->setOption(QFileDialog::DontUseNativeDialog, true);
                  saveStyleDialog->setWindowTitle(tr("MuseScore: Save Style"));
                  saveStyleDialog->setNameFilter(tr("MuseScore Style File (*.mss)"));
                  saveStyleDialog->setDirectory(currentPath);

                  QSettings settings;
                  saveStyleDialog->restoreState(settings.value("saveStyleDialog").toByteArray());
                  saveStyleDialog->setAcceptMode(QFileDialog::AcceptSave);
                  }
            dialog = saveStyleDialog;
            }
      // setup side bar urls
      dialog->setSidebarUrls(urls);

      if (dialog->exec()) {
            QStringList result = dialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getSoundFont
//---------------------------------------------------------

QString MuseScore::getSoundFont(const QString& d)
      {
      QString filter = tr("SoundFont Files (*.sf2 *.SF2);;All (*)");

      if (preferences.nativeDialogs) {
            QString s = QFileDialog::getOpenFileName(
               mscore,
               MuseScore::tr("Choose Synthesizer SoundFont"),
               d,
               filter
               );
            return s;
            }

      if (loadSoundFontDialog == 0) {
            loadSoundFontDialog = new QFileDialog(this);
            loadSoundFontDialog->setFileMode(QFileDialog::ExistingFile);
            loadSoundFontDialog->setOption(QFileDialog::DontUseNativeDialog, true);
            loadSoundFontDialog->setWindowTitle(tr("MuseScore: Choose Synthesizer SoundFont"));
            loadSoundFontDialog->setNameFilter(filter);
            loadSoundFontDialog->setDirectory(d);

            QSettings settings;
            loadSoundFontDialog->restoreState(settings.value("loadSoundFontDialog").toByteArray());
            loadSoundFontDialog->setAcceptMode(QFileDialog::AcceptOpen);
            }

      //
      // setup side bar urls
      //
      QFileInfo mySoundFonts(preferences.mySoundFontsPath);
      if (mySoundFonts.isRelative())
            mySoundFonts.setFile(QDir::home(), preferences.mySoundFontsPath);

      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(mySoundFonts.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));
      urls.append(QUrl::fromLocalFile(mscoreGlobalShare+"/sound"));
      loadSoundFontDialog->setSidebarUrls(urls);

      if (loadSoundFontDialog->exec()) {
            QStringList result = loadSoundFontDialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getChordStyleFilename
//---------------------------------------------------------

QString MuseScore::getChordStyleFilename(bool open)
      {
      QString filter = tr("MuseScore Chord Style File (*.xml)");
      if (open)
            filter.append(tr(";;All Files (*)"));

      QString currentPath = QDir::currentPath();
      if (preferences.nativeDialogs) {
            QString fn;
            if (open) {
                  fn = QFileDialog::getOpenFileName(
                     this, tr("MuseScore: Load Chord Style"),
                     QString(currentPath),
                     filter
                     );
                  }
            else {
                  fn = QFileDialog::getSaveFileName(
                     this, tr("MuseScore: Save Chord Style"),
                     QString(currentPath),
                     filter
                     );
                  }
            return fn;
            }

      QFileInfo myStyles(preferences.myStylesPath);
      if (myStyles.isRelative())
            myStyles.setFile(QDir::home(), preferences.myStylesPath);
      QFileDialog* dialog;
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(myStyles.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));

      QSettings settings;
      if (open) {
            if (loadChordStyleDialog == 0) {
                  loadChordStyleDialog = new QFileDialog(this);
                  loadChordStyleDialog->setFileMode(QFileDialog::ExistingFile);
                  loadChordStyleDialog->setOption(QFileDialog::DontUseNativeDialog, true);
                  loadChordStyleDialog->setWindowTitle(tr("MuseScore: Load Chord Style"));
                  loadChordStyleDialog->setNameFilter(filter);
                  loadChordStyleDialog->setDirectory(currentPath);

                  loadChordStyleDialog->restoreState(settings.value("loadChordStyleDialog").toByteArray());
                  loadChordStyleDialog->setAcceptMode(QFileDialog::AcceptOpen);
                  }
            // setup side bar urls
            urls.append(QUrl::fromLocalFile(mscoreGlobalShare+"/styles"));
            dialog = loadChordStyleDialog;
            }
      else {
            if (saveChordStyleDialog == 0) {
                  saveChordStyleDialog = new QFileDialog(this);
                  saveChordStyleDialog->setAcceptMode(QFileDialog::AcceptSave);
                  saveChordStyleDialog->setFileMode(QFileDialog::AnyFile);
                  saveChordStyleDialog->setOption(QFileDialog::DontConfirmOverwrite, false);
                  saveChordStyleDialog->setOption(QFileDialog::DontUseNativeDialog, true);
                  saveChordStyleDialog->setWindowTitle(tr("MuseScore: Save Style"));
                  saveChordStyleDialog->setNameFilter(filter);
                  saveChordStyleDialog->setDirectory(currentPath);

                  saveChordStyleDialog->restoreState(settings.value("saveChordStyleDialog").toByteArray());
                  saveChordStyleDialog->setAcceptMode(QFileDialog::AcceptSave);
                  }
            dialog = saveChordStyleDialog;
            }
      // setup side bar urls
      dialog->setSidebarUrls(urls);
      if (dialog->exec()) {
            QStringList result = dialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getScanFile
//---------------------------------------------------------

QString MuseScore::getScanFile(const QString& d)
      {
      QString filter = tr("PDF Scan File (*.pdf);;All (*)");

      if (preferences.nativeDialogs) {
            QString s = QFileDialog::getOpenFileName(
               mscore,
               MuseScore::tr("Choose PDF Scan"),
               d,
               filter
               );
            return s;
            }

      if (loadScanDialog == 0) {
            loadScanDialog = new QFileDialog(this);
            loadScanDialog->setFileMode(QFileDialog::ExistingFile);
            loadScanDialog->setOption(QFileDialog::DontUseNativeDialog, true);
            loadScanDialog->setWindowTitle(tr("MuseScore: Choose PDF Scan"));
            loadScanDialog->setNameFilter(filter);
            loadScanDialog->setDirectory(d);

            QSettings settings;
            loadScanDialog->restoreState(settings.value("loadScanDialog").toByteArray());
            loadScanDialog->setAcceptMode(QFileDialog::AcceptOpen);
            }

      //
      // setup side bar urls
      //
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));
      loadScanDialog->setSidebarUrls(urls);

      if (loadScanDialog->exec()) {
            QStringList result = loadScanDialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getAudioFile
//---------------------------------------------------------

QString MuseScore::getAudioFile(const QString& d)
      {
      QString filter = tr("OGG Audio File (*.ogg);;All (*)");

      if (preferences.nativeDialogs) {
            QString s = QFileDialog::getOpenFileName(
               mscore,
               MuseScore::tr("Choose Audio File"),
               d,
               filter
               );
            return s;
            }

      if (loadAudioDialog == 0) {
            loadAudioDialog = new QFileDialog(this);
            loadAudioDialog->setFileMode(QFileDialog::ExistingFile);
            loadAudioDialog->setOption(QFileDialog::DontUseNativeDialog, true);
            loadAudioDialog->setWindowTitle(tr("MuseScore: Choose OGG Audio File"));
            loadAudioDialog->setNameFilter(filter);
            loadAudioDialog->setDirectory(d);

            QSettings settings;
            loadAudioDialog->restoreState(settings.value("loadAudioDialog").toByteArray());
            loadAudioDialog->setAcceptMode(QFileDialog::AcceptOpen);
            }

      //
      // setup side bar urls
      //
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));
      loadAudioDialog->setSidebarUrls(urls);

      if (loadAudioDialog->exec()) {
            QStringList result = loadAudioDialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getFotoFilename
//---------------------------------------------------------

QString MuseScore::getFotoFilename()
      {
      QString filter =
         tr("PNG Bitmap Graphic (*.png);;")+
         tr("PDF File (*.pdf);;")+
         tr("Encapsulated PostScript File (*.eps);;")+
         tr("Scalable Vector Graphic (*.svg);;");

      QString title       = tr("MuseScore: Save Image");
      QString currentPath = QDir::currentPath();
      if (preferences.nativeDialogs) {
            QString fn;
            fn = QFileDialog::getSaveFileName(
               this,
               title,
               currentPath,
               filter
               );
            return fn;
            }

      QFileInfo myImages(preferences.myImagesPath);
      if (myImages.isRelative())
            myImages.setFile(QDir::home(), preferences.myImagesPath);
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(myImages.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));

      if (saveImageDialog == 0) {
            saveImageDialog = new QFileDialog(this);
            saveImageDialog->setFileMode(QFileDialog::AnyFile);
            saveImageDialog->setAcceptMode(QFileDialog::AcceptSave);
            saveImageDialog->setOption(QFileDialog::DontConfirmOverwrite, false);
            saveImageDialog->setOption(QFileDialog::DontUseNativeDialog, true);
            saveImageDialog->setWindowTitle(title);
            saveImageDialog->setNameFilter(filter);
            saveImageDialog->setDirectory(currentPath);

            QSettings settings;
            saveImageDialog->restoreState(settings.value("saveImageDialog").toByteArray());
            saveImageDialog->setAcceptMode(QFileDialog::AcceptSave);
            }

      // setup side bar urls
      saveImageDialog->setSidebarUrls(urls);

      if (saveImageDialog->exec()) {
            QStringList result = saveImageDialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

//---------------------------------------------------------
//   getDrumsetFilename
//---------------------------------------------------------

QString MuseScore::getDrumsetFilename(bool open)
      {
      QString title;
      QString filter;
      if (open) {
            title  = tr("MuseScore: Load Drumset");
            filter = tr("MuseScore Drumset (*.drm);;All Files (*)");
            }
      else {
            title  = tr("MuseScore: Save Drumset");
            filter = tr("MuseScore Drumset File (*.drm)");
            }

      QString currentPath(QDir::currentPath());
      if (preferences.nativeDialogs) {
            QString fn;
            if (open)
                  fn = QFileDialog::getOpenFileName(this, title, currentPath, filter);
            else
                  fn = QFileDialog::getSaveFileName(this, title, currentPath, filter);
            return fn;
            }

      QFileInfo myStyles(preferences.myStylesPath);
      if (myStyles.isRelative())
            myStyles.setFile(QDir::home(), preferences.myStylesPath);
      QFileDialog* dialog;
      QList<QUrl> urls;
      QString home = QDir::homePath();
      urls.append(QUrl::fromLocalFile(home));
      urls.append(QUrl::fromLocalFile(myStyles.absoluteFilePath()));
      urls.append(QUrl::fromLocalFile(QDir::currentPath()));

      if (open) {
            if (loadDrumsetDialog == 0) {
                  loadDrumsetDialog = new QFileDialog(this);
                  loadDrumsetDialog->setFileMode(QFileDialog::ExistingFile);
                  loadDrumsetDialog->setOption(QFileDialog::DontUseNativeDialog, true);
                  saveDrumsetDialog->setDirectory(currentPath);

                  QSettings settings;
                  loadDrumsetDialog->restoreState(settings.value("loadDrumsetDialog").toByteArray());
                  loadDrumsetDialog->setAcceptMode(QFileDialog::AcceptOpen);
                  }
            urls.append(QUrl::fromLocalFile(mscoreGlobalShare+"/styles"));
            dialog = loadDrumsetDialog;
            }
      else {
            if (saveDrumsetDialog == 0) {
                  saveDrumsetDialog = new QFileDialog(this);
                  saveDrumsetDialog->setAcceptMode(QFileDialog::AcceptSave);
                  saveDrumsetDialog->setFileMode(QFileDialog::AnyFile);
                  saveDrumsetDialog->setOption(QFileDialog::DontConfirmOverwrite, false);
                  saveDrumsetDialog->setOption(QFileDialog::DontUseNativeDialog, true);
                  saveDrumsetDialog->setDirectory(currentPath);

                  QSettings settings;
                  saveDrumsetDialog->restoreState(settings.value("saveDrumsetDialog").toByteArray());
                  saveDrumsetDialog->setAcceptMode(QFileDialog::AcceptSave);
                  }
            dialog = saveDrumsetDialog;
            }
      dialog->setWindowTitle(title);
      dialog->setNameFilter(filter);

      // setup side bar urls
      dialog->setSidebarUrls(urls);

      if (dialog->exec()) {
            QStringList result = dialog->selectedFiles();
            return result.front();
            }
      return QString();
      }

