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

/// \file GPUTRDTrack.h
/// \author Ole Schmidt

#ifndef GPUTRDTRACK_H
#define GPUTRDTRACK_H

#include "GPUTRDDef.h"
#include "GPUDef.h"

struct GPUTRDTrackDataRecord;
class AliHLTExternalTrackParam;

namespace o2
{
namespace tpc
{
class TrackTPC;
}
namespace dataformats
{
class TrackTPCITS;
}
} // namespace o2

//_____________________________________________________________________________
#if (defined(__CINT__) || defined(__ROOTCINT__)) && !defined(__CLING__)
namespace GPUCA_NAMESPACE
{
namespace gpu
{
template <typename T>
class GPUTRDTrack_t;
}
} // namespace GPUCA_NAMESPACE
#else
#include "GPUTRDInterfaces.h"

namespace GPUCA_NAMESPACE
{
namespace gpu
{

template <typename T>
class GPUTRDTrack_t : public T
{
 public:
  enum EGPUTRDTrack { kNLayers = 6 };

  GPUd() GPUTRDTrack_t();
  GPUTRDTrack_t(const typename T::baseClass& t) = delete;
  GPUd() GPUTRDTrack_t(const GPUTRDTrack_t& t);
  GPUd() GPUTRDTrack_t(const AliHLTExternalTrackParam& t);
#ifndef GPUCA_GPUCODE
  GPUd() GPUTRDTrack_t(const o2::dataformats::TrackTPCITS& t, float vDrift);
  GPUd() GPUTRDTrack_t(const o2::tpc::TrackTPC& t, float tbWidth, float vDrift, unsigned int iTrk);
#endif
  GPUd() GPUTRDTrack_t(const T& t);
  GPUd() GPUTRDTrack_t& operator=(const GPUTRDTrack_t& t);

  GPUd() int GetNlayers() const;
  GPUd() int GetTracklet(int iLayer) const;
  GPUd() int GetTPCtrackId() const { return mTPCTrackId; }
  GPUd() int GetNtracklets() const { return mNTracklets; }
  GPUd() int GetNtrackletsOffline(int type) const { return mNTrackletsOffline[type]; }
  GPUd() int GetLabelOffline() const { return mLabelOffline; }
  GPUd() int GetLabel() const { return mLabel; }
  GPUd() float GetChi2() const { return mChi2; }
  GPUd() float GetReducedChi2() const { return GetNlayers() == 0 ? mChi2 : mChi2 / GetNlayers(); }
  GPUd() float GetMass() const { return mMass; }
  GPUd() bool GetIsStopped() const { return mIsStopped; }
  GPUd() bool GetIsFindable(int iLayer) const { return mIsFindable[iLayer]; }
  GPUd() int GetTrackletIndex(int iLayer) const { return GetTracklet(iLayer); }
  GPUd() int GetNmissingConsecLayers(int iLayer) const;

  GPUd() void AddTracklet(int iLayer, int idx)
  {
    mAttachedTracklets[iLayer] = idx;
    mNTracklets++;
  }
  GPUd() void SetTPCtrackId(int v) { mTPCTrackId = v; }
  GPUd() void SetNtracklets(int nTrklts) { mNTracklets = nTrklts; }
  GPUd() void SetIsFindable(int iLayer) { mIsFindable[iLayer] = true; }
  GPUd() void SetNtrackletsOffline(int type, int nTrklts) { mNTrackletsOffline[type] = nTrklts; }
  GPUd() void SetLabelOffline(int lab) { mLabelOffline = lab; }
  GPUd() void SetIsStopped() { mIsStopped = true; }

  GPUd() void SetChi2(float chi2) { mChi2 = chi2; }
  GPUd() void SetMass(float mass) { mMass = mass; }
  GPUd() void SetLabel(int label) { mLabel = label; }

  // conversion to / from HLT track structure

  GPUd() void ConvertTo(GPUTRDTrackDataRecord& t) const;
  GPUd() void ConvertFrom(const GPUTRDTrackDataRecord& t);

 protected:
  float mChi2;                      // total chi2
  float mMass;                      // mass hypothesis
  int mLabel;                       // MC label
  int mTPCTrackId;                  // corresponding TPC track
  int mNTracklets;                  // number of attached TRD tracklets
  int mNMissingConsecLayers;        // number of missing consecutive layers
  int mNTrackletsOffline[4];        // for debugging: attached offline TRD tracklets (0: total, 1: match, 2: related, 3: fake)
  int mLabelOffline;                // offline TRD MC label of this track
  int mAttachedTracklets[kNLayers]; // IDs for attached tracklets sorted by layer
  bool mIsFindable[kNLayers];       // number of layers where tracklet should exist
  bool mIsStopped;                  // track ends in TRD

 private:
  GPUd() void Initialize();
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif // !((defined(__CINT__) || defined(__ROOTCINT__)) && !defined(__CLING__))

#endif // GPUTRDTRACK_H
