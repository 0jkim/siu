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
 * Description: This code transmits 100 packets from gNB to UE. In oder to do that,
 * there are two different branches, one that obtains the reception information and
 * another one that transmits the packets. These two branches (each one with two functions)
 * are executed for every single packet. The packets are scheduled in TDD mode, and they are
 * transmitted one after another.
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

  void Setup (Ptr<NetDevice> device, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t period);

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
  uint8_t         m_periodicity;


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
    m_periodicity(0)
{
}


MyModel::~MyModel()
{
}


void MyModel::Setup (Ptr<NetDevice> device, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, uint8_t period)
{
  m_device = device;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_running = true;
  m_packetsSent = 0;
  m_periodicity = period;
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
  Ptr<Packet> pkt = Create<Packet> (m_packetSize,m_periodicity);
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
  Ptr<Packet> pkt = Create<Packet> (m_packetSize,m_periodicity);
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

void MyModel::ScheduleTxUl (void)
{
  if (m_running)
    {
        Time tNext = MilliSeconds(10);	 //1  //10
    	m_sendEvent = Simulator::Schedule (tNext, &MyModel::SendPacketUl, this);
    }
}

void MyModel::ScheduleTxUl_Configuration (void) //configuration period
{
    Time tNext = MilliSeconds(30);
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
}


void ConnectPdcpRlcTraces ()
{
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/DataRadioBearerMap/*/LteRlc/RxPDU",
                      MakeCallback (&RxRlcPDU));
  NS_LOG_INFO ("Received PDCP RLC DL");
}

void
ConnectUlPdcpRlcTraces ()
{
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/DataRadioBearerMap/*/LteRlc/RxPDU",
                      MakeCallback (&RxRlcPDU));
  NS_LOG_INFO ("Received PDCP RLC UL");
}


