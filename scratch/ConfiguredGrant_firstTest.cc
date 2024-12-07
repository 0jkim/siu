/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2020 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Description: This code transmits 100 packets from UE to the gNB. It works with
 * dynamic or configured grant (CG) schedulers (both schedulers cannot work simultaneously).
 *
 * In case of CG, a configuration time is selected. In this time, the UEs transmit
 * their requirements to the gNB. The gNB creates a CG for each UE.
 *
 * You can use OFDMA or TDMA access mode.
 * However, we include two new scheduling policies to use with OFDMA access mode.
 *
 * This code is based on "cttc-3gpp-channel-simple-ran.cc" (5G-LENA) code.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/eps-bearer-tag.h"
#include "ns3/grid-scenario-helper.h"
#include "ns3/log.h"
#include "ns3/antenna-module.h"

// why ???

using namespace ns3;

/*
 * Enable the logs of the file by enabling the component "ConfiguredGrant",
 * in this way:
 * $ export NS_LOG="ConfiguredGrant=level_info|prefix_func|prefix_time"
 */
NS_LOG_COMPONENT_DEFINE ("ConfiguredGrant");

static bool g_rxPdcpCallbackCalled = false;
static bool g_rxRxRlcPDUCallbackCalled = false;


/*
 * Global variables
 */
Time g_txPeriod = Seconds(0.1);
Time delay;
std::fstream m_ScenarioFile;


/*
 * MyModel class. It contains the function that generates the event to send a packet from the UE to the gNB
*/

class MyModel : public Application
{
public:

  MyModel ();
  virtual ~MyModel();

  void Setup (Ptr<NetDevice> device, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t period, uint32_t deadline);

  // DL
  void SendPacketDl ();
  void ScheduleTxDl ();

  // UL
  void SendPacketUl ();
  void ScheduleTxUl (uint8_t period);
  void ScheduleTxUl_Configuration();

private:


  Ptr<NetDevice>  m_device;
  Address         m_addr;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  uint8_t         m_periodicity;
  uint32_t        m_deadline;
};


MyModel::MyModel ()
  : m_device(),
    m_addr (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
    m_periodicity(0),
    m_deadline(0)
{
}


MyModel::~MyModel()
{
}


void MyModel::Setup (Ptr<NetDevice> device, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t period, uint32_t deadline)
{
  m_device = device;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_running = true;
  m_packetsSent = 0;
  m_periodicity = period;
  m_deadline = deadline;
}

/*
 * This is the first event that is executed  for DL traffic.
 */
void StartApplicationDl (Ptr<MyModel> model)
{
  model -> SendPacketDl ();
}
/*
 * Function creates a single packet and directly calls the function send
 * of a device to send the packet to the destination address.
 * (DL TRAFFIC)
 */
void MyModel::SendPacketDl ()
{
  Ptr<Packet> pkt = Create<Packet> (m_packetSize,m_periodicity,m_deadline);
  Ipv4Header ipv4Header;
  ipv4Header.SetProtocol(Ipv4L3Protocol::PROT_NUMBER);
  pkt->AddHeader(ipv4Header);

  EpsBearerTag tag (1,1);
  pkt->AddPacketTag (tag);

  m_device->Send (pkt, m_addr, Ipv4L3Protocol::PROT_NUMBER);
  NS_LOG_INFO ("Sending DL");

  if (++m_packetsSent < m_nPackets)
  {
      ScheduleTxDl();
  }
}
/*
 * SendPacket creates the packet at tNext time instant.
 */

void MyModel::ScheduleTxDl ()
{
  if (m_running)
    {
      Time tNext = MilliSeconds(2);
      m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacketDl, this);
    }
}


/*
 * This is the first event that is executed  for UL traffic.
 */
void StartApplicationUl (Ptr<MyModel> model)
{
  model -> SendPacketUl ();
}
/*
 * Function creates a single packet and directly calls the function send
 * of a device to send the packet to the destination address.
 * (UL TRAFFIC)
 */
