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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "lua/lua.hpp"

extern QTextStream         *plog;
extern QTextCodec          *codec;


MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  work_dir = "../trash/*";

  std::srand(QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0);
  ui->setupUi(this);
  
  ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                                  QCP::iSelectLegend | QCP::iSelectPlottables);
  ui->customPlot->xAxis->setRange(-8, 8);
  ui->customPlot->yAxis->setRange(-5, 5);
  ui->customPlot->axisRect()->setupFullAxesBox();
  
  ui->customPlot->plotLayout()->insertRow(0);
  QCPTextElement *title = new QCPTextElement(ui->customPlot, "<Waveform Name>", QFont("sans", 10, QFont::Bold));
  ui->customPlot->plotLayout()->addElement(0, 0, title);
  
  ui->customPlot->xAxis->setLabel("time, mS");
  ui->customPlot->yAxis->setLabel("Value, V");
  ui->customPlot->legend->setVisible(true);
  QFont legendFont = font();
  legendFont.setPointSize(10);
  ui->customPlot->legend->setFont(legendFont);
  ui->customPlot->legend->setSelectedFont(legendFont);
  ui->customPlot->legend->setSelectableParts(QCPLegend::spItems); // legend box shall not be selectable, only legend items

  // connect slot that ties some axis selections together (especially opposite axes):
  connect(ui->customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
  // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
  connect(ui->customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
  connect(ui->customPlot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));
  
  // make bottom and left axes transfer their ranges to top and right axes:
  connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
  connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));
  
  // connect some interaction slots:
  connect(ui->customPlot, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));
  connect(ui->customPlot, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));
  connect(title, SIGNAL(doubleClicked(QMouseEvent*)), this, SLOT(titleDoubleClick(QMouseEvent*)));
  
  // connect slot that shows a message in the status bar when a graph is clicked:
  connect(ui->customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));
  
  // setup policy and connect slot for context menu popup:
  ui->customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->customPlot, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));


  //connect(ui->pushButton, SIGNAL( clicked() ), this, SLOT(slot_printer(void)));


}


MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::titleDoubleClick(QMouseEvent* event)
{
  Q_UNUSED(event)
  if (QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()))
  {
    // Set the plot title by double clicking on it
    bool ok;
    QString newTitle = QInputDialog::getText(this, "CSV WaveForm", "New plot title:", QLineEdit::Normal, title->text(), &ok);
    if (ok)
    {
      title->setText(newTitle);
      ui->customPlot->replot();
    }
  }
}


void MainWindow::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
  // Set an axis label by double clicking on it
  if (part == QCPAxis::spAxisLabel) // only react when the actual axis label is clicked, not tick label or axis backbone
  {
    bool ok;
    QString newLabel = QInputDialog::getText(this, "CSV WaveForm", "New axis label:", QLineEdit::Normal, axis->label(), &ok);
    if (ok)
    {
      axis->setLabel(newLabel);
      ui->customPlot->replot();
    }
  }
}


void MainWindow::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
  // Rename a graph by double clicking on its legend item
  Q_UNUSED(legend)
  if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
  {
    QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
    bool ok;
    QString newName = QInputDialog::getText(this, "CSV WaveForm", "New graph name:", QLineEdit::Normal, plItem->plottable()->name(), &ok);
    if (ok)
    {
      plItem->plottable()->setName(newName);
      ui->customPlot->replot();
    }
  }
}


void MainWindow::selectionChanged()
{
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.
   
   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.
   
   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */
  
  // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      ui->customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    ui->customPlot->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    ui->customPlot->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      ui->customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || ui->customPlot->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    ui->customPlot->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    ui->customPlot->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  
  // synchronize selection of graphs with selection of corresponding legend items:
  for (int i=0; i<ui->customPlot->graphCount(); ++i)
  {
    QCPGraph *graph = ui->customPlot->graph(i);
    QCPPlottableLegendItem *item = ui->customPlot->legend->itemWithPlottable(graph);
    if (item->selected() || graph->selected())
    {
      item->setSelected(true);
      graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
    }
  }
}


void MainWindow::mousePress()
{
  // if an axis is selected, only allow the direction of that axis to be dragged
  // if no axis is selected, both directions may be dragged
  
  if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->customPlot->axisRect()->setRangeDrag(ui->customPlot->xAxis->orientation());
  else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->customPlot->axisRect()->setRangeDrag(ui->customPlot->yAxis->orientation());
  else
    ui->customPlot->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}


