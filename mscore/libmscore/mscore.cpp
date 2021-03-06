//=============================================================================
//  MuseScore
//  Music Composition & Notation
//  $Id:$
//
//  Copyright (C) 2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "config.h"
#include "style.h"
#include "mscore.h"

qreal PDPI;
qreal DPI;
qreal DPMM;

Style*  MScore::_defaultStyle;
Style*  MScore::_baseStyle;
QString MScore::_globalShare;
int     MScore::_vRaster;
int     MScore::_hRaster;
QColor  MScore::selectColor[VOICES];
QColor  MScore::defaultColor;
QColor  MScore::layoutBreakColor;
QColor  MScore::bgColor;
QColor  MScore::dropColor;
bool    MScore::warnPitchRange;

QPrinter::PageSize MScore::paperSize;
qreal   MScore::paperWidth;
qreal   MScore::paperHeight;
bool    MScore::landscape;
bool    MScore::twosided;
bool    MScore::replaceFractions;
bool    MScore::playRepeats;
bool    MScore::panPlayback;
qreal   MScore::nudgeStep;
int     MScore::defaultPlayDuration;
QString MScore::partStyle;
QString MScore::soundFont;
qreal   MScore::spatium;
QString MScore::lastError;
bool    MScore::layoutDebug = false;
int     MScore::division = 480;
int     MScore::sampleRate = 44100;
bool    MScore::debugMsg = false;
int     MScore::mtcType;

extern void initSymbols(int);
extern void initStaffTypes();
extern void initDrumset();

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MScore::init()
      {
      spatium = SPATIUM20;

#ifdef __MINGW32__
      QDir dir(QCoreApplication::applicationDirPath() + QString("/../" INSTALL_NAME));
      _globalShare = dir.absolutePath() + "/";
#else
#ifdef Q_WS_MAC
      QDir dir(QCoreApplication::applicationDirPath() + QString("/../Resources"));
      _globalShare = dir.absolutePath() + "/";
#else
      _globalShare = QString( INSTPREFIX "/share/" INSTALL_NAME);
#endif
#endif
      selectColor[0].setRgb(0, 0, 255);     //blue
      selectColor[1].setRgb(0, 150, 0);     //green
      selectColor[2].setRgb(230, 180, 50);  //yellow
      selectColor[3].setRgb(200, 0, 200);   //purple
      defaultColor        = Qt::black;
      dropColor           = Qt::red;
      nudgeStep           = .1;       // in spatium units (default 0.1)
      defaultPlayDuration = 300;      // ms
      warnPitchRange      = true;
      paperSize           = QPrinter::A4;     // default paper size
      paperWidth          = (210 / INCH);
      paperHeight         = (297 / INCH);
      landscape           = false;
      twosided            = true;
      replaceFractions    = true;
      partStyle           = "";
      layoutBreakColor    = Qt::green;
      soundFont           = _globalShare + "sound/TimGM6mb.sf2";
      bgColor.setRgb(0x76, 0x76, 0x6e);

      _defaultStyle         = new Style();
      setDefaultStyle(_defaultStyle);     // initialize default style
      _baseStyle            = new Style(*_defaultStyle);
      //
      //  load internal fonts
      //
      //
      // do not load application specific fonts
      // for MAC, they are in Resources/fonts
      //
#if !defined(Q_WS_MAC) && !defined(Q_WS_UIKIT)
      static const char* fonts[] = {
            "mscore-20.otf",
            "mscore1-20.ttf",
            "MuseJazz.ttf",
            "FreeSans.ttf",
            "FreeSerifMscore.ttf",
            "FreeSerifBold.ttf",
            "gonville-20.otf",
            "mscore_tab_baroque.ttf",
            "mscore_tab_modern.ttf",
            "mscore_tab_renaiss.ttf",
            "mscore_tab_renaiss2.ttf"
            };
      for (unsigned i = 0; i < sizeof(fonts)/sizeof(*fonts); ++i) {
            QString s = QString(":/fonts/%1").arg(fonts[i]);
            if (-1 == QFontDatabase::addApplicationFont(s)) {
                  fprintf(stderr, "Mscore: fatal error: cannot load internal font <%s>\n", fonts[i]);
                  if (!debugMode)
                        exit(-1);
                  }
            }
#endif
      initSymbols(0);   // init emmentaler symbols
      initStaffTypes();
      initDrumset();
      }

//---------------------------------------------------------
//   defaultStyle
//---------------------------------------------------------

Style* MScore::defaultStyle()
      {
      return _defaultStyle;
      }

//---------------------------------------------------------
//   baseStyle
//---------------------------------------------------------

Style* MScore::baseStyle()
      {
      return _baseStyle;
      }

