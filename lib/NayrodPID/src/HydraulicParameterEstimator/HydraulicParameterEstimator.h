#pragma once

class HydraulicParameterEstimator {
public:
    HydraulicParameterEstimator();

    // Met à jour les estimations à partir des mesures
    bool update(float Q, float P);
    bool updateFilteredResistance(float R_input);

    // Accès aux estimations actuelles
    float getResistance() const { return R_est; }
    float getCompliance() const { return C_est; }
    int getSampleCount() const { return counter; }
    float getFilteredResitance() const {return R_filtered;};
    float getFilteredResistanceDerivative() const {return dRdt_filtered;};


    // Configuration
    void setResistanceBounds(float Rmin, float Rmax);
    void setComplianceBounds(float Cmin, float Cmax);
    float getResistanceMinBound();
    void setTs(float Ts){dt = Ts;};
    void setLambda(float l);
    void reset();
    bool hasConverged();
    bool updateFilteredPressure(float P_raw);
    
    float getFilteredPressure(){return P_filtered;};
    float getFilteredDerivPressure(){return dPdt_filtered;};    

private:
    // Estimateurs RLS
    float theta[2];     // [theta1, theta2] -> [-1/RC, 1/C]
    float P_cov[2][2];  // Matrice de covariance

    // Paramètres estimés physiquement interprétables
    float R_est;
    float C_est;
    float R_filtered = 0.0f;
    float dRdt_filtered = 0.0f;

    // Limites
    float R_min, R_max;
    float C_min, C_max;

    // Hyperparamètres
    float lambda;
    float meas_noise_var;
    float epsilon;

    int counter;

    // Filtrage
    float P_filtered;
    float dPdt_filtered;
    float dt = 1.0f;

    inline bool isValid(float x) const {
        return x == x && x > -1e10f && x < 1e10f; // NaN / Inf check
    }
};