void MyModel::SendPacketUl ()
{
  Ptr<Packet> pkt = Create<Packet> (m_packetSize,m_periodicity,m_deadline);
  Ipv4Header ipv4Header;
  ipv4Header.SetProtocol(Ipv4L3Protocol::PROT_NUMBER);
  pkt->AddHeader(ipv4Header);

  m_device->Send (pkt, m_addr, Ipv4L3Protocol::PROT_NUMBER);
  NS_LOG_INFO ("Sending UL");

  if (m_packetsSent==0){
      ScheduleTxUl_Configuration();
      m_packetsSent = 1;
  }else if (++m_packetsSent < m_nPackets)
    {
      ScheduleTxUl (m_periodicity);
    }
}
/*
 * SendPacket creates the packet at tNext time instant.
 */

void MyModel::ScheduleTxUl (uint8_t period)
{
  if (m_running)
    {
      Time tNext = MilliSeconds(period);
      m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacketUl, this);
    }
}

void MyModel::ScheduleTxUl_Configuration (void)
{
    uint8_t configurationTime = 60;
    Time tNext = MilliSeconds(configurationTime);
    m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacketUl, this);
}

/*
 * TraceSink, RxRlcPDU connects the trace sink with the trace source (RxPDU). It connects the UE with gNB and vice versa.
 */
void RxRlcPDU (std::string path, uint16_t rnti, uint8_t lcid, uint32_t bytes, uint64_t rlcDelay)
{
  g_rxRxRlcPDUCallbackCalled = true;
  delay = Time::FromInteger(rlcDelay,Time::NS);
  std::cout<<"\n rlcDelay in NS (Time):"<< delay<<std::endl;

  std::cout<<"\n\n Data received at RLC layer at:"<<Simulator::Now()<<std::endl;

  m_ScenarioFile << "\n\n Data received at RLC layer at:"  << Simulator::Now () << std::endl;
  m_ScenarioFile << "\n rnti:" << rnti  << std::endl;
  m_ScenarioFile << "\n delay :" << rlcDelay << std::endl;
}

void
RxPdcpPDU (std::string path, uint16_t rnti, uint8_t lcid, uint32_t bytes, uint64_t pdcpDelay)
{
  std::cout << "\n Packet PDCP delay:" << pdcpDelay << "\n";
  g_rxPdcpCallbackCalled = true;
}

void
ConnectUlPdcpRlcTraces ()
{
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/DataRadioBearerMap/*/LtePdcp/RxPDU",
                    MakeCallback (&RxPdcpPDU));

  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/DataRadioBearerMap/*/LteRlc/RxPDU",
                      MakeCallback (&RxRlcPDU));
  NS_LOG_INFO ("Received PDCP RLC UL");
}


