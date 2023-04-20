#include <cstdlib>
#include <time.h>
#include <stdio.h>
#include <string>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/error-model.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("WifiTopology");

void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, double em)
{
    uint16_t i = 0;

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();

    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);

        std::cout << "Flow ID      : "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
        std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
        std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
        std::cout << "Duration    : "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
        std::cout << "Last Received Packet  : "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
        std::cout << "Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024  << " Mbps" << std::endl;
    
        i++;

        std::cout << "---------------------------------------------------------------------------" << std::endl;
    }

    Simulator::Schedule (Seconds (10),&ThroughputMonitor, fmhelper, flowMon, em);
}

void
AverageDelayMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, double em)
{
    uint16_t i = 0;

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();
    Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
        Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
        std::cout << "Flow ID      : "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
        std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
        std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
        std::cout << "Duration    : "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
        std::cout << "Last Received Packet  : "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
        std::cout << "Sum of e2e Delay: " << stats->second.delaySum.GetSeconds () << " s" << std::endl;
        std::cout << "Average of e2e Delay: " << stats->second.delaySum.GetSeconds () / stats->second.rxPackets << " s" << std::endl;
    
        i++;

        std::cout << "---------------------------------------------------------------------------" << std::endl;
    }

    Simulator::Schedule (Seconds (10),&AverageDelayMonitor, fmhelper, flowMon, em);
}

class MyHeader : public Header 
{
public:
    MyHeader ();
    virtual ~MyHeader ();
    void SetData (uint16_t data);
    uint16_t GetData (void) const;
    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
    virtual uint32_t GetSerializedSize (void) const;
private:
    uint16_t m_data;
};

MyHeader::MyHeader ()
{
}

MyHeader::~MyHeader ()
{
}

TypeId
MyHeader::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::MyHeader")
        .SetParent<Header> ()
        .AddConstructor<MyHeader> ()
    ;
    return tid;
}

TypeId
MyHeader::GetInstanceTypeId (void) const
{
    return GetTypeId ();
}

void
MyHeader::Print (std::ostream &os) const
{
    os << "data = " << m_data << endl;
}

uint32_t
MyHeader::GetSerializedSize (void) const
{
    return 2;
}

void
MyHeader::Serialize (Buffer::Iterator start) const
{
    start.WriteHtonU16 (m_data);
}

uint32_t
MyHeader::Deserialize (Buffer::Iterator start)
{
    m_data = start.ReadNtohU16 ();

    return 2;
}

void 
MyHeader::SetData (uint16_t data)
{
    m_data = data;
}

uint16_t 
MyHeader::GetData (void) const
{
    return m_data;
}

class master : public Application
{
public:
    master (uint16_t port, Ipv4InterfaceContainer& ip,Ipv4InterfaceContainer& node_ip);
    virtual ~master ();
private:
    virtual void StartApplication (void);
    void HandleRead (Ptr<Socket> socket);
    void ConnectToMappers(Ipv4InterfaceContainer& m_ips);
    // void SendToMappers()
    void HandleSend (uint16_t data);

    uint16_t _port;
    Ipv4InterfaceContainer _ip;
    Ipv4InterfaceContainer _node_ip;
    Ptr<Socket> _rec_socket;
    vector< Ptr<Socket> > _mapper_sockets;
};


class client : public Application
{
public:
    client (uint16_t port, Ipv4InterfaceContainer& ip , Ipv4InterfaceContainer& self_ip);
    
    virtual ~client ();
    

private:
    virtual void StartApplication (void);
    void HandleRead (Ptr<Socket> socket);

    uint16_t _port;
    // Ptr<Socket> _socket; not needed
    Ipv4InterfaceContainer _ip;
    Ipv4InterfaceContainer _self_ip;
    Ptr<Socket> _rec_socket;
};


class mapper: public Application
{
public:
    mapper(uint16_t port, Ipv4InterfaceContainer& ip,
        uint8_t i, Ipv4InterfaceContainer& client_ip);
    virtual ~mapper();
private:
    virtual void StartApplication ();
    void HandleRead (Ptr<Socket> socket);
    void InitMap();

    uint16_t _port;
    Ptr<Socket> _rec_socket;
    Ptr<Socket> _send_socket;
    Ipv4InterfaceContainer _ip;
    Ipv4InterfaceContainer _client_ip;
    uint8_t _mapper_number;
    std::unordered_map<uint16_t, char>  _umap;
};

static const int MAX_MAPPER = 1;

