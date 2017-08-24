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
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 * and Greeshma Umapathi
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
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

#ifndef TCP_Bbr_H
#define TCP_Bbr_H

#include "tcp-congestion-ops.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"

namespace ns3 {

class Packet;
class TcpHeader;
class Time;
class EventId;

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP Bbr and Bbr+.
 *
 * Bbr and Bbr+ employ the AIAD (Additive Increase/Adaptive Decrease) 
 * congestion control paradigm. When a congestion episode happens, 
 * instead of halving the cwnd, these protocols try to estimate the network's
 * bandwidth and use the estimated value to adjust the cwnd. 
 * While Bbr performs the bandwidth sampling every ACK reception, 
 * Bbr+ samples the bandwidth every RTT.
 *
 * The two main methods in the implementation are the CountAck (const TCPHeader&)
 * and the EstimateBW (int, const, Time). The CountAck method calculates
 * the number of acknowledged segments on the receipt of an ACK.
 * The EstimateBW estimates the bandwidth based on the value returned by CountAck
 * and the sampling interval (last ACK inter-arrival time for Bbr and last RTT for Bbr+).
 */
class TcpBbr : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpBbr (void);
  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpBbr (const TcpBbr& sock);
  virtual ~TcpBbr (void);

  /**
   * \brief Protocol variant (Bbr or Bbr+)
   */
  enum ProtocolType 
  {
    Bbr,
    BbrPLUS
  };

  /**
   * \brief Filter type (None or Tustin)
   */
  enum FilterType 
  {
    NONE,
    TUSTIN
  };

  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);

  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                          const Time& rtt);

  virtual void Send(Ptr<TcpSocketState> tcb);

  virtual void Recv(const TcpHeader& tcpHeader,Time m_receiveTime,uint32_t size);
  /**
   * \brief Adjust cwnd following Bbr linear increase/decrease algorithm
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   */
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);



  virtual Ptr<TcpCongestionOps> Fork ();

private:
  
  void SetState(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,int i);

  void CheckState(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,bool increaseWindow);

 void EstimateMinMax(const Time& rtt,Ptr<TcpSocketState> tcb);

  /**
   * Update the total number of acknowledged packets during the current RTT
   *
   * \param [in] acked the number of packets the currently received ACK acknowledges
   */
  void UpdateAckedSegments (int acked);

  /**
   * Estimate the network's bandwidth
   *
   * \param [in] rtt the RTT estimation.
   * \param [in] tcb the socket state.
   */
  void EstimateBW (const Time& rtt, Ptr<TcpSocketState> tcb);

protected:
  TracedValue<uint32_t>    m_currentBW;              //!< Current value of the estimated BW
  Time                   m_lastSampleBW;           //!< Last bandwidth sample
  Time                m_nextTimeStamp;
  Time                    m_bdwEstimateTime;

  double                 m_currentTimeStamp;
  static uint32_t m_bdp;
  static int gain;
  double                 m_lastBW;                 //!< Last bandwidth sample after being filtered
  static Time            m_minRtt; 
  int i;
  Time m_probeTime;                //!< Minimum RTT
  enum ProtocolType      m_pType;                  //!< 0 for Bbr, 1 for Bbr+
  enum FilterType        m_fType;                  //!< 0 for none, 1 for Tustin

  int                    m_ackedSegments;          //!< The number of segments ACKed between RTTs
  bool                   m_IsCount;                //!< Start keeping track of m_ackedSegments for Bbr+ if TRUE
  EventId                m_bwEstimateEvent;       //!< The BW estimation event for Bbr+
  EventId m_probeEstimateEvent;
  EventId m_wndEstimateEvent;
  EventId m_drainEvent;
  uint32_t m_alpha;                  //!< Alpha threshold, lower bound of packets in network
  uint32_t m_beta;                   //!< Beta threshold, upper bound of packets in network
  uint32_t m_gamma;                  //!< Gamma threshold, limit on increase
  Time m_baseRtt;                    //!< Minimum of all Bbr RTT measurements seen during connection
  uint32_t m_cntRtt;                 //!< Number of RTT measurements during last RTT
  bool m_doingBbrNow;              //!< If true, do Bbr for this RTT
  SequenceNumber32 m_begSndNxt;      //!< Right edge during last RTT
  std::map<uint32_t, Time> m_rttWindow; //window for computing min RTT
  std::map<uint32_t, int> m_bdwWindow; //Window for computing max BDW
  std::map<uint32_t,Time> m_timeWindow;
  std::map<SequenceNumber32,Time> receiveTimeWindow;
  SequenceNumber32 m_counter;
  uint32_t m_minRttInterval; //the length of the rtt window <= maxRttTime - minRttTime
  uint32_t m_maxBwdRttCount; // RTT count of the length of m_bdwWindow
  uint32_t m_rttCounter;
  Time m_totalRTT;
  double m_totalBdw;
  double m_avgRTTwindow;
  static int m_maxBwd;
  double m_avgBdwWindow;
  bool m_slowStart;
  bool m_firstBdw;
  uint32_t m_slowStartPacketscount; //count to exit slowstart if throughput is not changing
  double m_drainFactor;
  double m_probeFactor;
  double m_steadyFactor;
  Time m_currentRtt;
  Time m_lastTimeStamp;
};

} // namespace ns3

#endif /* TCP_Bbr_H */
