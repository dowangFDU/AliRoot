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

/// \file GPUDisplay.h
/// \author David Rohr

#ifndef GPUDISPLAY_H
#define GPUDISPLAY_H

#ifdef GPUCA_BUILD_EVENT_DISPLAY

// GL EXT must be the first header
#include "GPUDisplayExt.h"

// Runtime minimum version defined in GPUDisplayBackend.h, keep in sync!
#if !defined(GL_VERSION_4_5) || GL_VERSION_4_5 != 1
#ifdef GPUCA_STANDALONE
#error Unsupported OpenGL version < 4.5
#elif defined(GPUCA_O2_LIB)
#pragma message "Unsupported OpenGL version < 4.5, disabling standalone event display"
#else
#warning Unsupported OpenGL version < 4.5, disabling standalone event display
#endif
#undef GPUCA_BUILD_EVENT_DISPLAY
#endif
#endif

#include "GPUSettings.h"
#include "GPUDisplayBackend.h"

#ifndef GPUCA_BUILD_EVENT_DISPLAY

namespace GPUCA_NAMESPACE
{
namespace gpu
{
class GPUDisplay
{
 public:
  GPUDisplay(void* backend, void* chain, void* qa, const void* param = nullptr, const void* calib = nullptr, const void* config = nullptr) {}
  ~GPUDisplay() = default;
  GPUDisplay(const GPUDisplay&) = delete;

  int StartDisplay() { return 1; }
  void ShowNextEvent(const GPUTrackingInOutPointers* ptrs = nullptr) {}
  void WaitForNextEvent() {}
  void SetCollisionFirstCluster(unsigned int collision, int slice, int cluster) {}

  void HandleKeyRelease(unsigned char key) {}
  int DrawGLScene(bool mixAnimation = false, float mAnimateTime = -1.f) { return 1; }
  void HandleSendKey(int key) {}
  int InitGL(bool initFailure = false) { return 1; }
  void ExitGL() {}
  void ReSizeGLScene(int width, int height, bool init = false) {}
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#else

#include "GPUChainTracking.h"
#include "../utils/vecpod.h"
#include "../utils/qsem.h"

#include <GL/gl.h>
#include <array>
#include "HandMadeMath.h"

#include "utils/timer.h"

namespace GPUCA_NAMESPACE
{
namespace gpu
{
class GPUTPCTracker;
struct GPUParam;
class GPUQA;

class GPUDisplay
{
 public:
  GPUDisplay(GPUDisplayBackend* backend, GPUChainTracking* chain, GPUQA* qa, const GPUParam* param = nullptr, const GPUCalibObjectsConst* calib = nullptr, const GPUSettingsDisplay* config = nullptr);
  ~GPUDisplay() = default;
  GPUDisplay(const GPUDisplay&) = delete;

  int StartDisplay();
  void ShowNextEvent(const GPUTrackingInOutPointers* ptrs = nullptr);
  void WaitForNextEvent();
  void SetCollisionFirstCluster(unsigned int collision, int slice, int cluster);

  void HandleKeyRelease(unsigned char key);
  int DrawGLScene(bool mixAnimation = false, float mAnimateTime = -1.f);
  void HandleSendKey(int key);
  int InitGL(bool initFailure = false);
  void ExitGL();
  void ReSizeGLScene(int width, int height, bool init = false);

 private:
  static constexpr int NSLICES = GPUChainTracking::NSLICES;

  static constexpr const int N_POINTS_TYPE = 11;
  static constexpr const int N_POINTS_TYPE_TPC = 9;
  static constexpr const int N_POINTS_TYPE_TRD = 2;
  static constexpr const int N_LINES_TYPE = 7;
  static constexpr const int N_FINAL_TYPE = 4;
  static constexpr int TRACK_TYPE_ID_LIMIT = 100;
  enum PointTypes { tCLUSTER = 0,
                    tINITLINK = 1,
                    tLINK = 2,
                    tSEED = 3,
                    tTRACKLET = 4,
                    tSLICETRACK = 5,
                    tGLOBALTRACK = 6,
                    tFINALTRACK = 7,
                    tMARKED = 8,
                    tTRDCLUSTER = 9,
                    tTRDATTACHED = 10 };
  enum LineTypes { RESERVED = 0 /*1 -- 6 = INITLINK to GLOBALTRACK*/ };

  typedef std::tuple<GLsizei, GLsizei, int> vboList;
  struct GLvertex {
    GLfloat x, y, z;
    GLvertex(GLfloat a, GLfloat b, GLfloat c) : x(a), y(b), z(c) {}
  };

  struct DrawArraysIndirectCommand {
    DrawArraysIndirectCommand(unsigned int a = 0, unsigned int b = 0, unsigned int c = 0, unsigned int d = 0) : count(a), instanceCount(b), first(c), baseInstance(d) {}
    unsigned int count;
    unsigned int instanceCount;

    unsigned int first;
    unsigned int baseInstance;
  };

