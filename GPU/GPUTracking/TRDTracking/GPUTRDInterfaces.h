//**************************************************************************\
//* This file is property of and copyright by the ALICE Project            *\
//* ALICE Experiment at CERN, All rights reserved.                         *\
//*                                                                        *\
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *\
//*                  for The ALICE HLT Project.                            *\
//*                                                                        *\
//* Permission to use, copy, modify and distribute this software and its   *\
//* documentation strictly for non-commercial purposes is hereby granted   *\
//* without fee, provided that the above copyright notice appears in all   *\
//* copies and that both the copyright notice and this permission notice   *\
//* appear in the supporting documentation. The authors make no claims     *\
//* about the suitability of this software for any purpose. It is          *\
//* provided "as is" without express or implied warranty.                  *\
//**************************************************************************

/// \file GPUTRDInterfaces.h
/// \author David Rohr, Ole Schmidt

#ifndef GPUTRDINTERFACES_H
#define GPUTRDINTERFACES_H

// This is an interface header for making the TRD tracking portable between O2, AliRoot, and HLT standalone framework

#include "GPUCommonDef.h"
#include "GPUCommonMath.h"
#include "GPUTPCGMMergedTrack.h"
#include "GPUTPCGMTrackParam.h"
#include "GPUTRDDef.h"

namespace GPUCA_NAMESPACE
{
namespace gpu
{
template <typename T>
class trackInterface;
template <typename T>
class propagatorInterface;
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#ifdef GPUCA_ALIROOT_LIB // Interface for AliRoot, build only with AliRoot
#include "AliExternalTrackParam.h"
#include "AliHLTExternalTrackParam.h"
#include "AliTrackerBase.h"

namespace GPUCA_NAMESPACE
{
namespace gpu
{

template <>
class trackInterface<AliExternalTrackParam> : public AliExternalTrackParam
{

 public:
  trackInterface<AliExternalTrackParam>() : AliExternalTrackParam(){};
  trackInterface<AliExternalTrackParam>(const trackInterface<AliExternalTrackParam>& param) : AliExternalTrackParam(param){};
  trackInterface<AliExternalTrackParam>(const AliExternalTrackParam& param) CON_DELETE;
  trackInterface<AliExternalTrackParam>(const AliHLTExternalTrackParam& param) : AliExternalTrackParam()
  {
    float paramTmp[5] = {param.fY, param.fZ, param.fSinPhi, param.fTgl, param.fq1Pt};
    Set(param.fX, param.fAlpha, paramTmp, param.fC);
  }
  trackInterface<AliExternalTrackParam>(const GPUTPCGMMergedTrack& trk) : AliExternalTrackParam()
  {
    Set(trk.GetParam().GetX(), trk.GetAlpha(), trk.GetParam().GetPar(), trk.GetParam().GetCov());
  }
  trackInterface<AliExternalTrackParam>(const GPUTPCGMTrackParam::GPUTPCOuterParam& param) : AliExternalTrackParam()
  {
    Set(param.X, param.alpha, param.P, param.C);
  }

  // parameter + covariance
  float getX() const { return GetX(); }
  float getAlpha() const { return GetAlpha(); }
  float getY() const { return GetY(); }
  float getZ() const { return GetZ(); }
  float getSnp() const { return GetSnp(); }
  float getTgl() const { return GetTgl(); }
  float getQ2Pt() const { return GetSigned1Pt(); }
  float getEta() const { return Eta(); }
  float getPt() const { return Pt(); }
  float getSigmaY2() const { return GetSigmaY2(); }
  float getSigmaZ2() const { return GetSigmaZ2(); }

  const My_Float* getPar() const { return GetParameter(); }
  const My_Float* getCov() const { return GetCovariance(); }
  void resetCovariance(float s) { ResetCovariance(10.); }
  float getTime() const { return -1.f; }
  float getTimeMax() const { return 0.f; }
  float getTimeMin() const { return 0.f; }
  short getSide() const { return 0; }
  void setZShift(float) {}
  float getZShift() const { return 0.f; }
  bool CheckNumericalQuality() const { return true; }

  // parameter manipulation
  bool update(const My_Float p[2], const My_Float cov[3]) { return Update(p, cov); }
  float getPredictedChi2(const My_Float p[2], const My_Float cov[3]) const { return GetPredictedChi2(p, cov); }
  bool rotate(float alpha) { return Rotate(alpha); }

