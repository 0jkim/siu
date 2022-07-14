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
 * Description: This code transmits 100 packets from gNB to UE. The packets are scheduled in TDD mode, and they are
 * transmitted one after another. The first transmission is used to configure all UEs with
 * fixed resources (Configured Grant), while the rest of the packets are transmitted periodically
 * on the already allocated resources.
 *
 * This code is based on "cttc-3gpp-channel-simple-ran.cc" (5G-LENA) and "fifth.cc" (ns3 Examples) codes.
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


/*
 * MyModel class. It contains the scheduling function.
*/

class MyModel : public Application
{
public:

  MyModel ();
  virtual ~MyModel();

  void Setup (Ptr<NetDevice> device, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

  void SendPacket ();
  void SendPacketUl ();
  void ScheduleTx (void);
  void ScheduleTxUl (void);
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


};


MyModel::MyModel ()
  : m_device(),
    m_addr (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}


MyModel::~MyModel()
{
}


void MyModel::Setup (Ptr<NetDevice> device, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_device = device;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_running = true;
  m_packetsSent = 0;
}

/*
 * This is the first event that is executed.
 */
void StartApplication (Ptr<MyModel> model)
{
  model -> SendPacket ();
}

void StartApplicationUl (Ptr<MyModel> model)
{
  model -> SendPacketUl ();
}


/*
 * Function creates a single packet and directly calls the function send
 * of a device to send the packet to the destination address.
 * @param device Device that will send the packet to the destination address.
 * @param addr Destination address for a packet.
 * @param packetSize The packet size.
 */
void MyModel::SendPacket ()
{
  Ptr<Packet> pkt = Create<Packet> (m_packetSize);
  Ipv4Header ipv4Header;
  ipv4Header.SetProtocol(UdpL4Protocol::PROT_NUMBER);
  pkt->AddHeader(ipv4Header);
  EpsBearerTag tag (1, 1);
  pkt->AddPacketTag (tag);
  m_device->Send (pkt, m_addr, Ipv4L3Protocol::PROT_NUMBER);
  NS_LOG_INFO ("Sending DL");

  if (++m_packetsSent < m_nPackets) //m_nPackets = Ns
    {
      ScheduleTx ();
    }
}

void MyModel::SendPacketUl ()
{
  Ptr<Packet> pkt = Create<Packet> (m_packetSize);
  Ipv4Header ipv4Header;
  ipv4Header.SetProtocol(UdpL4Protocol::PROT_NUMBER);
  pkt->AddHeader(ipv4Header);
  m_device->Send (pkt, m_addr, Ipv4L3Protocol::PROT_NUMBER);
  NS_LOG_INFO ("Sending UL");

  if (m_packetsSent==0){
	  ScheduleTxUl_Configuration();
	  m_packetsSent = 1;
  }else if (++m_packetsSent < m_nPackets) //m_nPackets = Ns
    {
      ScheduleTxUl ();
    }
}


/*
 * SendPacket creates the packet and it sends based on ScheduleTx scheduler.
 */

void MyModel::ScheduleTx (void)
{
  if (m_running)
    {
	    Time tNext = MilliSeconds(2);
    	m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacket, this);

    }
}

// CG Periodic UL transmissions
void MyModel::ScheduleTxUl (void)
{
  if (m_running)
    {
        uint8_t CGPeriod = 10;
        Time tNext = MilliSeconds(CGPeriod);	
    	m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacketUl, this);
    }
}

// UL transmission for configuration 
void MyModel::ScheduleTxUl_Configuration (void) 
{
    uint8_t configurationTime = 30;
    Time tNext = MilliSeconds(configurationTime);
    m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacketUl, this);
}


