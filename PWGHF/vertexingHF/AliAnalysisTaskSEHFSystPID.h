#ifndef ALIANALYSISTASKSEHFSYSTNSIGMAPID_H
#define ALIANALYSISTASKSEHFSYSTNSIGMAPID_H

/* Copyright(c) 1998-2008, ALICE Experiment at CERN, All rights reserved. */

/////////////////////////////////////////////////////////////////////////////////////////
// \class AliAnalysisTaskSEHFSystPID                                                   //
// \brief analysis task for the study of PID systematic uncertainties of HF particles  //
// \author: A. M. Barbano, anastasia.maria.barbano@cern.ch                             //
// \author: F. Grosa, fabrizio.grosa@cern.ch                                           //
/////////////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>

#include "AliAnalysisTaskSE.h"
#include "AliAODv0KineCuts.h"
#include "AliAODTrack.h"
#include "AliAODEvent.h"
#include "AliESDtrackCuts.h"
#include "AliPIDResponse.h"
#include "AliAODPidHF.h"
#include "AliEventCuts.h"

using namespace std;

class AliAnalysisTaskSEHFSystPID : public AliAnalysisTaskSE {
  
public:

  enum tagflags {
    kIsPionFromK0s       = BIT(0),
    kIsPionFromL         = BIT(1),
    kIsProtonFromL       = BIT(2),
    kIsElectronFromGamma = BIT(3),
    kIsKaonFromKinks     = BIT(4),
    kIsKaonFromTOF       = BIT(5),
    kIsKaonFromTPC       = BIT(6),
    kPositiveTrack       = BIT(12),
    kNegativeTrack       = BIT(13),
    kHasNoTPC            = BIT(14),
    kHasNoTOF            = BIT(15)
  };

  enum trackinfo {
    kHasSPDAny   = BIT(0),
    kHasSPDFirst = BIT(1),
    kHasITSrefit = BIT(2),
    kHasTPCrefit = BIT(3),
    kPassGeomCut = BIT(4)
  };

  enum centest {
    kCentOff,
    kCentV0M,
    kCentV0A,
    kCentZNA,
    kCentCL0,
    kCentCL1
  };

  AliAnalysisTaskSEHFSystPID();
  AliAnalysisTaskSEHFSystPID(const char *name, int system=0);
  virtual ~AliAnalysisTaskSEHFSystPID();

  virtual void   UserCreateOutputObjects();
  virtual void   UserExec(Option_t *option);

  void SetReadMC(bool flag = true)                                            {fIsMC = flag;}
  void SetCentralityLimits(int mincent, int maxcent)                          {fCentMin = mincent; fCentMax = maxcent;}
  void SetCentralityEstimator(int centest=kCentV0M)                           {fCentEstimator=centest;}
  void SetESDtrackCuts(AliESDtrackCuts * trackCuts)                           {fESDtrackCuts = trackCuts;}
  void SetTriggerInfo(TString trigClass, unsigned long long mask=0)           {fTriggerClass = trigClass; fTriggerMask = mask;}
  void SetNsigmaKaonForTagging(float nsigmamax = 0.02)                        {fNsigmaMaxForTag=nsigmamax;}
  void SetKinksSelections(float qtmin=0.15, float Rmin=120, float Rmax=210)   {fQtMinKinks=qtmin; fRMinKinks=Rmin; fRMaxKinks=Rmax;}
  void SetfFillTreeWithPIDInfo(bool fillPID=true)                             {fFillTreeWithPIDInfo=fillPID;}
  void SetfFillTreeWithNsigmaPIDOnly(bool fillonlyNsigma=true)                {fFillTreeWithNsigmaPIDOnly=fillonlyNsigma;}
  void SetfFillTreeWithTrackQualityInfo(bool fillTrack=true)                  {fFillTreeWithTrackQualityInfo=fillTrack;}
  void EnableDownSampling(double fractokeep=0.1, double ptmax=1.5, int opt=0) {fEnabledDownSampling=true; fFracToKeepDownSampling=fractokeep; fPtMaxDownSampling=ptmax; fDownSamplingOpt=opt;}
  void SetAODMismatchProtection(int opt=1)                                    {fAODProtection=opt;}
  void SetDownSamplingOption(int opt=0)                                       {fDownSamplingOpt=opt;}

  void EnableNsigmaDataDrivenCorrection(int syst)                             {fEnableNsigmaTPCDataCorr=true; fSystNsigmaTPCDataCorr=syst;}

