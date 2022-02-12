#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "lua/lua.hpp"

extern QTextStream         *plog;
extern QTextCodec          *codec;


MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
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

  /*
  addRandomGraph();
  addRandomGraph();
  addRandomGraph();
  addRandomGraph();
  ui->customPlot->rescaleAxes();
  */

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


void MainWindow::slot_printer(void)
{
     QPrinter printer;

    QPrintDialog printDialog(&printer, this);
    printDialog.setWindowTitle(tr("Print Document"));

    if (printDialog.exec() != QDialog::Accepted)
        return;

    QPixmap pixmap = QPixmap::grabWidget(ui->customPlot, 0, 0, -1, -1);
    QPainter painter;
    painter.begin(&printer);
    painter.drawImage(0, 0, pixmap.toImage());
    painter.end();




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
  lua_State *L = luaL_newstate();
  luaL_openlibs( L );

  int err = luaL_loadfile( L, "../etc/loadfile.lua" /*filename.toLocal8Bit().data()*/ );
  if ( err != LUA_OK )
  {
    QString err = codec->toUnicode("WARNING: ������ � ����� '") +
                  "../etc/loadfile.lua"/*filename*/ + QObject::tr("' :") +
                  codec->toUnicode( lua_tostring(L, -1) );
    *plog << err << endl;
    lua_close( L );
    return;
  }
  lua_pcall(L, 0, 0, 0);

  /* push functions and arguments */
  lua_getglobal(L, "file_supported");  /* function to be called */

  /* do the call (0 arguments, 1 result) */
  if (lua_pcall(L, 0, 1, 0) != 0)
    luaL_error(L, "error running function `f': %s", lua_tostring(L, -1));

  /* retrieve result */
  if (!lua_isstring(L, -1))
    luaL_error(L, "function `f' must return a number");
  const char *c_filter = lua_tostring(L, -1);
  lua_pop(L, 1);  /* pop returned value */

  QString filter = QString::fromLocal8Bit(c_filter);

/*
  lua_getglobal(L, "x_Format_mm");
  if( lua_type(L, -1) == LUA_TNUMBER )
    x_Format_mm = (float)(lua_tonumberx) (L, -1, NULL);
*/

  QString fileName;
  fileName = QFileDialog::getOpenFileName(this,
     tr("Open Waveform"), "../trash/*", filter);


  if (fileName.isEmpty())
  {
    lua_close( L );
    return;
  }

/*
    QFile file(fileName);
    if ( !file.open(QIODevice::ReadOnly))
    {
       QMessageBox::information(this, tr("Unable to open file"),
       file.errorString());
       return;
    }
*/
*plog << fileName << endl;
  /* push functions and arguments */
  lua_getglobal(L, "open");  /* function to be called */
  lua_pushstring(L, fileName.toStdString().c_str() );   /* push 1st argument */
  /* do the call (1 arguments, 2 result) */
  if (lua_pcall(L, 1, 2, 0) != 0)
    luaL_error(L, "error running function `f': %s", lua_tostring(L, -1));


  /* retrieve result */
  if (!lua_isinteger(L, 1))
    luaL_error(L, "function `f' must return a integer");
  int result = lua_tointeger(L, 1);

  if(result < 0)
  {
    /* retrieve result */
    if (!lua_isstring(L, 2))
      luaL_error(L, "function `f' must return a number");
   // const char *c_filter = lua_tostring(L, -1);
    lua_pop(L, 2);  /* pop returned value */
    lua_close( L );
    return;
  }

  if (!lua_isinteger(L, 2))
    luaL_error(L, "function `f' must return a integer");
  int size = lua_tointeger(L, 2);
  lua_pop(L, 2);  /* pop returned value */

*plog << size << endl;

  QVector<double> t, v1;
  for (int i = 0; i < size; i++)
  {
    /* push functions and arguments */
    lua_getglobal(L, "get_record");  /* function to be called */
    lua_pushinteger(L, (i + 1));   /* push 1st argument */
    /* do the call (1 arguments, 2 result) */
    if (lua_pcall(L, 1, 2, 0) != 0)
      luaL_error(L, "error running function `f': %s", lua_tostring(L, -1));

    /* retrieve result */
    if (!lua_isnumber(L, 1))
      luaL_error(L, "function `f' must return a integer");
    t.append(lua_tonumber(L, 1));

    if (!lua_isnumber(L, 2))
      luaL_error(L, "function `f' must return a integer");
    v1.append(lua_tonumber(L, 2));
    lua_pop(L, 2);  /* pop returned value */

//    x[i] = (i/(double)n-0.5)*10.0*xScale + xOffset;
//    y[i] = (qSin(x[i]*r1*5)*qSin(qCos(x[i]*r2)*r4*3)+r3*qCos(qSin(x[i])*r4*2))*yScale + yOffset;
  }



  ui->customPlot->addGraph();
  ui->customPlot->graph()->setName(QString("New graph %1").arg(ui->customPlot->graphCount()-1));
  ui->customPlot->graph()->setData(t, v1);
 // ui->customPlot->graph()->setLineStyle((QCPGraph::LineStyle)(std::rand()%5+1));
  if (std::rand()%100 > 50)
    ui->customPlot->graph()->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(std::rand()%14+1)));
  QPen graphPen;
  graphPen.setColor(QColor(std::rand()%245+10, std::rand()%245+10, std::rand()%245+10));
  graphPen.setWidthF(std::rand()/(double)RAND_MAX*2+1);
  ui->customPlot->graph()->setPen(graphPen);
  ui->customPlot->replot();
}


void MainWindow::on_actionOpen_triggered()
{
  addRandomGraph();
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
    menu->addAction("Add random graph", this, SLOT(addRandomGraph()));
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