void MainWindow::mouseWheel()
{
  // if an axis is selected, only allow the direction of that axis to be zoomed
  // if no axis is selected, both directions may be zoomed
  
  if (ui->customPlot->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->xAxis->orientation());
  else if (ui->customPlot->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    ui->customPlot->axisRect()->setRangeZoom(ui->customPlot->yAxis->orientation());
  else
    ui->customPlot->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
}


void MainWindow::addRandomGraph()
{
  *plog << "----------" << endl;

  lua_State *L = luaL_newstate();
  luaL_openlibs( L );

  int err = luaL_loadfile( L, "../etc/loadfile.lua" /*filename.toLocal8Bit().data()*/ );
  if ( err != LUA_OK )
  {
    QString err = codec->toUnicode("WARNING: ?????? ? ????? '") +
                  "../etc/loadfile.lua"/*filename*/ + QObject::tr("' :") +
                  codec->toUnicode( lua_tostring(L, -1) );
    *plog << err << endl;
    lua_close( L );
    return;
  }
  lua_pcall(L, 0, 0, 0);


//-------------------------------------------------------
  /* push functions and arguments */
  lua_getglobal(L, "file_supported");  /* function to be called */

  /* do the call (0 arguments, 1 result) */
  if (lua_pcall(L, 0, 1, 0) != 0)
  {
    QString err = "ERROR: running function 'file_supported()'";
    *plog << err << endl;
    lua_close( L );
    return;
  }
  /* retrieve result */
  if (!lua_isstring(L, -1))
  {
    QString err = "ERROR: function 'file_supported()' must return a string";
    *plog << err << endl;
    lua_close( L );
    return;
  }
  const char *c_filter = lua_tostring(L, -1);
  lua_pop(L, 1);  /* pop returned value */

  QString filter = QString::fromLocal8Bit(c_filter);


  QString fileName;
  fileName = QFileDialog::getOpenFileName(this,
     tr("Open Waveform"), work_dir, filter);


  if (fileName.isEmpty())
  {
    QString err = "ERROR: File name is empty";
    *plog << err << endl;
    lua_close( L );
    return;
  }

  *plog << fileName << endl;

  QFileInfo fi(fileName);
  work_dir = fi.absolutePath();



//----------------------------------------------
  /* push functions and arguments */
  lua_getglobal(L, "open");  /* function to be called */
  lua_pushstring(L, fileName.toStdString().c_str() );   /* push 1st argument */
  /* do the call (1 arguments, 2 result) */
  if (lua_pcall(L, 1, 2, 0) != 0)
  {
    QString err = "ERROR: running function 'open()'";
    *plog << err << endl;
    lua_close( L );
    return;
  }

  /* retrieve result */
  if (!lua_isinteger(L, 1))
  {    
    QString err = "ERROR: function 'open()' must return first argument is integer'";
    *plog << err << endl;
    lua_close( L );
    return;
  }
  int result = lua_tointeger(L, 1);

  if(result < 0)
  {
    QString err = "ERROR: function 'open()' return first argument value '"+ QString(result) + "' < 0";
    *plog << err << endl;
    lua_close( L );
    return;
  }

  if ( !lua_isinteger(L, 2) )
  {
    QString err = "ERROR: function 'open()' must return second argument is integer";
    *plog << err << endl;
    lua_close( L );
    return;
  }

  int columns_num = lua_tointeger(L, 2);
  lua_pop(L, 2);  /* pop returned value */

  *plog << "size = " << result << endl;
  *plog << "columns_num = " << columns_num << endl;

  if ( columns_num < 2 )
  {
    QString err = "ERROR: function 'open()' return second argument value '"+ QString(columns_num) + "' < 2";
    *plog << err << endl;
    return;
  }


  QVector<double> t, v[50];

  for (int col = 2; col <= columns_num; col++)
  {
    for (int i = 0; i < result; i++)
    {
      /* push functions and arguments */
      lua_getglobal(L, "get_record");  /* function to be called */
      lua_pushinteger(L, (i + 1));   /* push 1st argument */
      lua_pushinteger(L, col);   /* push 2st argument */
      /* do the call (1 arguments, 2 result) */
      if (lua_pcall(L, 2, 2, 0) != 0)
      {
        QString err = "ERROR: running function 'get_record()'";
        *plog << err << endl;
        lua_close( L );
        return;
      }

      /* retrieve result */
      if (!lua_isnumber(L, 1))
      {
        QString err = "ERROR: function 'get_record()' must return first argument is number";
        *plog << err << endl;
        lua_close( L );
        return;
      }

      t.append(lua_tonumber(L, 1));
      if (!lua_isnumber(L, 2))
      {
        QString err = "ERROR: function 'get_record()' must return second argument is number";
        *plog << err << endl;
        lua_close( L );
        return;
      }
      v[col].append(lua_tonumber(L, 2));
      lua_pop(L, 2);  /* pop returned value */
    }

    ui->customPlot->addGraph();
    ui->customPlot->graph()->setName(QString("New graph %1").arg(ui->customPlot->graphCount()-1));
    ui->customPlot->graph()->setData(t, v[col]);
    //ui->customPlot->graph()->rescaleAxes();
    //ui->customPlot->graph()->setLineStyle((QCPGraph::LineStyle)(std::rand()%5+1));
    if (std::rand()%100 > 50)
      ui->customPlot->graph()->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(std::rand()%14+1)));
    QPen graphPen;
    graphPen.setColor(QColor(std::rand()%245+10, std::rand()%245+10, std::rand()%245+10));
    graphPen.setWidthF(std::rand()/(double)RAND_MAX*2+1);
    ui->customPlot->graph()->setPen(graphPen);
  }
  lua_close( L );

  ui->customPlot->rescaleAxes();
  ui->customPlot->replot();
}


