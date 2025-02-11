// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \brief check the v0 phase-space
/// \dependencies: o2-analysis-lf-lambdakzeromcfinder
/// \author daiki.sekihata@cern.ch felix.schlepper@cern.ch

#include "TDatabasePDG.h"

#include "Framework/runDataProcessing.h"
#include "Framework/AnalysisTask.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/HistogramRegistry.h"
#include "Framework/StaticFor.h"
#include "ReconstructionDataFormats/Track.h"
#include "DetectorsBase/Propagator.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "PWGEM/PhotonMeson/Utils/TrackSelection.h"
#include "PWGEM/PhotonMeson/DataModel/mcV0Tables.h"
#include "DataFormatsParameters/GRPMagField.h"
#include "CCDB/BasicCCDBManager.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::framework::expressions;
using namespace o2::constants::physics;
using namespace o2::pwgem::photonmeson;

struct CheckMCV0 {
  // Output Objects
  Produces<aod::MCV0> mcV0Table;
  HistogramRegistry registry{"output", {}, OutputObjHandlingPolicy::AnalysisObject};

  // Track selection
  Configurable<bool> ptLogAxis{"ptLogAxis", false, "Flag to use a log momentum axis"};
  Configurable<float> minpt{"minpt", 0.001, "min pt for track in GeV/c"};
  Configurable<float> maxpt{"maxpt", 20.0, "max pt for track in GeV/c"};
  Configurable<float> maxeta{"maxeta", 999.0, "eta acceptance"};
  Configurable<float> dcamin{"dcamin", 0.1, "dcamin"};
  Configurable<float> dcamax{"dcamax", 1e+10, "dcamax"};
  Configurable<float> maxZ{"maxZ", 200.0, "max z for track"};
  Configurable<float> maxX{"maxX", 200.0, "maximum X (starting point of track X)"};
  Configurable<float> maxY{"maxY", 200.0, "maximum Y (starting point of track Y)"};
  Configurable<float> maxChi2TPC{"maxChi2TPC", 100.0, "max chi2/NclsTPC"};
  Configurable<int> minCrossedRowsTPC{"minCrossedRowsTPC", 10, "min crossed rows tpc"};

  // Filters
  Filter trackPt = aod::track::pt > minpt&& aod::track::pt < maxpt;
  Filter trackZ = nabs(aod::track::z) < maxZ;
  Filter trackX = aod::track::x < maxX;
  Filter trackY = nabs(aod::track::y) < maxY;
  Filter trackEta = nabs(aod::track::eta) < maxeta;
  Filter trackDCA = nabs(aod::track::dcaXY) > dcamin&& nabs(aod::track::dcaXY) < dcamax;
  using TracksMC = soa::Join<aod::TracksIU, aod::TracksExtra, aod::TracksDCA, aod::TracksCovIU, aod::McTrackLabels>;
  using FilteredTracksMC = soa::Filtered<TracksMC>;
  using CollisionsMC = soa::Join<aod::McCollisionLabels, aod::Collisions>;

