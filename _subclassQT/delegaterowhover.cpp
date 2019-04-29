#include "delegaterowhover.h"

#include <QTableView>
#include <QDebug>
#include <QPainter>

#include "qwt_plot_renderer.h"
#include "interval.h"
#include "workout.h"
#include "sortfilterproxymodel.h"
#include "workouttablemodel.h"
#include "workoutplotlist.h"
#include "util.h"
#include "workoutplot.h"


delegateRowHover::delegateRowHover(bool hovered, QObject *parent) :QStyledItemDelegate(parent) {

    this->hovered = hovered;
}


void delegateRowHover::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    /// Keep trace of last edited workout when coming back to Workout List
    if (option.state & QStyle::State_HasFocus) {

         if (option.state & QStyle::State_Selected) {
             //do nothing;
         }
         else {
             painter->setPen(QColor(Qt::red));
             painter->drawRect(option.rect);
         }

//        QModelIndex leftIndex = index.model()->index(index.row(), 0, QModelIndex());
    }



    /// Name
    if (index.column() == 0) {
        opt.font.setBold(true);

#if defined(Q_OS_MAC)
        opt.font.setPointSize(12);
#else
        opt.font.setPointSize(10);
#endif
        QStyledItemDelegate::paint(painter, opt, index);
    }
    /// Graph, special painting to do
    else if(index.column() == 10)
    {


        QVariant variant = index.data(Qt::UserRole);;
        Workout workout;
        workout = variant.value<Workout>();


        WorkoutPlotList plot;
        plot.setWorkoutData(workout);

        QwtPlotRenderer plotRender;
        plotRender.setDiscardFlag(QwtPlotRenderer::DiscardBackground, true);
        plotRender.setDiscardFlag(QwtPlotRenderer::DiscardCanvasBackground, true);
        plotRender.setDiscardFlag(QwtPlotRenderer::DiscardCanvasFrame, true);
        plotRender.setDiscardFlag(QwtPlotRenderer::DiscardLegend,true);
        plotRender.setDiscardFlag(QwtPlotRenderer::DiscardTitle,true);
        plotRender.setDiscardFlag(QwtPlotRenderer::DiscardFooter,true);
        QStyledItemDelegate::paint(painter, option, index);  /// To get grey background when row select, etc.
        plotRender.render(&plot, painter, option.rect);



    }

    else {
        QStyledItemDelegate::paint(painter, opt, index);
    }








}
