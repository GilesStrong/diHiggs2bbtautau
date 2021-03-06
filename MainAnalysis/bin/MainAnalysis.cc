#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <strstream>

#include <TH1F.h>
#include <TTree.h>
#include <TROOT.h>
#include <TFile.h>
#include <TSystem.h>
#include <TLorentzVector.h>
#include <math.h>

#include "DataFormats/FWLite/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/FWLite/interface/FWLiteEnabler.h"

#include "DataFormats/FWLite/interface/Run.h"
#include "DataFormats/Luminosity/interface/LumiSummary.h"
#include "DataFormats/FWLite/interface/LuminosityBlock.h"

#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenLumiInfoProduct.h"

#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Tau.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "PhysicsTools/FWLite/interface/TFileService.h"
#include "PhysicsTools/FWLite/interface/CommandLineParser.h"

#include "SimDataFormats/GeneratorProducts/interface/GenRunInfoProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/LHERunInfoProduct.h"
#include "GeneratorInterface/LHEInterface/interface/LHERunInfo.h"

#include "DataFormats/Common/interface/MergeableCounter.h"


int main(int argc, char* argv[])
{
  using pat::Muon;

  gSystem->Load( "libFWCoreFWLite" );
  FWLiteEnabler::enable();

  // command line parser:
  optutl::CommandLineParser parser ("Analyze FWLite Histograms");
  parser.integerValue ("maxEvents"  ) =    -1;
  parser.integerValue ("outputEvery") = 10000;
  parser.stringValue  ("outputFile" ) = "analyzeFWLiteHistograms.root";
  parser.addOption ("runOnData",     optutl::CommandLineParser::kBool, "", false);
  parser.addOption ("setdebug",      optutl::CommandLineParser::kBool, "", false);
  parser.addOption ("monitorFile",   optutl::CommandLineParser::kString, "", "");

  parser.parseArguments (argc, argv);
  int maxEvents_ = parser.integerValue("maxEvents");
  unsigned int outputEvery_ = parser.integerValue("outputEvery");
  std::string outputFile_ = parser.stringValue("outputFile");
  std::string monitorFile_ = parser.stringValue("monitorFile");
  std::vector<std::string> inputFiles_ = parser.stringVector("inputFiles");
  bool runOnData = parser.boolValue("runOnData");
  bool debug = parser.boolValue("setdebug");

  // book a set of histograms:
  fwlite::TFileService fs = fwlite::TFileService(outputFile_.c_str());
  TFileDirectory dir_noPU = fs.mkdir("noPU");
  TH1F* hnVert_noPU  = dir_noPU.make<TH1F>("nVert" ,   "nVert" ,  50,    0,   50);
  
  TFileDirectory dir_weighted = fs.mkdir("weighted");
  TH1F* hmuonPt      = dir_weighted.make<TH1F>("muonPt"  , "pt"  ,   30,   0., 300.);
  TH1F* hmuonEta     = dir_weighted.make<TH1F>("muonEta" , "eta" ,   100,  -3.,   3.);
  TH1F* hmuonPhi     = dir_weighted.make<TH1F>("muonPhi" , "phi" ,   100,  -5.,   5.); 
  TH1F* hnVert       = dir_weighted.make<TH1F>("nVert" ,   "nVert" ,  50,    0,   50);   
  TH1F* hSVFit       = dir_weighted.make<TH1F>("svFitMass"  , "svFitMass"  ,   50,   0., 500.);
  TH1F* h_m_kinfit   = dir_weighted.make<TH1F>("m_kinfit", "KinFit Mass" ,  50,    0,   500);
  TH1F* h_P_fit      = dir_weighted.make<TH1F>("P_chi2" ,  "FitProb" ,  22,    0,   1.1);
  TH1F* h_m_bbar     = dir_weighted.make<TH1F>("m_bbar" ,  "invariant di-bjet mass" ,  40,    0,   400);
  TH1F* h_m_tauvis   = dir_weighted.make<TH1F>("m_tauvis" ,"invariant visible di-tau mass" ,  40,    0,   400);
  TH1F* h_m_t_mu     = dir_weighted.make<TH1F>("m_t_mu" ,  "transverse mass" ,  30,    0,   300);
  TH1F* h_met        = dir_weighted.make<TH1F>("m_met" ,  "missing transverse energy" ,  30,    0,   300);
  
  TH1F* h_cov_xx     = dir_weighted.make<TH1F>("h_cov_xx" ,  "h_cov_xx" ,  20,    0,   100);
  TH1F* h_cov_xy     = dir_weighted.make<TH1F>("h_cov_xy" ,  "h_cov_xy" ,  40,    -200,   200);
  TH1F* h_cov_yx     = dir_weighted.make<TH1F>("h_cov_yx" ,  "h_cov_yx" ,  40,    -200,   200);
  TH1F* h_cov_yy     = dir_weighted.make<TH1F>("h_cov_yy" ,  "h_cov_yy" ,  20,    0,   100);
  
  TH1F* h_monitor       = dir_weighted.make<TH1F>("h_monitor" ,  "h_monitor" ,  10,    0,  10);
    
  TFileDirectory dir_weights = fs.mkdir("weights");
  TH1F* hPUWeight = dir_weights.make<TH1F>("hPUWeight" ,   "hPUWeight" ,  400,    -2,   2);   
  TH1F* hBTaggingSF   = dir_weights.make<TH1F>("hBTaggingSF" ,   "hBTaggingSF" ,  400,    -2,   2);   
  TH1F* hMuonIdScaleFactor = dir_weights.make<TH1F>("hMuonIdScaleFactor" , "hMuonIdScaleFactor" ,  400,    -2,   2);
  TH1F* hMuonTriggerScaleFactor = dir_weights.make<TH1F>("hMuonTriggerScaleFactor" , "hMuonTriggerScaleFactor" ,  400,    -2,   2);
  TH1F* hLumiWeight = dir_weights.make<TH1F>("hLumiWeight" ,   "hLumiWeight" ,  10000,    0,   0.1);   

  double xsec = -1;
  int nev_total = 0;
  int nev_selected = 0;
  double weight = 1;
  double weight_noPU = 1;
  double weight_lumi = 1;
  double weight_PU = 1;
  double weight_btag = 1;
  double svfitmass = 1;
  double weight_muon_IdScaleFactor = 1;
  double weight_muon_triggerScaleFactor = 1;

  double m_kinfit = -1;
  double FitProb = -1;
  double m_t_mu = -1;

  bool invIso = false;
  bool LS = false;

  math::XYZTLorentzVector muon_p4;
  math::XYZTLorentzVector tau_p4;
  math::XYZTLorentzVector bjet1_p4;
  math::XYZTLorentzVector bjet2_p4;
  math::XYZTLorentzVector met_p4;

  // loop over all input files:
  int ievt=0;
  for(unsigned int iFile=0; iFile<inputFiles_.size(); ++iFile){
    std::string filename = inputFiles_[iFile];
    TFile* inFile = TFile::Open(filename.c_str());
    
    if(monitorFile_ != "none") {
        TFile *monitorfile = new TFile(monitorFile_.c_str());
        TH1F *tmphisto = new TH1F("tmphisto","" , 10, 0, 10);
        // fill monitor histogram:
        tmphisto = (TH1F*)monitorfile->Get("mon1/yield");
        double normpar = 2402;
        h_monitor->SetBinContent(1, tmphisto->GetEntries()/normpar);
        
        tmphisto = (TH1F*)monitorfile->Get("mon2/yield");
        h_monitor->SetBinContent(2, tmphisto->GetEntries()/normpar);
        
        tmphisto = (TH1F*)monitorfile->Get("mon3/yield");
        h_monitor->SetBinContent(3, tmphisto->GetEntries()/normpar);
        
        tmphisto = (TH1F*)monitorfile->Get("mon4/yield");
        h_monitor->SetBinContent(4, tmphisto->GetEntries()/normpar);
        
        tmphisto = (TH1F*)monitorfile->Get("mon5/yield");
        h_monitor->SetBinContent(5, tmphisto->GetEntries()/normpar);
        
        tmphisto = (TH1F*)monitorfile->Get("mon6/yield");
        h_monitor->SetBinContent(6, tmphisto->GetEntries()/normpar);
        
        monitorfile->Close();
    }

    if(filename.find("Data") != std::string::npos || filename.find("Run201") != std::string::npos ) runOnData = true;
    if(filename.find("InvIso") != std::string::npos) invIso = true;
    if(filename.find("LS_") != std::string::npos) LS = true;

    if( inFile ){
      
      // get luminosity weights:
      if(!runOnData){
          // get cross section from GenInfo:
          TTree *runs = (TTree*)inFile->Get("Runs");
          runs->GetEntry();
          GenRunInfoProduct myRunInfoProduct;
          TBranch* genInfoBranch = runs->GetBranch("GenRunInfoProduct_generator__SIM.obj");
          
          if(genInfoBranch){
            genInfoBranch->SetAddress(&myRunInfoProduct);
            genInfoBranch->GetEntry(0);
            xsec = (double)myRunInfoProduct.crossSection();
          }
          
          // loop over all luminosity blocks and get number of events:
          fwlite::LuminosityBlock lumiBlock(inFile);
          int ilumiBlock = 0;
          for(lumiBlock.toBegin(); !lumiBlock.atEnd(); ++lumiBlock, ++ilumiBlock){
            edm::Handle<edm::MergeableCounter> hCnt;
            lumiBlock.getByLabel(std::string("nEventsTotal"), hCnt);
            nev_total = nev_total + hCnt->value;
            
            edm::Handle<edm::MergeableCounter> hCntSel;
            lumiBlock.getByLabel(std::string("nEventsSelected"), hCntSel);
            nev_selected = nev_selected + hCntSel->value;
          }

          if(xsec>0) weight_lumi = xsec / nev_total;
          
          // for now, hardcoded xsections for all MC:
          
          if(filename.find("GluGluToRadionToHHTo2B2Tau") != std::string::npos){
              weight_lumi = 100*0.58*0.063*2 / nev_total;   // HARD CODED WEIGTHS FOR MCSIGNAL //*0.58*0.063
          } 

          if(filename.find("TT_TuneCUETP8M1") != std::string::npos) {
              weight_lumi = 831.76 / nev_total;
          }
          if(filename.find("WJetsToLNu") != std::string::npos) {
              weight_lumi = 61526.7 / nev_total;
          }
          
          if(filename.find("GluGluToRadionToHHTo2B2Tau") == std::string::npos){
              weight_lumi *= 0.55;
          } 
          
          weight_lumi *= 12.876*1000;
          
          if(debug){
            std::cout << " xsec = " << xsec << "\n";
            std::cout << " nev_total = " << nev_total << "\n";
            std::cout << " nev_selected = " << nev_selected << "\n";
            std::cout << " weight_lumi = " << weight_lumi << "\n";
          }
      }

      // loop over all events:
      fwlite::Event ev(inFile);
      for(ev.toBegin(); !ev.atEnd(); ++ev, ++ievt){
        
        try{
        
        edm::EventBase const & event = ev;
        if(maxEvents_>0 ? ievt+1>maxEvents_ : false) break;
        if(outputEvery_!=0 ? (ievt>0 && ievt%outputEvery_==0) : false)
           std::cout << "  processing event: " << ievt << std::endl;

        // get all handles present in both data and MC:
        edm::Handle<std::vector<Muon>> selectedmuons;
        try {
            event.getByLabel(edm::InputTag("selectedMuons"), selectedmuons);
        } catch (cms::Exception& iException) {
            std::cout << "  critical: no selectedMuons in event! " << std::endl;
            continue;
        }

        edm::Handle<std::vector<pat::Tau>> selectedtaus;
        if(!invIso) event.getByLabel(edm::InputTag("selectedTaus"), selectedtaus);
        else event.getByLabel(edm::InputTag("selectedTausinvertedIso"), selectedtaus);
        
        edm::Handle<std::vector<pat::Jet>> selectedjets;
        if(!invIso) event.getByLabel(edm::InputTag("selectedJets"), selectedjets);
        else event.getByLabel(edm::InputTag("selectedJetsInvTauIso"), selectedjets);
        
        edm::Handle<std::vector<pat::Jet>> updatedPatJetsUpdatedJEC;
        event.getByLabel(edm::InputTag("updatedPatJetsUpdatedJEC", ""), updatedPatJetsUpdatedJEC);
        
        edm::Handle<std::vector<pat::MET>> slimmedMET;
        event.getByLabel(edm::InputTag("slimmedMETs","","SKIM"), slimmedMET);
        edm::Handle<edm::ValueMap<float>> offlineSlimmedPrimaryVertices;
        event.getByLabel(edm::InputTag("offlineSlimmedPrimaryVertices"), offlineSlimmedPrimaryVertices);

        edm::Handle<double> bTaggingSF;
        if(!invIso){
            if(!LS) event.getByLabel(edm::InputTag("bTaggingSF", "BTaggingSF"), bTaggingSF);
            else event.getByLabel(edm::InputTag("bTaggingSFLSIso", "BTaggingSF"), bTaggingSF);
        } else {
            if(!LS) event.getByLabel(edm::InputTag("bTaggingSFOSInvIso", "BTaggingSF"), bTaggingSF);
            else event.getByLabel(edm::InputTag("bTaggingSFLSInvIso", "BTaggingSF"), bTaggingSF);
        }
                
        edm::Handle<double> METSignificance;
        event.getByLabel(edm::InputTag("METSignificance"), METSignificance);

        edm::Handle<double> kinfit_p;
        edm::Handle<double> kinfit_chi2;
        edm::Handle<double> kinfit_mH;
        edm::Handle<double> kinfit_convergence;
        
        if(!invIso){
            event.getByLabel(edm::InputTag("kinfit", "P"), kinfit_p);
            event.getByLabel(edm::InputTag("kinfit", "chi2"), kinfit_chi2);
            event.getByLabel(edm::InputTag("kinfit", "mH"), kinfit_mH);
            event.getByLabel(edm::InputTag("kinfit", "convergence"), kinfit_convergence);
        } else {
            event.getByLabel(edm::InputTag("kinfitInvTauIso", "P"), kinfit_p);
            event.getByLabel(edm::InputTag("kinfitInvTauIso", "chi2"), kinfit_chi2);
            event.getByLabel(edm::InputTag("kinfitInvTauIso", "mH"), kinfit_mH);
            event.getByLabel(edm::InputTag("kinfitInvTauIso", "convergence"), kinfit_convergence);
        }
        
        edm::Handle<ROOT::Math::SMatrix<double,2,2,ROOT::Math::MatRepSym<double,2>>> covMatrixHandle;
        event.getByLabel(edm::InputTag("METSignificance", "METCovariance"), covMatrixHandle);

        FitProb = *kinfit_p.product();
        edm::Handle<double> svfitmassHandle;
        if (!invIso) event.getByLabel(edm::InputTag("SVFit", ""), svfitmassHandle);
        else event.getByLabel(edm::InputTag("SVFitInvTauIso", ""), svfitmassHandle);
        
        // get muon weights:
	    edm::Handle<double> muon_triggerSF;
        edm::Handle<double> muon_IDSF;
        event.getByLabel(edm::InputTag("muonTriggerScaleFactor", "ScaleFactorValue"), muon_triggerSF);
        weight_muon_triggerScaleFactor = *muon_triggerSF.product();
	    event.getByLabel(edm::InputTag("muonIDScaleFactor", "ScaleFactorValue"), muon_IDSF);
        weight_muon_IdScaleFactor = *muon_IDSF.product();
                
        // get specific handles for MC:
        if(!runOnData){
            edm::Handle<double> PUweight;
            event.getByLabel(edm::InputTag("PUWeightProducer", "PUWeight"), PUweight);
            weight_PU = *PUweight.product();
            if(debug) std::cout << " weight_PU = " << weight_PU << std::endl;
        }
        
        // read product with handles:
        try {
            svfitmass = *svfitmassHandle.product();
            if(debug) std::cout << " svfitmass = " << svfitmass << std::endl;
        } catch (cms::Exception& iException) {
            svfitmass = -1;
        }
        
        // get btag weights:
        try {
            weight_btag = *bTaggingSF.product();
            if(debug) std::cout << " bTaggingSF = " << weight_btag << std::endl;
        } catch (cms::Exception& iException) {
            weight_btag = -1;
        }
         
        if(debug) std::cout << " b-tagging weight = " << weight_btag << std::endl;
        if(debug) std::cout << " weight_muon_IdScaleFactor * weight_btag = " << weight_muon_IdScaleFactor * weight_btag << std::endl;
      
        // construct weighting factor, don't actually apply negative weights:
        weight_noPU = weight_lumi * weight_muon_IdScaleFactor * weight_muon_triggerScaleFactor;
        if(weight_btag>0) weight_noPU *= weight_btag;
        weight = weight_noPU * weight_PU;
        
        if(debug) std::cout << " weight = " << weight << std::endl;
        
        //Getting objects
        pat::Muon muon = selectedmuons->at(0);
        pat::Tau  tau = selectedtaus->at(0);
        pat::Jet  bjet1 = selectedjets->at(0);
        pat::Jet  bjet2 = selectedjets->at(1);
        pat::MET  met = slimmedMET->at(0);

        muon_p4 = muon.p4();
        tau_p4 = tau.p4();
        bjet1_p4 = bjet1.p4();
        bjet2_p4 = bjet2.p4();
        met_p4 = met.p4();

        //Observables
        m_t_mu = sqrt(2 * muon_p4.Pt() * met_p4.Pt() * (1-cos(muon_p4.Phi()-met_p4.Phi())) );
               
    
        // fill histograms:
        for(std::vector<Muon>::const_iterator mu1=selectedmuons->begin(); mu1!=selectedmuons->end(); ++mu1){
            hmuonPt->Fill( mu1->pt (), weight );
            hmuonEta->Fill( mu1->eta(), weight );
            hmuonPhi->Fill( mu1->phi(), weight );
        }

        h_m_bbar->Fill((bjet1_p4+bjet2_p4).M(), weight);  
        h_m_tauvis->Fill((tau_p4+muon_p4+met_p4).M(), weight);
        h_m_t_mu->Fill(m_t_mu, weight);    

        // fill number of vertices:
        double nVert = offlineSlimmedPrimaryVertices->size();
        hnVert->Fill( nVert, weight );
        hnVert_noPU->Fill( nVert, weight_noPU );
        
        if (svfitmass != -1) hSVFit->Fill(svfitmass, weight);
        
        // plot Fitprob
        m_kinfit = *kinfit_mH.product();
        h_m_kinfit->Fill(m_kinfit, weight);
	    h_P_fit->Fill(FitProb, weight);
        h_m_kinfit->Fill(m_kinfit, weight);
	    h_P_fit->Fill(FitProb, weight);
        
        // fill weight histograms:
        hLumiWeight->Fill(weight_lumi);
        hPUWeight->Fill(weight_PU);
        hMuonIdScaleFactor->Fill(weight_muon_IdScaleFactor);
        hMuonTriggerScaleFactor->Fill(weight_muon_triggerScaleFactor);
        hBTaggingSF->Fill(weight_btag);
        
        // fill covmatrix histos:
        ROOT::Math::SMatrix<double,2,2,ROOT::Math::MatRepSym<double,2>> mymatrix;
        mymatrix = *covMatrixHandle.product();
        h_cov_xx->Fill( sqrt(mymatrix[0][0]) );
        h_cov_xy->Fill( mymatrix[0][1] );
        h_cov_yx->Fill( mymatrix[1][0] );
        h_cov_yy->Fill( sqrt(mymatrix[1][1]) );
        
        h_met->Fill(met_p4.Pt(), weight);
        
        } catch (cms::Exception& iException) {
            std::cout << " skipping single event " << std::endl;
            continue;
        }
        
      }
    
      inFile->Close();
    }
    if(maxEvents_>0 ? ievt+1>maxEvents_ : false) break;
  }
  return 0;
}
