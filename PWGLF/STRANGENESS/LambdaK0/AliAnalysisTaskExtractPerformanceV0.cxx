/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Modified version of AliAnalysisTaskCheckCascade.cxx.
// This is a 'hybrid' output version, in that it uses a classic TTree
// ROOT object to store the candidates, plus a couple of histograms filled on
// a per-event basis for storing variables too numerous to put in a tree. 
//
// --- Adapted to look for lambdas as well, using code from 
//        AliAnalysisTaskCheckPerformanceStrange.cxx
//
//  --- Algorithm Description 
//   1. Loop over primaries in stack to acquire generated charged Xi
//   2. Loop over stack to find V0s, fill TH3Fs "PrimRawPt"s for Efficiency
//   3. Perform Physics Selection
//   4. Perform Primary Vertex |z|<10cm selection
//   5. Perform Primary Vertex NoTPCOnly vertexing selection (>0 contrib.)
//   6. Perform Pileup Rejection
//   7. Analysis Loops: 
//    7a. Fill TH3Fs "PrimAnalysisPt" for control purposes only
//    7b. Fill TTree object with V0 information, candidates
//
//  Please Report Any Bugs! 
//
//   --- David Dobrigkeit Chinellato
//        (david.chinellato@gmail.com)
//
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

class TTree;
class TParticle;
class TVector3;

//class AliMCEventHandler;
//class AliMCEvent;
//class AliStack;

class AliESDVertex;
class AliAODVertex;
class AliESDv0;
class AliAODv0;

#include <Riostream.h>
#include "TList.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TFile.h"
#include "THnSparse.h"
#include "TVector3.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TLegend.h"
#include "AliLog.h"

#include "AliESDEvent.h"
#include "AliAODEvent.h"
#include "AliV0vertexer.h"
#include "AliCascadeVertexer.h"
#include "AliESDpid.h"
#include "AliESDtrack.h"
#include "AliESDtrackCuts.h"
#include "AliInputEventHandler.h"
#include "AliAnalysisManager.h"
#include "AliMCEventHandler.h"
#include "AliMCEvent.h"
#include "AliStack.h"

#include "AliCFContainer.h"
#include "AliMultiplicity.h"
#include "AliAODMCParticle.h"
#include "AliESDcascade.h"
#include "AliAODcascade.h"
#include "AliESDUtils.h"
#include "AliGenEventHeader.h"

#include "AliAnalysisTaskExtractPerformanceV0.h"

using std::cout;
using std::endl;

ClassImp(AliAnalysisTaskExtractPerformanceV0)

AliAnalysisTaskExtractPerformanceV0::AliAnalysisTaskExtractPerformanceV0() 
  : AliAnalysisTaskSE(), fListHistV0(0), fTree(0), fTreeCascade(0), fPIDResponse(0),
   fkIsNuclear   ( kFALSE ), 
   fkLowEnergyPP ( kFALSE ),
   fkUseOnTheFly ( kFALSE ),

//------------------------------------------------
// HISTOGRAMS
// --- Filled on an Event-by-event basis
//------------------------------------------------
   fHistV0MultiplicityBeforeTrigSel(0),
   fHistV0MultiplicityForTrigEvt(0), 
   fHistV0MultiplicityForSelEvt(0),
   fHistV0MultiplicityForSelEvtNoTPCOnly(0),
   fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup(0),
   fHistMultiplicityBeforeTrigSel(0),
   fHistMultiplicityForTrigEvt(0),
   fHistMultiplicity(0),
   fHistMultiplicityNoTPCOnly(0),
   fHistMultiplicityNoTPCOnlyNoPileup(0),

//------------------------------------------------
// PARTICLE HISTOGRAMS
// --- Filled on a Particle-by-Particle basis
//------------------------------------------------
   f3dHistPrimAnalysisPtVsYVsMultLambda(0),
   f3dHistPrimAnalysisPtVsYVsMultAntiLambda(0),
   f3dHistPrimAnalysisPtVsYVsMultK0Short(0),
   f3dHistPrimRawPtVsYVsMultLambda(0),
   f3dHistPrimRawPtVsYVsMultAntiLambda(0),
   f3dHistPrimRawPtVsYVsMultK0Short(0),
   f3dHistPrimCloseToPVPtVsYVsMultLambda(0),
   f3dHistPrimCloseToPVPtVsYVsMultAntiLambda(0),
   f3dHistPrimCloseToPVPtVsYVsMultK0Short(0),
   f3dHistPrimRawPtVsYVsDecayLengthLambda(0),
   f3dHistPrimRawPtVsYVsDecayLengthAntiLambda(0),
   f3dHistPrimRawPtVsYVsDecayLengthK0Short(0),
   f3dHistGenPtVsYVsMultXiMinus(0),
   f3dHistGenPtVsYVsMultXiPlus(0),
   f3dHistGenPtVsYVsMultOmegaMinus(0),
   f3dHistGenPtVsYVsMultOmegaPlus(0),
   f3dHistGenSelectedPtVsYVsMultXiMinus(0),
   f3dHistGenSelectedPtVsYVsMultXiPlus(0),
   f3dHistGenSelectedPtVsYVsMultOmegaMinus(0),
   f3dHistGenSelectedPtVsYVsMultOmegaPlus(0),
   fHistPVx(0),
   fHistPVy(0),
   fHistPVz(0),
   fHistPVxAnalysis(0),
   fHistPVyAnalysis(0),
   fHistPVzAnalysis(0),
   fHistPVxAnalysisHasHighPtLambda(0),
   fHistPVyAnalysisHasHighPtLambda(0),
   fHistPVzAnalysisHasHighPtLambda(0),
   fHistSwappedV0Counter(0)
{
  // Dummy Constructor
}

AliAnalysisTaskExtractPerformanceV0::AliAnalysisTaskExtractPerformanceV0(const char *name) 
  : AliAnalysisTaskSE(name), fListHistV0(0), fTree(0), fTreeCascade(0), fPIDResponse(0),
   fkIsNuclear   ( kFALSE ), 
   fkLowEnergyPP ( kFALSE ),
   fkUseOnTheFly ( kFALSE ),
     
//------------------------------------------------
// HISTOGRAMS
// --- Filled on an Event-by-event basis
//------------------------------------------------
   fHistV0MultiplicityBeforeTrigSel(0),
   fHistV0MultiplicityForTrigEvt(0), 
   fHistV0MultiplicityForSelEvt(0),
   fHistV0MultiplicityForSelEvtNoTPCOnly(0),
   fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup(0),
   fHistMultiplicityBeforeTrigSel(0),
   fHistMultiplicityForTrigEvt(0),
   fHistMultiplicity(0),
   fHistMultiplicityNoTPCOnly(0),
   fHistMultiplicityNoTPCOnlyNoPileup(0),


//------------------------------------------------
// PARTICLE HISTOGRAMS
// --- Filled on a Particle-by-Particle basis
//------------------------------------------------
   f3dHistPrimAnalysisPtVsYVsMultLambda(0),
   f3dHistPrimAnalysisPtVsYVsMultAntiLambda(0),
   f3dHistPrimAnalysisPtVsYVsMultK0Short(0),
   f3dHistPrimRawPtVsYVsMultLambda(0),
   f3dHistPrimRawPtVsYVsMultAntiLambda(0),
   f3dHistPrimRawPtVsYVsMultK0Short(0),
   f3dHistPrimCloseToPVPtVsYVsMultLambda(0),
   f3dHistPrimCloseToPVPtVsYVsMultAntiLambda(0),
   f3dHistPrimCloseToPVPtVsYVsMultK0Short(0),
   f3dHistPrimRawPtVsYVsDecayLengthLambda(0),
   f3dHistPrimRawPtVsYVsDecayLengthAntiLambda(0),
   f3dHistPrimRawPtVsYVsDecayLengthK0Short(0),
   f3dHistGenPtVsYVsMultXiMinus(0),
   f3dHistGenPtVsYVsMultXiPlus(0),
   f3dHistGenPtVsYVsMultOmegaMinus(0),
   f3dHistGenPtVsYVsMultOmegaPlus(0),
   f3dHistGenSelectedPtVsYVsMultXiMinus(0),
   f3dHistGenSelectedPtVsYVsMultXiPlus(0),
   f3dHistGenSelectedPtVsYVsMultOmegaMinus(0),
   f3dHistGenSelectedPtVsYVsMultOmegaPlus(0),
   fHistPVx(0),
   fHistPVy(0),
   fHistPVz(0),
   fHistPVxAnalysis(0),
   fHistPVyAnalysis(0),
   fHistPVzAnalysis(0),
   fHistPVxAnalysisHasHighPtLambda(0),
   fHistPVyAnalysisHasHighPtLambda(0),
   fHistPVzAnalysisHasHighPtLambda(0),
   fHistSwappedV0Counter(0)
{
   // Constructor
   // Output slot #0 writes into a TList container (Cascade)
   DefineOutput(1, TList::Class());
   DefineOutput(2, TTree::Class());
   DefineOutput(3, TTree::Class());
}


AliAnalysisTaskExtractPerformanceV0::~AliAnalysisTaskExtractPerformanceV0()
{
//------------------------------------------------
// DESTRUCTOR
//------------------------------------------------

   if (fListHistV0){
      delete fListHistV0;
      fListHistV0 = 0x0;
   }
   if (fTree){
      delete fTree;
      fTree = 0x0;
   }
}

