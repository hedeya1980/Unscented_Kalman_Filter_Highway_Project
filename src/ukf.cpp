#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using Eigen::MatrixXd;
using Eigen::VectorXd;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  //std_a_ = 30;
  //std_a_ = 3.0;
  std_a_ = 2.0;

  // Process noise standard deviation yaw acceleration in rad/s^2
  //std_yawdd_ = 30;
  //std_yawdd_ = 0.2;
  std_yawdd_ = 1.0;

  /**
   * DO NOT MODIFY measurement noise values below.
   * These are provided by the sensor manufacturer.
   */

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  
  /**
   * End DO NOT MODIFY section for measurement noise values 
   */
  
  /**
   * TODO: Complete the initialization. See ukf.h for other member properties.
   * Hint: one or more values initialized above might be wildly off...
   */
  time_us_=0;
  /*
  for(int i=0;i<5;i++)
  {
    P_(i,i)=1;
  }
  P_ <<   0.0043,   -0.0013,    0.0030,   -0.0022,   -0.0020,
        -0.0013,    0.0077,    0.0011,    0.0071,    0.0060,
         0.0030,    0.0011,    0.0054,    0.0007,    0.0008,
        -0.0022,    0.0071,    0.0007,    0.0098,    0.0100,
        -0.0020,    0.0060,    0.0008,    0.0100,    0.0123;
  */
  P_ <<   1,    0,    0,    0,    0,
          0,    1,    0,    0,    0,
          0,    0,    1,    0,    0,
          0,    0,    0,  0.5,    0,
          0,    1,    0,    0,  0.5;
  // set state dimension
  n_x_ = 5;

  // set augmented dimension
  n_aug_ = 7;

  // define spreading parameter
  lambda_ = 3 - n_aug_;
  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);
    // create vector for weights
  weights_ = VectorXd(2*n_aug_+1);
}

UKF::~UKF() {}

void UKF::AugmentedSigmaPoints(MatrixXd* Xsig_out) {

  // create augmented mean vector
  VectorXd x_aug = VectorXd(7);

  // create augmented state covariance
  MatrixXd P_aug = MatrixXd(7, 7);

  // create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  /**
   * Student part begin
   */
 
  // create augmented mean state
  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;

  // create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_*std_a_;
  P_aug(6,6) = std_yawdd_*std_yawdd_;

  // create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  // create augmented sigma points
  Xsig_aug.col(0)  = x_aug;
  for (int i = 0; i< n_aug_; ++i) {
    Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
  }
  
  /**
   * Student part end
   */

  // print result
  //std::cout << "Xsig_aug = " << std::endl << Xsig_aug << std::endl;

  // write result
  *Xsig_out = Xsig_aug;
}

void UKF::SigmaPointPrediction(MatrixXd Xsig_aug, double delta_t) {
  // predict sigma points
  for (int i = 0; i< 2*n_aug_+1; ++i) {
    // extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    // predicted state values
    double px_p, py_p;

    // avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    } else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    // add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
    yawd_p = yawd_p + nu_yawdd*delta_t;

    // write predicted sigma point into right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;
  }

  /**
   * Student part end
   */

  // print result
  std::cout << "Xsig_pred = " << std::endl << Xsig_pred_ << std::endl;
}

void UKF::PredictMeanAndCovariance() {
  // create vector for predicted state
  VectorXd x = VectorXd(n_x_);

  // create covariance matrix for prediction
  MatrixXd P = MatrixXd(n_x_, n_x_);

  // set weights
  double weight_0 = lambda_/(lambda_+n_aug_);
  weights_(0) = weight_0;
  for (int i=1; i<2*n_aug_+1; ++i) {  // 2n+1 weights
    double weight = 0.5/(n_aug_+lambda_);
    weights_(i) = weight;
  }

  // predicted state mean
  x.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // iterate over sigma points
    x = x + weights_(i) * Xsig_pred_.col(i);
  }

  // predicted state covariance matrix
  P.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // iterate over sigma points
    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x;
    // angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    P = P + weights_(i) * x_diff * x_diff.transpose() ;
  }

  /**
   * Student part end
   */

  // print result
  std::cout << "Predicted state" << std::endl;
  std::cout << x << std::endl;
  std::cout << "Predicted covariance matrix" << std::endl;
  std::cout << P << std::endl;

  // write result
  x_ = x;
  P_ = P;
}