  // Histogram Parameters
  Configurable<int> tglNBins{"tglNBins", 500, "nBins for tgl"};
  Configurable<int> zNBins{"zNBins", 2 * static_cast<int>(maxZ), "nBins for z"};
  Configurable<int> xNBins{"xNBins", static_cast<int>(maxX), "nBins for x"};
  Configurable<int> yNBins{"yNBins", static_cast<int>(maxY), "nBins for y"};
  Configurable<int> ptNBins{"ptNBins", 200, "nBins for pt"};
  Configurable<float> ptLowCut{"ptLowCut", 0.1, "low pt cut"};
  // Axes
  AxisSpec axisTgl{tglNBins, -5.f, +5.f, "tan(#lambda)"};
  AxisSpec axisTglEle{tglNBins, -5.f, +5.f, "tan(#lambda) of e^{#minus}"};
  AxisSpec axisTglPos{tglNBins, -5.f, +5.f, "tan(#lambda) of e^{#plus}"};
  AxisSpec axisEta{300, -1.6, +1.6, "#eta"};
  AxisSpec axisX{xNBins, 0.0, maxX, "reco. x"};
  AxisSpec axisXMC{xNBins, 0.0, maxX, "MC prop. x"};
  AxisSpec axisXEle{xNBins, 0.0, maxX, "reco. x of e^{#minus}"};
  AxisSpec axisXPos{xNBins, 0.0, maxX, "reco. x of e^{#plus}"};
  AxisSpec axisXMCEle{xNBins, 0.0, maxX, "MC prop. x of e^{#minus}"};
  AxisSpec axisXMCPos{xNBins, 0.0, maxX, "MC prop. x of e^{#plus}"};
  AxisSpec axisY{yNBins, -maxY, maxY, "reco. y"};
  AxisSpec axisYMC{yNBins, -maxY, maxY, "MC prop. y"};
  AxisSpec axisYEle{yNBins, -maxY, maxY, "reco. y of e^{#minus}"};
  AxisSpec axisYPos{yNBins, -maxY, maxY, "reco. y of e^{#plus}"};
  AxisSpec axisYMCEle{yNBins, -maxY, maxY, "MC prop. y of e^{#minus}"};
  AxisSpec axisYMCPos{yNBins, -maxY, maxY, "MC prop. y of e^{#plus}"};
  AxisSpec axisZ{zNBins, -maxZ, maxZ, "reco. z"};
  AxisSpec axisZMC{zNBins, -maxZ, maxZ, "MC prop.z"};
  AxisSpec axisZEle{zNBins, -maxZ, maxZ, "reco. z of e^{#minus}"};
  AxisSpec axisZPos{zNBins, -maxZ, maxZ, "reco. z of e^{#plus}"};
  AxisSpec axisZMCEle{zNBins, -maxZ, maxZ, "MC prop. z of e^{#minus}"};
  AxisSpec axisZMCPos{zNBins, -maxZ, maxZ, "MC prop. z of e^{#plus}"};
  AxisSpec axisYDiff{yNBins, -maxY, maxY, "reco. y of e^{#plus} - reco. y of e^{#minus}"};
  AxisSpec axisZDiff{zNBins, -maxZ, maxZ, "reco. z of e^{#plus} - reco. z of e^{#minus}"};
  AxisSpec axisPt{ptNBins, minpt, maxpt, "#it{p}_{T} GeV/#it{c}"};
  AxisSpec axisPtPos{ptNBins, minpt, maxpt, "#it{p}_{T} GeV/#it{c} of e^{#plus}"};
  AxisSpec axisPtEle{ptNBins, minpt, maxpt, "#it{p}_{T} GeV/#it{c} of e^{#minus}"};

  static constexpr std::array<std::string_view, 7> cutsBinLabels{"Pre", "checkV0leg", "sign", "propagationFailed", "lowPt", "highPt", "survived"};
  enum cutsBinEnum : uint8_t {
    PRE = 1,
    CHECKV0LEG,
    SIGN,
    PROPFAIL,
    LOWPT,
    HIGHPT,
    SURVIVED,
  };
  static_assert(cutsBinLabels.size() == cutsBinEnum::SURVIVED);

  static constexpr std::array<std::string_view, 8> checkV0legLabels{"Pt<minPt", "Pt>maxPt", "dca<dcamin", "dca>dcamax", "tpcChi2NCl>maxchi2tpc", "no ITS||TPC", "eta>maxEta", "tpcCrossedRows<minCrossedRowsTPC"};
  enum checkV0legEnum : uint8_t {
    PTMIN = 1,
    PTMAX,
    DCAMIN,
    DCAMAX,
    TPCCHI2NCL,
    NOITSTPC,
    MAXETA,
    MINCROSSEDROWSTPC,
  };
  static_assert(checkV0legLabels.size() == checkV0legEnum::MINCROSSEDROWSTPC);

  // CCDB
  Configurable<std::string> ccdbPath{"ccdb-path", "GLO/GRP/GRP", "path to the ccdb object"};
  Configurable<std::string> grpmagPath{"grpmagPath", "GLO/Config/GRPMagField", "path to the GRPMagField object"};
  Configurable<std::string> lutPath{"lutPath", "GLO/Param/MatLUT", "Path of the Lut parametrization"};
  Configurable<std::string> ccdbUrl{"ccdb-url", "http://alice-ccdb.cern.ch", "url of the ccdb repository"};
  Service<o2::ccdb::BasicCCDBManager> ccdb;
  int runNumber{-1};
  o2::base::MatLayerCylSet* lut{nullptr};
  o2::parameters::GRPMagField* grpmag{nullptr};

