// @(#)root/graf:$Name:  $:$Id: TGraphQQ.cxx,v 1.5 2006/05/09 15:41:37 brun Exp $
// Author: Anna Kreshuk 18/11/2005

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TGraphQQ.h"
#include "TF1.h"
#include "TMath.h"
#include "TVirtualPad.h"
#include "TLine.h"

ClassImp(TGraphQQ)

//______________________________________________________________________________
//
// This class allows to draw quantile-quantile plots
// 
// Plots can be drawn for 2 datasets or for a dataset and a theoretical 
// distribution function
// 
// 2 datasets:
//   Quantile-quantile plots are used to determine whether 2 samples come from
//   the same distribution. 
//   A qq-plot draws the quantiles of one dataset against the quantile of the 
//   the other. The quantiles of the dataset with fewer entries are on Y axis,
//   with more entries - on X axis. 
//   A straight line, going through 0.25 and 0.75 quantiles is also plotted 
//   for reference. It represents a robust linear fit, not sensitive to the 
//   extremes of the datasets.
//   If the datasets come from the same distribution, points of the plot should 
//   fall approximately on the 45 degrees line. If they have the same 
//   distribution function, but location or scale different parameters, 
//   they should still fall on the straight line, but not the 45 degrees one. 
//   The greater their departure from the straight line, the more evidence there
//   is, that the datasets come from different distributions.
//   The advantage of qq-plot is that it not only shows that the underlying
//   distributions are different, but, unlike the analytical methods, it also
//   gives information on the nature of this difference: heavier tails, 
//   different location/scale, different shape, etc.
//      
//   Some examples of qqplots of 2 datasets:
//Begin_Html
/*
<img src="gif/qqplots.gif">
*/
//End_Html
//
// 1 dataset: 
//   Quantile-quantile plots are used to determine if the dataset comes from the 
//   specified theoretical distribution, such as normal.
//   A qq-plot draws quantiles of the dataset against quantiles of the specified
//   theoretical distribution. 
//   (NOTE, that density, not CDF should be specified)
//   A straight line, going through 0.25 and 0.75 quantiles can also be plotted 
//   for reference. It represents a robust linear fit, not sensitive to the 
//   extremes of the dataset. 
//   As in the 2 datasets case, departures from straight line indicate departures
//   from the specified distribution.
//
//   " The correlation coefficient associated with the linear fit to the data 
//     in the probability plot (qq plot in our case) is a measure of the 
//     goodness of the fit. 
//     Estimates of the location and scale parameters  of the distribution 
//     are given by the intercept and slope. Probability plots can be generated 
//     for several competing distributions to see which provides the best fit, 
//     and the probability plot generating the highest correlation coefficient 
//     is the best choice since it generates the straightest probability plot."
//   From "Engineering statistic handbook", 
//   http://www.itl.nist.gov/div898/handbook/eda/section3/probplot.htm
//
//   Example of a qq-plot of a dataset from N(3, 2) distribution and 
//           TMath::Gaus(0, 1) theoretical function. Fitting parameters
//           are estimates of the distribution mean and sigma.
//
//Begin_Html
/*
<img src="gif/qqnormal.gif">
*/
//End_Html//
//   
//   
// References:
// http://www.itl.nist.gov/div898/handbook/eda/section3/qqplot.htm
// http://www.itl.nist.gov/div898/handbook/eda/section3/probplot.htm
//   
 


//______________________________________________________________________________
TGraphQQ::TGraphQQ()
{
   //default constructor
   
   fF=0;
   fY0=0;
}


//______________________________________________________________________________
TGraphQQ::TGraphQQ(Int_t n, Double_t *x) 
   : TGraph(n)
{
   //Creates a quantile-quantile plot of dataset x. 
   //Theoretical distribution function can be defined later by SetFunction method

   Int_t *index = new Int_t[n];
   TMath::Sort(n, x, index, kFALSE);
   for (Int_t i=0; i<fNpoints; i++)
      fY[i] = x[index[i]];
   fF=0;
   fY0=0;
   delete [] index;
} 

//______________________________________________________________________________
TGraphQQ::TGraphQQ(Int_t n, Double_t *x, TF1 *f)
   : TGraph(n)
{
   //Creates a quantile-quantile plot of dataset x against function f

   Int_t *index = new Int_t[n];
   TMath::Sort(n, x, index, kFALSE);
   for (Int_t i=0; i<fNpoints; i++)
      fY[i] = x[index[i]];
   delete [] index;
   fF = f;
   fY0=0;
   MakeFunctionQuantiles();
} 


//______________________________________________________________________________
TGraphQQ::TGraphQQ(Int_t nx, Double_t *x, Int_t ny, Double_t *y)
{
   //Creates a quantile-quantile plot of dataset x against dataset y
   //Parameters nx and ny are respective array sizes

   nx<=ny ? fNpoints=nx : fNpoints=ny;

   if (!CtorAllocate()) return;
   fF=0;
   Int_t *index = new Int_t[TMath::Max(nx, ny)];
   TMath::Sort(nx, x, index, kFALSE);
   if (nx <=ny){
      for (Int_t i=0; i<fNpoints; i++)
         fY[i] = x[index[i]];
      TMath::Sort(ny, y, index, kFALSE);
      if (nx==ny){
         for (Int_t i=0; i<fNpoints; i++)
            fX[i] = y[index[i]];
         fY0 = 0;
         Quartiles();
      } else {
         fNy0 = ny;
         fY0 = new Double_t[ny];
         for (Int_t i=0; i<ny; i++)
            fY0[i] = y[i];
         MakeQuantiles();
      }
   } else {
      fNy0 = nx;
      fY0 = new Double_t[nx];
      for (Int_t i=0; i<nx; i++)
         fY0[i] = x[index[i]];
      TMath::Sort(ny, y, index, kFALSE);
      for (Int_t i=0; i<ny; i++)
         fY[i] = y[index[i]];
      MakeQuantiles();
   }


   delete [] index;
}

