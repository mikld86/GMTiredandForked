#include "SimpleKalmanFilter/SimpleKalmanFilter.h"
#include "PressureController.h"
#include "HydraulicParameterEstimator/HydraulicParameterEstimator.h"
#include <algorithm>
#include <math.h>
// Helper function to return the sign of a float
inline float sign(float x) { return (x > 0.0f) - (x < 0.0f); }

PressureController::PressureController(float dt, float *rawSetpoint, float *sensorOutput, float *controllerOutput,
                                       int *ValveStatus) {
    this->_rawSetpoint = rawSetpoint;
    this->_rawPressure = sensorOutput;
    this->_ctrlOutput = controllerOutput;
    this->_ValveStatus = ValveStatus;
    this->_dt = dt;

    this->pressureKF = new SimpleKalmanFilter(0.1f, 10.0f, powf(4 * _dt, 2));
    this->_P_previous = *sensorOutput;
    this->R_estimator = new HydraulicParameterEstimator();
}

void PressureController::filterSetpoint() {
    if (!_filterInitialised)
        initSetpointFilter();
    float _wn = 2 * M_PI * _filtfreqHz;
    float d2r = (_wn * _wn) * (*_rawSetpoint - _r) - 2.0f * _filtxi * _wn * _dr;
    _dr += constrain(d2r * _dt,-_maxSpeedP, _maxSpeedP);
    _r += _dr * _dt;
}

void PressureController::initSetpointFilter(float val) {
    _r = *_rawSetpoint;
    if(val !=0.0f)
        _r = val;
    _dr = 0.0f;
    _filterInitialised = true;
}

void PressureController::setupSetpointFilter(float freq, float damping) {
    // Reset the filter if values have changed
    if (_filtxi != damping || _filtfreqHz != freq)
        initSetpointFilter();
    _filtfreqHz = freq;
    _filtxi = damping;
}

void PressureController::filterSensor() {
     _filteredPressureSensor = this->pressureKF->updateEstimate(*_rawPressure); 
}

void PressureController::tare() { coffeeOutput = 0.0; }

void PressureController::update() {
    if (*_ValveStatus == 1 && old_ValveStatus == 0)
        reset();
    old_ValveStatus = *_ValveStatus;

    filterSetpoint();
    filterSensor();
    computePumpDutyCycle();
    virtualScale();
}

float PressureController::computeAdustedCoffeeFlowRate(float pressure = 0.0f){
    // Output: flow (mL/s)
    // Hypothesis : 
    //  Low flow rate ==> laminar flow, darcy's law can apply. 
    //  High flow rate ==> partially turbulent flow calls for pressure exponent between 1 and 0.5

    if(pressure == 0.0f){
        pressure = _filteredPressureSensor;
    }

    // flow rate model parameters: 
    float Q1 = 1.0f; // Transition flow rate
    float n_max = 1.5; // Max pressure inverse exponent

    // Compute Darcy's flow rate
    float Qi = pressure/ R_estimator->getResistance() * 1e6f;

     float n = 1.0f;
     //If the function is used with cumulated pressure history( ie: blooming pressure dicreasing )
    if(pressure < 12.0f){// 
        // Use Darcy's flow estimation to compute the pressure exponent(empirical) value
        float n = 1+ (n_max-1)*Qi/(Qi+Q1); 
    }
    // Recompute the flow rate with compensation exponent
    return  pow(pressure,1/n) / R_estimator->getResistance() * 1e6f;
}

float PressureController::pumpFlowModel(float alpha = 100.0f){
    // Compute the instantaneous pump flow based on model :

    // Affine model based on Ulka nominal values chart
    //return alpha / 100.0f * _Q0 * (1 - _filteredPressureSensor / _Pmax);

    //Afine model base on one Gaggia Classic Pro Unit measurements 
    return alpha/100.0f* _Q1*_filteredPressureSensor + _Q0;
}

void PressureController::setPumpFlowCoeff(float oneBarFlow, float nineBarFlow ){
    // Set the affine pump flow model coefficients based on flow measurement at 1 bar and 9 bar
    _Q1 = (nineBarFlow-oneBarFlow)/8;
    _Q0 = oneBarFlow-_Q1*1.0f;
}

