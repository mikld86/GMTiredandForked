#include "HydraulicParameterEstimator.h"
#include <cmath>
#include <deque>

HydraulicParameterEstimator::HydraulicParameterEstimator()
    : dt(0.03),R_est(1e7f), C_est(1.0e-7f), R_min(1e6f), R_max(1e8f),
      C_min(1e-9f), C_max(1e-3f), lambda(0.8f),
      meas_noise_var(1e-4f), epsilon(1e-12f), counter(0)
{
    theta[0] = -1.0f / (R_est * C_est);  // theta1 = -1/RC
    theta[1] = 1.0f / C_est;             // theta2 = 1/C

    P_cov[0][0] = 1000.0f;
    P_cov[0][1] = 0.0f;
    P_cov[1][0] = 0.0f;
    P_cov[1][1] = 1000.0f;
}

float HydraulicParameterEstimator::getResistanceMinBound() {
    return R_min;
}

void HydraulicParameterEstimator::setResistanceBounds(float Rmin, float Rmax) {
    R_min = Rmin;
    R_max = Rmax;
}

void HydraulicParameterEstimator::setComplianceBounds(float Cmin, float Cmax) {
    C_min = Cmin;
    C_max = Cmax;
}

void HydraulicParameterEstimator::setLambda(float l) {
    lambda = l;
}

void HydraulicParameterEstimator::reset() {
    counter = 0;
    R_est = 1e7f;
    C_est = 1.0e-7f;
    theta[0] = -1.0f / (R_est * C_est);
    theta[1] = 1.0f / C_est;
    P_cov[0][0] = 1000.0f;
    P_cov[0][1] = 0.0f;
    P_cov[1][0] = 0.0f;
    P_cov[1][1] = 1000.0f;
}

bool HydraulicParameterEstimator::hasConverged() {
    float traceP = P_cov[0][0] + P_cov[1][1];
    bool low_uncertainty = traceP < 5.0f;

    // return low_uncertainty;
    bool state = P_cov[0][0] < 1e-3;
    return state;
}

bool HydraulicParameterEstimator::updateFilteredPressure(float P_raw) {
    static std::deque<float> buffer;

    constexpr int N = 5;
    constexpr float smooth_coeffs[N] = { -3, 12, 17, 12, -3 };  // smoothing: divided by 35
    constexpr float deriv_coeffs[N]  = { -2, -1, 0, 1, 2 };     // derivative: divided by 10

    // Ajout du nouvel échantillon en fin de buffer
    buffer.push_back(P_raw);
    if (buffer.size() < N) return false;  // pas assez de points
    if (buffer.size() > N) buffer.pop_front();

    // Application du filtre lissage
    float smooth = 0.0f;
    float deriv = 0.0f;
    for (int i = 0; i < N; ++i) {
        smooth += smooth_coeffs[i] * buffer[i];
        deriv  += deriv_coeffs[i]  * buffer[i];
    }

    P_filtered = smooth / 35.0f;
    dPdt_filtered = deriv / (10.0f * dt);
    return true;
}


bool HydraulicParameterEstimator::updateFilteredResistance(float R_input) {
    static std::deque<float> R_buffer;

    constexpr int N = 5;
    constexpr float smooth_coeffs[N] = { -3, 12, 17, 12, -3 };  // smoothing (÷35)
    constexpr float deriv_coeffs[N]  = { -2, -1, 0, 1, 2 };     // derivative (÷10)

    // Ajout du nouvel échantillon dans le buffer
    R_buffer.push_back(R_input);
    if (R_buffer.size() < N) return false;
    if (R_buffer.size() > N) R_buffer.pop_front();

    // Application du filtre
    float R_smooth = 0.0f;
    float R_deriv = 0.0f;
    for (int i = 0; i < N; ++i) {
        R_smooth += smooth_coeffs[i] * R_buffer[i];
        R_deriv  += deriv_coeffs[i]  * R_buffer[i];
    }

    R_filtered = R_smooth / 35.0f;
    dRdt_filtered = R_deriv / (10.0f * dt);
    return true;
}