void MainWindow::on_actionOpen_triggered()
{
  addRandomGraph();
}


void MainWindow::on_actionPrint_triggered(void)
{
  QPrinter printer;

  double Form_x = this->width();
  double Form_y = this->height();

  double new_Form_x;
  double new_Form_y;

  double x = ui->customPlot->width();
  double y = ui->customPlot->height();

  double dx = Form_x - x;
  double dy = Form_y - y;

  if (x > y)
  {
    y = 0.707 * x;
    new_Form_x = Form_x;
    new_Form_y = y + dy;
  }
  else
  {
    x = 0.707 * y;
    new_Form_x = x + dx;
    new_Form_y = Form_y;
  }

  // ??? ???????????? ????????? ????? ? ???????????? ?????? 0.707
  this->resize(new_Form_x, new_Form_y);

  QPrintDialog printDialog(&printer, this);
  printDialog.setWindowTitle(tr("Print Document"));

  if (printDialog.exec() != QDialog::Accepted)
    return;

  printer.setOrientation(QPrinter::Landscape); //?????????? ?????????


  QPainter painter;
  painter.begin(&printer);
  double xscale = printer.pageRect().width()/double(ui->customPlot->width());
  double yscale = printer.pageRect().height()/double(ui->customPlot->height());
  double scale = qMin(xscale, yscale);
  painter.scale(scale, scale);
  ui->customPlot->render(&painter);
  painter.end();

  this->resize(Form_x, Form_y);
}


void MainWindow::removeSelectedGraph()
{
  if (ui->customPlot->selectedGraphs().size() > 0)
  {
    ui->customPlot->removeGraph(ui->customPlot->selectedGraphs().first());
    ui->customPlot->replot();
  }
}


void MainWindow::removeAllGraphs()
{
  ui->customPlot->clearGraphs();
  ui->customPlot->replot();
}


void MainWindow::contextMenuRequest(QPoint pos)
{
  QMenu *menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose);
  
  if (ui->customPlot->legend->selectTest(pos, false) >= 0) // context menu on legend requested
  {
    menu->addAction("Move to top left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignLeft));
    menu->addAction("Move to top center", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignHCenter));
    menu->addAction("Move to top right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignRight));
    menu->addAction("Move to bottom right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignRight));
    menu->addAction("Move to bottom left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignLeft));
  } else  // general context menu on graphs requested
  {
    menu->addAction("Add graph", this, SLOT(addRandomGraph()));
    if (ui->customPlot->selectedGraphs().size() > 0)
      menu->addAction("Remove selected graph", this, SLOT(removeSelectedGraph()));
    if (ui->customPlot->graphCount() > 0)
      menu->addAction("Remove all graphs", this, SLOT(removeAllGraphs()));
  }
  
  menu->popup(ui->customPlot->mapToGlobal(pos));
}


void MainWindow::moveLegend()
{
  if (QAction* contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
  {
    bool ok;
    int dataInt = contextAction->data().toInt(&ok);
    if (ok)
    {
      ui->customPlot->axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)dataInt);
      ui->customPlot->replot();
    }
  }
}


void MainWindow::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
  // since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
  // usually it's better to first check whether interface1D() returns non-zero, and only then use it.
  double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
  QString message = QString("Clicked on graph '%1' at data point #%2 with value %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
  ui->statusBar->showMessage(message, 2500);
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this,
                       "CSV WaveForm",
                       "<h2><b>CSV WaveForm</b></h2>"
                       "<p><i>version " CSV_WAVEFORM_VERSION "</i></p>"
                       "<p>CSV WaveForm is a GUI for visualization of timing diagrams from CSV - files. It is licensed under the "
                       "<a href=\"https://www.gnu.org/licenses/gpl-3.0.en.html\">GNU General "
                       "Public License, version 3.0</a>.</p>"
                       "<p>For more info and updates, visit: "
                       "<a href=\"https://github.com/yrasik/csv_waveform\">https://github.com/yrasik/csv_waveform</a></p>");
}


void MainWindow::on_actionUsage_triggered()
{
  QMessageBox::about(this,
                 "Usage",
                 "<p><b>Select the axes</b> to drag and zoom them individually;</p>"
                 "<p><b>Double click</b> labels or legend items to set user specified strings;</p>"
                 "<p><b>Left click</b> on graphs or legend to select graphs;</p>"
                 "<p><b>Right click</b> for a popup menu to add/remove graphs and move the legend.</p>");
}


void MainWindow::on_actionExit_triggered()
{
  QApplication::quit();
}