int
main (int argc, char *argv[]){

  //BWP 1 (UL)
    uint16_t numerologyBwp1 = 2;    
    uint32_t udpPacketSize = 20;//udp + 22
    uint32_t udpPacketSize_2 = 20;
    double centralFrequencyBand1 = 3550e6;
    double bandwidthBand1 = 5e6;

    uint8_t period = uint8_t(10);


    uint16_t gNbNum = 1;
    uint16_t ueNumPergNb = 15; 
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
    if (scheduler_CG)
      {
        //nrHelper-> SetSchedulerType(BooleanValue (true));
        nrHelper->SetGnbPhyAttribute ("TbUlEncodeLatency", TimeValue (MicroSeconds (50)));

        // UE
        nrHelper->SetUeMacAttribute ("CG", BooleanValue (true));
        nrHelper->SetUePhyAttribute ("CG", BooleanValue (true));

        // gNB
        nrHelper->SetGnbMacAttribute ("CG", BooleanValue (true));
        nrHelper->SetGnbPhyAttribute ("CG", BooleanValue (true));

        //Configuration time and CG periodicity
        // UE
        // MAC
        nrHelper->SetUeMacAttribute ("ConfigurationTime", UintegerValue (30));
        nrHelper->SetUeMacAttribute ("CGPeriod", UintegerValue (10)); 
        // PHY
        nrHelper->SetUePhyAttribute ("ConfigurationTime", UintegerValue (30));
        nrHelper->SetUePhyAttribute ("CGPeriod", UintegerValue (10)); 

        // gNB
        // MAC
        nrHelper->SetGnbMacAttribute ("ConfigurationTime", UintegerValue (30));
        // PHY
        nrHelper->SetGnbPhyAttribute ("ConfigurationTime", UintegerValue (30));

      }
    else
      {
        nrHelper->SetUeMacAttribute ("CG", BooleanValue (false));
        nrHelper->SetUePhyAttribute ("CG", BooleanValue (false));

        // gNB
        nrHelper->SetGnbMacAttribute ("CG", BooleanValue (false));
        nrHelper->SetGnbPhyAttribute ("CG", BooleanValue (false));

        nrHelper->SetSchedulerAttribute ("CG", BooleanValue (false));
      }


    nrHelper->SetEpcHelper (epcHelper);

    //Disable the SRS
    nrHelper->SetSchedulerAttribute ("SrsSymbols", UintegerValue (0));

    //Add the desired flexible pattern (the needed DL DATA symbols (default 0))
    nrHelper->SetSchedulerAttribute ("DlDataSymbolsFpattern", UintegerValue (0)); //symStart - 1

    // enable or disable HARQ retransmissions
    nrHelper->SetSchedulerAttribute ("EnableHarqReTx", BooleanValue (false));
    Config::SetDefault ("ns3::NrHelper::HarqEnabled", BooleanValue (false));

    // Configure scheduler // Comment out the following two lines if you want to use 5GL-TDMA (5G LENA TDMA)
    //nrHelper->SetSchedulerTypeId (NrMacSchedulerOfdmaRR::GetTypeId ());
    //nrHelper->SetSchedulerAttribute ("schOFDMA", UintegerValue (2)); // 1 for 5GL-OFDMA
                                                                       // 2 for Sym-OFDMA
                                                                       // 3 For RB-OFDMA


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
    std::string errorModel = "ns3::NrEesmIrT1"; // ns3::NrEesmIrT2 (256QAM), ns3::NrEesmIrT1 (64QAM) more robust but with less througput
    nrHelper->SetUlErrorModel (errorModel);
    nrHelper->SetDlErrorModel (errorModel);

    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel

    bool fadingEnabled = true; //false//true si quiero añadir fading (si no solo tengo un canal con un pathloss)
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

  //  Ptr<MyModel> modelDL = CreateObject<MyModel> ();


    Ptr<MyModel> modelUl_1 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_2 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_3 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_4 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_5 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_6 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_7 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_8 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_9 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_10 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_11 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_12 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_13 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_14 = CreateObject<MyModel> ();
    Ptr<MyModel> modelUl_15 = CreateObject<MyModel> ();

    modelUl_1 -> Setup(ueNetDev.Get(0), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period); //1Mbps
    modelUl_2 -> Setup(ueNetDev.Get(1), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_3 -> Setup(ueNetDev.Get(2), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_4 -> Setup(ueNetDev.Get(3), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_5 -> Setup(ueNetDev.Get(4), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_6 -> Setup(ueNetDev.Get(5), enbNetDev.Get(0)->GetAddress(), udpPacketSize_2, nPackets, DataRate("1Mbps"),period);
    modelUl_7 -> Setup(ueNetDev.Get(6), enbNetDev.Get(0)->GetAddress(), udpPacketSize_2, nPackets, DataRate("1Mbps"),period);
    modelUl_8 -> Setup(ueNetDev.Get(7), enbNetDev.Get(0)->GetAddress(), udpPacketSize_2, nPackets, DataRate("1Mbps"),period);
    modelUl_9 -> Setup(ueNetDev.Get(8), enbNetDev.Get(0)->GetAddress(), udpPacketSize_2, nPackets, DataRate("1Mbps"),period);
    modelUl_10 -> Setup(ueNetDev.Get(9), enbNetDev.Get(0)->GetAddress(), udpPacketSize_2, nPackets, DataRate("1Mbps"),period);
    modelUl_11 -> Setup(ueNetDev.Get(10), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_12 -> Setup(ueNetDev.Get(11), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_13 -> Setup(ueNetDev.Get(12), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_14 -> Setup(ueNetDev.Get(13), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);
    modelUl_15 -> Setup(ueNetDev.Get(14), enbNetDev.Get(0)->GetAddress(), udpPacketSize, nPackets, DataRate("1Mbps"),period);


    float t_process = 0;
    if (numerologyBwp1 == 0)
    {
        t_process = 0.000175;
    }
    else
    {
        t_process = 0.000096; //numerology 1 and numerology 2
        if (numerologyBwp1 == 1)
        {
            t_process = t_process - 0.00003571;
        }
        else
        {
            t_process = t_process - 0.00001785;
        }

    }
    float startTime = 0.1 - t_process;

    // Schedule the event of Start Application
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_1);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_2);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_3);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_4);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_5);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_6);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_7);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_8);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_9);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_10);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_11);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_12);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_13);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_14);
    Simulator::Schedule(Seconds(startTime), &StartApplicationUl, modelUl_15);


   // attach UEs to the closest eNB
   nrHelper->AttachToClosestEnb (ueNetDev, enbNetDev);

   nrHelper->EnableTraces();

    Simulator::Stop (Seconds (0.4)); //0.4 //11
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