bool HydraulicParameterEstimator::update(float Q, float P_raw) {
    if (!isValid(Q) || !isValid(P_raw)) return false;
    counter++;

    // Filtrage de P et calcul de sa dérivée filtrée
    if (!updateFilteredPressure(P_raw))return false;
    float P = P_filtered;
    float dPdt = dPdt_filtered;

    if(P<0.3)return false;

    // Backup
    float theta_prev[2] = {theta[0], theta[1]};
    float P_cov_prev[2][2] = {
        {P_cov[0][0], P_cov[0][1]},
        {P_cov[1][0], P_cov[1][1]}
    };

    // phi = [P, Q]
    float phi[2] = {P, Q};

    float phi_Pcov_phi =
        phi[0]*(P_cov[0][0]*phi[0] + P_cov[0][1]*phi[1]) +
        phi[1]*(P_cov[1][0]*phi[0] + P_cov[1][1]*phi[1]) + epsilon;

    float denom = lambda + phi_Pcov_phi;

    float K[2] = {
        (P_cov[0][0]*phi[0] + P_cov[0][1]*phi[1]) / denom,
        (P_cov[1][0]*phi[0] + P_cov[1][1]*phi[1]) / denom
    };

    float y_est = theta[0]*phi[0] + theta[1]*phi[1];
    float error = dPdt - y_est;

    theta[0] += K[0] * error;
    theta[1] += K[1] * error;

    // Mise à jour covariance (forme Joseph)
    float temp0[2] = {1.0f - K[0]*phi[0], -K[0]*phi[1]};
    float temp1[2] = {-K[1]*phi[0], 1.0f - K[1]*phi[1]};

    float new_P_cov[2][2];
    new_P_cov[0][0] = temp0[0]*P_cov[0][0] + temp0[1]*P_cov[1][0];
    new_P_cov[0][1] = temp0[0]*P_cov[0][1] + temp0[1]*P_cov[1][1];
    new_P_cov[1][0] = temp1[0]*P_cov[0][0] + temp1[1]*P_cov[1][0];
    new_P_cov[1][1] = temp1[0]*P_cov[0][1] + temp1[1]*P_cov[1][1];

    new_P_cov[0][0] += K[0]*K[0]*meas_noise_var;
    new_P_cov[0][1] += K[0]*K[1]*meas_noise_var;
    new_P_cov[1][0] += K[1]*K[0]*meas_noise_var;
    new_P_cov[1][1] += K[1]*K[1]*meas_noise_var;

    if (!isValid(new_P_cov[0][0]) || !isValid(new_P_cov[1][1])) {
        theta[0] = theta_prev[0]; theta[1] = theta_prev[1];
        P_cov[0][0] = P_cov_prev[0][0]; P_cov[0][1] = P_cov_prev[0][1];
        P_cov[1][0] = P_cov_prev[1][0]; P_cov[1][1] = P_cov_prev[1][1];
        return false;
    }

    P_cov[0][0] = new_P_cov[0][0]; P_cov[0][1] = new_P_cov[0][1];
    P_cov[1][0] = new_P_cov[1][0]; P_cov[1][1] = new_P_cov[1][1];

    P_cov[0][0] = std::max(P_cov[0][0], 0.9e-3f); // Force a minimum of uncertainty, avoid RLS stiffness and keep updating
    P_cov[1][1] = std::max(P_cov[1][1], 0.9e-3f); // Force a minimum of uncertainty, avoid RLS stiffness and keep updating

    // Extraction des paramètres physiques
    if (fabs(theta[1]) > 1e-9f && fabs(theta[0]) > 1e-12f) {
        C_est = 1.0f / theta[1];
        R_est = -theta[1] / theta[0];

        if (C_est < C_min) C_est = C_min;
        if (C_est > C_max) C_est = C_max;
        if (R_est < R_min) R_est = R_min;
        if (R_est > R_max) R_est = R_max;
    }

    updateFilteredResistance(R_est);
    return true;
}