int
main (int argc, char *argv[]){

    uint16_t numerologyBwp1 = 2;  
    uint32_t udpPacketSize = 10;
    double centralFrequencyBand1 = 3550e6;
    double bandwidthBand1 = 20e6;


    uint16_t gNbNum = 1;
    uint16_t ueNumPergNb = 4; 
    bool enableUl = true;
    uint32_t nPackets = 100;
    Time sendPacketTime = Seconds(0.2);

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
                   udpPacketSize);
    cmd.AddValue ("enableUl",
                  "Enable Uplink",
                  enableUl);
    cmd.Parse (argc, argv);

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
    gridScenario.SetScenarioHeight (10);   // Create a 3x3 scenario where the UE will
    gridScenario.SetScenarioLength (10);   // be distribuited.
    randomStream += gridScenario.AssignStreams (randomStream);
    gridScenario.CreateScenario ();


    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();
    nrHelper->SetBeamformingHelper (idealBeamformingHelper);

    // Scheduler type: configured grant or grant based
    /* false -> grant based : true -> configured grant */
    bool scheduler_CG = true;
    uint8_t configurationTime = 30;
    uint8_t CGPeriod = 10;
    if (scheduler_CG)
      {
        // UE
        nrHelper->SetUeMacAttribute ("CG", BooleanValue (true));
        nrHelper->SetUePhyAttribute ("CG", BooleanValue (true));

        // gNB
        nrHelper->SetGnbMacAttribute ("CG", BooleanValue (true));
        nrHelper->SetGnbPhyAttribute ("CG", BooleanValue (true));

        //Configuration time and CG periodicity
        // UE
        // MAC
        nrHelper->SetUeMacAttribute ("ConfigurationTime", UintegerValue (configurationTime));
        nrHelper->SetUeMacAttribute ("CGPeriod", UintegerValue (CGPeriod)); 
        // PHY
        nrHelper->SetUePhyAttribute ("ConfigurationTime", UintegerValue (configurationTime));
        nrHelper->SetUePhyAttribute ("CGPeriod", UintegerValue (CGPeriod)); 

        // gNB
        // MAC
        nrHelper->SetGnbMacAttribute ("ConfigurationTime", UintegerValue (configurationTime));
        // PHY
        nrHelper->SetGnbPhyAttribute ("ConfigurationTime", UintegerValue (configurationTime));
      }
    else
      {
        nrHelper->SetUeMacAttribute ("CG", BooleanValue (false));
        nrHelper->SetUePhyAttribute ("CG", BooleanValue (false));

        // gNB
        nrHelper->SetGnbMacAttribute ("CG", BooleanValue (false));
        nrHelper->SetGnbPhyAttribute ("CG", BooleanValue (false));
      }


    nrHelper->SetEpcHelper (epcHelper);

    //Disable the SRS
    nrHelper->SetSchedulerAttribute ("SrsSymbols", UintegerValue (0));

    //Add the desired flexible pattern (the needed DL DATA symbols (default 0))
    nrHelper->SetSchedulerAttribute ("DlDataSymbolsFpattern", UintegerValue (0)); //symStart - 1

    // enable or disable HARQ retransmissions
    nrHelper->SetSchedulerAttribute ("EnableHarqReTx", BooleanValue (false));
    Config::SetDefault ("ns3::NrHelper::HarqEnabled", BooleanValue (false));

    // Configure scheduler (TDMA (comment), or FlexTDMA, FlexOFDMA (uncomment))
     nrHelper->SetSchedulerTypeId (NrMacSchedulerOfdmaRR::GetTypeId ()); // Comment out this line if you want to use TDMA
     nrHelper->SetSchedulerAttribute ("FlexTDMA", BooleanValue (true)); // True if you want to use FlexTDMA and false if
                                                                        // you want to use FlexOFDMA

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
    nrHelper->SetSchedulerAttribute ("StartingMcsUl", UintegerValue(6));

    nrHelper->SetChannelConditionModelAttribute ("UpdatePeriod", TimeValue (MilliSeconds (0)));
    nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false)); //false

    // Error Model: UE and GNB with same spectrum error model.
    std::string errorModel = "ns3::NrEesmIrT1"; // ns3::NrEesmIrT2 (256QAM), ns3::NrEesmIrT1 (64QAM) mas robusta pero con menos througput
    nrHelper->SetUlErrorModel (errorModel);
    nrHelper->SetDlErrorModel (errorModel);

    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel

    bool fadingEnabled = false; 
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

    // Create a model for each UE
    Ptr<MyModel> modelUl_1 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_2 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_3 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_4 = CreateObject<MyModel> ();

    // Setup configuration from UE to gNB in order to execute UL transmissions
    modelUl_1 -> Setup(ueNetDev.Get(0), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps")); //1Mbps
    modelUl_2 -> Setup(ueNetDev.Get(1), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"));
    modelUl_3 -> Setup(ueNetDev.Get(2), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"));
    modelUl_4 -> Setup(ueNetDev.Get(3), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"));

    // Schedule the event of Start Application
    /*Simulator::Schedule(Seconds(0.10005), &StartApplicationUl, modelUl_1);
    Simulator::Schedule(Seconds(0.1001), &StartApplicationUl, modelUl_2);
    Simulator::Schedule(Seconds(0.10015), &StartApplicationUl, modelUl_3);
    Simulator::Schedule(Seconds(0.1002), &StartApplicationUl, modelUl_4);*/

    // Here we define in which time instant we want to transmit the periodical data packet
    Simulator::Schedule(Seconds(0.10005), &StartApplicationUl, modelUl_1); //0.10005 Problemas Period //0.100
    Simulator::Schedule(Seconds(0.1001), &StartApplicationUl, modelUl_2); //0.1001 //0.101
    Simulator::Schedule(Seconds(0.101), &StartApplicationUl, modelUl_3); //0.101 //0.102
    Simulator::Schedule(Seconds(0.1025), &StartApplicationUl, modelUl_4); // 0.1025 // 0.103


   // attach UEs to the closest eNB
   nrHelper->AttachToClosestEnb (ueNetDev, enbNetDev);

   nrHelper->EnableTraces();

    Simulator::Stop (Seconds (0.4)); //0.4 //11
    Simulator::Run ();

    std::cout<<"\n La conexión (Config::Connect) se llama desde el main. "<<std::endl;

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