  void set(float x, float alpha, const float param[5], const float cov[15]) { Set(x, alpha, param, cov); }

  typedef AliExternalTrackParam baseClass;
};

template <>
class propagatorInterface<AliTrackerBase> : public AliTrackerBase
{

 public:
  typedef void propagatorParam;
  propagatorInterface<AliTrackerBase>(const propagatorParam* = nullptr) : AliTrackerBase(), mParam(nullptr){};
  propagatorInterface<AliTrackerBase>(const propagatorInterface<AliTrackerBase>&) CON_DELETE;
  propagatorInterface<AliTrackerBase>& operator=(const propagatorInterface<AliTrackerBase>&) CON_DELETE;

  bool propagateToX(float x, float maxSnp, float maxStep) { return PropagateTrackToBxByBz(mParam, x, 0.13957, maxStep, false, maxSnp); }
  int getPropagatedYZ(float x, float& projY, float& projZ)
  {
    Double_t yz[2] = {0.};
    mParam->GetYZAt(x, GetBz(), yz);
    projY = yz[0];
    projZ = yz[1];
    return 0;
  }

  void setTrack(trackInterface<AliExternalTrackParam>* trk) { mParam = trk; }
  void setFitInProjections(bool flag) {}

  float getAlpha() { return (mParam) ? mParam->GetAlpha() : 99999.f; }
  bool update(const My_Float p[2], const My_Float cov[3]) { return (mParam) ? mParam->update(p, cov) : false; }
  float getPredictedChi2(const My_Float p[2], const My_Float cov[3]) { return (mParam) ? mParam->getPredictedChi2(p, cov) : 99999.f; }
  bool rotate(float alpha) { return (mParam) ? mParam->rotate(alpha) : false; }

  trackInterface<AliExternalTrackParam>* mParam;
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif // GPUCA_ALIROOT_LIB

#if defined(HAVE_O2HEADERS) // Interface for O2, build only with O2

#include "ReconstructionDataFormats/Track.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "ReconstructionDataFormats/GlobalTrackID.h"
#include "DetectorsBase/Propagator.h"
#include "GPUTRDO2BaseTrack.h"
#ifndef GPUCA_GPUCODE_DEVICE
#include <cmath>
#endif

namespace GPUCA_NAMESPACE
{

namespace gpu
{

template <>
class trackInterface<GPUTRDO2BaseTrack> : public GPUTRDO2BaseTrack
{
 public:
  GPUdDefault() trackInterface<GPUTRDO2BaseTrack>() = default;
  trackInterface<GPUTRDO2BaseTrack>(const GPUTRDO2BaseTrack& param) = delete;
  GPUd() trackInterface<GPUTRDO2BaseTrack>(const o2::dataformats::TrackTPCITS& trkItsTpc, float vDrift) : GPUTRDO2BaseTrack(trkItsTpc.getParamOut())
  {
    mTime = trkItsTpc.getTimeMUS().getTimeStamp();
    mTimeAddMax = trkItsTpc.getTimeMUS().getTimeStampError();
    mTimeSubMax = trkItsTpc.getTimeMUS().getTimeStampError();
    mRefITS = trkItsTpc.getRefITS();
    mRefTPC = trkItsTpc.getRefTPC();
    float tmp = trkItsTpc.getTimeMUS().getTimeStampError() * vDrift;
    updateCov(tmp * tmp, o2::track::CovLabels::kSigZ2); // account for time uncertainty by increasing sigmaZ2
  }
  GPUd() trackInterface<GPUTRDO2BaseTrack>(const o2::tpc::TrackTPC& trkTpc, float tbWidth, float vDrift, unsigned int iTrk) : GPUTRDO2BaseTrack(trkTpc.getParamOut())
  {
    mRefTPC = {iTrk, o2::dataformats::GlobalTrackID::TPC};
    mTime = trkTpc.getTime0() * tbWidth;
    mTimeAddMax = trkTpc.getDeltaTFwd() * tbWidth;
    mTimeSubMax = trkTpc.getDeltaTBwd() * tbWidth;
    if (trkTpc.hasASideClustersOnly()) {
      mSide = -1;
    } else if (trkTpc.hasCSideClustersOnly()) {
      mSide = 1;
    } else {
      // CE-crossing tracks are not shifted along z, but the time uncertainty is taken into account by increasing sigmaZ2
      float timeWindow = (mTimeAddMax + mTimeSubMax) * .5f;
      float tmp = timeWindow * vDrift;
      updateCov(tmp * tmp, o2::track::CovLabels::kSigZ2);
    }
  }
  GPUd() void set(float x, float alpha, const float* param, const float* cov)
  {
    setX(x);
    setAlpha(alpha);
    for (int i = 0; i < 5; i++) {
      setParam(param[i], i);
    }
    for (int i = 0; i < 15; i++) {
      setCov(cov[i], i);
    }
  }
  GPUdi() trackInterface<GPUTRDO2BaseTrack>(const GPUTPCGMMergedTrack& trk) { set(trk.OuterParam().X, trk.OuterParam().alpha, trk.OuterParam().P, trk.OuterParam().C); }
  GPUdi() trackInterface<GPUTRDO2BaseTrack>(const GPUTPCGMTrackParam::GPUTPCOuterParam& param) { set(param.X, param.alpha, param.P, param.C); }

