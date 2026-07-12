#include "black_scholes.h"
#include <cmath>

namespace risk {
namespace greeks {

double norm_cdf(double x) {
  return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
}

double norm_pdf(double x) {
  return std::exp(-0.5 * x * x) / std::sqrt(2.0 * M_PI);
}

static void compute_d1d2(double S, double K, double T, double r,
                         double sigma, double& d1, double& d2) {
  d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) /
       (sigma * std::sqrt(T));
  d2 = d1 - sigma * std::sqrt(T);
}

double call_price(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return S * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
}

double put_price(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return K * std::exp(-r * T) * norm_cdf(-d2) - S * norm_cdf(-d1);
}

double delta_call(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return norm_cdf(d1);
}

double delta_put(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return norm_cdf(d1) - 1.0;
}

double gamma(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return norm_pdf(d1) / (S * sigma * std::sqrt(T));
}

double vega(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return S * norm_pdf(d1) * std::sqrt(T) / 100.0;
}

double theta_call(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return (-S * norm_pdf(d1) * sigma / (2 * std::sqrt(T)) -
          r * K * std::exp(-r * T) * norm_cdf(d2)) /
         365.0;
}

double theta_put(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return (-S * norm_pdf(d1) * sigma / (2 * std::sqrt(T)) +
          r * K * std::exp(-r * T) * norm_cdf(-d2)) /
         365.0;
}

double rho_call(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return K * T * std::exp(-r * T) * norm_cdf(d2) / 100.0;
}

double rho_put(double S, double K, double T, double r, double sigma) {
  double d1, d2;
  compute_d1d2(S, K, T, r, sigma, d1, d2);
  return -K * T * std::exp(-r * T) * norm_cdf(-d2) / 100.0;
}

double implied_volatility(double market_price, double S, double K,
                          double T, double r, bool is_call,
                          int max_iter, double tol) {
  double sigma = 0.5;
  auto price_fn = is_call ? call_price : put_price;

  for (int i = 0; i < max_iter; i++) {
    double price = price_fn(S, K, T, r, sigma);
    double diff = price - market_price;

    if (std::abs(diff) < tol) return sigma;

    double d1, d2;
    compute_d1d2(S, K, T, r, sigma, d1, d2);
    double vega_val = S * norm_pdf(d1) * std::sqrt(T);

    if (vega_val < 1e-12) break;
    sigma -= diff / vega_val;
    if (sigma <= 0.001) sigma = 0.001;
    if (sigma > 10.0) sigma = 10.0;
  }
  return sigma;
}

} // namespace greeks
} // namespace risk
