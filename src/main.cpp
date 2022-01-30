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
