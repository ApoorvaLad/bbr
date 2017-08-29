/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>,
 *          Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 *          Greeshma Umapathi
 *
 * James. P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "tcp-bbr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "rtt-estimator.h"
//#include "tcp-socket-base.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("TcpBbr");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TcpBbr);

TypeId
TcpBbr::GetTypeId (void)
{
  static TypeId tid = TypeId("ns3::TcpBbr")
    .SetParent<TcpNewReno>()
    .SetGroupName ("Internet")
    .AddConstructor<TcpBbr>()
    .AddAttribute("FilterType", "Use this to choose no filter or Tustin's approximation filter",
                  EnumValue(TcpBbr::TUSTIN), MakeEnumAccessor(&TcpBbr::m_fType),
                  MakeEnumChecker(TcpBbr::NONE, "None", TcpBbr::TUSTIN, "Tustin"))
    .AddAttribute("ProtocolType", "Use this to let the code run as Bbr or BbrPlus",
                  EnumValue(TcpBbr::Bbr),
                  MakeEnumAccessor(&TcpBbr::m_pType),
                  MakeEnumChecker(TcpBbr::Bbr, "Bbr",TcpBbr::BbrPLUS, "BbrPlus"))
    .AddTraceSource("EstimatedBW", "The estimated bandwidth",
                    MakeTraceSourceAccessor(&TcpBbr::m_currentBW),
                    "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

TcpBbr::TcpBbr (void) :
  TcpNewReno (),
  m_currentBW (0), 
  m_lastSampleBW (Seconds(0)),
  m_nextTimeStamp (Seconds(0)),
  m_bdwEstimateTime(Seconds(0)), 
  m_currentTimeStamp (Simulator::Now().GetSeconds()),
  m_lastBW (0),
 
  i(0),
  m_probeTime(Time(0)),
  m_ackedSegments (0),
  m_IsCount (false),
  m_alpha (2),
  m_beta (4),
  m_gamma (1),
  m_baseRtt (Time::Max ()),
  m_cntRtt (0),
  m_doingBbrNow (true),
  m_begSndNxt (0),
 // m_counter(0),
  m_minRttInterval (10),
  m_maxBwdRttCount (8),
  

  m_rttCounter(0), 
   m_totalRTT(Time (0)),
   m_totalBdw(0),
  m_avgRTTwindow(0),
 
  m_avgBdwWindow(0),
  m_slowStart(true),
 // m_firstBdw(true),
  m_slowStartPacketscount(3),
  m_drainFactor(0.75),
  m_probeFactor(1.25),
  m_steadyFactor(1.0),
  
  m_currentRtt(Time(0))
 

{
  NS_LOG_FUNCTION (this);

}

TcpBbr::TcpBbr (const TcpBbr& sock) :
  TcpNewReno (sock),
  m_currentBW (sock.m_currentBW),
 
  m_nextTimeStamp(sock.m_nextTimeStamp),
  m_currentTimeStamp(sock.m_currentTimeStamp),
  m_lastBW (sock.m_lastBW),
 // m_minRtt (Time (0)),
  m_pType (sock.m_pType),
  m_fType (sock.m_fType),
  m_IsCount (sock.m_IsCount),
  m_alpha (sock.m_alpha),
    m_beta (sock.m_beta),
    m_gamma (sock.m_gamma),
    m_baseRtt (sock.m_baseRtt),
    m_cntRtt (sock.m_cntRtt),
    m_doingBbrNow (true),
    m_begSndNxt (0),
  m_minRttInterval(sock.m_minRttInterval),
  m_maxBwdRttCount(sock.m_maxBwdRttCount),
 
  m_rttCounter(sock.m_rttCounter),
    m_totalRTT(sock.m_totalRTT),
  m_avgRTTwindow(sock.m_avgRTTwindow),
 // m_maxBwd(sock.m_maxBwd),
  m_slowStart(sock.m_slowStart),
 //  m_firstBdw(true),
  m_slowStartPacketscount(sock.m_slowStartPacketscount),
  m_drainFactor(sock.m_drainFactor),
  m_probeFactor(sock.m_probeFactor),
  m_steadyFactor(sock.m_steadyFactor),
  m_currentRtt(sock.m_currentRtt)

{

  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}

TcpBbr::~TcpBbr (void)
{

}

int TcpBbr::m_maxBwd = 1900000;
Time TcpBbr::m_minRtt = Time(MilliSeconds(22));
double TcpBbr::m_bdp = 0;
double TcpBbr::gain = 1.0;
int TcpBbr::m_firstBdw = 1;
Time TcpBbr::m_lastRtt = Time(0);
//map functionality to get min RTT and MaxThroughput 
//Saturday
void
TcpBbr::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                        const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << packetsAcked << rtt);
   m_ackedSegments += packetsAcked;
  // m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,m_ackedSegments,true);
//  m_wndEstimateEvent = Simulator::Schedule(rtt,&TcpBbr::EstimateMinMax,this,rtt,tcb);
  //
 
   m_lastRtt = rtt;
 // m_wndEstimateEvent.Cancel();
  if (rtt.IsZero ())
    {
      NS_LOG_WARN ("RTT measured is zero!");
      return;
    }

    m_ackedSegments = 1;
   // if(m_firstBdw) {
       // std::cerr<<m_firstBdw<<std::endl;
       // tcb->m_cWnd =  32000;
      //  m_currentBW = (double)tcb->m_cWnd / rtt.GetSeconds();
       // std::cerr<<m_currentBW<<std::endl;
       // m_minRtt = rtt;
       
      //  m_firstBdw =false;
       // SetState(tcb,m_ackedSegments,1);
    //  m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,m_ackedSegments,true);
     // }

   // std::cerr<<"Received"<<std::endl;
    

 //    std::cerr<<Simulator::Now().GetSeconds()<<" "<<rtt.GetSeconds()<<std::endl;
 // std::cerr<<"Curremt RTT "<<rtt.GetSeconds()<<std::endl;   
 // std::cerr<<<<std::endl;
 // std::cerr << rtt.GetSeconds()<< " "<< Simulator::Now().GetSeconds()<<std::endl; 

 //std::cerr << tcb->m_lastAckedSeq<<std::endl;  
  //std::cerr<<"PktsAcked: current Timestamp: "<<rtt.GetSeconds()<<std::endl;
  //std::cerr<<"PktsAcked: current Window size: "<<tcb->m_cWnd<<std::endl;
// min rtt window
 
 // std::cerr<<m_ackedSegments<< " "<<Simulator::Now().GetSeconds()<<std::endl;

  //std::cerr << Simulator::Now().GetSeconds()<< " " <<rtt.GetSeconds()<<std::endl; 


  
//  std::cerr<<m_bdp<<std::endl;

  

}

