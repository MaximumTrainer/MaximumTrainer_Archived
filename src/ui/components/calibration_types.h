#ifndef CALIBRATION_TYPES_H
#define CALIBRATION_TYPES_H

/*
 * Standalone calibration type definitions.
 *
 * Previously defined in protocol-specific headers (BPS_pages.h, fec_controller.h).
 * Extracted here so the UI layer has no hardware-protocol dependency.
 */

// Power-meter calibration request type (used by TopMenuWorkout)
typedef enum
{
    CALIBRATION_TYPE_MANUAL,
    CALIBRATION_TYPE_AUTO,
    CALIBRATION_TYPE_SLOPE,
    CALIBRATION_TYPE_SERIAL
} CalibrationType;

// FEC trainer calibration enums (used by DialogCalibrate)
namespace FEC_Controller {

    enum CALIBRATION_TYPE
    {
        ZERO_OFFSET,
        SPIN_DOWN,
    };

    enum TEMPERATURE_CONDITION
    {
        TEMP_NOT_APPLICABLE,
        TEMP_TOO_LOW,
        TEMP_OK,
        TEMP_TOO_HIGH,
    };

    enum SPEED_CONDITION
    {
        SPEED_NOT_APPLICABLE,
        SPEED_TOO_LOW,
        SPEED_OK,
        RESERVED,
    };

} // namespace FEC_Controller

#endif // CALIBRATION_TYPES_H
