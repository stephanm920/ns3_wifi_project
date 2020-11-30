/*
 * Basic WiFi network Topology
 * Node n2 acts as a UDP echo server, while n1 acts as client
 * Stephan Muller, Danny Faruqi
 * ECE 4614 Final Project
*/

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

// Topology Diagram
//
//	Wifi 10.1.1.0
//			AP
// *	*	*
// |	|	|
// n1	n2	n0


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BasicWiFi");

int main(int argc, char* argv[]){
	
	bool verbose = true;
	uint32_t nWifi = 2; //2 WiFi STA nodes connected to AP
	//TODO: add pcap tracing so packets can be viewed in Wireshark
	//bool tracing = true;
	
	CommandLine cmd (__FILE__);
	cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
	//cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
	
	cmd.Parse(argc, argv);
	
	if (verbose)
    {
		LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
	
	//Create the STA nodes
	NodeContainer wifiStaNodes;
	wifiStaNodes.Create(nWifi);
	
	//Create the access point
	NodeContainer wifiApNode;
	wifiApNode.Create(1);
	
	//Create the physical channel
	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetChannel(channel.Create());
	
	//Set up the STA nodes
	//TODO: figure out how to specify wifi bandwidth/delay
	WifiHelper wifi;
	wifi.SetRemoteStationManager("ns3::AarfWifiManager");
	
	WifiMacHelper mac;
	Ssid ssid = Ssid("sim-ssid");
	mac.SetType("ns3::StaWifiMac", 
		"Ssid", SsidValue(ssid),
		"ActiveProbing", BooleanValue(false));
	
	NetDeviceContainer wifiStaDevices;
	wifiStaDevices = wifi.Install(phy, mac, wifiStaNodes);
	
	//Set up the AP node
	mac.SetType("ns3::ApWifiMac",
		"Ssid", SsidValue(ssid));
		
	NetDeviceContainer wifiApDevice;
	wifiApDevice = wifi.Install(phy, mac, wifiApNode);
	
	//Set Mobility to static (i.e. nodes don't move)
	MobilityHelper mobility;
	mobility.SetPositionAllocator("ns3::GridPositionAllocator",
		"DeltaX", DoubleValue(5.0),
		"DeltaY", DoubleValue(10.0),
		"GridWidth", UintegerValue(2),
		"LayoutType", StringValue("RowFirst"));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(wifiStaNodes);
	mobility.Install(wifiApNode);
	
	//Install internet stack to both STA and AP nodes
	InternetStackHelper stack;
	stack.Install (wifiApNode);
	stack.Install (wifiStaNodes);
	
	//Assign IPv4 addresses to nodes in subnet 10.1.1.0/24
	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
	address.Assign(wifiApDevice);
	Ipv4InterfaceContainer wifiStaInterfaces;
	wifiStaInterfaces = address.Assign(wifiStaDevices);
	
	//TODO: add TCP? maybe not for intermediate report though
	
	//Install the UDP echo server on n2
	UdpEchoServerHelper echoServer(9); //create echo server application listening on port 9
	ApplicationContainer serverApps = echoServer.Install(wifiStaNodes.Get(1));
	serverApps.Start(Seconds(1.0));
	serverApps.Stop(Seconds(10.0));
	
	//Install the UDP echo client on n1, which sends request to n2
	UdpEchoClientHelper echoClient(wifiStaInterfaces.GetAddress(1), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
	ApplicationContainer clientApps = echoClient.Install(wifiStaNodes.Get(0));
	clientApps.Start(Seconds(2.0));
	clientApps.Stop(Seconds(10.0));
	
	//START THE SIM
	Simulator::Stop(Seconds(10.0));
	Simulator::Run();
	Simulator::Destroy();
	
	return 0;
}