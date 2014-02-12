/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/satellite-module.h"
#include "ns3/applications-module.h"
#include "ns3/cbr-helper.h"


using namespace ns3;

/**
* \ingroup satellite
*
* \brief  Another example of CBR application usage in satellite network.
*         The scripts is using user defined scenario, which means that user
*         can change the scenario size quite to be whatever between 1 and
*         full scenario (72 beams). Currently it is configured to using only
*         one beam. CBR application is sending packets in RTN link, i.e. from UT
*         side to GW side. Packet trace and KpiHelper are enabled by default.
*         End user may change the number of UTs and end users from
*         the command line.
*
*         execute command -> ./waf --run "sat-cbr-example --PrintHelp"
*/

NS_LOG_COMPONENT_DEFINE ("sat-cbr-user-defined-example");

int
main (int argc, char *argv[])
{
  uint32_t beamId = 1;
  uint32_t endUsersPerUt (3);
  uint32_t utsPerBeam (3);
  uint32_t packetSize (128);
  Time interval (Seconds(1.0));
  Time simLength (Seconds(20.0));
  Time appStartTime = Seconds(0.1);

  // read command line parameters given by user
  CommandLine cmd;
  cmd.AddValue("endUsersPerUt", "Number of end users per UT", endUsersPerUt);
  cmd.AddValue("utsPerBeam", "Number of UTs per spot-beam", utsPerBeam);
  cmd.Parse (argc, argv);

  // Configure error model
  SatPhyRxCarrierConf::ErrorModel em (SatPhyRxCarrierConf::EM_NONE);
  Config::SetDefault ("ns3::SatUtHelper::FwdLinkErrorModel", EnumValue (em));
  Config::SetDefault ("ns3::SatGwHelper::RtnLinkErrorModel", EnumValue (em));
  //Config::SetDefault ("ns3::SatUtMac::CrUpdatePeriod", TimeValue(Seconds(10.0)));

  // Create reference system, two options:
  // - "Scenario72"
  // - "Scenario98"
  std::string scenarioName = "Scenario72";
  //std::string scenarioName = "Scenario98";

  Ptr<SatHelper> helper = CreateObject<SatHelper> (scenarioName);

  // create user defined scenario
  SatBeamUserInfo beamInfo = SatBeamUserInfo (utsPerBeam,endUsersPerUt);
  std::map<uint32_t, SatBeamUserInfo > beamMap;
  beamMap[beamId] = beamInfo;
  helper->SetBeamUserInfo (beamMap);
  helper->EnablePacketTrace ();

  helper->CreateScenario (SatHelper::USER_DEFINED);

  // enable info logs
  //LogComponentEnable ("CbrApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("sat-cbr-user-defined-example", LOG_LEVEL_INFO);

  // get users
  NodeContainer utUsers = helper->GetUtUsers();
  NodeContainer gwUsers = helper->GetGwUsers();

  // >>> Start of actual test using Full scenario >>>

  // port used for packet delivering
  uint16_t port = 9; // Discard port (RFC 863)

  CbrHelper cbrHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (helper->GetUserAddress (utUsers.Get (0)), port)));
  cbrHelper.SetAttribute("Interval", TimeValue (interval));
  cbrHelper.SetAttribute("PacketSize", UintegerValue (packetSize) );

  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (helper->GetUserAddress (utUsers.Get (0)), port)));

  // initialized time values for simulation
  uint32_t maxTransmitters = utUsers.GetN ();

  ApplicationContainer gwApps;
  ApplicationContainer utApps;

  Time cbrStartDelay = appStartTime;

  // Cbr and Sink applications creation
  for ( uint32_t i = 0; i < maxTransmitters; i++)
    {
      cbrHelper.SetAttribute("Remote", AddressValue(Address (InetSocketAddress (helper->GetUserAddress (gwUsers.Get (0)), port))));
      sinkHelper.SetAttribute("Local", AddressValue(Address (InetSocketAddress (helper->GetUserAddress (gwUsers.Get (0)), port))));

      utApps.Add(cbrHelper.Install (utUsers.Get (i)));
      gwApps.Add(sinkHelper.Install (gwUsers.Get (0)));

      cbrStartDelay += Seconds (0.05);

      utApps.Get(i)->SetStartTime (cbrStartDelay);
      utApps.Get(i)->SetStopTime (simLength);
    }

  // Add the created applications to CbrKpiHelper
  CbrKpiHelper kpiHelper (KpiHelper::KPI_RTN);
  kpiHelper.AddSink (gwApps);
  kpiHelper.AddSender (utApps);

  utApps.Start (appStartTime);
  utApps.Stop (simLength);

  NS_LOG_INFO("--- Cbr-user-defined-example ---");
  NS_LOG_INFO("  Packet size in bytes: " << packetSize);
  NS_LOG_INFO("  Packet sending interval: " << interval.GetSeconds ());
  NS_LOG_INFO("  Simulation length: " << interval.GetSeconds ());
  NS_LOG_INFO("  Number of UTs: " << utsPerBeam);
  NS_LOG_INFO("  Number of end users per UT: " << endUsersPerUt);
  NS_LOG_INFO("  ");

  Simulator::Stop (simLength);
  Simulator::Run ();

  kpiHelper.Print ();

  Simulator::Destroy ();

  return 0;
}