//Sunday - Monday
//One day to wrap up and test out throughput
//already have max throughtput and minRtt every packet received
  //
  //slow start & drain phase
  //probe max Bnadwidth & minRtt
  //adjust sending rate on cwnd

//for today
//implement window 
//estimate bdw properly
//tune parameters
// need to log estimate bandwidth to see if it works properly
//in conclusion
//estimating bandwidth is not correct and thus we end up with situations where increasewindow is not called very usually
void 
TcpBbr::Send(Ptr<TcpSocketState> tcb)
{
  

  if(m_firstBdw == 1){
     tcb->m_cWnd =  1900000;
    m_firstBdw = 0;
    SetState(tcb,1);

  }
  
  
  m_bdp = m_minRtt.GetSeconds() * m_maxBwd;
  std::cerr<<Simulator::Now().GetSeconds()<<" bdp  gain "<<m_bdp<<std::endl;
  std::cerr<<Simulator::Now().GetSeconds()<<" Gain factor "<<gain<<std::endl;
  m_bdp = m_bdp * gain;
   std::cerr<<Simulator::Now().GetSeconds()<<" bdp  after gain "<<m_bdp<<std::endl;
  //std::cerr<<"BDP is "<<m_bdp<<std::endl;
 // std::cerr<<"BDP is "<<m_bdp<<std::endl;
  tcb->m_cWnd = m_bdp;
 
  std::cerr<<Simulator::Now().GetSeconds()<<" "<<tcb->m_cWnd<<std::endl;
 // std::cerr<<"Send "<<m_bdp<<" "<<Simulator::Now().GetSeconds()<<std::endl;
  //
  

  //std::cerr<<"Send"<<std::endl;
 /* m_bdp = m_minRtt.GetSeconds() * m_maxBwd;
  std::cerr<<"Min rtt "<<m_minRtt.GetSeconds()<<std::endl;
  std::cerr<<"M"m_maxBwd<<std::endl;
//std::cerr<<m_bdp<<std::endl;*/
 // tcb->m_cWnd = m_bdp;
  //}
//  std::cerr<<"Send"<<std::endl;
  //tcb->m_cWnd = m_bdp;
 // std::cerr<<tcb->m_cWnd<<std::endl;

}