  GPUdi() const float* getPar() const { return getParams(); }
  GPUdi() float getTime() const { return mTime; }
  GPUdi() void setTime(float t) { mTime = t; }
  GPUdi() float getTimeMax() const { return mTime + mTimeAddMax; }
  GPUdi() float getTimeMin() const { return mTime - mTimeSubMax; }
  GPUdi() short getSide() const { return mSide; }
  GPUdi() float getZShift() const { return mZShift; }
  GPUdi() void setZShift(float z) { mZShift = z; }

  GPUdi() bool CheckNumericalQuality() const { return true; }

  typedef GPUTRDO2BaseTrack baseClass;

 private:
  o2::dataformats::GlobalTrackID mRefTPC; // reference on TPC track entry in its original container
  o2::dataformats::GlobalTrackID mRefITS; // reference on ITS track entry in its original container
  float mTime{-1.f};                      // time estimate for this track in us
  float mTimeAddMax{0.f};                 // max. time that can be added to this track in us
  float mTimeSubMax{0.f};                 // max. time that can be subtracted to this track in us
  short mSide{0};                         // -1 : A-side, +1 : C-side (relevant only for TPC-only tracks)
  float mZShift{0.f};                     // calculated new for each TRD trigger candidate for this track
};

template <>
class propagatorInterface<o2::base::Propagator>
{
 public:
  typedef o2::base::Propagator propagatorParam;
  GPUd() propagatorInterface<o2::base::Propagator>(const propagatorParam* prop) : mProp(prop){};
  GPUd() propagatorInterface<o2::base::Propagator>(const propagatorInterface<o2::base::Propagator>&) = delete;
  GPUd() propagatorInterface<o2::base::Propagator>& operator=(const propagatorInterface<o2::base::Propagator>&) = delete;

  GPUdi() bool propagateToX(float x, float maxSnp, float maxStep) { return mProp->PropagateToXBxByBz(*mParam, x, maxSnp, maxStep); }
  GPUdi() int getPropagatedYZ(float x, float& projY, float& projZ) { return static_cast<int>(mParam->getYZAt(x, mProp->getNominalBz(), projY, projZ)); }

  GPUdi() void setTrack(trackInterface<GPUTRDO2BaseTrack>* trk) { mParam = trk; }
  GPUdi() void setFitInProjections(bool flag) {}

  GPUdi() float getAlpha() { return (mParam) ? mParam->getAlpha() : 99999.f; }
  GPUdi() bool update(const My_Float p[2], const My_Float cov[3])
  {
    if (mParam) {
      gpustd::array<float, 2> pTmp = {p[0], p[1]};
      gpustd::array<float, 3> covTmp = {cov[0], cov[1], cov[2]};
      return mParam->update(pTmp, covTmp);
    } else {
      return false;
    }
  }
  GPUdi() float getPredictedChi2(const My_Float p[2], const My_Float cov[3])
  {
    if (mParam) {
      gpustd::array<float, 2> pTmp = {p[0], p[1]};
      gpustd::array<float, 3> covTmp = {cov[0], cov[1], cov[2]};
      return mParam->getPredictedChi2(pTmp, covTmp);
    } else {
      return 99999.f;
    }
  }
  GPUdi() bool rotate(float alpha) { return (mParam) ? mParam->rotate(alpha) : false; }