void PressureController::virtualScale() {

    // Estimate pump output flow 
    pumpFlowRate = pumpFlowModel(*_ctrlOutput);

    // Update puck resistance estimation: 
    float R = this->R_estimator->getResistance();

    // Condition for starting coffee flow estimation: 
    // Option 0: Pressure is establised and estimation is running
    bool isPpressurized = this->R_estimator->update(pumpFlowRate, _filteredPressureSensor);

    // Option 1: R is greater than the lower bound established on possible R value
    // Option 2: Trace of covariance matrix is small enough
    // Option 3: Derivative of R is small enough 
    bool isRconverged =  R > R_estimator->getResistanceMinBound() && R_estimator->hasConverged() ;
    
    // Option 4: Setpoint has been reached pressure is stable 
    bool isStable = _filteredPressureSensor/_r < 0.93f;

    // Trigger for the estimation flow output
    if (isRconverged && isStable){
        estimationHasConvergedOnce = true;
    }

    // Flow estimation : 
    if (isPpressurized  && estimationHasConvergedOnce ) { 
        flowPerSecond = computeAdustedCoffeeFlowRate();
        if(retroCoffeeOutputPressureHistory!=0){
            // Some coffee might have dripped before flow estimation occured, we need to account for that for the predictive scale
            coffeeOutput += (computeAdustedCoffeeFlowRate(retroCoffeeOutputPressureHistory)+flowPerSecond)* _dt;
            retroCoffeeOutputPressureHistory = 0.0f;
        }
        coffeeOutput += flowPerSecond * _dt;
    } else if (*_rawSetpoint !=0 ){// Shot just started (no pressure yet, no R converge but setpoint not 0)
        retroCoffeeOutputPressureHistory += _filteredPressureSensor;        
    }else if( estimationHasConvergedOnce){ // We're in a low pressure profil phase but we know R ->we can compute flow rate
        flowPerSecond = computeAdustedCoffeeFlowRate();
    }
    ESP_LOGI("","R:%.2e\tC:%.2e\tTrace:%.2e\tdRdt:%.2e\tFlow:%1.2f,Coffee:%1.2f",
            this->R_estimator->getResistance(),this->R_estimator->getCompliance(),this->R_estimator->hasConverged(),this->R_estimator->getFilteredResistanceDerivative(),
            flowPerSecond,coffeeOutput);
}

void PressureController::computePumpDutyCycle() {
    
    // BOILER NOT PRESSURISED : Do not start control before the boiler is filled up. 
    // Threshold value needs to be as low as possible while escaping disturbance surge or pressure from the pump
    if(_filteredPressureSensor < 0.5 && *_rawSetpoint !=0){
        reset();
        *_ctrlOutput = 100.0f;
        return;
    }
    // COMMANDE IS ACTUALLY ZERO: The profil is asking for no pressure (ex: blooming phase)
    // Until otherwise, make the controller ready to start as if it is a new shot comming
    // Do not reset the estimation of R since the estimation has to converge still
    if(*_rawSetpoint ==0.0f){
        initSetpointFilter();
         _errorInteg = 0.0f;
        *_ctrlOutput = 0.0f;
        _P_previous = 0.0f;
        _dP_previous = 0.0f;
        return;
    }

    // CONTROL: The boiler is pressurise, the profil is something specific, let's try to 
    // control that pressure now that all conditions are reunited
    float P = _filteredPressureSensor;
    float P_ref = _r;
    float dP_ref = _dr;

    float error = P - P_ref;
    float dP_actual = (P - _P_previous) / _dt;
    _dP_previous = dP_actual;
    float error_dot = dP_actual - dP_ref;
    _P_previous = P;

    // Switching surface
    _epsilon = 0.15*_r;
    deadband = 0.1*_r;
    float s = _lambda * error +  error_dot*0.1;
    float sat_s=0.0f;
    if(error >0){
        float tan = tanhf(s / _epsilon - deadband * _lambda / _epsilon);
        sat_s = std::max(0.0f, tan);
    }else if(error<0){
        float tan = tanhf(s / _epsilon + deadband * _lambda / _epsilon);
        sat_s = std::min(0.0f,tan);
    }
    
    // Integrator
    float Ki = _Ki/ (1-P/_Pmax);
    _errorInteg += error * _dt;
    float iterm = Ki * _errorInteg;

    float Qa = pumpFlowModel();
    float K = _K/ (1-P/_Pmax) *  Qa/_Co; 
    alpha = _Co  / Qa * (- _lambda * error  - K * sat_s  ) - iterm;

    // Antiwindup
    if ((sign(error) == -sign(alpha)) && (fabs(alpha) > 1.0f)) {
        _errorInteg -= error * _dt;
        iterm = Ki * _errorInteg;
    }

    alpha = _Co /Qa * (- _lambda * error  - K * sat_s  ) - iterm;
    *_ctrlOutput = constrain(alpha *100.0f,0.0f,100.0f);

}

void PressureController::reset() { 
    this->R_estimator->reset();
    initSetpointFilter();
    _errorInteg = 0.0f;
    retroCoffeeOutputPressureHistory = 0;
    estimationHasConvergedOnce = false;
}

