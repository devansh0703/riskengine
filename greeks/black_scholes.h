#pragma once
#include <cmath>

namespace risk {
namespace greeks {

double norm_cdf(double x);
double norm_pdf(double x);

double call_price(double S, double K, double T, double r, double sigma);
double put_price(double S, double K, double T, double r, double sigma);

double delta_call(double S, double K, double T, double r, double sigma);
double delta_put(double S, double K, double T, double r, double sigma);
double gamma(double S, double K, double T, double r, double sigma);
double vega(double S, double K, double T, double r, double sigma);
double theta_call(double S, double K, double T, double r, double sigma);
double theta_put(double S, double K, double T, double r, double sigma);
double rho_call(double S, double K, double T, double r, double sigma);
double rho_put(double S, double K, double T, double r, double sigma);

double implied_volatility(double market_price, double S, double K,
                          double T, double r, bool is_call,
                          int max_iter = 100, double tol = 1e-8);

} // namespace greeks
} // namespace risk