  struct GLfb {
    GLuint fb_id = 0, fbCol_id = 0, fbDepth_id = 0;
    bool tex = false;
    bool msaa = false;
    bool depth = false;
    bool created = false;
  };

  struct threadVertexBuffer {
    vecpod<GLvertex> buffer;
    vecpod<GLint> start[N_FINAL_TYPE];
    vecpod<GLsizei> count[N_FINAL_TYPE];
    std::pair<vecpod<GLint>*, vecpod<GLsizei>*> vBuf[N_FINAL_TYPE];
    threadVertexBuffer() : buffer()
    {
      for (int i = 0; i < N_FINAL_TYPE; i++) {
        vBuf[i].first = start + i;
        vBuf[i].second = count + i;
      }
    }
    void clear()
    {
      for (int i = 0; i < N_FINAL_TYPE; i++) {
        start[i].clear();
        count[i].clear();
      }
    }
  };

  class opengl_spline
  {
   public:
    opengl_spline() : ma(), mb(), mc(), md(), mx() {}
    void create(const vecpod<float>& x, const vecpod<float>& y);
    float evaluate(float x);
    void setVerbose() { mVerbose = true; }

   private:
    vecpod<float> ma, mb, mc, md, mx;
    bool mVerbose = false;
  };

  int DrawGLScene_internal(bool mixAnimation, float mAnimateTime);
  int InitGL_internal();
  int getNumThreads();
  void disableUnsupportedOptions();
  const GPUTPCTracker& sliceTracker(int iSlice);
  const GPUTRDTrackerGPU& trdTracker();
  const GPUTRDGeometry& trdGeometry();
  const GPUTrackingInOutPointers* mIOPtrs = nullptr;
  void drawVertices(const vboList& v, const GLenum t);
  void insertVertexList(std::pair<vecpod<GLint>*, vecpod<GLsizei>*>& vBuf, size_t first, size_t last);
  void insertVertexList(int iSlice, size_t first, size_t last);
  template <typename... Args>
  void SetInfo(Args... args)
  {
    snprintf(mInfoText2, 1024, args...);
    GPUInfo("%s", mInfoText2);
    mInfoText2Timer.ResetStart();
  }
  void PrintGLHelpText(float colorValue);
  void calcXYZ(const float*);
  void mAnimationCloseAngle(float& newangle, float lastAngle);
  void mAnimateCloseQuaternion(float* v, float lastx, float lasty, float lastz, float lastw);
  void setAnimationPoint();
  void resetAnimation();
  void removeAnimationPoint();
  void startAnimation();
  void showInfo(const char* info);
  void ActivateColor();
  void SetColorTRD();
  void SetColorClusters();
  void SetColorInitLinks();
  void SetColorLinks();
  void SetColorSeeds();
  void SetColorTracklets();
  void SetColorTracks();
  void SetColorGlobalTracks();
  void SetColorFinal();
  void SetColorGrid();
  void SetColorGridTRD();
  void SetColorMarked();
  void SetCollisionColor(int col);
  void setQuality();
  void setDepthBuffer();
  void createFB_texture(GLuint& id, bool msaa, GLenum storage, GLenum attachment);
  void createFB_renderbuffer(GLuint& id, bool msaa, GLenum storage, GLenum attachment);
  void createFB(GLfb& fb, bool tex, bool withDepth, bool msaa);
  void deleteFB(GLfb& fb);
  void setFrameBuffer(int updateCurrent = -1, GLuint newID = 0);
  void UpdateOffscreenBuffers(bool clean = false);
  void updateConfig();
  void drawPointLinestrip(int iSlice, int cid, int id, int id_limit = TRACK_TYPE_ID_LIMIT);
  vboList DrawClusters(int iSlice, int select, int iCol);
  vboList DrawSpacePointsTRD(int iSlice, int select, int iCol);
  vboList DrawSpacePointsTRD(const GPUTPCTracker& tracker, int select, int iCol);
  vboList DrawLinks(const GPUTPCTracker& tracker, int id, bool dodown = false);
  vboList DrawSeeds(const GPUTPCTracker& tracker);
  vboList DrawTracklets(const GPUTPCTracker& tracker);
  vboList DrawTracks(const GPUTPCTracker& tracker, int global);
  template <class T>
  void DrawFinal(int iSlice, int /*iCol*/, GPUTPCGMPropagator* prop, std::array<vecpod<int>, 2>& trackList, threadVertexBuffer& threadBuffer);
  vboList DrawGrid(const GPUTPCTracker& tracker);
  vboList DrawGridTRD(int sector);
  void DoScreenshot(char* filename, float mAnimateTime = -1.f);
  void PrintHelp();
  void createQuaternionFromMatrix(float* v, const float* mat);

  unsigned int mVertexShader;
  unsigned int mFragmentShader;
  unsigned int mShaderProgram;
  unsigned int mVertexArray;
  int mModelViewProjId;
  int mColorId;