  void EnableSelectionWithAliEventCuts(bool useAliEventCuts=true, int opt=2)  {fUseAliEventCuts=useAliEventCuts; fApplyPbPbOutOfBunchPileupCuts=opt;}

  void ConfigureCutGeoNcrNcl(double dz, double len, double onept, double fncr, double fncl) {
    fDeadZoneWidth=dz;  
    fCutGeoNcrNclLength=len; 
    fCutGeoNcrNclGeom1Pt=onept;
    fCutGeoNcrNclFractionNcr=fncr; 
    fCutGeoNcrNclFractionNcl=fncl;
  }

private:

  bool IsVertexAccepted();
  bool IsCentralitySelected();
  void GetTaggedV0s(vector<short> &idPionFromK0s, vector<short> &idPionFromL, vector<short> &idProtonFromL, vector<short> &idElectronFromGamma);
  short GetPDGcodeFromMC(AliAODTrack* track, TClonesArray* arrayMC);
  AliAODTrack* IsKinkDaughter(AliAODTrack* track);
  void GetTaggedKaonsFromKinks(vector<short> &idKaonFromKinks);
  float MaxOpeningAngleKnu(float p);
  float GetTOFmomentum(AliAODTrack* track);
  short ConvertFloatToShort(float num);
  unsigned short ConvertFloatToUnsignedShort(float num);
  void GetNsigmaTPCMeanSigmaData(float &mean, float &sigma, AliPID::EParticleType species, float pTPC, float eta);
  void SetNsigmaTPCDataCorr(int run);
  int IsEventSelectedWithAliEventCuts();
  bool IsSelectedByGeometricalCut(AliAODTrack* track);

  enum hypos{kPion,kKaon,kProton};
  static const int kNHypo = 3;
  const TString hyponames[kNHypo] = {"Pion","Kaon","Proton"};

  const float kCSPEED = 2.99792457999999984e-02; // cm / ps

  TList *fOutputList;                                                                //!<! output list for histograms

  TH1F *fHistNEvents;                                                                //!<! histo with number of events
  TH2F *fHistArmenteroPlot[5];                                                       //!<! histo for armenteros-podolanski plot
  TH2F *fHistQtVsMassKinks;                                                          //!<! histo for mother-kink qt vs. mass distribution
  TH2F *fHistPDaughterVsMotherKink;                                                  //!<! histo for pT daughter vs. pT mother kink
  TH2F *fHistdEdxVsPMotherKink;                                                      //!<! histo for mother kink TPC dEdx vs. p
  TH2F *fHistOpeningAngleVsPMotherKink;                                              //!<! histo for opening angle vs. pT mother kink
  TH2F *fHistNTPCclsVsRadius;                                                        //!<! histo for nTPC clusters vs. R mother kink
  TH2F *fHistNsigmaTPCvsPt[kNHypo];                                                  //!<! array of histos for nsigmaTPC vs pt (MC truth)
  TH2F *fHistNsigmaTOFvsPt[kNHypo];                                                  //!<! array of histos for nsigmaTPC vs pt (MC truth)
  TTree* fPIDtree;                                                                   //!<! tree with PID info

  short fPIDNsigma[6];                                                               /// Nsigma PID to fill the tree
  unsigned short fPTPC;                                                              /// TPC momentum to fill the tree
  unsigned short fPTOF;                                                              /// TOF momentum to fill the tree
  unsigned short fdEdxTPC;                                                           /// TPC dEdX to fill the tree
  unsigned short fToF;                                                               /// ToF signal to fill the tree
  unsigned short fPt;                                                                /// transverse momentum to fill the tree
  unsigned char fTPCNcls;                                                            /// number of clusters in TPC to fill the tree
  unsigned char fTPCNclsPID;                                                         /// number of PID clusters in TPC to fill the tree
  unsigned short fTrackLength;                                                       /// track length for TOF PID
  unsigned short fStartTimeRes;                                                      /// start time resolution for TOF PID
  unsigned short fTPCNcrossed;                                                       /// number of TPC crossed rows
  unsigned short fTPCFindable;                                                       /// number of TPC findable clusters
  unsigned char fTrackInfoMap;                                                       /// bit map with some track info (see enum above)
  short fEta;                                                                        /// pseudorapidity of the track
  unsigned short fPhi;                                                               /// azimuthal angle of the track
  short fPDGcode;                                                                    /// PDG code in case of MC to fill the tree
  unsigned short fTag;                                                               /// bit map for tag (see enum above)
  float fNsigmaMaxForTag;                                                            /// max nSigma value to tag kaons
  float fQtMinKinks;                                                                 /// min qt for kinks
  float fRMinKinks;                                                                  /// min radius in XY for kinks
  float fRMaxKinks;                                                                  /// max radius in XY for kink
  float fDeadZoneWidth;                                                              /// 1st parameter geometrical cut
  float fCutGeoNcrNclLength;                                                         /// 2nd parameter geometrical cut
  float fCutGeoNcrNclGeom1Pt;                                                        /// 3rd parameter geometrical cut
  float fCutGeoNcrNclFractionNcr;                                                    /// 4th parameter geometrical cut
  float fCutGeoNcrNclFractionNcl;                                                    /// 5th parameter geometrical cut

