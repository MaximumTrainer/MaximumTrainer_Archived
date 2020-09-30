#ifndef POWERCURVE_H
#define POWERCURVE_H

#include <QtCore>
#include <QString>

class PowerCurve
{
public:

    ///Trainer with formulas not inside DB,
    /// keep enum value = ID database
    enum TRAINER_FORMULA
    {
        IN_DB = 0,
        E_MOTION_MAG0 = 243,
        E_MOTION_MAG1 = 244,
        E_MOTION_MAG2 = 245,
        E_MOTION_MAG3 = 246,
    };


    PowerCurve();


    void setId(int id);
    void setName(QString companyName, QString trainerName);
    void setCoefs(double coef0, double coef1, double coef2, double coef3);
    void setFormulaInCode(bool formulaInCode);
    void setRiderWeightKg(double weightKg);

    double calculatePower(double speedKMH) const;


    int getId() const;
    QString getFullName() const;



//-------------------------------------------------------------------------------------------------------------
private :
    double kmhToMiles(double speedKMH) const;
    double calculatePowerHelper(double speedMPH, TRAINER_FORMULA modelFormula) const;


private :
    int id;


    bool formulaInCode;
    TRAINER_FORMULA formulaType;
    double riderWeightKg;
    double riderWeightLbs;


    QString companyName;
    QString trainerName;

    double coef0;
    double coef1;
    double coef2;
    double coef3;



};
Q_DECLARE_METATYPE(PowerCurve)

#endif // POWERCURVE_H
