#include "mydialbox.h"
#include "qwt_dial_needle.h"
#include "util.h"


#include <QtGlobal>
#include <QVBoxLayout>
#include <QColor>




MyDialBox::MyDialBox(QWidget *parent): QWidget( parent ) {

    createDial();

    target = 0;

    gridLayout = new QGridLayout(this);
    d_label_icon = new QLabel(this);
    d_label_current_value = new QLabel(this);
    d_label_icon->setMinimumSize(35,35);
    d_dial->setMinimumSize(200,200);


    QFont fontValue;
    fontValue.setPointSize(16);
    d_label_current_value->setFont(fontValue);
    d_label_current_value->setText("0");


    gridLayout->setMargin(0);
    gridLayout->setRowMinimumHeight(0, 10);
    gridLayout->setColumnMinimumWidth(1, 5);
    gridLayout->setColumnMinimumWidth(3, 25);


    gridLayout->addWidget(d_label_icon, 1, 0, 1, 1, Qt::AlignRight);
    gridLayout->addWidget(d_label_current_value, 1, 2, 1, 1, Qt::AlignLeft);
    gridLayout->addWidget(d_dial, 2, 0, 1, 4, Qt::AlignHCenter);


    this->setLayout(gridLayout);



}




//////////////////////////////////////////////////////////////////////////////////////////////
void MyDialBox::createDial()  {

    d_dial = new myQwtDial(this);

    d_dial->setScale( target-40, target+40 );


    QColor needleColor( QColor( "Goldenrod" ) );


    QwtDialSimpleNeedle *needle = new QwtDialSimpleNeedle(
                QwtDialSimpleNeedle::Arrow, true, needleColor,
                QColor( Qt::gray ).light( 130 ) );
    d_dial->setNeedle( needle );

    ////////////////////////////////////////////////////////////
    ///COLOR
    ////////////////////////////////////////////////////////////
    QColor fillColor25 = Util::getColor(Util::SQUARE_POWER);
    QColor niceBlue = Util::getColor(Util::TOO_LOW);
    QColor brownColor = Util::getColor(Util::TOO_HIGH);
    const QColor base( QColor( Qt::darkGray ).dark( 150 ) );


    paletteGood.setColor( QPalette::Base, base );
    paletteGood.setColor( QPalette::Window, base.dark( 150 ) );
    paletteGood.setColor( QPalette::Mid, base.dark( 110 ) );
    //    paletteGood.setColor( QPalette::Light, base.light( 170 ) );
    paletteGood.setColor( QPalette::Light, fillColor25 );
    paletteGood.setColor( QPalette::Dark, base.dark( 170 ) );
    paletteGood.setColor( QPalette::Text, base.dark( 200 ).light( 800 ) );

    paletteToLow = QPalette(paletteGood);
    paletteToLow.setColor( QPalette::Light, niceBlue );
    paleteToHigh= QPalette(paletteGood);
    paleteToHigh.setColor( QPalette::Light, brownColor );

    d_dial->setPalette( paletteToLow );
    d_dial->setLineWidth( 6 );
    d_dial->setFrameShadow( QwtDial::Sunken );



}


//////////////////////////////////////////////////////////////////////////////////////////////
void MyDialBox::setValue( int v ) {

    d_dial->setValue(v);
    int diff = v - target;

    //    qDebug() << QString::number(diff);

    /// Set label Text
    QString valueTxt;
    //    qDebug() << valueTxt;
    if (diff>0) {
        valueTxt = QString(" (+%1)").arg(diff);
    }
    else if (diff<0) {
        valueTxt = QString(" (%1)").arg(diff);
    }
    d_label_current_value->setText(QString::number(v) + valueTxt);



    QColor myFontColor;
    /// Change color
    if (diff < -25) {
        d_dial->setPalette( paletteToLow );
        myFontColor = Util::getColor(Util::TOO_LOW);
    }
    else if(diff > 25) {
        d_dial->setPalette( paleteToHigh );
        myFontColor = Util::getColor(Util::TOO_HIGH);
    }
    else {
        d_dial->setPalette( paletteGood );
        myFontColor = Util::getColor(Util::ON_TARGET);
    }


    QString style = "color: rgb(%1, %2, %3);";
    d_label_current_value->setStyleSheet(style.arg(myFontColor.red()).arg(myFontColor.green()).arg(myFontColor.blue()));

}



///////////////////////////////////////////////////////////////////////////////////////////////
void MyDialBox::setTypeSpeedo(QString type) {

    if (type == "power") {
        d_dial->setLabelText(tr("watts"));
        d_label_current_value->setObjectName("d_label_current_valuePower");
        d_label_icon->setObjectName("d_label_power");
        d_label_icon->setStyleSheet("image: url(:/image/icon/power2)");
        d_dial->setObjectName( "MyQwtDialPower" );
    }
    else if (type == "hr") {
        d_dial->setLabelText(tr("bpm"));
        d_label_current_value->setObjectName("d_label_current_valueHr");
        d_label_icon->setObjectName("d_label_hr");
        d_label_icon->setStyleSheet("image: url(:/image/icon/heart2)");
        d_dial->setObjectName( "MyQwtDialHr" );
    }
    else if (type == "cad") {
        d_dial->setLabelText(tr("rpm"));
        d_label_current_value->setObjectName("d_label_current_valueCad");
        d_label_icon->setObjectName("d_label_cad");
        d_label_icon->setStyleSheet("image: url(:/image/icon/crank2)");
        d_dial->setObjectName( "MyQwtDialCad" );
    }
    else {
        d_dial->setLabelText(tr("label_"));
        d_label_current_value->setObjectName("d_label_current_value");
        d_label_icon->setObjectName("d_label__");
        d_label_icon->setStyleSheet("image: url(:/image/icon/heart2)");
        d_dial->setObjectName( "MyQwtDial__" );
    }

}



//////////////////////////////////////////////////////////////////////////////////////////////
void MyDialBox::targetChanged( int target ) {

    //    qDebug() << "MyDialBox::targetChanged, value:" << target;

    this->target = target;
    d_dial->setScale( target-40, target+40 );



}