void UKF::PredictRadarMeasurement(MatrixXd* Zsig, VectorXd* z_out, MatrixXd* S_out) {
  // set measurement dimension, radar can measure r, phi, and r_dot
  int n_z = 3;

  // set vector for weights
  double weight_0 = lambda_/(lambda_+n_aug_);
  double weight = 0.5/(lambda_+n_aug_);
  weights_(0) = weight_0;

  for (int i=1; i<2*n_aug_+1; ++i) {  
    weights_(i) = weight;
  }

  // create matrix for sigma points in measurement space
  MatrixXd Zsig_ = MatrixXd(n_z, 2 * n_aug_ + 1);

  // mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  
  // measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z,n_z);

  /**
   * Student part begin
   */

  // transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // extract values for better readability
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    // measurement model
    Zsig_(0,i) = sqrt(p_x*p_x + p_y*p_y);                       // r
    Zsig_(1,i) = atan2(p_y,p_x);                                // phi
    Zsig_(2,i) = (p_x*v1 + p_y*v2) / sqrt(p_x*p_x + p_y*p_y);   // r_dot
  }

  // mean predicted measurement
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; ++i) {
    z_pred = z_pred + weights_(i) * Zsig_.col(i);
  }

  // innovation covariance matrix S
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig_.col(i) - z_pred;

    // angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  // add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<  std_radr_*std_radr_, 0, 0,
        0, std_radphi_*std_radphi_, 0,
        0, 0,std_radrd_*std_radrd_;
  S = S + R;

  /**
   * Student part end
   */

  // print result
  std::cout << "z_pred: " << std::endl << z_pred << std::endl;
  std::cout << "S: " << std::endl << S << std::endl;

  // write result
  *z_out = z_pred;
  *S_out = S;
  *Zsig = Zsig_;
}