int
main (int argc, char *argv[])
{
    // Packet::EnablePrinting();
    // PacketMetadata::Enable();
    double error = 0.000001;
    string bandwidth = "1Mbps";
    bool verbose = true;
    double duration = 60.0;
    bool tracing = false;

    srand(time(NULL));

    CommandLine cmd (__FILE__);
    cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
    cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

    cmd.Parse (argc,argv);

    if (verbose)
    {
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();

    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");

    // Node Containers:
    NodeContainer wifiStaNodeClient;
    wifiStaNodeClient.Create (1);

    NodeContainer wifiStaNodeMaster;
    wifiStaNodeMaster.Create (1);

    NodeContainer mapperNodeContainer;
    mapperNodeContainer.Create (MAX_MAPPER);

    // Net Device Container:
    NetDeviceContainer mapperNetDeviceConatainer;

    NetDeviceContainer staDeviceClient;

    NetDeviceContainer staDeviceMaster;


    // Installing WiFi on the corresponding Net Devices for Nodes in the Node Container
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
    staDeviceMaster = wifi.Install (phy, mac, wifiStaNodeMaster);

    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing",
                BooleanValue (false));

    staDeviceClient = wifi.Install (phy, mac, wifiStaNodeClient);
    mapperNetDeviceConatainer = wifi.Install (phy, mac, mapperNodeContainer);
    
    // Initializing error values
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (error));
    phy.SetErrorRateModel ("ns3::YansErrorRateModel");

    // Setup mobility
    MobilityHelper mobility;

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
    // Walking mobility
    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                               "Bounds",
                                RectangleValue (Rectangle (-50, 50, -50, 50)));
    mobility.Install (wifiStaNodeClient);

    // Standstill mobility
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiStaNodeMaster);
    mobility.Install (mapperNodeContainer);

    // Adding nodes to the event tracker stack
    InternetStackHelper stack;
    stack.Install (wifiStaNodeClient);
    stack.Install (wifiStaNodeMaster);
    stack.Install (mapperNodeContainer);

    // Assigning IPs
    Ipv4AddressHelper address;

    Ipv4InterfaceContainer staNodeClientInterface;
    Ipv4InterfaceContainer staNodesMasterInterface;
    Ipv4InterfaceContainer mapperIPInterface;

    address.SetBase ("10.1.3.0", "255.255.255.0");
    staNodeClientInterface = address.Assign (staDeviceClient);
    staNodesMasterInterface = address.Assign (staDeviceMaster);
    mapperIPInterface = address.Assign (mapperNetDeviceConatainer);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    uint16_t port = 1102;

    // Creating Client
    Ptr<client> clientApp = CreateObject<client> (port, staNodesMasterInterface , staNodeClientInterface);
    wifiStaNodeClient.Get (0)->AddApplication (clientApp);
    clientApp->SetStartTime (Seconds (0.0));
    clientApp->SetStopTime (Seconds (duration));  

    // Creating Master
    Ptr<master> masterApp = CreateObject<master> (port, staNodesMasterInterface, mapperIPInterface);
    wifiStaNodeMaster.Get (0)->AddApplication (masterApp);
    masterApp->SetStartTime (Seconds (0.0));
    masterApp->SetStopTime (Seconds (duration));  

    // Creating Mapper
    for (uint8_t mapper_num = 0; mapper_num < MAX_MAPPER; mapper_num++)
    {
        Ptr<mapper> mapperApp = CreateObject<mapper> (port,
                                                    mapperIPInterface,
                                                    mapper_num,
                                                    staNodeClientInterface);
        mapperNodeContainer.Get (mapper_num)->AddApplication(mapperApp);
        mapperApp->SetStartTime (Seconds (0.0));
        mapperApp->SetStopTime (Seconds (duration));
    }

    // Logging
    NS_LOG_INFO ("Run Simulation");

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    ThroughputMonitor (&flowHelper, flowMonitor, error);
    AverageDelayMonitor (&flowHelper, flowMonitor, error);

    Simulator::Stop (Seconds (duration));
    Simulator::Run ();

    return 0;
}



client::client (uint16_t port, Ipv4InterfaceContainer& ip , Ipv4InterfaceContainer& self_ip)
        : _port (port),
          _ip (ip),
          _self_ip(self_ip)
{
    std::srand (time(0));
}

client::~client ()
{
}

static void GenerateTraffic (Ptr<Socket> socket, uint16_t data)
{
    Ptr<Packet> packet = new Packet();
    MyHeader m;
    m.SetData(data);

    packet->AddHeader (m);
    // packet->Print (std::cout); does not work without one of the two functions commented at the top of int main
    socket->Send (packet);

    Simulator::Schedule (Seconds (0.1), &GenerateTraffic, socket, rand() % 26);
}

