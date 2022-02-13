/*
 * This file is part of the "CSV WaveForm" distribution
 * (https://github.com/yrasik/csv_waveform).
 * Copyright (c) 2022 Yuri Stepanenko.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include "mainwindow.h"

#include <iostream>
#include <string>


QTextStream         *plog;
QTextCodec          *codec;



int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  MainWindow w;
  QFile               *log_file;

  codec = QTextCodec::codecForName( "Windows-1251" );
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("Windows-1251"));

  log_file = new QFile( "../log/csv_waveform.log" );
  if ( !log_file->open(QIODevice::WriteOnly | QIODevice::Text) )
  {
    std::cout << "ERROR:   " << "Error creating log file: '" << "../log/csv_waveform.log" << "'" << std::endl;
    return -1;
  }

  QTextStream log ( log_file );
  log.setCodec( "UTF-8" );
  plog = &log;






  w.show();
  
  return a.exec();
}