  // params
  std::array<float, 3> mcPosXYZProp{};
  std::array<float, 3> mcEleXYZProp{};

  // Track Types
  static constexpr std::array<std::string_view, 6> v0Types{"ITSTPC_ITSTPC/", "TPConly_TPConly/", "ITSonly_ITSonly/", "ITSTPC_TPConly/", "ITSTPC_ITSonly/", "TPConly_ITSonly/"};
  std::array<bool, v0Types.size()> v0TypesPassed{};
  static constexpr std::array<std::string_view, 3> ptBins{"lowPt/", "highPt/", "all/"};

  void init(InitContext const& /*unused*/)
  {
    // setup CCDB
    ccdb->setURL(ccdbUrl);
    ccdb->setCaching(true);
    ccdb->setLocalObjectValidityChecking();
    lut = o2::base::MatLayerCylSet::rectifyPtrFromFile(ccdb->get<o2::base::MatLayerCylSet>(lutPath));

    // maybe logarithmic
    if (ptLogAxis) {
      axisPt.makeLogarithmic();
      axisPtPos.makeLogarithmic();
      axisPtEle.makeLogarithmic();
    }

    // Create histograms
    for (const auto& v0type : v0Types) {
      for (const auto& ptbin : ptBins) {
        const auto path = Form("%s%s", v0type.data(), ptbin.data());
        registry.add(Form("%sTglTgl", path), "tan(#lambda) vs. tan(#lambda)", HistType::kTH2F, {axisTglPos, axisTglEle});
        registry.add(Form("%sPtPt", path), "p_{T} vs. p_{T}", HistType::kTH2F, {axisPtPos, axisPtEle});
        registry.add(Form("%sXX", path), "x vs. x", HistType::kTH2F, {axisXPos, axisXEle});
        registry.add(Form("%sYY", path), "y vs. y", HistType::kTH2F, {axisYPos, axisYEle});
        registry.add(Form("%sZZ", path), "z vs. z", HistType::kTH2F, {axisZPos, axisZEle});
        registry.add(Form("%sZPt", path), "z vs. p_{T}", HistType::kTH2F, {axisZ, axisPt});
        registry.add(Form("%sZTgl", path), "z vs. tan(#lambda)", HistType::kTH2F, {axisZ, axisTgl});
        registry.add(Form("%sPtTgl", path), "p_{T} vs. tan(#lambda)", HistType::kTH2F, {axisPt, axisTgl});
        registry.add(Form("%sMC/XX", path), Form("MC x vs. reconstructed x (ptLowCut=%.2f)", ptLowCut.value), HistType::kTH2F, {axisXMC, axisX});
        registry.add(Form("%sMC/YY", path), Form("MC y vs. rconstructed y (ptLowCut=%.2f)", ptLowCut.value), HistType::kTH2F, {axisYMC, axisY});
        registry.add(Form("%sMC/ZZ", path), Form("MC z vs. reconstructed z (ptLowCut=%.2f)", ptLowCut.value), HistType::kTH2F, {axisZMC, axisZ});
        registry.add(Form("%sMC/VertexPropagationX", path), Form("MC vertex X propagated to track (ptLowCut=%.2f)", ptLowCut.value), HistType::kTH2F, {{axisXMCPos, axisXMCEle}});
        registry.add(Form("%sMC/VertexPropagationY", path), Form("MC vertex Y propagated to track (ptLowCut=%.2f)", ptLowCut.value), HistType::kTH2F, {{axisYMCPos, axisYMCEle}});
        registry.add(Form("%sMC/VertexPropagationZ", path), Form("MC vertex Z propagated to track (ptLowCut=%.2f)", ptLowCut.value), HistType::kTH2F, {{axisZMCPos, axisZMCEle}});
      }
    }

    registry.add("V0Counter", "V0 counter", HistType::kTH1F, {{cutsBinLabels.size(), 0.5, 0.5 + cutsBinLabels.size()}});
    for (int iBin = 0; iBin < cutsBinLabels.size(); ++iBin) {
      registry.get<TH1>(HIST("V0Counter"))->GetXaxis()->SetBinLabel(iBin + 1, cutsBinLabels[iBin].data());
    }

    registry.add("V0TypeCounter", "V0 Type counter", HistType::kTH1F, {{v0Types.size(), 0.5, 0.5 + v0Types.size()}});
    for (int iBin = 0; iBin < v0Types.size(); ++iBin) {
      registry.get<TH1>(HIST("V0TypeCounter"))->GetXaxis()->SetBinLabel(iBin + 1, v0Types[iBin].data());
    }

    registry.add("CheckV0Leg", "CheckV0Leg", HistType::kTH1F, {{checkV0legLabels.size(), 0.5, 0.5 + checkV0legLabels.size()}});
    for (int iBin = 0; iBin < checkV0legLabels.size(); ++iBin) {
      registry.get<TH1>(HIST("CheckV0Leg"))->GetXaxis()->SetBinLabel(iBin + 1, checkV0legLabels[iBin].data());
    }
  }