  trackInterface<GPUTRDO2BaseTrack>* mParam{nullptr};
  const o2::base::Propagator* mProp;
};

} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif // HAVE_O2HEADERS

#include "GPUTPCGMPropagator.h"
#include "GPUParam.h"
#include "GPUDef.h"
#ifdef GPUCA_O2_LIB
#include "DataFormatsTPC/TrackTPC.h"
#endif

namespace GPUCA_NAMESPACE
{
namespace gpu
{

template <>
class trackInterface<GPUTPCGMTrackParam> : public GPUTPCGMTrackParam
{
 public:
  GPUdDefault() trackInterface<GPUTPCGMTrackParam>() = default;
  GPUd() trackInterface<GPUTPCGMTrackParam>(const GPUTPCGMTrackParam& param) CON_DELETE;
  GPUd() trackInterface<GPUTPCGMTrackParam>(const GPUTPCGMMergedTrack& trk) : GPUTPCGMTrackParam(trk.GetParam()), mAlpha(trk.GetAlpha()) {}
  GPUd() trackInterface<GPUTPCGMTrackParam>(const GPUTPCGMTrackParam::GPUTPCOuterParam& param) : GPUTPCGMTrackParam(), mAlpha(param.alpha)
  {
    SetX(param.X);
    for (int i = 0; i < 5; i++) {
      SetPar(i, param.P[i]);
    }
    for (int i = 0; i < 15; i++) {
      SetCov(i, param.C[i]);
    }
  };
#ifdef GPUCA_NOCOMPAT
  GPUdDefault() trackInterface<GPUTPCGMTrackParam>(const trackInterface<GPUTPCGMTrackParam>& param) = default;
  GPUdDefault() trackInterface<GPUTPCGMTrackParam>& operator=(const trackInterface<GPUTPCGMTrackParam>& param) = default;
#endif
#ifdef GPUCA_ALIROOT_LIB
  trackInterface<GPUTPCGMTrackParam>(const AliHLTExternalTrackParam& param) : GPUTPCGMTrackParam(), mAlpha(param.fAlpha)
  {
    SetX(param.fX);
    SetPar(0, param.fY);
    SetPar(1, param.fZ);
    SetPar(2, param.fSinPhi);
    SetPar(3, param.fTgl);
    SetPar(4, param.fq1Pt);
    for (int i = 0; i < 15; i++) {
      SetCov(i, param.fC[i]);
    }
  };
#endif
#if defined(GPUCA_O2_LIB) && !defined(GPUCA_GPUCODE)
  trackInterface<GPUTPCGMTrackParam>(const o2::dataformats::TrackTPCITS& param, float) : GPUTPCGMTrackParam(), mAlpha(param.getParamOut().getAlpha())
  {
    SetX(param.getParamOut().getX());
    SetPar(0, param.getParamOut().getY());
    SetPar(1, param.getParamOut().getZ());
    SetPar(2, param.getParamOut().getSnp());
    SetPar(3, param.getParamOut().getTgl());
    SetPar(4, param.getParamOut().getQ2Pt());
    for (int i = 0; i < 15; i++) {
      SetCov(i, param.getParamOut().getCov()[i]);
    }
  }
  trackInterface<GPUTPCGMTrackParam>(const o2::tpc::TrackTPC& param, float, float, unsigned int) : GPUTPCGMTrackParam(), mAlpha(param.getParamOut().getAlpha())
  {
    SetX(param.getParamOut().getX());
    SetPar(0, param.getParamOut().getY());
    SetPar(1, param.getParamOut().getZ());
    SetPar(2, param.getParamOut().getSnp());
    SetPar(3, param.getParamOut().getTgl());
    SetPar(4, param.getParamOut().getQ2Pt());
    for (int i = 0; i < 15; i++) {
      SetCov(i, param.getParamOut().getCov()[i]);
    }
  }
#endif