void 
TcpBbr::Recv(const TcpHeader& tcpHeader,Time m_receiveTime,uint32_t size)
{

 
 
 
  m_counter = tcpHeader.GetSequenceNumber();
  receiveTimeWindow[m_counter] = m_receiveTime;
  Time m_prevTime;
 
 // uint32_t seqNumber = ;
  if(tcpHeader.GetSequenceNumber().GetValue() != 1) 
  {
    m_prevTime = receiveTimeWindow[m_counter - 536];
   // std::cerr<<"Prev Time "<<m_receiveTime<<std::endl;
    //std::cerr<<"Receive Time "<<m_prevTime<<std::endl;
    m_currentBW = (uint32_t) size / (m_receiveTime.GetSeconds() - m_prevTime.GetSeconds());


  } else {
    m_lastRtt = Time(MilliSeconds(22));
    m_currentBW = 1900000;
   // tcb->m_cWnd = 1900000;
  }
 
  m_rttWindow[m_rttCounter]= m_lastRtt;
 
    if(m_rttWindow.size()>20000) 
  
    m_rttWindow.erase(m_rttWindow.begin());//erase oldest


  m_minRtt = Time::Max();
    for(std::map<uint32_t,Time>::iterator it = m_rttWindow.begin();it!=m_rttWindow.end();++it )
    {                  
      m_minRtt = std::min(m_minRtt,it->second);
    }

 

 // std::cerr<<"Prev time "<<m_prevTime.GetSeconds()<<std::endl;
  //std::cerr<<"current time "<<m_receiveTime.GetSeconds()<<std::endl;
  
 
// std::cerr<<Simulator::Now().GetSeconds()<<m_minRtt.GetSeconds()<<std::endl;
 
  m_bdwWindow[m_rttCounter] = m_currentBW;

    // m_maxBwd = 0;
  if(m_bdwWindow.size()>500) //We need to set window size to be 10 secs instead of 8 packets received
  
    m_bdwWindow.erase(m_bdwWindow.begin());//erase oldest
  for(std::map<uint32_t,int>::iterator it = m_bdwWindow.begin();it!=m_bdwWindow.end();++it )
  {  m_maxBwd = (int) std::max(m_maxBwd,it->second);
    // std::cerr<<"Data rate "<<it->second<<std::endl;
     //std::cerr<<"MAx data rate "<<m_maxBwd<<std::endl;
    // std::cerr<<"REceive "<<m_maxBwd<<std::endl;
                //std::cerr<<"PktsAcked: bdw  "<<it->second<<std::endl;

   }


 //  std::cerr<<"Min RTT registered "<<" "<<m_minRtt.GetSeconds()<<std::endl;
  // std::cerr<<"Maximum bandwidth registered "<<" "<<m_maxBwd<<std::endl;

   // std::cerr<<Simulator::Now().GetSeconds()<<" "<<m_currentBW<<std::endl;

  


}
void
TcpBbr::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  