  void processMCV0(CollisionsMC const& collisions, aod::V0s const& v0s, FilteredTracksMC const& /*unused*/, aod::McParticles const& /*unused*/, aod::McCollisions const& /*unused*/, aod::BCsWithTimestamps const& /*unused*/)
  {
    // Check for new ccdb parameters
    const auto bc = collisions.begin().bc_as<aod::BCsWithTimestamps>();
    initCCDB(bc);

    // loop over all true photon pairs
    for (const auto& v0 : v0s) {
      registry.fill(HIST("V0Counter"), PRE);

      // tracks
      const auto pos = v0.template posTrack_as<TracksMC>(); // positive daughter
      const auto ele = v0.template negTrack_as<TracksMC>(); // negative daughter
      if (!checkV0leg(pos) || !checkV0leg(ele)) {
        registry.fill(HIST("V0Counter"), CHECKV0LEG);
        continue;
      }

      // set correct track types
      checkPassed(pos, ele);

      // corresponding generated particles
      const auto posMC = pos.template mcParticle_as<aod::McParticles>();
      const auto eleMC = ele.template mcParticle_as<aod::McParticles>();
      if (std::signbit(pos.sign()) || !std::signbit(ele.sign())) { // wrong sign and same sign reject (e.g. loopers)
        registry.fill(HIST("V0Counter"), SIGN);
        continue;
      }

      // Propagate the vertex position of the MC track to the reconstructed vertex position of the track
      if (!recacluateMCVertex(ele, eleMC, mcEleXYZProp) || !recacluateMCVertex(pos, posMC, mcPosXYZProp)) {
        registry.fill(HIST("V0Counter"), PROPFAIL);
        continue;
      }
      registry.fill(HIST("V0Counter"), SURVIVED);

      static_for<0, v0Types.size() - 1>([&](auto i) {
        static_for<0, ptBins.size() - 1>([&](auto j) {
          fillHistograms<i, j>(pos, ele);
        });
      });

      mcV0Table(pos.pt(), pos.eta(), pos.tgl(), pos.x(), pos.y(), pos.z(),
                ele.pt(), ele.eta(), ele.tgl(), ele.x(), ele.y(), ele.z(),
                posMC.pt(), posMC.eta(), posMC.vx(), posMC.vy(), posMC.vz(),
                eleMC.pt(), eleMC.eta(), eleMC.vx(), eleMC.vy(), eleMC.vz(),
                mcPosXYZProp[0], mcPosXYZProp[1], mcPosXYZProp[2],
                mcEleXYZProp[0], mcEleXYZProp[1], mcEleXYZProp[2],
                std::distance(v0TypesPassed.cbegin(), std::find(v0TypesPassed.cbegin(), v0TypesPassed.cend(), true)));
    }
  }
  PROCESS_SWITCH(CheckMCV0, processMCV0, "process reconstructed MC info", true);

