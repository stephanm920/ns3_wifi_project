/*
 * TCP Bulk Send WiFi network Topology
 * Node n2 acts as a TCP sink, while n1 acts as TCP bulk sender
 * Stephan Muller, Danny Faruqi
 * ECE 4614 Final Project
*/

#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"
#include "ns3/packet-sink.h"

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
	bool tracing = false;
	uint32_t maxBytes = 0;
	std::string phyMode ("VhtMcs1"); //wave 1 AC model (866 Mpbs link)
	double rss = -80; //signal strength (dBm)
	
	CommandLine cmd (__FILE__);
	cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
	cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
	cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
	cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
	cmd.AddValue ("rss", "received signal strength", rss);
	
	
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
	channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	channel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.Set ("RxGain", DoubleValue (0) );
	phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
	phy.SetChannel(channel.Create());
	
	//Set up the STA nodes
	WifiHelper wifi;
	wifi.SetStandard (WIFI_STANDARD_80211ac); //Set to AC mode
	
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode",StringValue (phyMode),
                                 "ControlMode",StringValue (phyMode));
	
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
	
	
	// Install bulk send application on node 1
	uint16_t port = 9;
	
	BulkSendHelper source("ns3::TcpSocketFactory",
                         InetSocketAddress (wifiStaInterfaces.GetAddress (1), port));
	// Set the amount of data to send in bytes.  Zero is unlimited.
	source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
	ApplicationContainer sourceApps = source.Install (wifiStaNodes.Get (0));
	sourceApps.Start (Seconds (0.0));
	sourceApps.Stop (Seconds (10.0));
	
	// Create sink application and install on node 2
	PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
	ApplicationContainer sinkApps = sink.Install (wifiStaNodes.Get (1));
	sinkApps.Start (Seconds (0.0));
	sinkApps.Stop (Seconds (10.0));
	
	//
	// Set up tracing if enabled
	//
	if (tracing)
    {
      AsciiTraceHelper ascii;
      phy.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      phy.EnablePcapAll ("tcp-bulk-send", false);
    }
    
    //animate the nodes with NetAnim
    AnimationInterface anim ("TCP_bulk_send.xml");
    anim.SetConstantPosition(wifiStaNodes.Get(0), 2.0, 1.0);
    anim.SetConstantPosition(wifiStaNodes.Get(1), 4.0, 1.0);
	anim.SetConstantPosition(wifiApNode.Get(0), 3.0, 2.0);
	
	//START THE SIM
	NS_LOG_INFO ("Run Simulation.");
	Simulator::Stop (Seconds (10.0));
	Simulator::Run ();
	Simulator::Destroy ();
	NS_LOG_INFO ("Done.");
	
	Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
	std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  
	return 0;
}