//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0::UserCreateOutputObjects()
{

   OpenFile(2);	
   // Called once

//------------------------------------------------

   fTree = new TTree("fTree","V0Candidates");

//------------------------------------------------
// fTree Branch definitions - V0 Tree
//------------------------------------------------

//-----------BASIC-INFO---------------------------
/* 1*/   fTree->Branch("fTreeVariablePrimaryStatus",&fTreeVariablePrimaryStatus,"fTreeVariablePrimaryStatus/I");	
/* 1*/   fTree->Branch("fTreeVariablePrimaryStatusMother",&fTreeVariablePrimaryStatusMother,"fTreeVariablePrimaryStatusMother/I");	
/* 2*/   fTree->Branch("fTreeVariableChi2V0",&fTreeVariableChi2V0,"Chi2V0/F");
/* 3*/   fTree->Branch("fTreeVariableDcaV0Daughters",&fTreeVariableDcaV0Daughters,"fTreeVariableDcaV0Daughters/F");
/* 4*/   fTree->Branch("fTreeVariableDcaPosToPrimVertex",&fTreeVariableDcaPosToPrimVertex,"fTreeVariableDcaPosToPrimVertex/F");
/* 5*/   fTree->Branch("fTreeVariableDcaNegToPrimVertex",&fTreeVariableDcaNegToPrimVertex,"fTreeVariableDcaNegToPrimVertex/F");
/* 6*/   fTree->Branch("fTreeVariableV0Radius",&fTreeVariableV0Radius,"fTreeVariableV0Radius/F");
/* 7*/   fTree->Branch("fTreeVariablePt",&fTreeVariablePt,"fTreeVariablePt/F");
/* 7*/   fTree->Branch("fTreeVariablePtMC",&fTreeVariablePtMC,"fTreeVariablePtMC/F");
/* 8*/   fTree->Branch("fTreeVariableRapK0Short",&fTreeVariableRapK0Short,"fTreeVariableRapK0Short/F");
/* 9*/   fTree->Branch("fTreeVariableRapLambda",&fTreeVariableRapLambda,"fTreeVariableRapLambda/F");
/*10*/   fTree->Branch("fTreeVariableRapMC",&fTreeVariableRapMC,"fTreeVariableRapMC/F");
/*11*/   fTree->Branch("fTreeVariableInvMassK0s",&fTreeVariableInvMassK0s,"fTreeVariableInvMassK0s/F");
/*12*/   fTree->Branch("fTreeVariableInvMassLambda",&fTreeVariableInvMassLambda,"fTreeVariableInvMassLambda/F");
/*13*/   fTree->Branch("fTreeVariableInvMassAntiLambda",&fTreeVariableInvMassAntiLambda,"fTreeVariableInvMassAntiLambda/F");
/*14*/   fTree->Branch("fTreeVariableAlphaV0",&fTreeVariableAlphaV0,"fTreeVariableAlphaV0/F");
/*15*/   fTree->Branch("fTreeVariablePtArmV0",&fTreeVariablePtArmV0,"fTreeVariablePtArmV0/F");
/*16*/   fTree->Branch("fTreeVariableNegTransvMomentum",&fTreeVariableNegTransvMomentum,"fTreeVariableNegTransvMomentum/F");
/*17*/   fTree->Branch("fTreeVariablePosTransvMomentum",&fTreeVariablePosTransvMomentum,"fTreeVariablePosTransvMomentum/F");
/*18*/   fTree->Branch("fTreeVariableNegTransvMomentumMC",&fTreeVariableNegTransvMomentumMC,"fTreeVariableNegTransvMomentumMC/F");
/*19*/   fTree->Branch("fTreeVariablePosTransvMomentumMC",&fTreeVariablePosTransvMomentumMC,"fTreeVariablePosTransvMomentumMC/F");
/*20*/   fTree->Branch("fTreeVariableLeastNbrCrossedRows",&fTreeVariableLeastNbrCrossedRows,"fTreeVariableLeastNbrCrossedRows/I");
/*21*/   fTree->Branch("fTreeVariableLeastRatioCrossedRowsOverFindable",&fTreeVariableLeastRatioCrossedRowsOverFindable,"fTreeVariableLeastRatioCrossedRowsOverFindable/F");
/*22*/   fTree->Branch("fTreeVariablePID",&fTreeVariablePID,"fTreeVariablePID/I");
/*23*/   fTree->Branch("fTreeVariablePIDPositive",&fTreeVariablePIDPositive,"fTreeVariablePIDPositive/I");
/*24*/   fTree->Branch("fTreeVariablePIDNegative",&fTreeVariablePIDNegative,"fTreeVariablePIDNegative/I");
/*25*/   fTree->Branch("fTreeVariablePIDMother",&fTreeVariablePIDMother,"fTreeVariablePIDMother/I");
/*26*/   fTree->Branch("fTreeVariablePtXiMother",&fTreeVariablePtMother,"fTreeVariablePtMother/F");
/*27*/   fTree->Branch("fTreeVariableV0CosineOfPointingAngle",&fTreeVariableV0CosineOfPointingAngle,"fTreeVariableV0CosineOfPointingAngle/F");
//-----------MULTIPLICITY-INFO--------------------
/*28*/   fTree->Branch("fTreeVariableMultiplicity",&fTreeVariableMultiplicity,"fTreeVariableMultiplicity/I");
//------------------------------------------------
/*29*/   fTree->Branch("fTreeVariableDistOverTotMom",&fTreeVariableDistOverTotMom,"fTreeVariableDistOverTotMom/F");
/*30*/   fTree->Branch("fTreeVariableNSigmasPosProton",&fTreeVariableNSigmasPosProton,"fTreeVariableNSigmasPosProton/F");
/*31*/   fTree->Branch("fTreeVariableNSigmasPosPion",&fTreeVariableNSigmasPosPion,"fTreeVariableNSigmasPosPion/F");
/*32*/   fTree->Branch("fTreeVariableNSigmasNegProton",&fTreeVariableNSigmasNegProton,"fTreeVariableNSigmasNegProton/F");
/*33*/   fTree->Branch("fTreeVariableNSigmasNegPion",&fTreeVariableNSigmasNegPion,"fTreeVariableNSigmasNegPion/F");
//------------------------------------------------
/*34*/   fTree->Branch("fTreeVariableNegEta",&fTreeVariableNegEta,"fTreeVariableNegEta/F");
/*35*/   fTree->Branch("fTreeVariablePosEta",&fTreeVariablePosEta,"fTreeVariablePosEta/F");
/*36*/   fTree->Branch("fTreeVariableV0CreationRadius",&fTreeVariableV0CreationRadius,"fTreeVariableV0CreationRadius/F");
/*37*/   fTree->Branch("fTreeVariableIndexStatus",&fTreeVariableIndexStatus,"fTreeVariableIndexStatus/I");
/*38*/   fTree->Branch("fTreeVariableIndexStatusMother",&fTreeVariableIndexStatusMother,"fTreeVariableIndexStatusMother/I");

/*39*/ 	 fTree->Branch("fTreeVariableRunNumber",&fTreeVariableRunNumber,"fTreeVariableRunNumber/I");
/*40*/   fTree->Branch("fTreeVariableEventNumber",&fTreeVariableEventNumber,"fTreeVariableEventNumber/l");


//------------------------------------------------

   fTreeCascade = new TTree("fTreeCascade","CascadeCandidates");

//------------------------------------------------
// fTreeCascade Branch definitions - Cascade Tree
//------------------------------------------------

//------------------------------------------------
// fTreeCascade Branch definitions
//------------------------------------------------

//-----------BASIC-INFO---------------------------
/* 1*/		fTreeCascade->Branch("fTreeCascVarCharge",&fTreeCascVarCharge,"fTreeCascVarCharge/I");	
/* 2*/		fTreeCascade->Branch("fTreeCascVarMassAsXi",&fTreeCascVarMassAsXi,"fTreeCascVarMassAsXi/F");
/* 3*/		fTreeCascade->Branch("fTreeCascVarMassAsOmega",&fTreeCascVarMassAsOmega,"fTreeCascVarMassAsOmega/F");
/* 4*/		fTreeCascade->Branch("fTreeCascVarPt",&fTreeCascVarPt,"fTreeCascVarPt/F");
/* 5*/		fTreeCascade->Branch("fTreeCascVarRapXi",&fTreeCascVarRapXi,"fTreeCascVarRapXi/F");
/* 6*/		fTreeCascade->Branch("fTreeCascVarRapOmega",&fTreeCascVarRapOmega,"fTreeCascVarRapOmega/F");
/* 7*/		fTreeCascade->Branch("fTreeCascVarNegEta",&fTreeCascVarNegEta,"fTreeCascVarNegEta/F");
/* 8*/		fTreeCascade->Branch("fTreeCascVarPosEta",&fTreeCascVarPosEta,"fTreeCascVarPosEta/F");
/* 9*/		fTreeCascade->Branch("fTreeCascVarBachEta",&fTreeCascVarBachEta,"fTreeCascVarBachEta/F");
//-----------INFO-FOR-CUTS------------------------
/*10*/		fTreeCascade->Branch("fTreeCascVarDCACascDaughters",&fTreeCascVarDCACascDaughters,"fTreeCascVarDCACascDaughters/F");
/*11*/		fTreeCascade->Branch("fTreeCascVarDCABachToPrimVtx",&fTreeCascVarDCABachToPrimVtx,"fTreeCascVarDCABachToPrimVtx/F");
/*12*/		fTreeCascade->Branch("fTreeCascVarDCAV0Daughters",&fTreeCascVarDCAV0Daughters,"fTreeCascVarDCAV0Daughters/F");
/*13*/		fTreeCascade->Branch("fTreeCascVarDCAV0ToPrimVtx",&fTreeCascVarDCAV0ToPrimVtx,"fTreeCascVarDCAV0ToPrimVtx/F");
/*14*/		fTreeCascade->Branch("fTreeCascVarDCAPosToPrimVtx",&fTreeCascVarDCAPosToPrimVtx,"fTreeCascVarDCAPosToPrimVtx/F");
/*15*/		fTreeCascade->Branch("fTreeCascVarDCANegToPrimVtx",&fTreeCascVarDCANegToPrimVtx,"fTreeCascVarDCANegToPrimVtx/F");
/*16*/		fTreeCascade->Branch("fTreeCascVarCascCosPointingAngle",&fTreeCascVarCascCosPointingAngle,"fTreeCascVarCascCosPointingAngle/F");
/*17*/		fTreeCascade->Branch("fTreeCascVarCascRadius",&fTreeCascVarCascRadius,"fTreeCascVarCascRadius/F");
/*18*/		fTreeCascade->Branch("fTreeCascVarV0Mass",&fTreeCascVarV0Mass,"fTreeCascVarV0Mass/F");
/*19*/		fTreeCascade->Branch("fTreeCascVarV0CosPointingAngle",&fTreeCascVarV0CosPointingAngle,"fTreeCascVarV0CosPointingAngle/F");
/*20*/		fTreeCascade->Branch("fTreeCascVarV0Radius",&fTreeCascVarV0Radius,"fTreeCascVarV0Radius/F");
/*21*/		fTreeCascade->Branch("fTreeCascVarLeastNbrClusters",&fTreeCascVarLeastNbrClusters,"fTreeCascVarLeastNbrClusters/I");
//-----------MULTIPLICITY-INFO--------------------
/*22*/		fTreeCascade->Branch("fTreeCascVarMultiplicity",&fTreeCascVarMultiplicity,"fTreeCascVarMultiplicity/I");
//-----------DECAY-LENGTH-INFO--------------------
/*23*/		fTreeCascade->Branch("fTreeCascVarDistOverTotMom",&fTreeCascVarDistOverTotMom,"fTreeCascVarDistOverTotMom/F");
//-----------MC-PID-------------------------------
/*24*/		fTreeCascade->Branch("fTreeCascVarPID",&fTreeCascVarPID,"fTreeCascVarPID/I");
/*25*/		fTreeCascade->Branch("fTreeCascVarPIDBachelor",&fTreeCascVarPIDBachelor,"fTreeCascVarPIDBachelor/I");
/*26*/    fTreeCascade->Branch("fTreeCascVarPIDNegative",&fTreeCascVarPIDNegative,"fTreeCascVarPIDNegative/I");
/*27*/    fTreeCascade->Branch("fTreeCascVarPIDPositive",&fTreeCascVarPIDPositive,"fTreeCascVarPIDPositive/I");
/*28*/		fTreeCascade->Branch("fTreeCascVarPosTransMom",&fTreeCascVarPosTransMom,"fTreeCascVarPosTransMom/F");
/*29*/		fTreeCascade->Branch("fTreeCascVarNegTransMom",&fTreeCascVarNegTransMom,"fTreeCascVarNegTransMom/F");
/*30*/		fTreeCascade->Branch("fTreeCascVarPosTransMomMC",&fTreeCascVarPosTransMomMC,"fTreeCascVarPosTransMomMC/F");
/*31*/		fTreeCascade->Branch("fTreeCascVarNegTransMomMC",&fTreeCascVarNegTransMomMC,"fTreeCascVarNegTransMomMC/F");
//------------------------------------------------


//------------------------------------------------
// Particle Identification Setup
//------------------------------------------------

   AliAnalysisManager *man=AliAnalysisManager::GetAnalysisManager();
   AliInputEventHandler* inputHandler = (AliInputEventHandler*) (man->GetInputEventHandler());
   fPIDResponse = inputHandler->GetPIDResponse();

//------------------------------------------------
// V0 Multiplicity Histograms
//------------------------------------------------

   // Create histograms
   OpenFile(1);
   fListHistV0 = new TList();
   fListHistV0->SetOwner();  // See http://root.cern.ch/root/html/TCollection.html#TCollection:SetOwner


   if(! fHistV0MultiplicityBeforeTrigSel) {
      fHistV0MultiplicityBeforeTrigSel = new TH1F("fHistV0MultiplicityBeforeTrigSel", 
         "V0s per event (before Trig. Sel.);Nbr of V0s/Evt;Events", 
         25, 0, 25);
      fListHistV0->Add(fHistV0MultiplicityBeforeTrigSel);
   }
           
   if(! fHistV0MultiplicityForTrigEvt) {
      fHistV0MultiplicityForTrigEvt = new TH1F("fHistV0MultiplicityForTrigEvt", 
         "V0s per event (for triggered evt);Nbr of V0s/Evt;Events", 
         25, 0, 25);
      fListHistV0->Add(fHistV0MultiplicityForTrigEvt);
   }

   if(! fHistV0MultiplicityForSelEvt) {
      fHistV0MultiplicityForSelEvt = new TH1F("fHistV0MultiplicityForSelEvt", 
         "V0s per event;Nbr of V0s/Evt;Events", 
         25, 0, 25);
      fListHistV0->Add(fHistV0MultiplicityForSelEvt);
   }

   if(! fHistV0MultiplicityForSelEvtNoTPCOnly) {
      fHistV0MultiplicityForSelEvtNoTPCOnly = new TH1F("fHistV0MultiplicityForSelEvtNoTPCOnly", 
         "V0s per event;Nbr of V0s/Evt;Events", 
         25, 0, 25);
      fListHistV0->Add(fHistV0MultiplicityForSelEvtNoTPCOnly);
   }
   if(! fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup) {
      fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup = new TH1F("fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup", 
         "V0s per event;Nbr of V0s/Evt;Events", 
         25, 0, 25);
      fListHistV0->Add(fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup);
   }

//------------------------------------------------
// Track Multiplicity Histograms
//------------------------------------------------

   if(! fHistMultiplicityBeforeTrigSel) {
      fHistMultiplicityBeforeTrigSel = new TH1F("fHistMultiplicityBeforeTrigSel", 
         "Tracks per event;Nbr of Tracks;Events", 
         200, 0, 200); 		
      fListHistV0->Add(fHistMultiplicityBeforeTrigSel);
   }
   if(! fHistMultiplicityForTrigEvt) {
      fHistMultiplicityForTrigEvt = new TH1F("fHistMultiplicityForTrigEvt", 
         "Tracks per event;Nbr of Tracks;Events", 
         200, 0, 200); 		
      fListHistV0->Add(fHistMultiplicityForTrigEvt);
   }
   if(! fHistMultiplicity) {
      fHistMultiplicity = new TH1F("fHistMultiplicity", 
         "Tracks per event;Nbr of Tracks;Events", 
         200, 0, 200); 		
      fListHistV0->Add(fHistMultiplicity);
   }
   if(! fHistMultiplicityNoTPCOnly) {
      fHistMultiplicityNoTPCOnly = new TH1F("fHistMultiplicityNoTPCOnly", 
         "Tracks per event;Nbr of Tracks;Events", 
         200, 0, 200); 		
      fListHistV0->Add(fHistMultiplicityNoTPCOnly);
   }
   if(! fHistMultiplicityNoTPCOnlyNoPileup) {
      fHistMultiplicityNoTPCOnlyNoPileup = new TH1F("fHistMultiplicityNoTPCOnlyNoPileup", 
         "Tracks per event;Nbr of Tracks;Events", 
         200, 0, 200); 		
      fListHistV0->Add(fHistMultiplicityNoTPCOnlyNoPileup);
   }

//------------------------------------------------
// Generated Particle Histograms
//------------------------------------------------

   Int_t lCustomNBins = 200; 
   Double_t lCustomPtUpperLimit = 20; 
   Int_t lCustomNBinsMultiplicity = 100;

//----------------------------------
// Raw Generated (Pre-physics-selection)
//----------------------------------

//--- 3D Histo (Pt, Y, Multiplicity)  

   if(! f3dHistPrimRawPtVsYVsMultLambda) {
      f3dHistPrimRawPtVsYVsMultLambda = new TH3F( "f3dHistPrimRawPtVsYVsMultLambda", "Pt_{lambda} Vs Y_{#Lambda} Vs Multiplicity; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimRawPtVsYVsMultLambda);
   }
   if(! f3dHistPrimRawPtVsYVsMultAntiLambda) {
      f3dHistPrimRawPtVsYVsMultAntiLambda = new TH3F( "f3dHistPrimRawPtVsYVsMultAntiLambda", "Pt_{antilambda} Vs Y_{#Lambda} Vs Multiplicity; Pt_{antilambda} (GeV/c); Y_{#Lambda} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimRawPtVsYVsMultAntiLambda);
   }
   if(! f3dHistPrimRawPtVsYVsMultK0Short) {
      f3dHistPrimRawPtVsYVsMultK0Short = new TH3F( "f3dHistPrimRawPtVsYVsMultK0Short", "Pt_{K0S} Vs Y_{K0S} Vs Multiplicity; Pt_{K0S} (GeV/c); Y_{K0S} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimRawPtVsYVsMultK0Short);
   }

//--- 3D Histo (Pt, Y, Multiplicity), close to PV criterion

   if(! f3dHistPrimCloseToPVPtVsYVsMultLambda) {
      f3dHistPrimCloseToPVPtVsYVsMultLambda = new TH3F( "f3dHistPrimCloseToPVPtVsYVsMultLambda", "Pt_{lambda} Vs Y_{#Lambda} Vs Multiplicity; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimCloseToPVPtVsYVsMultLambda);
   }
   if(! f3dHistPrimCloseToPVPtVsYVsMultAntiLambda) {
      f3dHistPrimCloseToPVPtVsYVsMultAntiLambda = new TH3F( "f3dHistPrimCloseToPVPtVsYVsMultAntiLambda", "Pt_{antilambda} Vs Y_{#Lambda} Vs Multiplicity; Pt_{antilambda} (GeV/c); Y_{#Lambda} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimCloseToPVPtVsYVsMultAntiLambda);
   }
   if(! f3dHistPrimCloseToPVPtVsYVsMultK0Short) {
      f3dHistPrimCloseToPVPtVsYVsMultK0Short = new TH3F( "f3dHistPrimCloseToPVPtVsYVsMultK0Short", "Pt_{K0S} Vs Y_{K0S} Vs Multiplicity; Pt_{K0S} (GeV/c); Y_{K0S} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimCloseToPVPtVsYVsMultK0Short);
   }


//--- 3D Histo (Pt, Y, Proper Decay Length)

   if(! f3dHistPrimRawPtVsYVsDecayLengthLambda) {
      f3dHistPrimRawPtVsYVsDecayLengthLambda = new TH3F( "f3dHistPrimRawPtVsYVsDecayLengthLambda", "Pt_{lambda} Vs Y_{#Lambda} Vs DecayLength; Pt_{lambda} (GeV/c); Y_{#Lambda} ; DecayLength", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,200,0,50);
      fListHistV0->Add(f3dHistPrimRawPtVsYVsDecayLengthLambda);
   }
   if(! f3dHistPrimRawPtVsYVsDecayLengthAntiLambda) {
      f3dHistPrimRawPtVsYVsDecayLengthAntiLambda = new TH3F( "f3dHistPrimRawPtVsYVsDecayLengthAntiLambda", "Pt_{antilambda} Vs Y_{#Lambda} Vs DecayLength; Pt_{antilambda} (GeV/c); Y_{#Lambda} ; DecayLength", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,200,0,50);
      fListHistV0->Add(f3dHistPrimRawPtVsYVsDecayLengthAntiLambda);
   }
   if(! f3dHistPrimRawPtVsYVsDecayLengthK0Short) {
      f3dHistPrimRawPtVsYVsDecayLengthK0Short = new TH3F( "f3dHistPrimRawPtVsYVsDecayLengthK0Short", "Pt_{K0S} Vs Y_{K0S} Vs DecayLength; Pt_{K0S} (GeV/c); Y_{K0S} ; DecayLength", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,200,0,50);
      fListHistV0->Add(f3dHistPrimRawPtVsYVsDecayLengthK0Short);
   }

//--------------------------------------------------------------------------------------
//--- 3D Histo (Pt, Y, Multiplicity) for generated XiMinus/Plus, all generated

   if(! f3dHistGenPtVsYVsMultXiMinus) {
      f3dHistGenPtVsYVsMultXiMinus = new TH3F( "f3dHistGenPtVsYVsMultXiMinus", "Pt_{#Xi} Vs Y_{#Xi} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Xi} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenPtVsYVsMultXiMinus);
   }
   if(! f3dHistGenPtVsYVsMultXiPlus) {
      f3dHistGenPtVsYVsMultXiPlus = new TH3F( "f3dHistGenPtVsYVsMultXiPlus", "Pt_{#Xi} Vs Y_{#Xi} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Xi} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenPtVsYVsMultXiPlus);
   }
//--- 3D Histo (Pt, Y, Multiplicity) for generated OmegaMinus/Plus

   if(! f3dHistGenPtVsYVsMultOmegaMinus) {
      f3dHistGenPtVsYVsMultOmegaMinus = new TH3F( "f3dHistGenPtVsYVsMultOmegaMinus", "Pt_{#Omega} Vs Y_{#Omega} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Omega} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenPtVsYVsMultOmegaMinus);
   }
   if(! f3dHistGenPtVsYVsMultOmegaPlus) {
      f3dHistGenPtVsYVsMultOmegaPlus = new TH3F( "f3dHistGenPtVsYVsMultOmegaPlus", "Pt_{#Omega} Vs Y_{#Omega} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Omega} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenPtVsYVsMultOmegaPlus);
   }

//--------------------------------------------------------------------------------------
//--- 3D Histo (Pt, Y, Multiplicity) for generated XiMinus/Plus, at selected analysis evts

   if(! f3dHistGenSelectedPtVsYVsMultXiMinus) {
      f3dHistGenSelectedPtVsYVsMultXiMinus = new TH3F( "f3dHistGenSelectedPtVsYVsMultXiMinus", "Pt_{#Xi} Vs Y_{#Xi} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Xi} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenSelectedPtVsYVsMultXiMinus);
   }
   if(! f3dHistGenSelectedPtVsYVsMultXiPlus) {
      f3dHistGenSelectedPtVsYVsMultXiPlus = new TH3F( "f3dHistGenSelectedPtVsYVsMultXiPlus", "Pt_{#Xi} Vs Y_{#Xi} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Xi} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenSelectedPtVsYVsMultXiPlus);
   }
//--- 3D Histo (Pt, Y, Multiplicity) for generated OmegaMinus/Plus

   if(! f3dHistGenSelectedPtVsYVsMultOmegaMinus) {
      f3dHistGenSelectedPtVsYVsMultOmegaMinus = new TH3F( "f3dHistGenSelectedPtVsYVsMultOmegaMinus", "Pt_{#Omega} Vs Y_{#Omega} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Omega} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenSelectedPtVsYVsMultOmegaMinus);
   }
   if(! f3dHistGenSelectedPtVsYVsMultOmegaPlus) {
      f3dHistGenSelectedPtVsYVsMultOmegaPlus = new TH3F( "f3dHistGenSelectedPtVsYVsMultOmegaPlus", "Pt_{#Omega} Vs Y_{#Omega} Vs Multiplicity; Pt_{cascade} (GeV/c); Y_{#Omega} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistGenSelectedPtVsYVsMultOmegaPlus);
   }


//----------------------------------
// Histos at analysis level 
//----------------------------------

   if(! f3dHistPrimAnalysisPtVsYVsMultLambda) {
      f3dHistPrimAnalysisPtVsYVsMultLambda = new TH3F( "f3dHistPrimAnalysisPtVsYVsMultLambda", "Pt_{lambda} Vs Y_{#Lambda} Vs Multiplicity; Pt_{lambda} (GeV/c); Y_{#Lambda} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimAnalysisPtVsYVsMultLambda);
   }
   if(! f3dHistPrimAnalysisPtVsYVsMultAntiLambda) {
      f3dHistPrimAnalysisPtVsYVsMultAntiLambda = new TH3F( "f3dHistPrimAnalysisPtVsYVsMultAntiLambda", "Pt_{antilambda} Vs Y_{#Lambda} Vs Multiplicity; Pt_{antilambda} (GeV/c); Y_{#Lambda} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimAnalysisPtVsYVsMultAntiLambda);
   }
   if(! f3dHistPrimAnalysisPtVsYVsMultK0Short) {
      f3dHistPrimAnalysisPtVsYVsMultK0Short = new TH3F( "f3dHistPrimAnalysisPtVsYVsMultK0Short", "Pt_{K0S} Vs Y_{K0S} Vs Multiplicity; Pt_{K0S} (GeV/c); Y_{K0S} ; Mult", lCustomNBins, 0., lCustomPtUpperLimit, 48, -1.2,1.2,lCustomNBinsMultiplicity,0,lCustomNBinsMultiplicity);
      fListHistV0->Add(f3dHistPrimAnalysisPtVsYVsMultK0Short);
   }

//----------------------------------
// Primary Vertex Position Histos
//----------------------------------

   if(! fHistPVx) {
         fHistPVx = new TH1F("fHistPVx", 
            "PV x position;Nbr of Evts;x", 
            2000, -0.5, 0.5);       
      fListHistV0->Add(fHistPVx);
   }
   if(! fHistPVy) {
         fHistPVy = new TH1F("fHistPVy", 
            "PV y position;Nbr of Evts;y", 
            2000, -0.5, 0.5);       
      fListHistV0->Add(fHistPVy);
   }
   if(! fHistPVz) {
         fHistPVz = new TH1F("fHistPVz", 
            "PV z position;Nbr of Evts;z", 
            400, -20, 20);       
      fListHistV0->Add(fHistPVz);
   }

   if(! fHistPVxAnalysis) {
         fHistPVxAnalysis = new TH1F("fHistPVxAnalysis", 
            "PV x position;Nbr of Evts;x", 
            2000, -0.5, 0.5);       
      fListHistV0->Add(fHistPVxAnalysis);
   }
   if(! fHistPVyAnalysis) {
         fHistPVyAnalysis = new TH1F("fHistPVyAnalysis", 
            "PV y position;Nbr of Evts;y", 
            2000, -0.5, 0.5);       
      fListHistV0->Add(fHistPVyAnalysis);
   }
   if(! fHistPVzAnalysis) {
         fHistPVzAnalysis = new TH1F("fHistPVzAnalysis", 
            "PV z position;Nbr of Evts;z", 
            400, -20, 20);       
      fListHistV0->Add(fHistPVzAnalysis);
   }

   if(! fHistPVxAnalysisHasHighPtLambda) {
         fHistPVxAnalysisHasHighPtLambda = new TH1F("fHistPVxAnalysisHasHighPtLambda", 
            "PV x position;Nbr of Evts;x", 
            2000, -0.5, 0.5);       
      fListHistV0->Add(fHistPVxAnalysisHasHighPtLambda);
   }
   if(! fHistPVyAnalysisHasHighPtLambda) {
         fHistPVyAnalysisHasHighPtLambda = new TH1F("fHistPVyAnalysisHasHighPtLambda", 
            "PV y position;Nbr of Evts;y", 
            2000, -0.5, 0.5);       
      fListHistV0->Add(fHistPVyAnalysisHasHighPtLambda);
   }
   if(! fHistPVzAnalysisHasHighPtLambda) {
         fHistPVzAnalysisHasHighPtLambda = new TH1F("fHistPVzAnalysisHasHighPtLambda", 
            "PV z position;Nbr of Evts;z", 
            400, -20, 20);       
      fListHistV0->Add(fHistPVzAnalysisHasHighPtLambda);
   }
   if(! fHistSwappedV0Counter) {
      fHistSwappedV0Counter = new TH1F("fHistSwappedV0Counter", 
         "Swap or not histo;Swapped (1) or not (0); count", 
         2, 0, 2); 		
      fListHistV0->Add(fHistSwappedV0Counter);
   }

   //List of Histograms: Normal
   PostData(1, fListHistV0);

   //TTree Object: Saved to base directory. Should cache to disk while saving. 
   //(Important to avoid excessive memory usage, particularly when merging)
   PostData(2, fTree);

   //TTree Object for cascades, output slot 3
   PostData(3, fTreeCascade);

}// end UserCreateOutputObjects


//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0::UserExec(Option_t *) 
{
  // Main loop
  // Called for each event

   AliESDEvent *lESDevent = 0x0;
   AliMCEvent  *lMCevent  = 0x0; 
   AliStack    *lMCstack  = 0x0; 

   Int_t    lNumberOfV0s                      = -1;
   Double_t lTrkgPrimaryVtxPos[3]          = {-100.0, -100.0, -100.0};
   Double_t lBestPrimaryVtxPos[3]          = {-100.0, -100.0, -100.0};
   Double_t lMagneticField                 = -10.;
   
  // Connect to the InputEvent   
  // After these lines, we should have an ESD/AOD event + the number of V0s in it.

   // Appropriate for ESD analysis! 
      
   lESDevent = dynamic_cast<AliESDEvent*>( InputEvent() );
   if (!lESDevent) {
      AliWarning("ERROR: lESDevent not available \n");
      return;
   }
        
   fTreeVariableRunNumber = lESDevent->GetRunNumber();
   fTreeVariableEventNumber =  
    ( ( ((ULong64_t)lESDevent->GetPeriodNumber() ) << 36 ) |
      ( ((ULong64_t)lESDevent->GetOrbitNumber () ) << 12 ) |
        ((ULong64_t)lESDevent->GetBunchCrossNumber() )  );

   lMCevent = MCEvent();
   if (!lMCevent) {
      Printf("ERROR: Could not retrieve MC event \n");
      cout << "Name of the file with pb :" <<  fInputHandler->GetTree()->GetCurrentFile()->GetName() << endl;   
      return;
   }

   lMCstack = lMCevent->Stack();
   if (!lMCstack) {
      Printf("ERROR: Could not retrieve MC stack \n");
      cout << "Name of the file with pb :" <<  fInputHandler->GetTree()->GetCurrentFile()->GetName() << endl;
      return;
   }
   TArrayF mcPrimaryVtx;
   AliGenEventHeader* mcHeader=lMCevent->GenEventHeader();
   if(!mcHeader) return;
   mcHeader->PrimaryVertex(mcPrimaryVtx);
        
//------------------------------------------------
// Multiplicity Information Acquistion
//------------------------------------------------

   //REVISED multiplicity estimator after 'multiplicity day' (2011)
   Int_t lMultiplicity = -100; 

   //testing purposes
   if(fkIsNuclear == kFALSE) lMultiplicity = AliESDtrackCuts::GetReferenceMultiplicity( lESDevent );

   //---> If this is a nuclear collision, then go nuclear on "multiplicity" variable...
   //---> Warning: Experimental
   if(fkIsNuclear == kTRUE){ 
      AliCentrality* centrality;
      centrality = lESDevent->GetCentrality();
      lMultiplicity = ( ( Int_t ) ( centrality->GetCentralityPercentile( "V0M" ) ) );
      if (centrality->GetQuality()>1) {
        PostData(1, fListHistV0);
        PostData(2, fTree);
        PostData(3, fTreeCascade);
        return;
      }
   }
  
   //Set variable for filling tree afterwards!
   //---> pp case......: GetReferenceMultiplicity
   //---> Pb-Pb case...: Centrality by V0M
   fTreeVariableMultiplicity = lMultiplicity;

   fHistV0MultiplicityBeforeTrigSel->Fill ( lESDevent->GetNumberOfV0s() );
   fHistMultiplicityBeforeTrigSel->Fill ( lMultiplicity );
        
//------------------------------------------------
// MC Information Acquistion
//------------------------------------------------

   Int_t iNumberOfPrimaries = -1;
   iNumberOfPrimaries = lMCstack->GetNprimary();
   if(iNumberOfPrimaries < 1) return; 
   Bool_t lHasHighPtLambda = kFALSE;

//------------------------------------------------
// Variable Definition
//------------------------------------------------

   Int_t lNbMCPrimary        = 0;

   Int_t lPdgcodeCurrentPart = 0;
   Double_t lRapCurrentPart  = 0;
   Double_t lPtCurrentPart   = 0;
  
   //Int_t lComeFromSigma      = 0;

   // current mc particle 's mother
   //Int_t iCurrentMother  = 0;
   lNbMCPrimary = lMCstack->GetNprimary();

//------------------------------------------------
// Pre-Physics Selection
//------------------------------------------------

//----- Loop on primary Xi, Omega --------------------------------------------------------------
   for (Int_t iCurrentLabelStack = 0; iCurrentLabelStack < lNbMCPrimary; iCurrentLabelStack++) 
   {// This is the begining of the loop on primaries
      
      TParticle* lCurrentParticlePrimary = 0x0; 
      lCurrentParticlePrimary = lMCstack->Particle( iCurrentLabelStack );
      if(!lCurrentParticlePrimary){
         Printf("Cascade loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skip ...\n", iCurrentLabelStack );
         continue;
      }
      if ( TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3312 || TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3334 ) { 
         Double_t lRapXiMCPrimary = -100;
         if( (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) != 0 ) { 
           if ( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) !=0 ){
             lRapXiMCPrimary = 0.5*TMath::Log( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) );
           }
         }

         //=================================================================================
         // Xi Histograms
         if( lCurrentParticlePrimary->GetPdgCode() == 3312 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenPtVsYVsMultXiMinus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
         if( lCurrentParticlePrimary->GetPdgCode() == -3312 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenPtVsYVsMultXiPlus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
         // Omega Histograms
         if( lCurrentParticlePrimary->GetPdgCode() == 3334 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenPtVsYVsMultOmegaMinus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
         if( lCurrentParticlePrimary->GetPdgCode() == -3334 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenPtVsYVsMultOmegaPlus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
      } 
   }
//----- End Loop on primary Xi, Omega ----------------------------------------------------------

//----- Loop on Lambda, K0Short ----------------------------------------------------------------
   for (Int_t iCurrentLabelStack = 0;  iCurrentLabelStack < (lMCstack->GetNtrack()); iCurrentLabelStack++) 
   {// This is the begining of the loop on tracks
      
      TParticle* lCurrentParticleForLambdaCheck = 0x0; 
      lCurrentParticleForLambdaCheck = lMCstack->Particle( iCurrentLabelStack );
      if(!lCurrentParticleForLambdaCheck){
         Printf("V0s loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skip ...\n", iCurrentLabelStack );
         continue;
      }

      //=================================================================================
      //Single-Strange checks
      // Keep only K0s, Lambda and AntiLambda:
      lPdgcodeCurrentPart = lCurrentParticleForLambdaCheck->GetPdgCode();	      

      if ( (lCurrentParticleForLambdaCheck->GetPdgCode() == 310   ) ||
           (lCurrentParticleForLambdaCheck->GetPdgCode() == 3122  ) ||
           (lCurrentParticleForLambdaCheck->GetPdgCode() == -3122 ) )
	   {
         lRapCurrentPart   = MyRapidity(lCurrentParticleForLambdaCheck->Energy(),lCurrentParticleForLambdaCheck->Pz());
         lPtCurrentPart    = lCurrentParticleForLambdaCheck->Pt();

          //Use Close to PV for filling CloseToPV histograms!
         Double_t dx, dy, dz; 

         dx = ( (mcPrimaryVtx.At(0)) - (lCurrentParticleForLambdaCheck->Vx()) ); 
         dy = ( (mcPrimaryVtx.At(1)) - (lCurrentParticleForLambdaCheck->Vy()) );
         dz = ( (mcPrimaryVtx.At(2)) - (lCurrentParticleForLambdaCheck->Vz()) );
         Double_t lDistToPV = TMath::Sqrt(dx*dx + dy*dy + dz*dz);
         if( lDistToPV <= 0.001){ 
           if( lPdgcodeCurrentPart == 3122 ){
              f3dHistPrimCloseToPVPtVsYVsMultLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
           }
           if( lPdgcodeCurrentPart == -3122 ){
              f3dHistPrimCloseToPVPtVsYVsMultAntiLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
           }
           if( lPdgcodeCurrentPart == 310 ){
              f3dHistPrimCloseToPVPtVsYVsMultK0Short->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
           }
         }

         //Use Physical Primaries only for filling PrimRaw Histograms!
         if ( lMCstack->IsPhysicalPrimary(iCurrentLabelStack)!=kTRUE ) continue;

         if( lPdgcodeCurrentPart == 3122 ){
            f3dHistPrimRawPtVsYVsMultLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
            if( TMath::Abs( lCurrentParticleForLambdaCheck->Eta() )<1.2 && lPtCurrentPart>2 ){
               lHasHighPtLambda = kTRUE; //Keep track of events with Lambda within |eta|<1.2 and pt>2
            }
         }
         if( lPdgcodeCurrentPart == -3122 ){
            f3dHistPrimRawPtVsYVsMultAntiLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
         }
         if( lPdgcodeCurrentPart == 310 ){
            f3dHistPrimRawPtVsYVsMultK0Short->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
         }
         //Decay Length Acquisition=====================================================
         Double_t decaylength = -1; 
         Double_t lV0Mass = -1; 
          
         if( !(lCurrentParticleForLambdaCheck->GetDaughter(0) < 0) ) {
            TParticle* lDght0ofV0 = lMCstack->Particle(  lCurrentParticleForLambdaCheck->GetDaughter(0) ); //get first daughter
            if(lDght0ofV0){ // skip if not defined. 
               decaylength = TMath::Sqrt(
				        TMath::Power( lCurrentParticleForLambdaCheck->Vx() - lDght0ofV0->Vx() , 2) + 
				        TMath::Power( lCurrentParticleForLambdaCheck->Vy() - lDght0ofV0->Vy() , 2) + 
				        TMath::Power( lCurrentParticleForLambdaCheck->Vz() - lDght0ofV0->Vz() , 2)
               );
               //Need to correct for relativitity! Involves multiplying by mass and dividing by momentum. 
               if(TMath::Abs( lPdgcodeCurrentPart ) == 3122 ) { lV0Mass = 1.115683; }
               if(TMath::Abs( lPdgcodeCurrentPart ) == 310 ) { lV0Mass = 0.497614; }
               if( lCurrentParticleForLambdaCheck->P() + 1e-10 != 0 ) decaylength = ( lV0Mass * decaylength ) / ( lCurrentParticleForLambdaCheck->P() + 1e-10 );
               if( lCurrentParticleForLambdaCheck->P() + 1e-10 == 0 ) decaylength = 1e+5;
            }
         }
         if( lPdgcodeCurrentPart == 3122) f3dHistPrimRawPtVsYVsDecayLengthLambda ->Fill( lPtCurrentPart, lRapCurrentPart , decaylength ); 
         if( lPdgcodeCurrentPart == -3122) f3dHistPrimRawPtVsYVsDecayLengthAntiLambda ->Fill( lPtCurrentPart, lRapCurrentPart , decaylength ); 
         if( lPdgcodeCurrentPart == 310) f3dHistPrimRawPtVsYVsDecayLengthK0Short ->Fill( lPtCurrentPart, lRapCurrentPart , decaylength ); 
      }
   }//End of loop on tracks
//----- End Loop on Lambda, K0Short ------------------------------------------------------------

// ---> Set Variables to Zero again
// ---> Variable Definition

   lPdgcodeCurrentPart = 0;
   lRapCurrentPart  = 0;
   lPtCurrentPart   = 0;

//------------------------------------------------
// Physics Selection
//------------------------------------------------

   UInt_t maskIsSelected = ((AliInputEventHandler*)(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler()))->IsEventSelected();
   Bool_t isSelected = 0;
   isSelected = (maskIsSelected & AliVEvent::kMB) == AliVEvent::kMB;

   //pp at 2.76TeV: special case, ignore FastOnly
   if ( (fkLowEnergyPP == kTRUE) && (maskIsSelected& AliVEvent::kFastOnly) ){
      PostData(1, fListHistV0);
      PostData(2, fTree);
      PostData(3, fTreeCascade);
      return;
   } 
   //Standard Min-Bias Selection
   if ( ! isSelected ) { 
      PostData(1, fListHistV0);
      PostData(2, fTree);
      PostData(3, fTreeCascade);
      return;
   }

//------------------------------------------------
// After Trigger Selection
//------------------------------------------------

   lNumberOfV0s          = lESDevent->GetNumberOfV0s();
  
   //Set variable for filling tree afterwards!
   fHistV0MultiplicityForTrigEvt->Fill(lNumberOfV0s);
   fHistMultiplicityForTrigEvt->Fill ( lMultiplicity );

//------------------------------------------------
// Getting: Primary Vertex + MagField Info
//------------------------------------------------

   const AliESDVertex *lPrimaryTrackingESDVtx = lESDevent->GetPrimaryVertexTracks();
   // get the vtx stored in ESD found with tracks
   lPrimaryTrackingESDVtx->GetXYZ( lTrkgPrimaryVtxPos );
        
   const AliESDVertex *lPrimaryBestESDVtx = lESDevent->GetPrimaryVertex();	
   // get the best primary vertex available for the event
   // As done in AliCascadeVertexer, we keep the one which is the best one available.
   // between : Tracking vertex > SPD vertex > TPC vertex > default SPD vertex
   // This one will be used for next calculations (DCA essentially)
   lPrimaryBestESDVtx->GetXYZ( lBestPrimaryVtxPos );

   Double_t lPrimaryVtxPosition[3];
   const AliVVertex *primaryVtx = lESDevent->GetPrimaryVertex();
   lPrimaryVtxPosition[0] = primaryVtx->GetX();
   lPrimaryVtxPosition[1] = primaryVtx->GetY();
   lPrimaryVtxPosition[2] = primaryVtx->GetZ();
   fHistPVx->Fill( lPrimaryVtxPosition[0] );
   fHistPVy->Fill( lPrimaryVtxPosition[1] );
   fHistPVz->Fill( lPrimaryVtxPosition[2] );

//------------------------------------------------
// Primary Vertex Z position: SKIP
//------------------------------------------------

   if(TMath::Abs(lBestPrimaryVtxPos[2]) > 10.0 ) { 
      AliWarning("Pb / | Z position of Best Prim Vtx | > 10.0 cm ... return !"); 
      PostData(1, fListHistV0);
      PostData(2, fTree);
      PostData(3, fTreeCascade);
      return; 
   }

   lMagneticField = lESDevent->GetMagneticField( );
   fHistV0MultiplicityForSelEvt ->Fill( lNumberOfV0s );
   fHistMultiplicity->Fill(lMultiplicity);

//------------------------------------------------
// SKIP: Events with well-established PVtx
//------------------------------------------------
	
   const AliESDVertex *lPrimaryTrackingESDVtxCheck = lESDevent->GetPrimaryVertexTracks();
   const AliESDVertex *lPrimarySPDVtx = lESDevent->GetPrimaryVertexSPD();
   if (!lPrimarySPDVtx->GetStatus() && !lPrimaryTrackingESDVtxCheck->GetStatus() ){
      AliWarning("Pb / No SPD prim. vertex nor prim. Tracking vertex ... return !");
      PostData(1, fListHistV0);
      PostData(2, fTree);
      PostData(3, fTreeCascade);
      return;
   }
   fHistV0MultiplicityForSelEvtNoTPCOnly ->Fill( lNumberOfV0s );
   fHistMultiplicityNoTPCOnly->Fill(lMultiplicity);

//------------------------------------------------
// Pileup Rejection Studies
//------------------------------------------------

   // FIXME : quality selection regarding pile-up rejection 
   if(lESDevent->IsPileupFromSPD() && !fkIsNuclear ){// minContributors=3, minZdist=0.8, nSigmaZdist=3., nSigmaDiamXY=2., nSigmaDiamZ=5.  -> see http://alisoft.cern.ch/viewvc/trunk/STEER/AliESDEvent.h?root=AliRoot&r1=41914&r2=42199&pathrev=42199
      AliWarning("Pb / This is tagged as Pileup from SPD... return !");
      PostData(1, fListHistV0);
      PostData(2, fTree);
      PostData(3, fTreeCascade);
      return;
   }
   fHistV0MultiplicityForSelEvtNoTPCOnlyNoPileup ->Fill( lNumberOfV0s );
   fHistMultiplicityNoTPCOnlyNoPileup->Fill(lMultiplicity);

   //Do control histograms without the IsFromVertexerZ events, but consider them in analysis...
   if( ! (lESDevent->GetPrimaryVertex()->IsFromVertexerZ() )	 ){ 
      fHistPVxAnalysis->Fill( lPrimaryVtxPosition[0] );
      fHistPVyAnalysis->Fill( lPrimaryVtxPosition[1] );
      fHistPVzAnalysis->Fill( lPrimaryVtxPosition[2] );
      if ( lHasHighPtLambda == kTRUE ){ 
         fHistPVxAnalysisHasHighPtLambda->Fill( lPrimaryVtxPosition[0] );
         fHistPVyAnalysisHasHighPtLambda->Fill( lPrimaryVtxPosition[1] );
         fHistPVzAnalysisHasHighPtLambda->Fill( lPrimaryVtxPosition[2] );
      }
   }

//------------------------------------------------
// stack loop starts here
//------------------------------------------------

//---> Loop over ALL PARTICLES
 
   for (Int_t iMc = 0; iMc < (lMCstack->GetNtrack()); iMc++) {  
      TParticle *p0 = lMCstack->Particle(iMc); 
      if (!p0) {
         //Printf("ERROR: particle with label %d not found in lMCstack (mc loop)", iMc);
         continue;
      }
      lPdgcodeCurrentPart = p0->GetPdgCode();

      // Keep only K0s, Lambda and AntiLambda:
      if ( (lPdgcodeCurrentPart != 310 ) && (lPdgcodeCurrentPart != 3122 ) && (lPdgcodeCurrentPart != -3122 ) ) continue;
	
      lRapCurrentPart   = MyRapidity(p0->Energy(),p0->Pz());
      lPtCurrentPart    = p0->Pt();

        //Use Physical Primaries only for filling PrimRaw Histograms!
      if ( lMCstack->IsPhysicalPrimary(iMc)!=kTRUE ) continue;

      if( lPdgcodeCurrentPart == 3122 ){
         f3dHistPrimAnalysisPtVsYVsMultLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
      }
      if( lPdgcodeCurrentPart == -3122 ){
         f3dHistPrimAnalysisPtVsYVsMultAntiLambda->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
      }
      if( lPdgcodeCurrentPart == 310 ){
         f3dHistPrimAnalysisPtVsYVsMultK0Short->Fill(lPtCurrentPart, lRapCurrentPart, lMultiplicity);
      }
   }

//----- Loop on primary Xi, Omega --------------------------------------------------------------
   for (Int_t iCurrentLabelStack = 0; iCurrentLabelStack < lNbMCPrimary; iCurrentLabelStack++) 
   {// This is the begining of the loop on primaries
      
      TParticle* lCurrentParticlePrimary = 0x0; 
      lCurrentParticlePrimary = lMCstack->Particle( iCurrentLabelStack );
      if(!lCurrentParticlePrimary){
         Printf("Cascade loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skip ...\n", iCurrentLabelStack );
         continue;
      }
      if ( TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3312 || TMath::Abs(lCurrentParticlePrimary->GetPdgCode()) == 3334 ) { 
         Double_t lRapXiMCPrimary = -100;
         if( (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) != 0 ) { 
           if ( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) !=0 ){
             lRapXiMCPrimary = 0.5*TMath::Log( (lCurrentParticlePrimary->Energy() + lCurrentParticlePrimary->Pz()) / (lCurrentParticlePrimary->Energy() - lCurrentParticlePrimary->Pz() +1.e-13) );
           }
         }

         //=================================================================================
         // Xi Histograms
         if( lCurrentParticlePrimary->GetPdgCode() == 3312 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenSelectedPtVsYVsMultXiMinus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
         if( lCurrentParticlePrimary->GetPdgCode() == -3312 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenSelectedPtVsYVsMultXiPlus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
         // Omega Histograms
         if( lCurrentParticlePrimary->GetPdgCode() == 3334 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenSelectedPtVsYVsMultOmegaMinus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
         if( lCurrentParticlePrimary->GetPdgCode() == -3334 ){ 
            lPtCurrentPart    = lCurrentParticlePrimary->Pt();
            f3dHistGenSelectedPtVsYVsMultOmegaPlus->Fill(lPtCurrentPart, lRapXiMCPrimary, lMultiplicity);
         }
      } 
   }
//----- End Loop on primary Xi, Omega ----------------------------------------------------------

//------------------------------------------------
// MAIN LAMBDA LOOP STARTS HERE
//------------------------------------------------

   //Variable definition
   Int_t    lOnFlyStatus = 0;
   Double_t lChi2V0 = 0;
   Double_t lDcaV0Daughters = 0, lDcaV0ToPrimVertex = 0;
   Double_t lDcaPosToPrimVertex = 0, lDcaNegToPrimVertex = 0;
   Double_t lV0CosineOfPointingAngle = 0;
   Double_t lV0Radius = 0, lPt = 0;
   Double_t lRapK0Short = 0, lRapLambda = 0;
   Double_t lInvMassK0s = 0, lInvMassLambda = 0, lInvMassAntiLambda = 0;
   Double_t lAlphaV0 = 0, lPtArmV0 = 0;
   Double_t fMinV0Pt = 0; 
   Double_t fMaxV0Pt = 100; 

   Int_t nv0s = 0;
   nv0s = lESDevent->GetNumberOfV0s();
   
   for (Int_t iV0 = 0; iV0 < nv0s; iV0++) 
	{// This is the begining of the V0 loop
      AliESDv0 *v0 = ((AliESDEvent*)lESDevent)->GetV0(iV0);
      if (!v0) continue;

      //---> Fix On-the-Fly candidates
      if( v0->GetParamN()->Charge() > 0 && v0->GetParamP()->Charge() < 0 ){
        fHistSwappedV0Counter -> Fill( 1 );
      }else{
        fHistSwappedV0Counter -> Fill( 0 ); 
      }
      if ( fkUseOnTheFly ) CheckChargeV0(v0); 


      Double_t tV0mom[3];
      v0->GetPxPyPz( tV0mom[0],tV0mom[1],tV0mom[2] ); 
      Double_t lV0TotalMomentum = TMath::Sqrt(
         tV0mom[0]*tV0mom[0]+tV0mom[1]*tV0mom[1]+tV0mom[2]*tV0mom[2] );

      Double_t tDecayVertexV0[3]; v0->GetXYZ(tDecayVertexV0[0],tDecayVertexV0[1],tDecayVertexV0[2]); 
      lV0Radius = TMath::Sqrt(tDecayVertexV0[0]*tDecayVertexV0[0]+tDecayVertexV0[1]*tDecayVertexV0[1]);
      lPt = v0->Pt();
      lRapK0Short = v0->RapK0Short();
      lRapLambda  = v0->RapLambda();
      if ((lPt<fMinV0Pt)||(fMaxV0Pt<lPt)) continue;

      UInt_t lKeyPos = (UInt_t)TMath::Abs(v0->GetPindex());
      UInt_t lKeyNeg = (UInt_t)TMath::Abs(v0->GetNindex());

      Double_t lMomPos[3]; v0->GetPPxPyPz(lMomPos[0],lMomPos[1],lMomPos[2]);
      Double_t lMomNeg[3]; v0->GetNPxPyPz(lMomNeg[0],lMomNeg[1],lMomNeg[2]);

      AliESDtrack *pTrack=((AliESDEvent*)lESDevent)->GetTrack(lKeyPos);
      AliESDtrack *nTrack=((AliESDEvent*)lESDevent)->GetTrack(lKeyNeg);
      if (!pTrack || !nTrack) {
         Printf("ERROR: Could not retreive one of the daughter track");
         continue;
      }

      fTreeVariableNegEta = nTrack->Eta();
      fTreeVariablePosEta = pTrack->Eta();

      // Filter like-sign V0 (next: add counter and distribution)
      if ( pTrack->GetSign() == nTrack->GetSign()){
         continue;
      } 

      //________________________________________________________________________
      // Track quality cuts 
      Float_t lPosTrackCrossedRows = pTrack->GetTPCClusterInfo(2,1);
      Float_t lNegTrackCrossedRows = nTrack->GetTPCClusterInfo(2,1);
      fTreeVariableLeastNbrCrossedRows = (Int_t) lPosTrackCrossedRows;
      if( lNegTrackCrossedRows < fTreeVariableLeastNbrCrossedRows )
         fTreeVariableLeastNbrCrossedRows = (Int_t) lNegTrackCrossedRows;

      // TPC refit condition (done during reconstruction for Offline but not for On-the-fly)
      if( !(pTrack->GetStatus() & AliESDtrack::kTPCrefit)) continue;      
      if( !(nTrack->GetStatus() & AliESDtrack::kTPCrefit)) continue;

      if ( ( ( pTrack->GetTPCClusterInfo(2,1) ) < 70 ) || ( ( nTrack->GetTPCClusterInfo(2,1) ) < 70 ) ) continue;
	
      //GetKinkIndex condition
      if( pTrack->GetKinkIndex(0)>0 || nTrack->GetKinkIndex(0)>0 ) continue;

      //Findable clusters > 0 condition
      if( pTrack->GetTPCNclsF()<=0 || nTrack->GetTPCNclsF()<=0 ) continue;

      //Compute ratio Crossed Rows / Findable clusters
      //Note: above test avoids division by zero! 
      Float_t lPosTrackCrossedRowsOverFindable = -1;
      Float_t lNegTrackCrossedRowsOverFindable = -1;
      if ( ((double)(pTrack->GetTPCNclsF()) ) != 0 ) lPosTrackCrossedRowsOverFindable = lPosTrackCrossedRows / ((double)(pTrack->GetTPCNclsF())); 
      if ( ((double)(nTrack->GetTPCNclsF()) ) != 0 ) lNegTrackCrossedRowsOverFindable = lNegTrackCrossedRows / ((double)(nTrack->GetTPCNclsF())); 

      fTreeVariableLeastRatioCrossedRowsOverFindable = lPosTrackCrossedRowsOverFindable;
      if( lNegTrackCrossedRowsOverFindable < fTreeVariableLeastRatioCrossedRowsOverFindable )
         fTreeVariableLeastRatioCrossedRowsOverFindable = lNegTrackCrossedRowsOverFindable;

      //Lowest Cut Level for Ratio Crossed Rows / Findable = 0.8, set here
      if ( fTreeVariableLeastRatioCrossedRowsOverFindable < 0.8 ) continue;

      //End track Quality Cuts
      //________________________________________________________________________

      lDcaPosToPrimVertex = TMath::Abs(pTrack->GetD(lPrimaryVtxPosition[0],
							lPrimaryVtxPosition[1],
							lMagneticField) );

      lDcaNegToPrimVertex = TMath::Abs(nTrack->GetD(lPrimaryVtxPosition[0],
							lPrimaryVtxPosition[1],
							lMagneticField) );

      lOnFlyStatus = v0->GetOnFlyStatus();
      lChi2V0 = v0->GetChi2V0();
      lDcaV0Daughters = v0->GetDcaV0Daughters();
      lDcaV0ToPrimVertex = v0->GetD(lPrimaryVtxPosition[0],lPrimaryVtxPosition[1],lPrimaryVtxPosition[2]);
      lV0CosineOfPointingAngle = v0->GetV0CosineOfPointingAngle(lPrimaryVtxPosition[0],lPrimaryVtxPosition[1],lPrimaryVtxPosition[2]);
      fTreeVariableV0CosineOfPointingAngle=lV0CosineOfPointingAngle;

      // Getting invariant mass infos directly from ESD
      v0->ChangeMassHypothesis(310);
      lInvMassK0s = v0->GetEffMass();
      v0->ChangeMassHypothesis(3122);
      lInvMassLambda = v0->GetEffMass();
      v0->ChangeMassHypothesis(-3122);
      lInvMassAntiLambda = v0->GetEffMass();
      lAlphaV0 = v0->AlphaV0();
      lPtArmV0 = v0->PtArmV0();

      //fTreeVariableOnFlyStatus = lOnFlyStatus;
      //fHistV0OnFlyStatus->Fill(lOnFlyStatus);

//===============================================
// Monte Carlo Association starts here
//===============================================

      //---> Set Everything to "I don't know" before starting

      fTreeVariablePIDPositive = 0;
      fTreeVariablePIDNegative = 0;

      fTreeVariableIndexStatus = 0;
      fTreeVariableIndexStatusMother = 0;

      fTreeVariablePtMother = -1;
      fTreeVariablePtMC = -1;
      fTreeVariableRapMC = -100;

      fTreeVariablePID = -1; 
      fTreeVariablePIDMother = -1;

      fTreeVariablePrimaryStatus = 0; 
      fTreeVariablePrimaryStatusMother = 0; 
      fTreeVariableV0CreationRadius = -1;

      Int_t lblPosV0Dghter = (Int_t) TMath::Abs( pTrack->GetLabel() );  
      Int_t lblNegV0Dghter = (Int_t) TMath::Abs( nTrack->GetLabel() );
		
      TParticle* mcPosV0Dghter = lMCstack->Particle( lblPosV0Dghter );
      TParticle* mcNegV0Dghter = lMCstack->Particle( lblNegV0Dghter );
	    
      fTreeVariablePosTransvMomentumMC = mcPosV0Dghter->Pt();
      fTreeVariableNegTransvMomentumMC = mcNegV0Dghter->Pt();

      Int_t lPIDPositive = mcPosV0Dghter -> GetPdgCode();
      Int_t lPIDNegative = mcNegV0Dghter -> GetPdgCode();

      fTreeVariablePIDPositive = lPIDPositive;
      fTreeVariablePIDNegative = lPIDNegative;

      Int_t lblMotherPosV0Dghter = mcPosV0Dghter->GetFirstMother() ; 
      Int_t lblMotherNegV0Dghter = mcNegV0Dghter->GetFirstMother();

      if( lblMotherPosV0Dghter == lblMotherNegV0Dghter && lblMotherPosV0Dghter > -1 ){
         //either label is fine, they're equal at this stage
         TParticle* pThisV0 = lMCstack->Particle( lblMotherPosV0Dghter ); 
         //Set tree variables
         fTreeVariablePID   = pThisV0->GetPdgCode(); //PDG Code
         fTreeVariablePtMC  = pThisV0->Pt(); //Perfect Pt
         //Only Interested if it's a Lambda, AntiLambda or K0s 
         //Avoid the Junction Bug! PYTHIA has particles with Px=Py=Pz=E=0 occasionally, 
         //having particle code 88 (unrecognized by PDG), for documentation purposes.
         //Even ROOT's TParticle::Y() is not prepared to deal with that exception!
         //Note that TParticle::Pt() is immune (that would just return 0)...
         //Though granted that that should be extremely rare in this precise condition...
         if( TMath::Abs(fTreeVariablePID) == 3122 || fTreeVariablePID==310 ){
            fTreeVariableRapMC = pThisV0->Y(); //Perfect Y
         }
         fTreeVariableV0CreationRadius = TMath::Sqrt(
          TMath::Power(  ( (mcPrimaryVtx.At(0)) - (pThisV0->Vx()) ) , 2) + 
          TMath::Power(  ( (mcPrimaryVtx.At(1)) - (pThisV0->Vy()) ) , 2) + 
          TMath::Power(  ( (mcPrimaryVtx.At(2)) - (pThisV0->Vz()) ) , 2) 
         );
         if( lblMotherPosV0Dghter  < lNbMCPrimary ) fTreeVariableIndexStatus = 1; //looks primary
         if( lblMotherPosV0Dghter >= lNbMCPrimary ) fTreeVariableIndexStatus = 2; //looks secondary
         if( lMCstack->IsPhysicalPrimary       (lblMotherPosV0Dghter) ) fTreeVariablePrimaryStatus = 1; //Is Primary!
         if( lMCstack->IsSecondaryFromWeakDecay(lblMotherPosV0Dghter) ) fTreeVariablePrimaryStatus = 2; //Weak Decay!
         if( lMCstack->IsSecondaryFromMaterial (lblMotherPosV0Dghter) ) fTreeVariablePrimaryStatus = 3; //Material Int!
         
         //Now we try to acquire the V0 parent particle, if possible
         Int_t lblThisV0Parent = pThisV0->GetFirstMother();
         if ( lblThisV0Parent > -1 ){ //if it has a parent, get it and store specs
            TParticle* pThisV0Parent = lMCstack->Particle( lblThisV0Parent );
            fTreeVariablePIDMother   = pThisV0Parent->GetPdgCode(); //V0 Mother PDG
            fTreeVariablePtMother    = pThisV0Parent->Pt();         //V0 Mother Pt
            //Primary Status for the V0 Mother particle 
            if( lblThisV0Parent  < lNbMCPrimary ) fTreeVariableIndexStatusMother = 1; //looks primary
            if( lblThisV0Parent >= lNbMCPrimary ) fTreeVariableIndexStatusMother = 2; //looks secondary
            if( lMCstack->IsPhysicalPrimary       (lblThisV0Parent) ) fTreeVariablePrimaryStatusMother = 1; //Is Primary!
            if( lMCstack->IsSecondaryFromWeakDecay(lblThisV0Parent) ) fTreeVariablePrimaryStatusMother = 2; //Weak Decay!
            if( lMCstack->IsSecondaryFromMaterial (lblThisV0Parent) ) fTreeVariablePrimaryStatusMother = 3; //Material Int!
         }
      }

      fTreeVariablePt = v0->Pt();
      fTreeVariableChi2V0 = lChi2V0; 
      fTreeVariableDcaV0ToPrimVertex = lDcaV0ToPrimVertex;
      fTreeVariableDcaV0Daughters = lDcaV0Daughters;
      fTreeVariableV0CosineOfPointingAngle = lV0CosineOfPointingAngle; 
      fTreeVariableV0Radius = lV0Radius;
      fTreeVariableDcaPosToPrimVertex = lDcaPosToPrimVertex;
      fTreeVariableDcaNegToPrimVertex = lDcaNegToPrimVertex;
      fTreeVariableInvMassK0s = lInvMassK0s;
      fTreeVariableInvMassLambda = lInvMassLambda;
      fTreeVariableInvMassAntiLambda = lInvMassAntiLambda;
      fTreeVariableRapK0Short = lRapK0Short;

      fTreeVariableRapLambda = lRapLambda;
      fTreeVariableAlphaV0 = lAlphaV0;
      fTreeVariablePtArmV0 = lPtArmV0;

      //Official means of acquiring N-sigmas 
      fTreeVariableNSigmasPosProton = fPIDResponse->NumberOfSigmasTPC( pTrack, AliPID::kProton );
      fTreeVariableNSigmasPosPion   = fPIDResponse->NumberOfSigmasTPC( pTrack, AliPID::kPion );
      fTreeVariableNSigmasNegProton = fPIDResponse->NumberOfSigmasTPC( nTrack, AliPID::kProton );
      fTreeVariableNSigmasNegPion   = fPIDResponse->NumberOfSigmasTPC( nTrack, AliPID::kPion );

//tDecayVertexV0[0],tDecayVertexV0[1],tDecayVertexV0[2]
      Double_t lDistanceTravelled = TMath::Sqrt(
						TMath::Power( tDecayVertexV0[0] - lBestPrimaryVtxPos[0] , 2) +
						TMath::Power( tDecayVertexV0[1] - lBestPrimaryVtxPos[1] , 2) +
						TMath::Power( tDecayVertexV0[2] - lBestPrimaryVtxPos[2] , 2)
					);
      fTreeVariableDistOverTotMom = 1e+5;
      if( lV0TotalMomentum + 1e-10 != 0 ) fTreeVariableDistOverTotMom = lDistanceTravelled / (lV0TotalMomentum + 1e-10); //avoid division by zero, to be sure

      Double_t lMomentumPosTemp[3];
      pTrack->GetPxPyPz(lMomentumPosTemp);
      Double_t lPtPosTemporary = sqrt(pow(lMomentumPosTemp[0],2) + pow(lMomentumPosTemp[1],2));

      Double_t lMomentumNegTemp[3];
      nTrack->GetPxPyPz(lMomentumNegTemp);
      Double_t lPtNegTemporary = sqrt(pow(lMomentumNegTemp[0],2) + pow(lMomentumNegTemp[1],2));

      fTreeVariablePosTransvMomentum = lPtPosTemporary;
      fTreeVariableNegTransvMomentum = lPtNegTemporary;


//------------------------------------------------
// Fill Tree! 
//------------------------------------------------

      // The conditionals are meant to decrease excessive
      // memory usage! 

      //Modified version: Keep only OnFlyStatus == 0
      //Keep only if included in a parametric InvMass Region 20 sigmas away from peak

      //First Selection: Reject OnFly
      if( (lOnFlyStatus == 0 && fkUseOnTheFly == kFALSE) || (lOnFlyStatus != 0 && fkUseOnTheFly == kTRUE ) ){
         //Second Selection: rough 20-sigma band, parametric. 
         //K0Short: Enough to parametrize peak broadening with linear function.    
         Double_t lUpperLimitK0Short = (5.63707e-01) + (1.14979e-02)*fTreeVariablePt; 
         Double_t lLowerLimitK0Short = (4.30006e-01) - (1.10029e-02)*fTreeVariablePt;
         //Lambda: Linear (for higher pt) plus exponential (for low-pt broadening)
         //[0]+[1]*x+[2]*TMath::Exp(-[3]*x)
         Double_t lUpperLimitLambda = (1.13688e+00) + (5.27838e-03)*fTreeVariablePt + (8.42220e-02)*TMath::Exp(-(3.80595e+00)*fTreeVariablePt); 
         Double_t lLowerLimitLambda = (1.09501e+00) - (5.23272e-03)*fTreeVariablePt - (7.52690e-02)*TMath::Exp(-(3.46339e+00)*fTreeVariablePt);
         //Do Selection      
         if( (fTreeVariableInvMassLambda     < lUpperLimitLambda  && fTreeVariableInvMassLambda     > lLowerLimitLambda     ) || 
             (fTreeVariableInvMassAntiLambda < lUpperLimitLambda  && fTreeVariableInvMassAntiLambda > lLowerLimitLambda     ) || 
             (fTreeVariableInvMassK0s        < lUpperLimitK0Short && fTreeVariableInvMassK0s        > lLowerLimitK0Short    ) ){
             //Pre-selection in case this is AA...
             if( fkIsNuclear == kFALSE ) fTree->Fill();
             if( fkIsNuclear == kTRUE){ 
             //If this is a nuclear collision___________________
             // ... pre-filter with daughter eta selection only (not TPC)
               if ( TMath::Abs(fTreeVariableNegEta)<0.8 && TMath::Abs(fTreeVariablePosEta)<0.8 ) fTree->Fill();
             }//end nuclear_____________________________________
         }
      }

//------------------------------------------------
// Fill tree over.
//------------------------------------------------


   }// This is the end of the V0 loop

//------------------------------------------------


//------------------------------------------------
// MAIN CASCADE LOOP STARTS HERE
//------------------------------------------------
// Code Credit: Antonin Maire (thanks^100)
// ---> This is an adaptation

  Long_t ncascades = 0;
	ncascades = lESDevent->GetNumberOfCascades();


  for (Int_t iXi = 0; iXi < ncascades; iXi++){
    //------------------------------------------------
    // Initializations
    //------------------------------------------------	
	  //Double_t lTrkgPrimaryVtxRadius3D = -500.0;
	  //Double_t lBestPrimaryVtxRadius3D = -500.0;

	  // - 1st part of initialisation : variables needed to store AliESDCascade data members
	  Double_t lEffMassXi      = 0. ;
	  //Double_t lChi2Xi         = -1. ;
	  Double_t lDcaXiDaughters = -1. ;
	  Double_t lXiCosineOfPointingAngle = -1. ;
	  Double_t lPosXi[3] = { -1000.0, -1000.0, -1000.0 };
	  Double_t lXiRadius = -1000. ;
          
	  // - 2nd part of initialisation : Nbr of clusters within TPC for the 3 daughter cascade tracks
	  Int_t    lPosTPCClusters    = -1; // For ESD only ...//FIXME : wait for availability in AOD
	  Int_t    lNegTPCClusters    = -1; // For ESD only ...
	  Int_t    lBachTPCClusters   = -1; // For ESD only ...
          		
          // - 3rd part of initialisation : about V0 part in cascades
	  Double_t lInvMassLambdaAsCascDghter = 0.;
	  //Double_t lV0Chi2Xi         = -1. ;
	  Double_t lDcaV0DaughtersXi = -1.;
		
	  Double_t lDcaBachToPrimVertexXi = -1., lDcaV0ToPrimVertexXi = -1.;
	  Double_t lDcaPosToPrimVertexXi  = -1.;
	  Double_t lDcaNegToPrimVertexXi  = -1.;
	  Double_t lV0CosineOfPointingAngleXi = -1. ;
	  Double_t lPosV0Xi[3] = { -1000. , -1000., -1000. }; // Position of VO coming from cascade
	  Double_t lV0RadiusXi = -1000.0;
	  Double_t lV0quality  = 0.;
	
	  // - 4th part of initialisation : Effective masses
	  Double_t lInvMassXiMinus    = 0.;
	  Double_t lInvMassXiPlus     = 0.;
	  Double_t lInvMassOmegaMinus = 0.;
	  Double_t lInvMassOmegaPlus  = 0.;
    
	  // - 6th part of initialisation : extra info for QA
	  Double_t lXiMomX       = 0. , lXiMomY = 0., lXiMomZ = 0.;
	  Double_t lXiTransvMom  = 0. ;
	  Double_t lXiTotMom     = 0. ;
		
	  Double_t lBachMomX       = 0., lBachMomY  = 0., lBachMomZ   = 0.;
	  //Double_t lBachTransvMom  = 0.;
	  //Double_t lBachTotMom     = 0.;
	
	  Short_t  lChargeXi = -2;
	  //Double_t lV0toXiCosineOfPointingAngle = 0. ;
	
	  Double_t lRapXi   = -20.0, lRapOmega = -20.0; //  lEta = -20.0, lTheta = 360., lPhi = 720. ;
	  //Double_t lAlphaXi = -200., lPtArmXi  = -200.0;
	    
    // -------------------------------------
    // II.ESD - Calculation Part dedicated to Xi vertices (ESD)
    
	  AliESDcascade *xi = lESDevent->GetCascade(iXi);
	  if (!xi) continue;
	
          
                  // - II.Step 1 : around primary vertex
                  //-------------
          //lTrkgPrimaryVtxRadius3D = TMath::Sqrt(  lTrkgPrimaryVtxPos[0] * lTrkgPrimaryVtxPos[0] +
          //                                        lTrkgPrimaryVtxPos[1] * lTrkgPrimaryVtxPos[1] +
          //                                        lTrkgPrimaryVtxPos[2] * lTrkgPrimaryVtxPos[2] );

          //lBestPrimaryVtxRadius3D = TMath::Sqrt(  lBestPrimaryVtxPos[0] * lBestPrimaryVtxPos[0] +
          //                                        lBestPrimaryVtxPos[1] * lBestPrimaryVtxPos[1] +
          //                                        lBestPrimaryVtxPos[2] * lBestPrimaryVtxPos[2] );

		// - II.Step 2 : Assigning the necessary variables for specific AliESDcascade data members (ESD)	
		//-------------
	  lV0quality = 0.;
	  xi->ChangeMassHypothesis(lV0quality , 3312); // default working hypothesis : cascade = Xi- decay

	  lEffMassXi  			= xi->GetEffMassXi();
	  //lChi2Xi 			    = xi->GetChi2Xi();
	  lDcaXiDaughters 	= xi->GetDcaXiDaughters();
	  lXiCosineOfPointingAngle 	            = xi->GetCascadeCosineOfPointingAngle( lBestPrimaryVtxPos[0],
                                                                                 lBestPrimaryVtxPos[1],
                                                                                 lBestPrimaryVtxPos[2] );
		  // Take care : the best available vertex should be used (like in AliCascadeVertexer)
	
	  xi->GetXYZcascade( lPosXi[0],  lPosXi[1], lPosXi[2] ); 
	  lXiRadius			= TMath::Sqrt( lPosXi[0]*lPosXi[0]  +  lPosXi[1]*lPosXi[1] );		

		// - II.Step 3 : around the tracks : Bach + V0 (ESD)
		// ~ Necessary variables for ESDcascade data members coming from the ESDv0 part (inheritance)
		//-------------
		
        UInt_t lIdxPosXi 	= (UInt_t) TMath::Abs( xi->GetPindex() );
        UInt_t lIdxNegXi 	= (UInt_t) TMath::Abs( xi->GetNindex() );
        UInt_t lBachIdx 	= (UInt_t) TMath::Abs( xi->GetBindex() );
                // Care track label can be negative in MC production (linked with the track quality)
                // However = normally, not the case for track index ...
          
	  // FIXME : rejection of a double use of a daughter track (nothing but just a crosscheck of what is done in the cascade vertexer)
	  if(lBachIdx == lIdxNegXi) {
		  AliWarning("Pb / Idx(Bach. track) = Idx(Neg. track) ... continue!"); continue;
	  }
    if(lBachIdx == lIdxPosXi) {
    	AliWarning("Pb / Idx(Bach. track) = Idx(Pos. track) ... continue!"); continue;
	  }
          
	  AliESDtrack *pTrackXi		= lESDevent->GetTrack( lIdxPosXi );
	  AliESDtrack *nTrackXi		= lESDevent->GetTrack( lIdxNegXi );
	  AliESDtrack *bachTrackXi	= lESDevent->GetTrack( lBachIdx );

	  if (!pTrackXi || !nTrackXi || !bachTrackXi ) {
		  AliWarning("ERROR: Could not retrieve one of the 3 ESD daughter tracks of the cascade ...");
		  continue;
	  }

   fTreeCascVarPosEta = pTrackXi->Eta();
   fTreeCascVarNegEta = nTrackXi->Eta();
   fTreeCascVarBachEta = bachTrackXi->Eta();
  
    //------------------------------------------------
    // TPC Number of clusters info
    // --- modified to save the smallest number 
    // --- of TPC clusters for the 3 tracks
    //------------------------------------------------
              
	  lPosTPCClusters   = pTrackXi->GetTPCNcls();
	  lNegTPCClusters   = nTrackXi->GetTPCNcls();
	  lBachTPCClusters  = bachTrackXi->GetTPCNcls(); 

    // 1 - Poor quality related to TPCrefit
	  ULong_t pStatus    = pTrackXi->GetStatus();
	  ULong_t nStatus    = nTrackXi->GetStatus();
	  ULong_t bachStatus = bachTrackXi->GetStatus();
    if ((pStatus&AliESDtrack::kTPCrefit)    == 0) { AliWarning("Pb / V0 Pos. track has no TPCrefit ... continue!"); continue; }
    if ((nStatus&AliESDtrack::kTPCrefit)    == 0) { AliWarning("Pb / V0 Neg. track has no TPCrefit ... continue!"); continue; }
    if ((bachStatus&AliESDtrack::kTPCrefit) == 0) { AliWarning("Pb / Bach.   track has no TPCrefit ... continue!"); continue; }
	  // 2 - Poor quality related to TPC clusters: lowest cut of 70 clusters
    if(lPosTPCClusters  < 70) { AliWarning("Pb / V0 Pos. track has less than 70 TPC clusters ... continue!"); continue; }
	  if(lNegTPCClusters  < 70) { AliWarning("Pb / V0 Neg. track has less than 70 TPC clusters ... continue!"); continue; }
	  if(lBachTPCClusters < 70) { AliWarning("Pb / Bach.   track has less than 70 TPC clusters ... continue!"); continue; }
	  Int_t leastnumberofclusters = 1000; 
	  if( lPosTPCClusters < leastnumberofclusters ) leastnumberofclusters = lPosTPCClusters;
	  if( lNegTPCClusters < leastnumberofclusters ) leastnumberofclusters = lNegTPCClusters;
	  if( lBachTPCClusters < leastnumberofclusters ) leastnumberofclusters = lBachTPCClusters;

	  lInvMassLambdaAsCascDghter	= xi->GetEffMass();
	  // This value shouldn't change, whatever the working hyp. is : Xi-, Xi+, Omega-, Omega+
	  lDcaV0DaughtersXi 		= xi->GetDcaV0Daughters(); 
	  //lV0Chi2Xi 			= xi->GetChi2V0();
	
	  lV0CosineOfPointingAngleXi 	= xi->GetV0CosineOfPointingAngle( lBestPrimaryVtxPos[0],
									    lBestPrimaryVtxPos[1],
									    lBestPrimaryVtxPos[2] );

	  lDcaV0ToPrimVertexXi 		= xi->GetD( lBestPrimaryVtxPos[0], 
						      lBestPrimaryVtxPos[1], 
						      lBestPrimaryVtxPos[2] );
		
	  lDcaBachToPrimVertexXi = TMath::Abs( bachTrackXi->GetD(	lBestPrimaryVtxPos[0], 
						       		lBestPrimaryVtxPos[1], 
						       		lMagneticField  ) ); 
					  // Note : AliExternalTrackParam::GetD returns an algebraic value ...
		
	  xi->GetXYZ( lPosV0Xi[0],  lPosV0Xi[1], lPosV0Xi[2] ); 
	  lV0RadiusXi		= TMath::Sqrt( lPosV0Xi[0]*lPosV0Xi[0]  +  lPosV0Xi[1]*lPosV0Xi[1] );
	
	  lDcaPosToPrimVertexXi 	= TMath::Abs( pTrackXi	->GetD(	lBestPrimaryVtxPos[0], 
						   		lBestPrimaryVtxPos[1], 
						   		lMagneticField  )     ); 
	
	  lDcaNegToPrimVertexXi 	= TMath::Abs( nTrackXi	->GetD(	lBestPrimaryVtxPos[0], 
					      			lBestPrimaryVtxPos[1], 
					      			lMagneticField  )     ); 
		
	  // - II.Step 4 : around effective masses (ESD)
	  // ~ change mass hypotheses to cover all the possibilities :  Xi-/+, Omega -/+
		
	  if( bachTrackXi->Charge() < 0 )	{
		  lV0quality = 0.;
		  xi->ChangeMassHypothesis(lV0quality , 3312); 	
			  // Calculate the effective mass of the Xi- candidate. 
			  // pdg code 3312 = Xi-
		  lInvMassXiMinus = xi->GetEffMassXi();
		
		  lV0quality = 0.;
		  xi->ChangeMassHypothesis(lV0quality , 3334); 	
			  // Calculate the effective mass of the Xi- candidate. 
			  // pdg code 3334 = Omega-
		  lInvMassOmegaMinus = xi->GetEffMassXi();
					
		  lV0quality = 0.;
		  xi->ChangeMassHypothesis(lV0quality , 3312); 	// Back to default hyp.
	  }// end if negative bachelor
	
	
	  if( bachTrackXi->Charge() >  0 ){
		  lV0quality = 0.;
		  xi->ChangeMassHypothesis(lV0quality , -3312); 	
			  // Calculate the effective mass of the Xi+ candidate. 
			  // pdg code -3312 = Xi+
		  lInvMassXiPlus = xi->GetEffMassXi();
		
		  lV0quality = 0.;
		  xi->ChangeMassHypothesis(lV0quality , -3334); 	
			  // Calculate the effective mass of the Xi+ candidate. 
			  // pdg code -3334  = Omega+
		  lInvMassOmegaPlus = xi->GetEffMassXi();
		
		  lV0quality = 0.;
		  xi->ChangeMassHypothesis(lV0quality , -3312); 	// Back to "default" hyp.
	  }// end if positive bachelor
		  // - II.Step 6 : extra info for QA (ESD)
		  // miscellaneous pieces of info that may help regarding data quality assessment.
		  //-------------

	  xi->GetPxPyPz( lXiMomX, lXiMomY, lXiMomZ );
		  lXiTransvMom  	= TMath::Sqrt( lXiMomX*lXiMomX   + lXiMomY*lXiMomY );
		  lXiTotMom  	= TMath::Sqrt( lXiMomX*lXiMomX   + lXiMomY*lXiMomY   + lXiMomZ*lXiMomZ );
		
	  xi->GetBPxPyPz(  lBachMomX,  lBachMomY,  lBachMomZ );
		  //lBachTransvMom  = TMath::Sqrt( lBachMomX*lBachMomX   + lBachMomY*lBachMomY );
		  //lBachTotMom  	= TMath::Sqrt( lBachMomX*lBachMomX   + lBachMomY*lBachMomY  +  lBachMomZ*lBachMomZ  );

	  lChargeXi = xi->Charge();

	  //lV0toXiCosineOfPointingAngle = xi->GetV0CosineOfPointingAngle( lPosXi[0], lPosXi[1], lPosXi[2] );
	
	  lRapXi    = xi->RapXi();
	  lRapOmega = xi->RapOmega();
	  //lEta      = xi->Eta();
	  //lTheta    = xi->Theta() *180.0/TMath::Pi();
	  //lPhi      = xi->Phi()   *180.0/TMath::Pi();
	  //lAlphaXi  = xi->AlphaXi();
	  //lPtArmXi  = xi->PtArmXi();
	
//------------------------------------------------
// Associate Cascade Candidates to Monte Carlo!
//------------------------------------------------	

//Warning: Not using Continues... Need to fill tree later!
	
	Int_t lPDGCodeCascade = 0;	

	Int_t lPID_BachMother = 0;
	Int_t lPID_NegMother = 0;
	Int_t lPID_PosMother = 0;


	  fTreeCascVarPIDPositive = 0;
	  fTreeCascVarPIDNegative = 0;
	  fTreeCascVarPIDBachelor = 0;


	if(fDebug > 5)
		cout 	<< "MC EventNumber : " << lMCevent->Header()->GetEvent() 
			<< " / MC event Number in Run : " << lMCevent->Header()->GetEventNrInRun() << endl;
	
	// - Step 4.1 : level of the V0 daughters
		
//----------------------------------------
// Regular MC ASSOCIATION STARTS HERE
//----------------------------------------

	  Int_t lblPosV0Dghter = (Int_t) TMath::Abs( pTrackXi->GetLabel() );  
		  // Abs value = needed ! question of quality track association ...
	  Int_t lblNegV0Dghter = (Int_t) TMath::Abs( nTrackXi->GetLabel() );
	  Int_t lblBach        = (Int_t) TMath::Abs( bachTrackXi->GetLabel() );

	  TParticle* mcPosV0Dghter = lMCstack->Particle( lblPosV0Dghter );
	  TParticle* mcNegV0Dghter = lMCstack->Particle( lblNegV0Dghter );
	  TParticle* mcBach        = lMCstack->Particle( lblBach );	
	
    fTreeCascVarPosTransMomMC = mcPosV0Dghter->Pt();
    fTreeCascVarNegTransMomMC = mcNegV0Dghter->Pt();

	  fTreeCascVarPIDPositive = mcPosV0Dghter -> GetPdgCode();
	  fTreeCascVarPIDNegative = mcNegV0Dghter -> GetPdgCode();
	  fTreeCascVarPIDBachelor = mcBach->GetPdgCode();

	  // - Step 4.2 : level of the Xi daughters
		
	  Int_t lblMotherPosV0Dghter = mcPosV0Dghter->GetFirstMother() ; 
	  Int_t lblMotherNegV0Dghter = mcNegV0Dghter->GetFirstMother();
	
	  //Rather uncivilized: Open brackets for each 'continue'
	  if(! (lblMotherPosV0Dghter != lblMotherNegV0Dghter) ) { // same mother
	  if(! (lblMotherPosV0Dghter < 0) ) { // mother != primary (!= -1)
	  if(! (lblMotherNegV0Dghter < 0) ) {
					
		// mothers = Lambda candidate ... a priori
	
	  TParticle* mcMotherPosV0Dghter = lMCstack->Particle( lblMotherPosV0Dghter );
	  TParticle* mcMotherNegV0Dghter = lMCstack->Particle( lblMotherNegV0Dghter );
			
	  // - Step 4.3 : level of Xi candidate
	
	  Int_t lblGdMotherPosV0Dghter =   mcMotherPosV0Dghter->GetFirstMother() ;
	  Int_t lblGdMotherNegV0Dghter =   mcMotherNegV0Dghter->GetFirstMother() ;
				
		if(! (lblGdMotherPosV0Dghter != lblGdMotherNegV0Dghter) ) {
		if(! (lblGdMotherPosV0Dghter < 0) ) { // primary lambda ...
		if(! (lblGdMotherNegV0Dghter < 0) ) { // primary lambda ...

		  // Gd mothers = Xi candidate ... a priori
	
	  TParticle* mcGdMotherPosV0Dghter = lMCstack->Particle( lblGdMotherPosV0Dghter );
	  TParticle* mcGdMotherNegV0Dghter = lMCstack->Particle( lblGdMotherNegV0Dghter );
					
	  Int_t lblMotherBach = (Int_t) TMath::Abs( mcBach->GetFirstMother()  );
	
  //		if( lblMotherBach != lblGdMotherPosV0Dghter ) continue; //same mother for bach and V0 daughters
		  if(!(lblMotherBach != lblGdMotherPosV0Dghter)) { //same mother for bach and V0 daughters
	
	  TParticle* mcMotherBach = lMCstack->Particle( lblMotherBach );
	
    lPID_BachMother = mcMotherBach->GetPdgCode();
	  lPID_NegMother = mcGdMotherPosV0Dghter->GetPdgCode();
	  lPID_PosMother = mcGdMotherNegV0Dghter->GetPdgCode();
   
	  if(lPID_BachMother==lPID_NegMother && lPID_BachMother==lPID_PosMother){ 
		  lPDGCodeCascade = lPID_BachMother; 
	  }

  }}}}}}} //Ends all conditionals above...

  //----------------------------------------
  // Regular MC ASSOCIATION ENDS HERE
  //----------------------------------------

  //------------------------------------------------
  // Set Variables for adding to tree
  //------------------------------------------------		
	
/* 1*/		fTreeCascVarCharge	= lChargeXi;
/* 2*/		if(lInvMassXiMinus!=0)    fTreeCascVarMassAsXi = lInvMassXiMinus;
/* 2*/		if(lInvMassXiPlus!=0)     fTreeCascVarMassAsXi = lInvMassXiPlus;
/* 3*/		if(lInvMassOmegaMinus!=0) fTreeCascVarMassAsOmega = lInvMassOmegaMinus;
/* 3*/		if(lInvMassOmegaPlus!=0)  fTreeCascVarMassAsOmega = lInvMassOmegaPlus;
/* 4*/		fTreeCascVarPt = lXiTransvMom;
/* 5*/		fTreeCascVarRapXi = lRapXi ;
/* 6*/		fTreeCascVarRapOmega = lRapOmega ;
/* 7*/		fTreeCascVarDCACascDaughters = lDcaXiDaughters;
/* 8*/		fTreeCascVarDCABachToPrimVtx = lDcaBachToPrimVertexXi;
/* 9*/		fTreeCascVarDCAV0Daughters = lDcaV0DaughtersXi;
/*10*/		fTreeCascVarDCAV0ToPrimVtx = lDcaV0ToPrimVertexXi;
/*11*/		fTreeCascVarDCAPosToPrimVtx = lDcaPosToPrimVertexXi;
/*12*/		fTreeCascVarDCANegToPrimVtx = lDcaNegToPrimVertexXi;
/*13*/		fTreeCascVarCascCosPointingAngle = lXiCosineOfPointingAngle;
/*14*/		fTreeCascVarCascRadius = lXiRadius;
/*15*/		fTreeCascVarV0Mass = lInvMassLambdaAsCascDghter;
/*16*/		fTreeCascVarV0CosPointingAngle = lV0CosineOfPointingAngleXi;
/*17*/		fTreeCascVarV0Radius = lV0RadiusXi;
/*20*/		fTreeCascVarLeastNbrClusters = leastnumberofclusters;
/*21*/		fTreeCascVarMultiplicity = lMultiplicity; //multiplicity, whatever that may be

/*23*/		fTreeCascVarDistOverTotMom = TMath::Sqrt(
						TMath::Power( lPosXi[0] - lBestPrimaryVtxPos[0] , 2) +
						TMath::Power( lPosXi[1] - lBestPrimaryVtxPos[1] , 2) +
						TMath::Power( lPosXi[2] - lBestPrimaryVtxPos[2] , 2)
					);
/*23*/		fTreeCascVarDistOverTotMom /= (lXiTotMom+1e-13);
/*24*/    //Not specified here, it has been set already (TRunNumber)
/*25*/		fTreeCascVarPID = lPDGCodeCascade;

//------------------------------------------------
// Fill Tree! 
//------------------------------------------------

// The conditional is meant to decrease excessive
// memory usage! Be careful when loosening the 
// cut!

  //Xi    Mass window: 150MeV wide
  //Omega mass window: 150MeV wide

  if( (fTreeCascVarMassAsXi<1.32+0.075&&fTreeCascVarMassAsXi>1.32-0.075) ||
      (fTreeCascVarMassAsOmega<1.68+0.075&&fTreeCascVarMassAsOmega>1.68-0.075) ){
      fTreeCascade->Fill();
  }

//------------------------------------------------
// Fill tree over.
//------------------------------------------------

	}// end of the Cascade loop (ESD or AOD)

   // Post output data.
   PostData(1, fListHistV0);
   PostData(2, fTree);
   PostData(3, fTreeCascade);
}

//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0::Terminate(Option_t *)
{
   // Draw result to the screen
   // Called once at the end of the query

   TList *cRetrievedList = 0x0;
   cRetrievedList = (TList*)GetOutputData(1);
   if(!cRetrievedList){
      Printf("ERROR - AliAnalysisTaskExtractV0 : ouput data container list not available\n");
      return;
   }	
	
   fHistV0MultiplicityForTrigEvt = dynamic_cast<TH1F*> (  cRetrievedList->FindObject("fHistV0MultiplicityForTrigEvt")  );
   if (!fHistV0MultiplicityForTrigEvt) {
      Printf("ERROR - AliAnalysisTaskExtractV0 : fHistV0MultiplicityForTrigEvt not available");
      return;
   }
  
   TCanvas *canCheck = new TCanvas("AliAnalysisTaskExtractV0","V0 Multiplicity",10,10,510,510);
   canCheck->cd(1)->SetLogy();

   fHistV0MultiplicityForTrigEvt->SetMarkerStyle(22);
   fHistV0MultiplicityForTrigEvt->DrawCopy("E");
}

//----------------------------------------------------------------------------

Double_t AliAnalysisTaskExtractPerformanceV0::MyRapidity(Double_t rE, Double_t rPz) const
{
   // Local calculation for rapidity
   Double_t ReturnValue = -100;
   if( (rE-rPz+1.e-13) != 0 && (rE+rPz) != 0 ){ 
      ReturnValue =  0.5*TMath::Log((rE+rPz)/(rE-rPz+1.e-13));
   }
   return ReturnValue;
} 

//________________________________________________________________________
void AliAnalysisTaskExtractPerformanceV0::CheckChargeV0(AliESDv0 *v0)
{
   // This function checks charge of negative and positive daughter tracks. 
   // If incorrectly defined (onfly vertexer), swaps out. 
   if( v0->GetParamN()->Charge() > 0 && v0->GetParamP()->Charge() < 0 ){
      //V0 daughter track swapping is required! Note: everything is swapped here... P->N, N->P
      Long_t lCorrectNidx = v0->GetPindex();
      Long_t lCorrectPidx = v0->GetNindex();
      Double32_t	lCorrectNmom[3];
      Double32_t	lCorrectPmom[3];
      v0->GetPPxPyPz( lCorrectNmom[0], lCorrectNmom[1], lCorrectNmom[2] );
      v0->GetNPxPyPz( lCorrectPmom[0], lCorrectPmom[1], lCorrectPmom[2] );

      AliExternalTrackParam	lCorrectParamN(
        v0->GetParamP()->GetX() , 
        v0->GetParamP()->GetAlpha() , 
        v0->GetParamP()->GetParameter() , 
        v0->GetParamP()->GetCovariance() 
      );
      AliExternalTrackParam	lCorrectParamP(
        v0->GetParamN()->GetX() , 
        v0->GetParamN()->GetAlpha() , 
        v0->GetParamN()->GetParameter() , 
        v0->GetParamN()->GetCovariance() 
      );
      lCorrectParamN.SetMostProbablePt( v0->GetParamP()->GetMostProbablePt() );
      lCorrectParamP.SetMostProbablePt( v0->GetParamN()->GetMostProbablePt() );

      //Get Variables___________________________________________________
      Double_t lDcaV0Daughters = v0 -> GetDcaV0Daughters();
      Double_t lCosPALocal     = v0 -> GetV0CosineOfPointingAngle(); 
      Bool_t lOnFlyStatusLocal = v0 -> GetOnFlyStatus();

      //Create Replacement Object_______________________________________
      AliESDv0 *v0correct = new AliESDv0(lCorrectParamN,lCorrectNidx,lCorrectParamP,lCorrectPidx);
      v0correct->SetDcaV0Daughters          ( lDcaV0Daughters   );
      v0correct->SetV0CosineOfPointingAngle ( lCosPALocal       );
      v0correct->ChangeMassHypothesis       ( kK0Short          );
      v0correct->SetOnFlyStatus             ( lOnFlyStatusLocal );

      //Reverse Cluster info..._________________________________________
      v0correct->SetClusters( v0->GetClusters( 1 ), v0->GetClusters ( 0 ) );

      *v0 = *v0correct;
      //Proper cleanup..._______________________________________________
      v0correct->Delete();
      v0correct = 0x0;

      //Just another cross-check and output_____________________________
      if( v0->GetParamN()->Charge() > 0 && v0->GetParamP()->Charge() < 0 ) {
        AliWarning("Found Swapped Charges, tried to correct but something FAILED!");
      }else{
        //AliWarning("Found Swapped Charges and fixed.");
      }
      //________________________________________________________________
   }else{
      //Don't touch it! ---
      //Printf("Ah, nice. Charges are already ordered...");
   }
   return;
} 