  template <unsigned int idxV0Type, unsigned int idxPtBin, typename TTrack>
  inline void fillHistograms(TTrack const& pos, TTrack const& ele)
  {
    constexpr auto hist = HIST(v0Types[idxV0Type]) + HIST(ptBins[idxPtBin]);

    if (!v0TypesPassed[idxV0Type]) {
      return;
    }

    if constexpr (idxPtBin == 0) {
      if (pos.pt() > ptLowCut && ele.pt() > ptLowCut) {
        return;
      }
    } else if constexpr (idxPtBin == 1) {
      if (pos.pt() < ptLowCut && ele.pt() < ptLowCut) {
        return;
      }
    }

    registry.fill(hist + HIST("TglTgl"), pos.tgl(), ele.tgl());
    registry.fill(hist + HIST("PtPt"), pos.pt(), ele.pt());
    registry.fill(hist + HIST("XX"), pos.x(), ele.x());
    registry.fill(hist + HIST("YY"), pos.y(), ele.y());
    registry.fill(hist + HIST("ZZ"), pos.z(), ele.z());
    registry.fill(hist + HIST("ZPt"), pos.z(), pos.pt());
    registry.fill(hist + HIST("ZPt"), ele.z(), ele.pt());
    registry.fill(hist + HIST("ZTgl"), pos.z(), pos.tgl());
    registry.fill(hist + HIST("ZTgl"), ele.z(), ele.tgl());
    registry.fill(hist + HIST("PtTgl"), pos.pt(), pos.tgl());
    registry.fill(hist + HIST("PtTgl"), ele.pt(), ele.tgl());
    if constexpr (idxPtBin == 0) {
      if (pos.pt() < ptLowCut || ele.pt() < ptLowCut) {
        registry.fill(HIST("V0Counter"), cutsBinEnum::LOWPT);
      }
      if (pos.pt() < ptLowCut) {
        registry.fill(hist + HIST("MC/") + HIST("XX"), mcPosXYZProp[0], pos.x());
        registry.fill(hist + HIST("MC/") + HIST("YY"), mcPosXYZProp[1], pos.y());
        registry.fill(hist + HIST("MC/") + HIST("ZZ"), mcPosXYZProp[2], pos.z());
      }
      if (ele.pt() < ptLowCut) {
        registry.fill(hist + HIST("MC/") + HIST("XX"), mcEleXYZProp[0], ele.x());
        registry.fill(hist + HIST("MC/") + HIST("YY"), mcEleXYZProp[1], ele.y());
        registry.fill(hist + HIST("MC/") + HIST("ZZ"), mcEleXYZProp[2], ele.z());
      }
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationX"), mcPosXYZProp[0], mcEleXYZProp[0]);
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationY"), mcPosXYZProp[1], mcEleXYZProp[1]);
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationZ"), mcPosXYZProp[2], mcEleXYZProp[2]);
    } else if constexpr (idxPtBin == 1) {
      if (pos.pt() > ptLowCut || ele.pt() > ptLowCut) {
        registry.fill(HIST("V0Counter"), cutsBinEnum::HIGHPT);
      }
      if (pos.pt() > ptLowCut) {
        registry.fill(hist + HIST("MC/") + HIST("XX"), mcPosXYZProp[0], pos.x());
        registry.fill(hist + HIST("MC/") + HIST("YY"), mcPosXYZProp[1], pos.y());
        registry.fill(hist + HIST("MC/") + HIST("ZZ"), mcPosXYZProp[2], pos.z());
      }
      if (ele.pt() > ptLowCut) {
        registry.fill(hist + HIST("MC/") + HIST("XX"), mcEleXYZProp[0], ele.x());
        registry.fill(hist + HIST("MC/") + HIST("YY"), mcEleXYZProp[1], ele.y());
        registry.fill(hist + HIST("MC/") + HIST("ZZ"), mcEleXYZProp[2], ele.z());
      }
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationX"), mcPosXYZProp[0], mcEleXYZProp[0]);
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationY"), mcPosXYZProp[1], mcEleXYZProp[1]);
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationZ"), mcPosXYZProp[2], mcEleXYZProp[2]);
    } else {
      registry.fill(hist + HIST("MC/") + HIST("XX"), mcPosXYZProp[0], pos.x());
      registry.fill(hist + HIST("MC/") + HIST("YY"), mcPosXYZProp[1], pos.y());
      registry.fill(hist + HIST("MC/") + HIST("ZZ"), mcPosXYZProp[2], pos.z());
      registry.fill(hist + HIST("MC/") + HIST("XX"), mcEleXYZProp[0], ele.x());
      registry.fill(hist + HIST("MC/") + HIST("YY"), mcEleXYZProp[1], ele.y());
      registry.fill(hist + HIST("MC/") + HIST("ZZ"), mcEleXYZProp[2], ele.z());
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationX"), mcPosXYZProp[0], mcEleXYZProp[0]);
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationY"), mcPosXYZProp[1], mcEleXYZProp[1]);
      registry.fill(hist + HIST("MC/") + HIST("VertexPropagationZ"), mcPosXYZProp[2], mcEleXYZProp[2]);
    }
  }