int
main (int argc, char *argv[]){
    uint16_t numerologyBwp1 = 1;
    uint32_t packetSize = 10;
    double centralFrequencyBand1 = 3550e6;
    double bandwidthBand1 = 20e6;
    uint8_t period = uint8_t(10);

    uint16_t gNbNum = 1;
    uint16_t ueNumPergNb = 15;

    bool enableUl = true;
    uint32_t nPackets = 1000;
    Time sendPacketTime = Seconds(0.2);
    uint8_t sch = 2;

    delay = MicroSeconds(10);

    CommandLine cmd;
    cmd.AddValue ("numerologyBwp1",
                  "The numerology to be used in bandwidth part 1",
                  numerologyBwp1);
    cmd.AddValue ("centralFrequencyBand1",
                  "The system frequency to be used in band 1",
                  centralFrequencyBand1);
    cmd.AddValue ("bandwidthBand1",
                  "The system bandwidth to be used in band 1",
                  bandwidthBand1);
    cmd.AddValue ("packetSize",
                  "packet size in bytes",
                   packetSize);
    cmd.AddValue ("enableUl",
                  "Enable Uplink",
                  enableUl);
    cmd.AddValue ("scheduler",
                  "Scheduler",
                  sch);
    cmd.Parse (argc, argv);

    std::vector<uint32_t> v_init(ueNumPergNb);
    std::vector<uint32_t> v_period(ueNumPergNb);
    std::vector<uint32_t> v_deadline(ueNumPergNb);
    std::vector<uint32_t> v_packet(ueNumPergNb);

    std::cout << "\n Init values: " << '\n';
    v_init = std::vector<uint32_t> (ueNumPergNb,{100000});
    for (int val : v_init)
            std::cout << val << std::endl;

    std::cout << "Deadline values: " << '\n';
    v_deadline = std::vector<uint32_t> (ueNumPergNb,{10000000});
    for (int val : v_deadline)
            std::cout << val << std::endl;

    std::cout << "Packet values: " << '\n';
    v_packet = std::vector<uint32_t> (ueNumPergNb,{packetSize});
    for (int val : v_packet)
            std::cout << val << std::endl;

    std::cout << "Period values: " << '\n';
    v_period = std::vector<uint32_t> (ueNumPergNb,{10});
    for (int val : v_period)
            std::cout << val << "\t";


    m_ScenarioFile.open("Scenario.txt", std::ofstream::out | std::ofstream::trunc);

    std::ostream_iterator<std::uint32_t> output_iterator(m_ScenarioFile, "\n");
    m_ScenarioFile <<  "Nº UE" << "\t" << "Init" << "\t" <<
                       "Latency" << "\t" << "Periodicity" << std::endl;

    m_ScenarioFile <<ueNumPergNb << std::endl;
    std::copy(v_init.begin(), v_init.end(),  output_iterator);
    m_ScenarioFile << std::endl;
    std::copy(v_deadline.begin(), v_deadline.end(), output_iterator);
    m_ScenarioFile << std::endl;
    std::copy(v_period.begin(), v_period.end(), output_iterator);
    m_ScenarioFile << std::endl;

    int64_t randomStream = 1;

    //Create the scenario
    GridScenarioHelper gridScenario;
    gridScenario.SetRows (1);
    gridScenario.SetColumns (gNbNum);
    gridScenario.SetHorizontalBsDistance (5.0);
    gridScenario.SetBsHeight (10.0);
    gridScenario.SetUtHeight (1.5);

    // must be set before BS number
    gridScenario.SetSectorization (GridScenarioHelper::SINGLE);
    gridScenario.SetBsNumber (gNbNum);
    gridScenario.SetUtNumber (ueNumPergNb * gNbNum);
    gridScenario.SetScenarioHeight (10);   
    gridScenario.SetScenarioLength (10);   
    randomStream += gridScenario.AssignStreams (randomStream);
    gridScenario.CreateScenario ();


    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
    nrHelper->SetBeamformingHelper (idealBeamformingHelper);

    // Scheduler type: configured grant or grant based
    /* false -> grant based : true -> configured grant */
    bool scheduler_CG = true;
    uint8_t configurationTime = 60;

    nrHelper->SetUeMacAttribute ("CG", BooleanValue (scheduler_CG));
    nrHelper->SetUePhyAttribute ("CG", BooleanValue (scheduler_CG));
    nrHelper->SetGnbMacAttribute ("CG", BooleanValue (scheduler_CG));
    nrHelper->SetGnbPhyAttribute ("CG", BooleanValue (scheduler_CG));

    if (scheduler_CG)
      {
        //Configuration time
        // UE
        nrHelper->SetUeMacAttribute ("ConfigurationTime", UintegerValue (configurationTime));
        nrHelper->SetUePhyAttribute ("ConfigurationTime", UintegerValue (configurationTime));
        // gNB
        nrHelper->SetGnbMacAttribute ("ConfigurationTime", UintegerValue (configurationTime));
        nrHelper->SetGnbPhyAttribute ("ConfigurationTime", UintegerValue (configurationTime));
      }
    else
      {
        nrHelper->SetSchedulerAttribute ("CG", BooleanValue (scheduler_CG));
      }

    nrHelper->SetEpcHelper (epcHelper);

    //Disable the SRS
    nrHelper->SetSchedulerAttribute ("SrsSymbols", UintegerValue (0));

    //Add the desired flexible pattern (the needed DL DATA symbols (default 0))
    nrHelper->SetSchedulerAttribute ("DlDataSymbolsFpattern", UintegerValue (0)); //symStart - 1

    // enable or disable HARQ retransmissions
    nrHelper->SetSchedulerAttribute ("EnableHarqReTx", BooleanValue (false));
    Config::SetDefault ("ns3::NrHelper::HarqEnabled", BooleanValue (false));

    // Select scheduler
    if (sch != 0) 
    {
        nrHelper->SetSchedulerTypeId (NrMacSchedulerOfdmaRR::GetTypeId ());
        nrHelper->SetSchedulerAttribute ("schOFDMA", UintegerValue (sch)); // sch = 0 for TDMA
                                                                           // 1 for 5GL-OFDMA
                                                                           // 2 for Sym-OFDMA
                                                                           // 3 for RB-OFDMA
    }

    // Create one operational band containing one CC with one bandwidth part
    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;

    CcBwpCreator::SimpleOperationBandConf bandConf1 (centralFrequencyBand1, bandwidthBand1,
                                                         numCcPerBand, BandwidthPartInfo::InH_OfficeOpen_nLoS);


    // By using the configuration created, it is time to make the operation band
    OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc (bandConf1);

    Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds(0)));
    nrHelper->SetSchedulerAttribute ("FixedMcsDl", BooleanValue (true));
    nrHelper->SetSchedulerAttribute ("StartingMcsDl", UintegerValue(4));

    // For CG it has to be true
    nrHelper->SetSchedulerAttribute ("FixedMcsUl", BooleanValue (true));
    nrHelper->SetSchedulerAttribute ("StartingMcsUl", UintegerValue(12));

    nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
    nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (true)); //false

    // Error Model: UE and GNB with same spectrum error model.
    // ns3::NrEesmIrT2 (256QAM), ns3::NrEesmIrT1 (64QAM) more robust but with less througput
    std::string errorModel = "ns3::NrEesmIrT1"; 
    nrHelper->SetUlErrorModel (errorModel);
    nrHelper->SetDlErrorModel (errorModel);

    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel

    bool fadingEnabled = true; 
    auto bandMask = NrHelper::INIT_PROPAGATION | NrHelper::INIT_CHANNEL;
    if (fadingEnabled)
      {
        bandMask |= NrHelper::INIT_FADING;
      }

    nrHelper->InitializeOperationBand (&band1, bandMask);
    allBwps = CcBwpCreator::GetAllBwps ({band1});

    // Beamforming method
    idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (QuasiOmniDirectPathBeamforming::GetTypeId ()));

    // Antennas for all the UEs
    nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
    nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
    nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
    nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (4));
    nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

    //Install and get the pointers to the NetDevices
    NetDeviceContainer enbNetDev = nrHelper->InstallGnbDevice (gridScenario.GetBaseStations (), allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (gridScenario.GetUserTerminals (), allBwps);

    randomStream += nrHelper->AssignStreams (enbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);

    // Set the attribute of the netdevice (enbNetDev.Get (0)) and bandwidth part (0)
    nrHelper->GetGnbPhy (enbNetDev.Get (0), 0)->SetAttribute ("Numerology", UintegerValue (numerologyBwp1));

    for (auto it = enbNetDev.Begin (); it != enbNetDev.End (); ++it)
      {
        DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
      }

    for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it)
      {
        DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
      }

    InternetStackHelper internet;
    internet.Install (gridScenario.GetUserTerminals ());
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

    // UL traffic
    std::vector <Ptr<MyModel>> v_modelUl;
    v_modelUl = std::vector<Ptr<MyModel>> (ueNumPergNb,{0});
    for (uint8_t ii=0; ii<ueNumPergNb; ++ii)
    {
        Ptr<MyModel> modelUl = CreateObject<MyModel> ();
        modelUl -> Setup(ueNetDev.Get(ii), enbNetDev.Get(0)->GetAddress(), v_packet[ii], nPackets, DataRate("1Mbps"),v_period[ii], v_deadline[ii]);
        v_modelUl[ii] = modelUl;
        Simulator::Schedule(MicroSeconds(v_init[ii]), &StartApplicationUl, v_modelUl[ii]);
    }

    // DL traffic
    //Ptr<MyModel> modelDl = CreateObject<MyModel> ();
    //modelDl -> Setup(enbNetDev.Get(0), ueNetDev.Get(0)->GetAddress(), 10, nPackets, DataRate("1Mbps"),20, uint32_t(100000));
    //Simulator::Schedule(MicroSeconds(0.099625), &StartApplicationDl, modelDl);


   // attach UEs to the closest eNB
   nrHelper->AttachToClosestEnb (ueNetDev, enbNetDev);

   nrHelper->EnableTraces();
   Simulator::Schedule (Seconds (0.16), &ConnectUlPdcpRlcTraces);

    Simulator::Stop (Seconds (10));
    Simulator::Run ();

    std::cout<<"\n FIN. "<<std::endl;

    if (g_rxPdcpCallbackCalled && g_rxRxRlcPDUCallbackCalled)
      {
        return EXIT_SUCCESS;
      }
    else
      {
        return EXIT_FAILURE;
      }

    Simulator::Destroy ();

}
