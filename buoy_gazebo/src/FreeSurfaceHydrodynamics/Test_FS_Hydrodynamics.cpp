// Copyright 2022 Open Source Robotics Foundation, Inc. and Monterey Bay Aquarium Research Institute
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <Eigen/Dense>
#include "gnuplot-iostream.h"
#include "FS_Hydrodynamics.hpp"

//#include "EtaFunctor.hpp"
#include "LinearIncidentWave.hpp"
//#include <boost/tuple/tuple.hpp>
#include "gnuplot-iostream.h"

int main()
{
  const char *modes[6] = {"Surge", "Sway", "Heave", "Roll", "Pitch", "Yaw"};
  LinearIncidentWave Inc;
  Inc.SetToPiersonMoskowitzSpectrum(1, 0);
  // Inc.SetToMonoChromatic(1,5,0);
  LinearIncidentWave &IncRef = Inc;
  FS_HydroDynamics BuoyA5(IncRef, 1.0, 9.81, 1025);
  BuoyA5.ReadWAMITData_FD("HydrodynamicCoeffs/BuoyA5");
  BuoyA5.ReadWAMITData_TD("HydrodynamicCoeffs/BuoyA5");
  // BuoyA5.Plot_FD_Coeffs();
  BuoyA5.SetTimestepSize(.01);
  BuoyA5.Plot_TD_Coeffs();

#if 0 // Test Radiation Forces
  srand((unsigned)time(0));
  double A = .5 + ((float)(rand() % 20) / 10);
  double T = 3.0 + (rand() % 9);
  double tf = 5 * T;
  double omega = 2 * M_PI / T;

  for (int i = 0; i < 6; i++)   // i determines mode of motion.
    for (int j = 0; j < 6; j++) // j denotes direction of resulting force
    {
      double am = BuoyA5.AddedMass(omega, i, j);
      double dmp = BuoyA5.Damping(omega, i  j);
      double am_inf = BuoyA5.fd_X_inf_freq(i, j);

      std::vector<double> pts_t,pts_F_TD,pts_F_FD;
      double last_accel = 0;
      double F_max = -std::numeric_limits<double>::max();
      double F_min = std::numeric_limits<double>::max();
      for (int k = 0; k < tf / BuoyA5.m_dt; k++)
      {
        double tt = BuoyA5.m_dt * k;
        pts_t.push_back(tt);
        //double pos = A * cos(omega * tt);  //Not needed, but handy to see...
        double vel = -A * omega * sin(omega * tt);
        double accel = -A * pow(omega, 2) * cos(omega * tt);
        Eigen::VectorXd xddot(6);
        for (int n = 0; n < 6; n++)
          xddot(n) = 0;
        xddot(i) = last_accel;

        Eigen::VectorXd MemForce(6);
        MemForce = BuoyA5.RadiationForce(xddot);
        pts_F_TD.push_back(am_inf * accel + MemForce(j));
        last_accel = accel;
        double FD_Force = am * accel + dmp * vel;
        pts_F_FD.push_back(am * accel + dmp * vel); // FD_Force);
        if (FD_Force > F_max)
          F_max = FD_Force;
        if (FD_Force < F_min)
          F_min = FD_Force;
      }
      if ((F_min < -1) && (F_max > 1)) // Don't plot near-zero forces
      {
        Gnuplot gp;
        char Amp[10];
        sprintf(Amp, "%.1f", A);
        char Per[10];
        sprintf(Per, "%.1f", T);
        gp << "set term X11 title 'Radiation Forces: " << modes[i] << " Motions, " << modes[j] << " Forces:  A = " << Amp << "m  T = " << Per << "s  \n";
        gp << "set grid\n";
        gp << "set xlabel 'time (s)'\n";
        if (j < 3)
          gp << "set ylabel 'F (N)'\n";
        else
          gp << "set  ylabel 'M (N-m)'\n";
        gp << "plot '-' w l title 'Time-Domain'"
           << ",'-' w l title 'Freq-Domain'\n";
        gp.send1d(boost::make_tuple(pts_t, pts_F_TD));
        gp.send1d(boost::make_tuple(pts_t, pts_F_FD));
        gp << "set xlabel 'time (s)'\n";
        if (j < 3)
          gp << "set ylabel 'F (N)'\n";
        else
          gp << "set  ylabel 'M (N-m)'\n";
        gp << "set title '" << modes[i] << "(t) = " << std::fixed << std::setprecision(1) << Amp << "cos(2 pi t/" << Per << ")'  \n";
        gp << "replot\n";
      }
    }
#endif

#if 1 // Test Exciting Forces
  srand((unsigned)time(0));
  double Ac = .5 + ((float)(rand() % 20) / 10);
  double As = .5 + ((float)(rand() % 20) / 10);
  double T = 3.0 + (rand() % 9);
  double tf = 5 * T;
  double omega = 2 * M_PI / T;

  //for (int j = 0; j < 1; j++) // j denotes direction of resulting force
  int j = 2;
  {
    double XiRe, XiIm;
    BuoyA5.WaveExcitingForceComponents(&XiRe, &XiIm, omega, j);

    std::cout << "omega = " << omega << "  Ac =  " << Ac << "  As = " << As << "  XiRe = " << XiRe << "  " << XiIm << std::endl;

    std::vector<double> pts_t, pts_F_TD, pts_F_FD, pts_eta;
    double last_accel = 0;
    double F_max = -std::numeric_limits<double>::max();
    double F_min = std::numeric_limits<double>::max();
    for (int k = 0; k < tf / BuoyA5.m_dt; k++)
    {
      double tt = BuoyA5.m_dt * k;
      pts_t.push_back(tt);
      pts_F_FD.push_back((Ac * XiRe + As * XiIm) * cos(omega * tt) - (Ac * XiIm + As * XiRe) * sin(omega * tt));
      double eta = Ac * cos(omega * tt) + As * sin(omega * tt);
      pts_eta.push_back(eta);
      Eigen::VectorXd ExtForce(6);
      ExtForce = BuoyA5.ExcitingForce(eta,0);
      pts_F_TD.push_back(ExtForce(j));
      std::cout << tt << "  " << T << "  " << XiRe * cos(omega * tt) + XiIm * sin(omega * tt) << "   ExtForce(j) = " << ExtForce(j) <<  std::endl;
      //  if (Exciting_Force > F_max)
      //    F_max = Exciting_Force;
      //  if (Exciting_Force < F_min)
      //    F_min = Exciting_Force;
      //}
      // if ((F_min < -1) && (F_max > 1)) // Don't plot near-zero forces
    }
    Gnuplot gp;
    // char Amp[10];
    // sprintf(Amp, "%.1f", A);
    // char Per[10];
    // sprintf(Per, "%.1f", T);
    // gp << "set term X11 title 'Radiation Forces: " << modes[i] << " Motions, " << modes[j] << " Forces:  A = " << Amp << "m  T = " << Per << "s  \n";
    gp << "set term X11 title 'Exciting Forces: \n";
    gp << "set grid\n";
    gp << "set xlabel 'time (s)'\n";
    if (j < 3)
      gp << "set ylabel 'F (N)'\n";
    else
      gp << "set  ylabel 'M (N-m)'\n";
    gp << "plot '-' w l title 'Time-Domain'"
       << ",'-' w l title 'Freq-Domain'"
       << ",'-' w l title 'eta(t)'\n";
    // gp.send1d(boost::make_tuple(pts_t, pts_F_TD));
    gp.send1d(boost::make_tuple(pts_t, pts_F_TD));
    gp.send1d(boost::make_tuple(pts_t, pts_F_FD));
    gp.send1d(boost::make_tuple(pts_t, pts_eta));
    gp << "set xlabel 'time (s)'\n";
    if (j < 3)
      gp << "set ylabel 'F (N)'\n";
    else
      gp << "set  ylabel 'M (N-m)'\n";
    // gp << "set title '" << modes[i] << "(t) = " << std::fixed << std::setprecision(1) << Amp << "cos(2 pi t/" << Per << ")'  \n";
    gp << "replot\n";
  }
#endif
}
