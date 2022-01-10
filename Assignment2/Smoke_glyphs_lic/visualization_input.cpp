#include "visualization.h"

#include <QDebug>
#include <QMouseEvent>

void Visualization::mouseMoveEvent(QMouseEvent *ev)
{
    // The mx, my coordinates are given from the UI with (0,0) at top left.
    // Here (0,0) is converted to bottom left, as the Visualization requires it.
    int const mx = ev->x();
    int const my = height() - ev->y();

    drag(mx, my);
}