  float fCentMin;                                                                    /// min centrality
  float fCentMax;                                                                    /// max centrality
  int fCentEstimator;                                                                /// centrality estimator
  TString fTriggerClass;                                                             /// trigger class
  unsigned long long fTriggerMask;                                                   /// trigger mask
  bool fIsMC;                                                                        /// flag to switch on the MC analysis for the efficiency estimation
  int fSystem;                                                                       /// system: 0->pp,pPb 1->PbPb

  AliESDtrackCuts * fESDtrackCuts;                                                   /// single-track cut set 
  AliAODEvent *fAOD;                                                                 /// AOD object
  AliPIDResponse *fPIDresp;                                                          /// basic pid object
  AliAODv0KineCuts *fV0cuts;                                                         /// AOD V0 cuts

  bool fFillTreeWithPIDInfo;                                                         /// flag to enable filling of the tree with PID variables
  bool fFillTreeWithNsigmaPIDOnly;                                                   /// flag to enable filling of the tree with only Nsigma variables for the PID
  bool fFillTreeWithTrackQualityInfo;                                                /// flag to enable filling of the tree with track selections
  bool fEnabledDownSampling;                                                         /// flag to enable/disable downsampling
  double fFracToKeepDownSampling;                                                    /// fraction to keep when downsampling activated
  double fPtMaxDownSampling;                                                         /// pT max of tracks to downsample
  int fDownSamplingOpt;                                                              /// option for downsampling 

  int fAODProtection;                                                                /// flag to activate protection against AOD-dAOD mismatch

  int fRunNumberPrevEvent;                                                           /// run number of previous event
  bool fEnableNsigmaTPCDataCorr;                                                     /// flag to enable data-driven NsigmaTPC correction
  int fSystNsigmaTPCDataCorr;                                                        /// system for data-driven NsigmaTPC correction
  vector<vector<float> > fMeanNsigmaTPCPionData;                                     /// array of NsigmaTPC pion mean in data 
  vector<vector<float> > fMeanNsigmaTPCKaonData;                                     /// array of NsigmaTPC kaon mean in data 
  vector<vector<float> > fMeanNsigmaTPCProtonData;                                   /// array of NsigmaTPC proton mean in data 
  vector<vector<float> > fSigmaNsigmaTPCPionData;                                    /// array of NsigmaTPC pion mean in data 
  vector<vector<float> > fSigmaNsigmaTPCKaonData;                                    /// array of NsigmaTPC kaon mean in data 
  vector<vector<float> > fSigmaNsigmaTPCProtonData;                                  /// array of NsigmaTPC proton mean in data 
  float fPlimitsNsigmaTPCDataCorr[AliAODPidHF::kMaxPBins+1];                         /// array of p limits for data-driven NsigmaTPC correction
  int fNPbinsNsigmaTPCDataCorr;                                                      /// number of p bins for data-driven NsigmaTPC correction
  float fEtalimitsNsigmaTPCDataCorr[AliAODPidHF::kMaxEtaBins+1];                     /// vector of eta limits for data-driven NsigmaTPC correction
  int fNEtabinsNsigmaTPCDataCorr;                                                    /// number of eta bins for data-driven NsigmaTPC correction
  bool fUseAliEventCuts;                                                             /// flag to enable usage of AliEventCuts foe event-selection
  AliEventCuts fAliEventCuts;                                                        /// event-cut object for centrality correlation event cuts
  int fApplyPbPbOutOfBunchPileupCuts;                                                /// option for Pb-Pb out-of bunch pileup cuts with AliEventCuts

  ClassDef(AliAnalysisTaskSEHFSystPID, 10);
};

#endif