tcb->m_ssThresh = 2147483646;
  if ( false)
 // if(false)
    {
      
      
      segmentsAcked = TcpNewReno::SlowStart (tcb, segmentsAcked);
      if(m_slowStartPacketscount>=1)
        m_slowStartPacketscount--;
      if(m_slowStartPacketscount<1 && fabs(m_maxBwd-m_currentBW)<0.2*m_maxBwd)

        m_slowStart = false;
      m_lastTimeStamp = Simulator::Now();

    }
  //   m_nextTimeStamp = m_nextTimeStamp + 8;
   
  
    //Steady state
   

    //drain phase, bandwidth stays the same but Rtt increases
    //every 80ms will have drain or probe phase
   // if(false) 
   //else if( fabs(m_lastTimeStamp.GetSeconds()-Simulator::Now().GetSeconds())>8 && fabs(m_maxBwd-m_currentBW)<0.02*m_maxBwd && m_currentRtt.GetSeconds()/m_minRtt.GetSeconds()>1.2)//if (tcb->m_lastAckedSeq >= m_begSndNxt)
    //std::cerr<< "Max bdw: "<<m_maxBwd<<"current "<<m_currentBW<<std::endl;
    // if(  fabs(m_maxBwd-m_currentBW)<0.02*m_maxBwd )//&& m_currentRtt.GetSeconds()/m_minRtt.GetSeconds()>1.2)//if (tcb->m_lastAckedSeq >= m_begSndNxt)
   
    //BDP
    

 /* if (!m_doingBbrNow)
    {
      // If Bbr is not on, we follow NewReno algorithm
      NS_LOG_LOGIC ("Bbr is not turned on, we follow NewReno algorithm.");
      TcpNewReno::IncreaseWindow (tcb, segmentsAcked);
      return;
    }*/
  //slow start
  }


 void 
 TcpBbr::SetState(Ptr<TcpSocketState> tcb, int i){



/*
std::cerr<<"Average RTT "<<m_avgRTTwindow<<std::endl;
std::cerr<<"Min RTT "<<m_minRtt<<std::endl;*/
  

//   std::cerr<<m_nextTimeStamp<<std::endl;


 // std::cerr<<"probe"<<std::endl;


if( i == 1) {
 
 gain = m_probeFactor;
//std::cerr<<m_probeFactor<<std::endl;
// m_bdp = m_bdp * m_probeFactor;

 i = i+1;
 
// std::cerr<<"Probe factor at "<<Simulator::Now().GetSeconds()<<"is "<<gain<<std::endl; 
 m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,i);

  }
else if(i == 2) {
//  m_drainEvent.Cancel();
  gain = m_drainFactor;
  //m_bdp = m_bdp * m_drainFactor;
 
  i = i + 1;
 //  std::cerr<<"Drai factor at "<<Simulator::Now().GetSeconds()<<"is "<<gain<<std::endl; 
 // std::cerr<<Simulator::Now().GetSeconds()<<" "<<m_currentBW<<std::endl;
  m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,i);


} else {
  gain = m_steadyFactor;
  i++;
  
//  m_bwEstimateEvent.Cancel();
 // m_bdp = m_bdp * m_steadyFactor;
  // std::cerr<<"Steady state factor at "<<Simulator::Now().GetSeconds()<<"is "<<gain<<std::endl; 
  m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,i);
   
  //m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::CheckState,this,tcb,segmentsAcked,true);
 // std::cerr<<Simulator::Now().GetSeconds()<<" "<<m_currentBW<<std::endl;
  if(i == 9) {

  
    i =1;
    m_probeEstimateEvent.Cancel();
    m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,i);
  }

}
  //tcb->m_cWnd = tcb->m_cWnd*m_probeFactor;
 // std::cerr<<Simulator::Now().GetSeconds()<<std::endl;
  

 // std::cerr<<m_nextTimeStamp<<std::endl;

 // m_probeEstimateEvent.Cancel();
  
  
   
 
}


/*
    std::cerr<<"probe"<<std::endl;

     
    }
    else {
    
      std::cerr<<"Should drain"<<std::endl;
    //  tcb->m_cWnd = tcb->m_cWnd*m_drainFactor;

    }*/


 

///m_probeEstimateEvent.Cancel();
/*   if((m_maxBwd > m_currentBW) && (m_currentRtt.GetSeconds()/m_minRtt.GetSeconds()>1.2))
    {
     //std::cerr<<"In drain "<<std::endl;
       tcb->m_cWnd = tcb->m_cWnd*m_drainFactor;
          //  std::cerr<<"IncreaseWindow: drain phase window "<<tcb->m_cWnd<<std::endl;
            m_lastTimeStamp = Simulator::Now();


  }*/
  //probe phase
  //either probe or drain within 80ms
  //else if(false) {
   //else if(fabs(m_lastTimeStamp.GetSeconds()-Simulator::Now().GetSeconds())>8){ // A Bbr cycle has finished, we do Bbr cwnd adjustment every RTT.