  // Templates
  template <typename TTrack>
  inline bool checkV0leg(TTrack const& track)
  {
    if (track.pt() < minpt) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::PTMIN);
      return false;
    }
    if (track.pt() > maxpt) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::PTMAX);
      return false;
    }
    if (!track.hasITS() && !track.hasTPC()) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::NOITSTPC);
      return false;
    }
    if (abs(track.eta()) > maxeta) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::MAXETA);
      return false;
    }
    if (abs(track.dcaXY()) < dcamin) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::DCAMIN);
      return false;
    }
    if (abs(track.dcaXY()) > dcamax) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::DCAMAX);
      return false;
    }
    if (track.tpcChi2NCl() > maxChi2TPC) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::TPCCHI2NCL);
      return false;
    }
    if (track.tpcNClsCrossedRows() < minCrossedRowsTPC) {
      registry.fill(HIST("CheckV0Leg"), checkV0legEnum::MINCROSSEDROWSTPC);
      return false;
    }
    return true;
  }

  template <typename TTrack>
  inline void checkPassed(TTrack const& track0, TTrack const& track1)
  {
    v0TypesPassed.fill(false); // reset
    v0TypesPassed[0] = isITSTPC_ITSTPC(track0, track1);
    v0TypesPassed[1] = isTPConly_TPConly(track0, track1);
    v0TypesPassed[2] = isITSonly_ITSonly(track0, track1);
    v0TypesPassed[3] = isITSTPC_TPConly(track0, track1);
    v0TypesPassed[4] = isITSTPC_ITSonly(track0, track1);
    v0TypesPassed[5] = isTPConly_ITSonly(track0, track1);
    for (int i = 0; i < v0TypesPassed.size(); ++i) {
      if (v0TypesPassed[i]) {
        registry.fill(HIST("V0TypeCounter"), i + 1);
      }
    }
  }

  template <typename TTrack, typename MCTrack>
  inline bool recacluateMCVertex(TTrack const& track, MCTrack const& mcTrack, std::array<float, 3>& xyz)
  {
    std::array<float, 3> xyzMC{mcTrack.vx(), mcTrack.vy(), mcTrack.vz()};
    std::array<float, 3> pxyzMC{mcTrack.px(), mcTrack.py(), mcTrack.pz()};
    auto pPDG = TDatabasePDG::Instance()->GetParticle(mcTrack.pdgCode());
    if (!pPDG) {
      return false;
    }
    o2::track::TrackPar mctrO2(xyzMC, pxyzMC, TMath::Nint(pPDG->Charge() / 3), false);
    // rotate track into padplane of detector and then propagate to corresponding x of track
    if (!mctrO2.rotate(track.alpha()) || !o2::base::Propagator::Instance()->PropagateToXBxByBz(mctrO2, track.x())) {
      return false;
    }
    xyz = {mctrO2.getX(), mctrO2.getY(), mctrO2.getZ()};
    return true;
  }

  template <typename BC>
  inline void initCCDB(BC const& bc)
  {
    if (runNumber == bc.runNumber()) {
      return;
    }
    grpmag = ccdb->getForTimeStamp<o2::parameters::GRPMagField>(grpmagPath, bc.timestamp());
    o2::base::Propagator::initFieldFromGRP(grpmag);
    o2::base::Propagator::Instance()->setMatLUT(lut);
    runNumber = bc.runNumber();
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<CheckMCV0>(cfgc, TaskName{"check-mc-v0"})};
}