  GPUd() float getX() const
  {
    return GetX();
  }
  GPUd() float getAlpha() const { return mAlpha; }
  GPUd() float getY() const { return GetY(); }
  GPUd() float getZ() const { return GetZ(); }
  GPUd() float getSnp() const { return GetSinPhi(); }
  GPUd() float getTgl() const { return GetDzDs(); }
  GPUd() float getQ2Pt() const { return GetQPt(); }
  GPUd() float getEta() const { return -CAMath::Log(CAMath::Tan(0.5f * (0.5f * M_PI - CAMath::ATan(getTgl())))); }
  GPUd() float getPt() const { return CAMath::Abs(getQ2Pt()) > 0 ? CAMath::Abs(1.f / getQ2Pt()) : 99999.f; }
  GPUd() float getSigmaY2() const { return GetErr2Y(); }
  GPUd() float getSigmaZ2() const { return GetErr2Z(); }

  GPUd() const float* getPar() const { return GetPar(); }
  GPUd() const float* getCov() const { return GetCov(); }
  GPUd() float getTime() const { return -1.f; }
  GPUd() float getTimeMax() const { return 0.f; }
  GPUd() float getTimeMin() const { return 0.f; }
  GPUd() short getSide() const { return 0; }
  GPUd() void setZShift(float) {}
  GPUd() float getZShift() const { return 0.f; }
  GPUd() void resetCovariance(float s) { ResetCovariance(); }
  GPUd() void setAlpha(float alpha) { mAlpha = alpha; }
  GPUd() void set(float x, float alpha, const float param[5], const float cov[15])
  {
    SetX(x);
    for (int i = 0; i < 5; i++) {
      SetPar(i, param[i]);
    }
    for (int j = 0; j < 15; j++) {
      SetCov(j, cov[j]);
    }
    setAlpha(alpha);
  }

  typedef GPUTPCGMTrackParam baseClass;

 private:
  float mAlpha = 0.f;
};

template <>
class propagatorInterface<GPUTPCGMPropagator> : public GPUTPCGMPropagator
{
 public:
  typedef GPUTPCGMPolynomialField propagatorParam;
  GPUd() propagatorInterface<GPUTPCGMPropagator>(const propagatorParam* pField) : GPUTPCGMPropagator(), mTrack(nullptr)
  {
    this->SetMaterialTPC();
    this->SetPolynomialField(pField);
    this->SetMaxSinPhi(GPUCA_MAX_SIN_PHI);
    this->SetToyMCEventsFlag(0);
    this->SetFitInProjections(0);
    this->SelectFieldRegion(GPUTPCGMPropagator::TRD);
  };
  propagatorInterface<GPUTPCGMPropagator>(const propagatorInterface<GPUTPCGMPropagator>&) CON_DELETE;
  propagatorInterface<GPUTPCGMPropagator>& operator=(const propagatorInterface<GPUTPCGMPropagator>&) CON_DELETE;
  GPUd() void setTrack(trackInterface<GPUTPCGMTrackParam>* trk)
  {
    SetTrack(trk, trk->getAlpha());
    mTrack = trk;
  }
  GPUd() bool propagateToX(float x, float maxSnp, float maxStep)
  {
    //bool ok = PropagateToXAlpha(x, GetAlpha(), true) == 0 ? true : false;
    int retVal = PropagateToXAlpha(x, GetAlpha(), true);
    bool ok = (retVal == 0) ? true : false;
    ok = mTrack->CheckNumericalQuality();
    return ok;
  }
  GPUd() int getPropagatedYZ(float x, float& projY, float& projZ) { return GetPropagatedYZ(x, projY, projZ); }
  GPUd() void setFitInProjections(bool flag) { SetFitInProjections(flag); }
  GPUd() bool rotate(float alpha)
  {
    if (RotateToAlpha(alpha) == 0) {
      mTrack->setAlpha(alpha);
      return mTrack->CheckNumericalQuality();
    }
    return false;
  }
  GPUd() bool update(const My_Float p[2], const My_Float cov[3])
  {
    // TODO sigma_yz not taken into account yet, is not zero due to pad tilting!
    return Update(p[0], p[1], 0, false, cov[0], cov[2]) == 0 ? true : false;
  }
  GPUd() float getAlpha() { return GetAlpha(); }
  // TODO sigma_yz not taken into account yet, is not zero due to pad tilting!
  GPUd() float getPredictedChi2(const My_Float p[2], const My_Float cov[3]) const { return PredictChi2(p[0], p[1], cov[0], cov[2]); }

  trackInterface<GPUTPCGMTrackParam>* mTrack;
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif // GPUTRDINTERFACES_H