  GPUDisplayBackend* mBackend;
  GPUChainTracking* mChain;
  const GPUParam* mParam;
  const GPUCalibObjectsConst* mCalib;
  const GPUSettingsDisplay& mConfig;
  GPUSettingsDisplayLight mCfg;
  GPUQA* mQA;
  qSem mSemLockDisplay;

  GLfb mMixBuffer;

  GLuint mVBOId[NSLICES], mIndirectId;
  int mIndirectSliceOffset[NSLICES];
  vecpod<GLvertex> mVertexBuffer[NSLICES];
  vecpod<GLint> mVertexBufferStart[NSLICES];
  vecpod<GLsizei> mVertexBufferCount[NSLICES];
  vecpod<GLuint> mMainBufferStack{0};

  int mNDrawCalls = 0;
  bool mUseGLIndirectDraw = true;
  bool mUseMultiVBO = false;

  std::array<float, 3> mDrawColor = {};
  bool mInvertColors = false;
  const int mDrawQualityRenderToTexture = 1;
  int mDrawQualityMSAA = 0;
  int mDrawQualityDownsampleFSAA = 0;
  bool mDrawQualityVSync = false;
  bool mMaximized = true;
  bool mFullScreen = false;

  int mTestSetting = 0;

  bool mCamLookOrigin = false;
  bool mCamYUp = false;
  int mCameraMode = 0;
  bool mOpenGLCore = false;
  float mFOV = 45.f;

  float mAngleRollOrigin = -1e9;
  float mMaxClusterZ = -1;

  int screenshot_scale = 1;

  int mScreenwidth = GPUDisplayBackend::INIT_WIDTH, mScreenheight = GPUDisplayBackend::INIT_HEIGHT;
  int mRenderwidth = GPUDisplayBackend::INIT_WIDTH, mRenderheight = GPUDisplayBackend::INIT_HEIGHT;

  bool mSeparateGlobalTracks = 0;
  bool mPropagateLoopers = 0;

  hmm_mat4 mViewMatrix, mModelMatrix;
  float* const mViewMatrixP = &mViewMatrix.Elements[0][0];
  float mXYZ[3];
  float mAngle[3];
  float mRPhiTheta[3];
  float mQuat[4];

  int mProjectXY = 0;

  int mMarkClusters = 0;
  int mHideRejectedClusters = 1;
  int mHideUnmatchedClusters = 0;
  int mHideRejectedTracks = 1;
  int mMarkAdjacentClusters = 0;
  int mMarkFakeClusters = 0;
  int mTrackFilter = 0;

  vecpod<std::array<int, 37>> mCollisionClusters;
  int mNCollissions = 1;

  float mXadd = 0;
  float mZadd = 0;

  std::unique_ptr<float4[]> mGlobalPosPtr;
  std::unique_ptr<float4[]> mGlobalPosPtrTRD;
  std::unique_ptr<float4[]> mGlobalPosPtrTRD2;
  float4* mGlobalPos;
  float4* mGlobalPosTRD;
  float4* mGlobalPosTRD2;
  int mNMaxClusters = 0;
  int mNMaxSpacePointsTRD = 0;
  int mCurrentClusters = 0;
  int mCurrentSpacePointsTRD = 0;
  std::vector<int> mTRDTrackIds;

  int mGlDLrecent = 0;
  int mUpdateDLList = 0;

  int mAnimate = 0;
  HighResTimer mAnimationTimer;
  int mAnimationFrame = 0;
  int mAnimationLastBase = 0;
  int mAnimateScreenshot = 0;
  int mAnimationExport = 0;
  bool mAnimationChangeConfig = true;
  float mAnimationDelay = 2.f;
  vecpod<float> mAnimateVectors[9];
  vecpod<GPUSettingsDisplayLight> mAnimateConfig;
  opengl_spline mAnimationSplines[8];

  volatile int mResetScene = 0;

  int mPrintInfoText = 1;
  char mInfoText2[1024];
  HighResTimer mInfoText2Timer, mInfoHelpTimer;

  GLfb mOffscreenBuffer, mOffscreenBufferNoMSAA;
  std::vector<threadVertexBuffer> mThreadBuffers;
  std::vector<std::vector<std::array<std::array<vecpod<int>, 2>, NSLICES>>> mThreadTracks;
  volatile int mInitResult = 0;

  float mFPSScale = 1, mFPSScaleadjust = 0;
  int mFramesDone = 0, mFramesDoneFPS = 0;
  HighResTimer mTimerFPS, mTimerDisplay, mTimerDraw;
  vboList mGlDLLines[NSLICES][N_LINES_TYPE];
  vecpod<std::array<vboList, N_FINAL_TYPE>> mGlDLFinal[NSLICES];
  vecpod<vboList> mGlDLPoints[NSLICES][N_POINTS_TYPE];
  vboList mGlDLGrid[NSLICES];
  vboList mGlDLGridTRD[NSLICES / 2];
  vecpod<DrawArraysIndirectCommand> mCmdBuffer;
};
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
#endif