//______________________________________________________________________________
TGraphQQ::~TGraphQQ()
{
   //Destroys a TGraphQQ

   if (fY0)
      delete [] fY0;
   if (fF)
      fF = 0;
}

//______________________________________________________________________________
void TGraphQQ::MakeFunctionQuantiles()
{
   //Computes quantiles of theoretical distribution function

   if (!fF) return;
   TString s = fF->GetTitle();
   Double_t pk;
   if (s.Contains("TMath::Gaus") || s.Contains("gaus")){
      //use plotting positions optimal for normal distribution
      for (Int_t k=1; k<=fNpoints; k++){
         pk = (k-0.375)/(fNpoints+0.25);
         fX[k-1]=TMath::NormQuantile(pk);
      }
   } else {
      Double_t *prob = new Double_t[fNpoints];
      if (fNpoints > 10){
         for (Int_t k=1; k<=fNpoints; k++)
            prob[k-1] = (k-0.5)/fNpoints;
      } else {
         for (Int_t k=1; k<=fNpoints; k++)
            prob[k-1] = (k-0.375)/(fNpoints+0.25);
      }
      //fF->GetQuantiles(fNpoints, prob, fX);
      fF->GetQuantiles(fNpoints, fX, prob);
      delete [] prob;
   }

   Quartiles();
}

//______________________________________________________________________________

void TGraphQQ::MakeQuantiles()
{
   //When sample sizes are not equal, computes quantiles of the bigger sample
   //by linear interpolation

   if (!fY0) return;

   Double_t pi, pfrac;
   Int_t pint;
   for (Int_t i=0; i<fNpoints-1; i++){
      pi = (fNy0-1)*Double_t(i)/Double_t(fNpoints-1);
      pint = TMath::FloorNint(pi);
      pfrac = pi - pint;
      fX[i] = (1-pfrac)*fY0[pint]+pfrac*fY0[pint+1];
   }
   fX[fNpoints-1]=fY0[fNy0-1];

   Quartiles();
}

//______________________________________________________________________________
void TGraphQQ::Quartiles()
{
   // compute quartiles
   // a quartile is a 25 per cent or 75 per cent quantile
   
   Double_t prob[]={0.25, 0.75};
   Double_t x[2];
   Double_t y[2];
   TMath::Quantiles(fNpoints, 2, fY, y, prob, kTRUE);
   if (fY0)
      TMath::Quantiles(fNy0, 2, fY0, x, prob, kTRUE);
   else if (fF) {
      TString s = fF->GetTitle();
      if (s.Contains("TMath::Gaus") || s.Contains("gaus")){
         x[0] = TMath::NormQuantile(0.25);
         x[1] = TMath::NormQuantile(0.75);
      } else 
         fF->GetQuantiles(2, x, prob);
   }
   else 
      TMath::Quantiles(fNpoints, 2, fX, x, prob, kTRUE);

   fXq1=x[0]; fXq2=x[1]; fYq1=y[0]; fYq2=y[1];
}

//______________________________________________________________________________
void TGraphQQ::Paint(Option_t *opt)
{
   // paint this graphQQ. No options for the time being
   
   if (!fX){
      Error("TGraphQQ::Paint", "2nd dataset or theoretical function not specified");
      return;
   }

   if (fF){
      GetXaxis()->SetTitle("theoretical quantiles");
      GetYaxis()->SetTitle("data quantiles");
   } 

   TGraph::Paint(opt);

   Double_t xmin = gPad->GetUxmin();
   Double_t xmax = gPad->GetUxmax();
   Double_t ymin = gPad->GetUymin();
   Double_t ymax = gPad->GetUymax();
   Double_t yxmin, xymin, yxmax, xymax;
   Double_t xqmin = TMath::Max(xmin, fXq1);
   Double_t xqmax = TMath::Min(xmax, fXq2);
   Double_t yqmin = TMath::Max(ymin, fYq1);
   Double_t yqmax = TMath::Min(ymax, fYq2);

   TLine line1, line2, line3;
   line1.SetLineStyle(2);
   line3.SetLineStyle(2);
   yxmin = (fYq2-fYq1)*(xmin-fXq1)/(fXq2-fXq1) + fYq1;
   if (yxmin < ymin){
      xymin = (fXq2-fXq1)*(ymin-fYq1)/(fYq2-fYq1) + fXq1;
      line1.PaintLine(xymin, ymin, xqmin, yqmin);
   }
   else 
      line1.PaintLine(xmin, yxmin, xqmin, yqmin);

   line2.PaintLine(xqmin, yqmin, xqmax, yqmax);

   yxmax = (fYq2-fYq1)*(xmax-fXq1)/(fXq2-fXq1) + fYq1;
   if (yxmax > ymax){
      xymax = (fXq2-fXq1)*(ymax-fYq1)/(fYq2-fYq1) + fXq1;
      line3.PaintLine(xqmax, yqmax, xymax, ymax);
   }      
   else
      line3.PaintLine(xqmax, yqmax, xmax, yxmax);

}

//______________________________________________________________________________
void TGraphQQ::SetFunction(TF1 *f)
{
   //Sets the theoretical distribution function (density!) 
   //and computes its quantiles

   fF = f;
   MakeFunctionQuantiles();
}