//if(m_currentBW > m_maxBwd){

  
 /* }
 */
   // EstimateBW(rtt,tcb);
 



  void 
 TcpBbr:: CheckState(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, bool increaseWindow){ 
 // m_count++;
   
  m_bwEstimateEvent.Cancel();
  m_bdp = m_bdp * m_steadyFactor;
 m_bwEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::CheckState,this,tcb,segmentsAcked,true);
  //m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::CheckState,this,tcb,segmentsAcked,true);
 // std::cerr<<Simulator::Now().GetSeconds()<<" "<<m_currentBW<<std::endl;
  //if(m_count == 6) {

  
  //  m_count =0;
    m_probeEstimateEvent.Cancel();
    m_probeEstimateEvent = Simulator::Schedule(m_minRtt,&TcpBbr::SetState,this,tcb,true);
 // }
// std::cerr<<"Average RTT "<<m_avgRTTwindow<<std::endl;

 /* std::cerr<<"Max bandwidth "<<m_maxBwd<<std::endl;
  std::cerr<<"Current bandwidth"<<m_currentBW<<std::endl;
  std::cerr<<"min rtt "<<m_minRtt<<std::endl;
  std::cerr<<"current rtt "<<m_currentRtt<<std::endl;
 // std::cerr<<"Herer"<<std::endl;*/

  

/*if((m_avgRTTwindow/m_minRtt.GetSeconds()>1.2))
    {
     
       tcb->m_cWnd = tcb->m_cWnd*m_drainFactor;
     //dc  m_drainEvent.Cancel();
   //  std::cerr<<Simulator::Now().GetSeconds()<<" "<<m_avgBdwWindow<<std::endl;

      


 }*/
 /*if

*/
 
}


 void
 TcpBbr::EstimateMinMax(const Time& rtt,Ptr<TcpSocketState> tcb){


//EstimateBW(rtt,tcb);
 

   
   
   //m_bdp = m_minRtt.GetSeconds() * m_maxBwd;
   // std::cerr<<Simulator::Now().GetSeconds()<<" "<<rtt.GetSeconds()<<std::endl;
   //std::cerr<<m_minRtt.GetSeconds()<<std::endl;
   //std::cerr<<m_maxBwd<<std::endl;
   //tcb->m_cWnd = m_bdp;
   //std::cerr<<m_bdp<<std::endl;
   //std::cerr<<tcb->m_cWnd<<std::endl;
  
   //std::cerr<<rtt.GetSeconds()<<std::endl;

 // std::cerr<<m_bdp<<std::endl;
  
   
    

   //tcb->m_cWnd = m_maxBwd * m_minRtt.GetSeconds();
 //  std::cerr<<m_minRtt<<std::endl;
// /  std::cerr<<m_maxBwd*m_minRtt.GetSeconds()<<std::endl;

 


 // tcb->m_cWnd = m_maxBwd*m_minRtt.GetSeconds();

// 


  //std::cerr<<"RTT is "<<rtt.GetSeconds()<<std::endl;
   
  //std::cerr<<m_totalRTT.GetSeconds()<<std::endl;
 //  std::cerr<<m_totalRTT.GetSeconds()<<std::endl;
 // std::cerr<<m_totalRTT/m_rttWindow.size()<<std::endl;
 // m_avgRTTwindow = m_totalRTT/m_rttWindow.size();

  //std::cerr<<m_avgRTTwindow<<std::endl;
 // std::cerr<<  Simulator::Now().GetSeconds()<<" "<<rtt.GetSeconds()<<std::endl;


//  std::cerr<<"Current RTT "<<m_currentRtt.GetSeconds()<<std::endl;
  //std::cerr<<"Minimum RTT "<<m_minRtt.GetSeconds()<<std::endl;  
//std::cout<<"PktsAcked::m_minRtt="<<m_minRtt<<std::endl;
  
//max bdw window
 
  // m_bwEstimateEvent = Simulator::Schedule (rtt, &TcpBbr::EstimateBW,
    //                                               this, rtt, tcb);
  
  

 

 
 //   m_maxBwd =m_currentBW;
   


 }


