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
//  the file LICENSE.GPL
//=============================================================================

import QtQuick 1.0
import MuseScore 1.0
import "mobile" as Mobile

Item {
      id: screen
      property bool inScoreView: false
      width: 1024
      height: 768

      states: [
            State {
                  name: "landscape"
                  when: (runtime.orientation == Orientation.Landscape)
                  PropertyChanges {
                        target: screen
                        }
                  PropertyChanges {
                        target: background
                        color: "blue"
                        }
                  },
            State {
                  name: "landscapeInverted"
                  when: (runtime.orientation == Orientation.LandscapeInverted)
                  PropertyChanges {
                        target: screen
                        }
                  PropertyChanges {
                        target: background
                        color: "red"
                        }
                  },
            State {
                  name: "portrait"
                  when: (runtime.orientation == Orientation.Portrait)
                  PropertyChanges {
                        target: screen
                        }
                  PropertyChanges {
                        target: background
                        color: "green"
                        }
                  },
            State {
                  name: "portraitInverted"
                  when: (runtime.orientation == Orientation.PortraitInverted)
                  PropertyChanges {
                        target: screen
                        }
                  PropertyChanges {
                        target: background
                        color: "yellow"
                        }
                  }
            ]
      Rectangle {
            id: background;
            anchors.fill: parent
            state: "myscores"

//            color: "#343434"
            Image {
                  source: "mobile/images/stripes.png"
                  fillMode: Image.Tile
                  anchors.fill: parent
                  opacity: 0.3
                  }

            states: State {
                  name: "ScoreView"
                  when: screen.inScoreView == true
                  PropertyChanges {
                        target: scoreViewFlick
                        x: 0
                        }
                  PropertyChanges {
                        target: scoreListView
//                        x: -(parent.width * 1)
                        x: -width;
                        }
                  PropertyChanges {
                        target: toolBar
                        button1Visible: true
                        button2Visible: true
                        }
                  }
            transitions: Transition {
                  NumberAnimation {
                        properties: "x"
                        easing.type: Easing.InOutQuad
                        duration: 500
                        }
                  }

            ListModel {
                  id: scorelist
                  ListElement {
                        title: "Promenade"
                        author: "Modeste Moussorgsky"
                        path: ":/scores/promenade.mscz"
                        imagePath: "qrc:///scores/promenade.png"
                        }
                  ListElement {
                        title: "Leise rieselt der Schnee"
                        author: "Traditional"
                        path: ":/scores/schnee.mscz"
                        imagePath: "qrc:///scores/schnee.png"
                        }
                  ListElement {
                        title: "Italienisches Konzert"
                        author: "J.S. Bach"
                        path: ":/scores/italian-1.mscz"
                        imagePath: "qrc:///scores/italian-1.png"
                        }
                  }

            Component {
                  id: scorelistdelegate
                  Text {
                        id: label
                        font.pixelSize: 18
                        text: type
                        }
                  }

            ListView {
                  id: scoreListView
                  anchors.top: titleBar.bottom
                  anchors.bottom: toolBar.top

                  width: parent.width
                  height: parent.height
                  model: scorelist
                  delegate: Mobile.ListDelegate { }

                  MouseArea {
                        anchors.fill: parent
                        onClicked: {
                              var idx = scoreListView.indexAt(mouseX, mouseY)
                              if (idx >= 0) {
                                    scoreView.setScore(scorelist.get(idx).path)
                                    screen.inScoreView = true
                                    scoreViewFlick.contentY = 0
                                    }
                              }
                        }
                  }

            Flickable {
                  id: scoreViewFlick
                  anchors.top: titleBar.bottom
                  width:  parent.width
                  height: parent.height
//                  x: -(parent.width * 1.5)
                  x: width

                  contentWidth:  scoreView.width
                  contentHeight: scoreView.height

                  ScoreView {
                        id: scoreView
                        parentWidth: screen.width
                        parentHeight: screen.height

                        MouseArea {
                              anchors.fill: parent
                              onClicked: {
                                    if (mouseX < width * .3)
                                          parent.prevPage()
                                    else if (mouseX > width * .6)
                                          parent.nextPage()
                                    }
                              }
                        }
                  }

            Mobile.TitleBar {
                  id: titleBar
                  height: 40
                  width: parent.width
                  opacity: .9
                  anchors.top: parent.top
                  }

            Mobile.ToolBar {
                  id: toolBar
                  height: 40;
                  width: parent.width;
                  opacity: .9
                  anchors.bottom: parent.bottom

                  button1Label: "MyScores"; button2Label: "Play"
                  onButton1Clicked: {
                        if (screen.inScoreView == true)
                              screen.inScoreView = false
                        else
                              screen.inScoreView = true
                        }
                  onButton2Clicked: scoreView.play();
                  button1Visible: false
                  button2Visible: false
                  }
            }
      }

