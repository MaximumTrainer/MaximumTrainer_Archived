#include "powercurve.h"
#include <QDebug>
#include "myconstants.h"

PowerCurve::PowerCurve(){

    this->id = 0;

    this->formulaInCode = false;
    this->formulaType = PowerCurve::IN_DB;

    this->riderWeightKg = 70.0;
    this->riderWeightLbs = this->riderWeightKg*constants::GLOBAL_CONST_CONVERT_KG_TO_LBS;

    this->companyName = "-";
    this->trainerName = "TrainerNotSet";

    this->coef0 = 0.0;
    this->coef1 = 0.0;
    this->coef2 = 0.0;
    this->coef3 = 0.0;
}




//-------------------------------------------------------------------------------------------------------
QString PowerCurve::getFullName() const {

    return (this->companyName + " - " + this->trainerName);
}

//-------------------------------------------------------------------------------------------------------
void PowerCurve::setId(int id) {
    this->id = id;
}
int PowerCurve::getId() const {
    return this->id;
}

//-------------------------------------------------------------------------------------------------------
void PowerCurve::setName(QString companyName, QString trainerName) {

    this->companyName = companyName;
    this->trainerName = trainerName;
}

//-------------------------------------------------------------------------------------------------------
void PowerCurve::setCoefs(double coef0, double coef1, double coef2, double coef3) {

    this->coef0 = coef0;
    this->coef1 = coef1;
    this->coef2 = coef2;
    this->coef3 = coef3;

    qDebug() << "SET COEF! coef0:" << coef0 << "coef1:" << coef1 << "coef2:" << coef2 << "coef3:" << coef3;
}

//-------------------------------------------------------------------------------------------------------
void PowerCurve::setFormulaInCode(bool formulaInCode) {

    if (formulaInCode) {
        this->formulaInCode = true;
        this->formulaType = static_cast<PowerCurve::TRAINER_FORMULA>(this->id); //ID From DB == ENUM ID
    }
    else {
        this->formulaInCode = false;
        this->formulaType = PowerCurve::IN_DB;
    }

    qDebug() << "setFormulaInCode" << this->formulaInCode << "FormulaTypeEnum" << this->formulaType;

}


//-------------------------------------------------------------------------------------------------------
void PowerCurve::setRiderWeightKg(double weight) {

    this->riderWeightKg = weight;
    this->riderWeightLbs = weight * constants::GLOBAL_CONST_CONVERT_KG_TO_LBS;

    qDebug() << "Setting rider weight is" << riderWeightKg << "AND:" << riderWeightLbs;

}



/// Power (Watts) = (coef0* Speed^0) + (coef1* Speed^1)  + (coef2* Speed^2)  + (coef3* Speed^3)
//-------------------------------------------------------------------------------------------------------
double PowerCurve::calculatePower(double speedKMH) const {

    //    qDebug() << "CalculatePower with coef0:" << this->coef0 << " coef1" << this->coef1 << " coef2" << this->coef2 << " coef3"  << this->coef3;

    if (speedKMH <= 0 )
        return 0;

    /// Formula insideBD
    else if (!formulaInCode) {
        double speedMPH = kmhToMiles(speedKMH);
        return (coef0 + (coef1* speedMPH) + (coef2 * (speedMPH*speedMPH) ) + (coef3 * (speedMPH*speedMPH*speedMPH) ) );
    }

    /// Formula insideCode
    else {
        double speedMPH = kmhToMiles(speedKMH);
        return calculatePowerHelper(speedMPH, formulaType);
    }



}


//-------------------------------------------------------------------------------------------------------
double PowerCurve::calculatePowerHelper(double speedMPH, TRAINER_FORMULA formula) const {

    qDebug() << "calculateFormulaInsideCode";

    switch(formula)
    {
    // INSIDE E-RIDE ROLLERS
    //        MAG0=(SpeedMPH*14.04-33.6)-(((Speed*14.04-33.06)-(Speed*8.75-16.21))/90)*(220-RweightLBS)
    //        MAG1=MAG0 + (SPPEDMPH*2.57353) -9.60294
    //        MAG2=MAG0 + (SPPEDMPH*7.13235) -29.98529
    //        MAG3=MAG0 + (SPPEDMPH*10.95588) -13.33824
    case E_MOTION_MAG0:
    {
        return (speedMPH*14.04-33.6)-(((speedMPH*14.04-33.06)-(speedMPH*8.75-16.21))/90.0)*(220-riderWeightLbs);
        break;
    }
    case E_MOTION_MAG1:
    {
        double mag0 = calculatePowerHelper(speedMPH, PowerCurve::E_MOTION_MAG0);
        return mag0 + (speedMPH*2.57353) -9.60294;
        break;
    }
    case E_MOTION_MAG2:
    {
        double mag0 = calculatePowerHelper(speedMPH, PowerCurve::E_MOTION_MAG0);
        return mag0 + (speedMPH*7.13235) -29.98529;
        break;
    }
    case E_MOTION_MAG3:
    {
        double mag0 = calculatePowerHelper(speedMPH, PowerCurve::E_MOTION_MAG0);
        return mag0 + (speedMPH*10.95588) -13.33824;
        break;
    }
    default:
    {
        return 666.6; //evil, should never get here :)
        break;
    }
    }


}


//-------------------------------------------------------------------------------------------------------
double PowerCurve::kmhToMiles(double speedKMH) const {

    return speedKMH * constants::GLOBAL_CONST_CONVERT_KMH_TO_MILES;
}