void
client::StartApplication (void)
{
    Ptr<Socket> sock = Socket::CreateSocket (GetNode (),
                        UdpSocketFactory::GetTypeId ());
    InetSocketAddress sockAddr (_self_ip.GetAddress(0), _port);
    sock->Connect (sockAddr);

    GenerateTraffic (sock, 0);

    _rec_socket = Socket::CreateSocket (GetNode (),
                    UdpSocketFactory::GetTypeId ());
    InetSocketAddress local = InetSocketAddress (_ip.GetAddress(0), _port);
    _rec_socket->Bind (local);
    _rec_socket->SetRecvCallback (MakeCallback (&client::HandleRead, this));
}

master::master (uint16_t port, Ipv4InterfaceContainer& ip,Ipv4InterfaceContainer& node_ip)
        : _port (port),
          _ip (ip),
          _node_ip (node_ip)
{
    std::srand (time(0));
}

master::~master ()
{
}

void
master::StartApplication (void)
{
    _rec_socket = Socket::CreateSocket (GetNode (),
                    UdpSocketFactory::GetTypeId ());
    InetSocketAddress local = InetSocketAddress (_ip.GetAddress(0), _port);
    _rec_socket->Bind (local);
    // Ptr<Node> nodeptr = _rec_socket->GetNode (); not needed
    _rec_socket->SetRecvCallback (MakeCallback (&master::HandleRead, this));
    ConnectToMappers(_node_ip);
    // socket->SetSendCallback (MakeCallback (&master::HandleSend, this));

}

void 
master::HandleRead (Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv ()))
    {
        if (packet->GetSize () == 0)
        {
            break;
        }

        MyHeader destinationHeader;
        // packet->Print(std::cout); simply does not work
        packet->RemoveHeader (destinationHeader);
        uint16_t data = destinationHeader.GetData();
        // destinationHeader.Print(std::cout); simply does not work

        // send to 
        HandleSend(data);
    }
}

void master::HandleSend(uint16_t data)
{
    MyHeader Sendnode;
    Sendnode.SetData(data);
    Sendnode.Print(std::cout);
    Ptr<Packet> packet = new Packet();
    packet->AddHeader(Sendnode);

    for(int i = 0; i < MAX_MAPPER; i++)
    {
        _mapper_sockets [i] -> Send (packet);
    }
}

void master::ConnectToMappers(Ipv4InterfaceContainer& m_ips)
{
    for (uint32_t i_s = 0; i_s < m_ips.GetN(); i_s++)
    {
        Ptr<Socket> i_socket = Socket::CreateSocket (GetNode(), 
                                TcpSocketFactory::GetTypeId());
        i_socket->Connect(InetSocketAddress(m_ips.GetAddress(i_s), _port));
        _mapper_sockets.push_back(i_socket);
    }
}

mapper::mapper(uint16_t port, Ipv4InterfaceContainer& ip, uint8_t i, Ipv4InterfaceContainer& client_ip)
{
    _port = port;
    _ip = ip;
    _mapper_number = i;
    _client_ip = client_ip;
}

void mapper::StartApplication()
{
    InitMap();
   _rec_socket = Socket::CreateSocket(GetNode (),
                TcpSocketFactory::GetTypeId());
    InetSocketAddress sockadr = InetSocketAddress (_ip.GetAddress(_mapper_number), _port);
    _rec_socket->Bind (sockadr);
    _rec_socket->Listen();
    
    _send_socket = Socket::CreateSocket(GetNode (),
                    UdpSocketFactory::GetTypeId ());
    InetSocketAddress cli_sockadr = InetSocketAddress (_client_ip.GetAddress(0), _port);
    _send_socket->Connect(cli_sockadr);
    _rec_socket->SetRecvCallback (MakeCallback (&mapper::HandleRead, this));
}

void mapper::HandleRead (Ptr<Socket> socket)
{
    Ptr<Packet> packet;

    while ((packet = socket->Recv ()))
    {
        if (packet->GetSize () == 0)
        {
            break;
        }

        MyHeader receiverHeader;

        packet->RemoveHeader (receiverHeader);

        uint16_t test = receiverHeader.GetData();

        char sendChar = _umap[test];
        uint16_t sendData = static_cast<uint16_t>(sendChar);        

        MyHeader sendHeader;
        sendHeader.SetData (sendData);
        sendHeader.Print(std::cout);

        Ptr<Packet> sendPacket = new Packet();
        sendPacket->AddHeader (sendHeader);            

        _send_socket->Send (sendPacket);
    }
}


void mapper::InitMap()
{
    if(_mapper_number == 0)
    {
        _umap[0] = 'a';
        _umap[1] = 'b';
        _umap[2] = 'c';
        _umap[3] = 'd';
        _umap[4] = 'e';
        _umap[5] = 'f';
        _umap[6] = 'g';
        _umap[7] = 'h';
        _umap[8] = 'i';
    }
    else if(_mapper_number == 1)
    {

    }
    else if(_mapper_number == 2)
    {

    }
    else
    {
        // error
    }
}

mapper::~mapper()
{
}