void UKF::RadarUpdateState(Eigen::MatrixXd Zsig, Eigen::VectorXd z_pred, Eigen::MatrixXd S, MeasurementPackage meas_package) {
  // set measurement dimension, radar can measure r, phi, and r_dot
  int n_z = 3;

  // set vector for weights
  double weight_0 = lambda_/(lambda_+n_aug_);
  double weight = 0.5/(lambda_+n_aug_);
  weights_(0) = weight_0;

  for (int i=1; i<2*n_aug_+1; ++i) {  
    weights_(i) = weight;
  }

  // create vector for incoming radar measurement
  VectorXd z = VectorXd(n_z);
  z <<
      static_cast<float>(meas_package.raw_measurements_(0)),   // rho in m
      static_cast<float>(meas_package.raw_measurements_(1)),   // phi in rad
      static_cast<float>(meas_package.raw_measurements_(2));   // rho_dot in m/s

  // create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z);

  /**
   * Student part begin
   */

  // calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    // angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    // angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  // Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  // residual
  VectorXd z_diff = z - z_pred;

  // angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

  // update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K*S*K.transpose();

  /**
   * Student part end
   */

  // print result
  std::cout << "Updated state x: " << std::endl << x_ << std::endl;
  std::cout << "Updated state covariance P: " << std::endl << P_ << std::endl;
}

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Make sure you switch between lidar and radar
   * measurements.
   */
    if (!is_initialized_) {
    //cout << "Kalman Filter Initialization " << endl;
    if(meas_package.sensor_type_==MeasurementPackage::RADAR)
    {
      auto rho=static_cast<float>(meas_package.raw_measurements_(0));
      auto phi=static_cast<float>(meas_package.raw_measurements_(1));
      x_ << rho*cos(phi), rho*sin(phi), 0, 0, 0;

    }
    else if(meas_package.sensor_type_==MeasurementPackage::LASER)
    {
      // set the state with the initial location and zero velocity
      x_ << meas_package.raw_measurements_[0], 
            meas_package.raw_measurements_[1],
            0, 
            0, 
            0;
    }
  
    time_us_ = meas_package.timestamp_;
    is_initialized_ = true;
    return;
    }
  // compute the time elapsed between the current and previous measurements
  // dt - expressed in seconds
  float delta_t = (meas_package.timestamp_ - time_us_) / 1000000.0;
  time_us_ = meas_package.timestamp_;
  /*
  float dt_2=dt*dt;
  float dt_3=dt_2*dt;
  float dt_4=dt_3*dt;
  // TODO: YOUR CODE HERE
  // 1. Modify the F matrix so that the time is integrated
  kf_.F_(0, 2)=dt;
  kf_.F_(1, 3)=dt;
  // 2. Set the process covariance matrix Q
  kf_.Q_ = MatrixXd(4, 4);
  kf_.Q_ << dt_4*noise_ax/4, 0, dt_3*noise_ax/2, 0,
            0, dt_4*noise_ay/4, 0, dt_3*noise_ay/2,
            dt_3*noise_ax/2, 0, dt_2*noise_ax, 0,
            0, dt_3*noise_ay/2, 0, dt_2*noise_ay;
  */
  // 3. Call the Kalman Filter predict() function
  Prediction(delta_t);
  // 4. Call the Kalman Filter update() function
  //      with the most recent raw measurements_
  if(meas_package.sensor_type_==MeasurementPackage::LASER)
    UpdateLidar(meas_package);
  if(meas_package.sensor_type_==MeasurementPackage::RADAR)
    UpdateRadar(meas_package);
  //std::cout << "x_= " << x_ << std::endl
  //std::cout << "P_= " << P_ << std::endl;

}

void UKF::Prediction(double delta_t) {
  /**
   * TODO: Complete this function! Estimate the object's location. 
   * Modify the state vector, x_. Predict sigma points, the state, 
   * and the state covariance matrix.
   */
  MatrixXd Xsig_aug = MatrixXd(7,15);
  AugmentedSigmaPoints(&Xsig_aug);
  SigmaPointPrediction(Xsig_aug, delta_t);
  PredictMeanAndCovariance();
  
}

void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use lidar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the lidar NIS, if desired.
   */
  // measurement matrix
  MatrixXd H_ = MatrixXd(2,5);
  H_ << 1, 0, 0, 0, 0,
        0, 1, 0, 0, 0;
  
  // add measurement noise covariance matrix
  int n_z = 2;
  MatrixXd R = MatrixXd(n_z,n_z);
  R <<  std_laspx_*std_laspx_, 0,
        0, std_laspy_*std_laspy_;

  // create vector for incoming radar measurement
  VectorXd z = VectorXd(n_z);
  z <<
      static_cast<float>(meas_package.raw_measurements_(0)),   // px in m
      static_cast<float>(meas_package.raw_measurements_(1));   // py in m

  VectorXd z_pred = H_ * x_;
  VectorXd y = z - z_pred;
  MatrixXd Ht = H_.transpose();
  MatrixXd S = H_ * P_ * Ht + R;
  MatrixXd Si = S.inverse();
  MatrixXd PHt = P_ * Ht;
  MatrixXd K = PHt * Si;

  //new estimate
  x_ = x_ + (K * y);
  long x_size = x_.size();
  MatrixXd I = MatrixXd::Identity(x_size, x_size);
  P_ = (I - K * H_) * P_;

}

void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Use radar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the radar NIS, if desired.
   */
  int n_z = 3;
  VectorXd z_out = VectorXd(3);
  MatrixXd S_out = MatrixXd(3, 3);
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);
  PredictRadarMeasurement(&Zsig, &z_out, &S_out);
  RadarUpdateState(Zsig, z_out, S_out, meas_package);
}