void
TcpBbr::EstimateBW (const Time &rtt, Ptr<TcpSocketState> tcb)
{
//  NS_LOG_FUNCTION (this);
//  std::cerr<<"In estimate "<<tcb->m_cWnd<<std::endl;
  m_currentBW = (uint32_t)tcb->m_cWnd / rtt.GetSeconds();
//  std::cerr<<tcb->m_cWnd<<std::endl;
 // std::cerr<<Simulator::Now().GetSeconds()<< " "<<m_currentBW<<std::endl;
 
  //NS_ASSERT (!rtt.IsZero ());
  //m_currentBW = m_ackedSegments * tcb->m_segmentSize / rtt.GetSeconds ();

// m_currentBW = m_ackedSegments * tcb->m_segmentSize / rtt.GetSeconds ();
 // m_lastSampleBW = m_currentBW;
  //m_currentBW = (double)tcb->m_cWnd / rtt.GetSeconds();


//approximate accordding to last 20 packets received
  /*if(m_timeWindow.size()<=21)
      m_currentBW = (double)tcb->m_cWnd / rtt.GetSeconds();
    else {
      auto it = m_timeWindow.crbegin();
      auto ittwenty = it;
      for(int i =0;i<19;i++)
        ittwenty++;
    //  std::map<uint32_t,Time>::iterator itold = m_rttWindow.end()-20;


      // Divide by 19 (one packet less)
      //std::cerr<<m_timeWindow;
     // std::cerr<<(it->second.GetSeconds() - ittwenty->second.GetSeconds())<<std::endl;
      
      m_currentBW= m_ackedSegments*tcb->m_segmentSize/(it->second.GetSeconds()-ittwenty->second.GetSeconds());
    }*/
    //std::cerr<< "EstimateBW: raw bdw"<<m_currentBW<<" "<<std::endl;

  //std::cerr<< "cur segmentsize"<<tcb->m_segmentSize<<" "<<std::endl;
//  std::cerr<<"EstimateBW: rtt is "<<rtt.GetSeconds() <<std::endl;
 //   std::cerr<<"EstimateBW: cwnd is "<<tcb->m_cWnd <<std::endl;
     //std::cerr<<"m_ackedSegments is "<<m_ackedSegments <<std::endl;

  // if (m_pType == TcpBbr::BbrPLUS)
  //   {
  //     m_IsCount = false;
  //   }
  //if not in rtt
  m_ackedSegments = 0;
  //NS_LOG_LOGIC ("Estimated BW: " << m_currentBW);

  // Filter the BW sample

 /* double alpha = 0.5;

  m_currentBW = alpha*m_lastBW+(1-alpha)*m_currentBW;
  m_lastBW = m_currentBW;
  // if (m_fType == TcpBbr::NONE)
  //   {
  //   }
  // else if (m_fType == TcpBbr::TUSTIN)
  //   {
      double sample_bwe = m_currentBW;
      m_currentBW = (alpha * m_lastBW) + ((1 - alpha) * ((sample_bwe + m_lastSampleBW) / 2));
      m_lastSampleBW = sample_bwe;
      m_lastBW = m_currentBW;*/
  //  }
  //  std::cerr<< "EstimateBW: bdw"<<m_currentBW<<" "<<std::endl;

  //NS_LOG_LOGIC ("Estimated BW after filtering: " << m_currentBW);
}

uint32_t
TcpBbr::GetSsThresh (Ptr<const TcpSocketState> tcb,
                          uint32_t bytesInFlight)
{
  (void) bytesInFlight;
  NS_LOG_LOGIC ("CurrentBW: " << m_currentBW << " minRtt: " <<
                m_minRtt << " ssthresh: " <<
                m_currentBW * static_cast<double> (m_minRtt.GetSeconds ()));
  return 2147483646;
 // return std::max (2*tcb->m_segmentSize,
                   //uint32_t (m_currentBW * static_cast<double> (m_minRtt.GetSeconds ())));
}

Ptr<TcpCongestionOps>
TcpBbr::Fork ()
{
 // m_nextTimeStamp = Simulator::Now().GetSeconds(); 
  return CreateObject<TcpBbr> (*this);
}

} // namespace ns3
