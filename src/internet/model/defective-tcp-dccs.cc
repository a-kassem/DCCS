/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Author: Shravya K.S. <shravya.ks0@gmail.com>
 *
 */

#include "defective-tcp-dccs.h"

#include "tcp-socket-state.h"

#include "ns3/abort.h"
#include "ns3/log.h"

namespace ns3
{

    NS_LOG_COMPONENT_DEFINE("DefTcpDccs");

    NS_OBJECT_ENSURE_REGISTERED(DefTcpDccs);

    TypeId
    DefTcpDccs::GetTypeId()
    {
        static TypeId tid =
                TypeId("ns3::DefTcpDccs")
                        .SetParent<TcpNewReno>()
                        .AddConstructor<DefTcpDccs>()
                        .SetGroupName("Internet")
                        .AddAttribute("Capacity",
                                      "capacity of bottleneck",
                                      UintegerValue(1250000000),
                                      MakeUintegerAccessor(&DefTcpDccs::m_capacity),
                                      MakeUintegerChecker<uint64_t>())
                        .AddAttribute("NumFlows",
                                      "number of flows in bottleneck",
                                      UintegerValue(1),
                                      MakeUintegerAccessor(&DefTcpDccs::m_flow_num),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("DccsShiftG",
                                      "Parameter G for updating alpha",
                                      DoubleValue(0.0625),
                                      MakeDoubleAccessor(&DefTcpDccs::m_g),
                                      MakeDoubleChecker<double>(0, 1))
                        .AddAttribute("EcnAlphaOnInit",
                                      "Initial alpha_ecn",
                                      DoubleValue(0),
                                      MakeDoubleAccessor(&DefTcpDccs::InitializeEcnAlpha),
                                      MakeDoubleChecker<double>(0, 1))
                        .AddAttribute("DelayAlphaOnInit",
                                      "Initial alpha_d",
                                      DoubleValue(0),
                                      MakeDoubleAccessor(&DefTcpDccs::InitializeDelayAlpha),
                                      MakeDoubleChecker<double>(0, 1))
                        .AddAttribute("NQAvgOnInit",
                                      "Initial m_nq_avg",
                                      DoubleValue(0),
                                      MakeUintegerAccessor(&DefTcpDccs::InitializeNQAvg),
                                      MakeDoubleChecker<double>(0, 100000))
                        .AddAttribute("Kdcv",
                                      "Upper bound of packets in network",
                                      UintegerValue(16),
                                      MakeUintegerAccessor(&DefTcpDccs::m_k),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("Gamma",
                                      "Limit on increase",
                                      UintegerValue(1),
                                      MakeUintegerAccessor(&DefTcpDccs::m_gamma),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("DcVegasShiftG",
                                      "Parameter G for updating dcvegas_alpha",
                                      DoubleValue(0.0625),
                                      MakeDoubleAccessor(&DefTcpDccs::m_g),
                                      MakeDoubleChecker<double>(0, 1))
                        .AddAttribute("NQW",
                                      "Parameter w for updating nq_avg",
                                      DoubleValue(0.0625),
                                      MakeDoubleAccessor(&DefTcpDccs::m_nq_w),
                                      MakeDoubleChecker<double>(0, 1))
                        .AddAttribute("UseEct0",
                                      "Use ECT(0) for ECN codepoint, if false use ECT(1)",
                                      BooleanValue(true),
                                      MakeBooleanAccessor(&DefTcpDccs::m_useEct0),
                                      MakeBooleanChecker())
                        .AddTraceSource("CongestionEstimate",
                                        "Update sender-side congestion estimate state",
                                        MakeTraceSourceAccessor(&DefTcpDccs::m_traceCongestionEstimate),
                                        "ns3::DefTcpDccs::CongestionEstimateTracedCallback");
        return tid;
    }

    std::string
    DefTcpDccs::GetName() const
    {
        return "DefTcpDccs";
    }

    DefTcpDccs::DefTcpDccs()
            : TcpNewReno(),
              m_ackedBytesEcn(0),
              m_ackedBytesDelayed(0),
              m_ackedBytesTotal(0),
              m_drain_cwnd(0),
              m_drain_cycle(0),
              m_count(0),
              m_priorRcvNxt(SequenceNumber32(0)),
              m_priorRcvNxtFlag(false),
              m_nextSeq(SequenceNumber32(0)),
              m_nextSeqFlag(false),
              m_ceState(false),
              m_delayedAckReserved(false),
              m_baseRtt(Time::Max()),
              m_minRtt(Time::Max()),
              m_cntRtt(0),
              m_initialized(false),
              m_drain_updated(false)
    {
        NS_LOG_FUNCTION(this);
    }

    DefTcpDccs::DefTcpDccs(const DefTcpDccs& sock)
            : TcpNewReno(sock),
              m_capacity(sock.m_capacity),
              m_flow_num(sock.m_flow_num),
              m_ackedBytesEcn(sock.m_ackedBytesEcn),
              m_ackedBytesDelayed(sock.m_ackedBytesDelayed),
              m_ackedBytesTotal(sock.m_ackedBytesTotal),
              m_drain_cwnd(sock.m_drain_cwnd),
              m_drain_cycle(sock.m_drain_cycle),
              m_count(sock.m_count),
              m_priorRcvNxt(sock.m_priorRcvNxt),
              m_priorRcvNxtFlag(sock.m_priorRcvNxtFlag),
              m_alpha_ecn(sock.m_alpha_ecn),
              m_alpha_d(sock.m_alpha_d),
              m_nq_w(sock.m_nq_w),
              m_nq_avg(sock.m_nq_avg),
              m_nextSeq(sock.m_nextSeq),
              m_nextSeqFlag(sock.m_nextSeqFlag),
              m_ceState(sock.m_ceState),
              m_delayedAckReserved(sock.m_delayedAckReserved),
              m_g(sock.m_g),
              m_gamma(sock.m_gamma),
              m_k(sock.m_k),
              m_baseRtt(sock.m_baseRtt),
              m_minRtt(sock.m_minRtt),
              m_cntRtt(sock.m_cntRtt),
              m_useEct0(sock.m_useEct0),
              m_initialized(sock.m_initialized),
              m_drain_updated(sock.m_drain_updated)
    {
        NS_LOG_FUNCTION(this);
    }

    DefTcpDccs::~DefTcpDccs()
    {
        NS_LOG_FUNCTION(this);
    }

    Ptr<TcpCongestionOps>
    DefTcpDccs::Fork()
    {
        NS_LOG_FUNCTION(this);
        return CopyObject<DefTcpDccs>(this);
    }

    void
    DefTcpDccs::Init(Ptr<TcpSocketState> tcb)
    {
        NS_LOG_FUNCTION(this << tcb);
        NS_LOG_INFO(this << "Enabling DctcpEcn for DCCS");
        tcb->m_useEcn = TcpSocketState::On;
        tcb->m_ecnMode = TcpSocketState::DctcpEcn;
        tcb->m_ectCodePoint = m_useEct0 ? TcpSocketState::Ect0 : TcpSocketState::Ect1;
        m_initialized = true;
    }

// Step 9, Section 3.3 of RFC 8257.  GetSsThresh() is called upon
// entering the CWR state, and then later, when CWR is exited,
// cwnd is set to ssthresh (this value).  bytesInFlight is ignored.
    uint32_t
    DefTcpDccs::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) //TODO
    {
        NS_LOG_FUNCTION(this << tcb << bytesInFlight);
        if(m_drain_updated && m_count == m_drain_cycle)
        {
            std::cout << "Current CWND: " << tcb->m_cWnd << "       Drain CWND: " << m_drain_cwnd << std::endl;
            return static_cast<uint32_t>(std::min(static_cast<uint32_t>(m_drain_cwnd), static_cast<uint32_t>(tcb->m_cWnd)));
        }
        if (m_nq_avg < 1.5 * m_k)
            return static_cast<uint32_t>((1 - m_alpha_ecn / 2.0) * tcb->m_cWnd);
        else
            return static_cast<uint32_t>((1 - (m_alpha_ecn + m_alpha_d) / 2.0) * tcb->m_cWnd);
    }

    void
    DefTcpDccs::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) //TODO
    {
        NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);
        m_ackedBytesTotal += segmentsAcked * tcb->m_segmentSize;
        if (tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD)
        {
            m_ackedBytesEcn += segmentsAcked * tcb->m_segmentSize;
        }
        if (!m_nextSeqFlag)
        {
            m_nextSeq = tcb->m_nextTxSequence;
            m_nextSeqFlag = true;
        }
        if(rtt.GetMicroSeconds() > 30)
          m_minRtt = std::min(m_minRtt, rtt);
        NS_LOG_DEBUG("Updated m_minRtt = " << m_minRtt);

        if(rtt.GetMicroSeconds() > 30)
          m_baseRtt = std::min(m_baseRtt, rtt);
        NS_LOG_DEBUG("Updated m_baseRtt = " << m_baseRtt);

        double tmp = tcb->GetCwndInSegments() / rtt.GetSeconds();
        uint32_t targetCwnd = static_cast<uint32_t>((rtt.GetSeconds() - m_baseRtt.GetSeconds()) * tmp);
        if(targetCwnd > m_k)
            m_ackedBytesDelayed += segmentsAcked * tcb->m_segmentSize;
        m_cntRtt++;
        m_nq_avg = m_nq_avg + m_nq_w * (targetCwnd - m_nq_avg);
        NS_LOG_DEBUG("Updated m_cntRtt = " << m_cntRtt);
        if (tcb->m_lastAckedSeq >= m_nextSeq)
        {
            double bytesEcn = 0.0; // Corresponds to variable M in RFC 8257
            if (m_ackedBytesTotal > 0)
            {
                bytesEcn = static_cast<double>(m_ackedBytesEcn * 1.0 / m_ackedBytesTotal);
            }
            m_alpha_ecn = (1.0 - m_g) * m_alpha_ecn + m_g * bytesEcn;
            m_traceCongestionEstimate(m_ackedBytesEcn, m_ackedBytesTotal, m_alpha_ecn);
            NS_LOG_INFO(this << "bytesEcn " << bytesEcn << ", m_alpha " << m_alpha_ecn);
            if(!m_drain_updated && m_count == 0)
            {
                m_drain_cwnd = std::floor(0.6 * m_baseRtt.GetSeconds() * m_capacity / m_flow_num);
                m_drain_cycle = std::floor(0.6 * m_baseRtt.GetSeconds() * m_capacity / m_flow_num + (m_k - 10) * tcb->m_segmentSize);
            }
            if(!m_drain_updated && m_count == m_drain_cycle - 1)
            {
                m_drain_updated = true;
                m_drain_cwnd = std::floor(0.6 * m_baseRtt.GetSeconds() * m_capacity / m_flow_num);
                //std::cout << "DRAIN CWND : " << m_drain_cwnd << " = 0.6 * " << m_baseRtt.GetMicroSeconds() << " * " << m_capacity << std::endl;
                m_drain_cycle = std::floor(0.6 * m_baseRtt.GetSeconds() * m_capacity / m_flow_num + (m_k - 10) * tcb->m_segmentSize);
                //std::cout << "DRAIN CYCLE : " << m_drain_cycle << std::endl;
                m_count = m_drain_cycle - 1;
            }
            if(m_count == m_drain_cycle)
                m_count = 0;
            m_count += 1;
        }
    }

    void
    DefTcpDccs::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) //TODO
    {
        NS_LOG_FUNCTION(this << tcb << segmentsAcked);

        if (tcb->m_lastAckedSeq >= m_nextSeq)
        { // A cycle has finished, we do Vegas cwnd adjustment every RTT.

            // Save the current right edge for next Vegas cycle
            m_nextSeq = tcb->m_nextTxSequence;

            if(m_drain_updated && m_count == m_drain_cycle)
            {
                Reset(tcb);
                std::cout << "Current CWND: " << tcb->m_cWnd << "       Drain CWND: " << m_drain_cwnd << std::endl;
                tcb->m_cWnd = static_cast<uint32_t>(std::min(static_cast<uint32_t>(m_drain_cwnd), static_cast<uint32_t>(tcb->m_cWnd)));
                return;
            }

            if(m_ackedBytesEcn > 0)
            {
                Reset(tcb);
                return;
            }

            NS_LOG_LOGIC("A Vegas cycle has finished, we adjust cwnd once per RTT.");

            /*
             * We perform Vegas calculations only if we got enough RTT samples to
             * insure that at least 1 of those samples wasn't from a delayed ACK.
             */
            if (m_cntRtt <= 2)
            { // We do not have enough RTT samples, so we should behave like Reno
                NS_LOG_LOGIC(
                        "We do not have enough RTT samples to do Vegas, so we behave like NewReno.");
                TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
            }
            else
            {
                NS_LOG_LOGIC("We have enough RTT samples to perform Vegas calculations");
                /*
                 * We have enough RTT samples to perform Vegas algorithm.
                 * Now we need to determine if cwnd should be increased or decreased
                 * based on the calculated difference between the expected rate and actual sending
                 * rate and the predefined thresholds (alpha, beta, and gamma).
                 */
                uint32_t diff;
                uint32_t targetCwnd;
                uint32_t segCwnd = tcb->GetCwndInSegments();

                /*
                 * Calculate the cwnd we should have. baseRtt is the minimum RTT
                 * per-connection, minRtt is the minimum RTT in this window
                 *
                 * little trick:
                 * desidered throughput is currentCwnd * baseRtt
                 * target cwnd is throughput / minRtt
                 */
                double tmp = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();
                targetCwnd = static_cast<uint32_t>(segCwnd * tmp);
                NS_LOG_DEBUG("Calculated targetCwnd = " << targetCwnd);
                NS_ASSERT(segCwnd >= targetCwnd); // implies baseRtt <= minRtt

                /*
                 * Calculate the difference between the expected cWnd and
                 * the actual cWnd
                 */
                diff = segCwnd - targetCwnd;
                NS_LOG_DEBUG("Calculated diff = " << diff);

                if (diff > m_gamma && (tcb->m_cWnd < tcb->m_ssThresh))
                {
                    /*
                     * We are going too fast. We need to slow down and change from
                     * slow-start to linear increase/decrease mode by setting cwnd
                     * to target cwnd. We add 1 because of the integer truncation.
                     */
                    NS_LOG_LOGIC("We are going too fast. We need to slow down and "
                                 "change to linear increase/decrease mode.");
                    segCwnd = std::min(segCwnd, targetCwnd + 1);
                    tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
                    tcb->m_ssThresh = GetSsThresh(tcb, 0);
                    NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd << " ssthresh=" << tcb->m_ssThresh);
                }
                else if (tcb->m_cWnd < tcb->m_ssThresh)
                { // Slow start mode
                    NS_LOG_LOGIC("We are in slow start and diff < m_gamma, so we "
                                 "follow NewReno slow start");
                    TcpNewReno::SlowStart(tcb, segmentsAcked);
                }
                else
                { // Linear increase/decrease mode
                    NS_LOG_LOGIC("We are in linear increase/decrease mode");
                    if (m_ackedBytesDelayed > 0)
                    {
                        // We are going too fast, so we slow down
                        NS_LOG_LOGIC("We are going too fast, so we slow down by decreasing cwnd");
                        double FractionDelayed = static_cast<double>(m_ackedBytesDelayed * 1.0 / m_ackedBytesTotal);
                        m_alpha_d = (1.0 - m_g) * m_alpha_d + m_g * FractionDelayed;
                        tcb->m_cWnd = static_cast<uint32_t>((tcb->m_cWnd * (1 - m_alpha_d/2)));
                        tcb->m_ssThresh = std::max(std::min(tcb->m_ssThresh.Get(), tcb->m_cWnd.Get() - tcb->m_segmentSize),
                                                   2 * tcb->m_segmentSize);
                        NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd
                                                       << " ssthresh=" << tcb->m_ssThresh);
                    }
                    else
                    {
                        // We are going too slow (having too little data in the network),
                        // so we speed up.
                        NS_LOG_LOGIC("We are going too slow, so we speed up by incrementing cwnd");
                        segCwnd++;
                        tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
                        NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd
                                                       << " ssthresh=" << tcb->m_ssThresh);
                    }
                }
                tcb->m_ssThresh = std::max(tcb->m_ssThresh, 3 * tcb->m_cWnd / 4);
                NS_LOG_DEBUG("Updated ssThresh = " << tcb->m_ssThresh);
            }

            Reset(tcb);
        }
        else if (m_drain_cwnd < 10 * tcb->m_segmentSize && tcb->m_cWnd < 10 * tcb->m_segmentSize)
        {
                TcpNewReno::SlowStart(tcb, segmentsAcked);
        }
        else if (m_drain_cwnd > 10 * tcb->m_segmentSize && tcb->m_cWnd < tcb->m_ssThresh)
            TcpNewReno::SlowStart(tcb, segmentsAcked);
    }

    void
    DefTcpDccs::InitializeEcnAlpha(double alpha)
    {
        NS_LOG_FUNCTION(this << alpha);
        NS_ABORT_MSG_IF(m_initialized, "DCCS has already been initialized");
        m_alpha_ecn = alpha;
    }

    void
    DefTcpDccs::InitializeDelayAlpha(double alpha)
    {
        NS_LOG_FUNCTION(this << alpha);
        NS_ABORT_MSG_IF(m_initialized, "DCCS has already been initialized");
        m_alpha_d = alpha;
    }

    void
    DefTcpDccs::InitializeNQAvg(double nq)
    {
        NS_LOG_FUNCTION(this << nq);
        NS_ABORT_MSG_IF(m_initialized, "DCCS has already been initialized");
        m_nq_avg = nq;
    }

    void
    DefTcpDccs::Reset(Ptr<TcpSocketState> tcb)
    {
        NS_LOG_FUNCTION(this << tcb);
        m_nextSeq = tcb->m_nextTxSequence;
        m_ackedBytesEcn = 0;
        m_ackedBytesDelayed = 0;
        m_ackedBytesTotal = 0;
        m_cntRtt = 0;
        m_minRtt = Time::Max();
    }

    void
    DefTcpDccs::CeState0to1(Ptr<TcpSocketState> tcb)
    {
        NS_LOG_FUNCTION(this << tcb);
        if (!m_ceState && m_delayedAckReserved && m_priorRcvNxtFlag)
        {
            SequenceNumber32 tmpRcvNxt;
            /* Save current NextRxSequence. */
            tmpRcvNxt = tcb->m_rxBuffer->NextRxSequence();

            /* Generate previous ACK without ECE */
            tcb->m_rxBuffer->SetNextRxSequence(m_priorRcvNxt);
            tcb->m_sendEmptyPacketCallback(TcpHeader::ACK);

            /* Recover current RcvNxt. */
            tcb->m_rxBuffer->SetNextRxSequence(tmpRcvNxt);
        }

        if (!m_priorRcvNxtFlag)
        {
            m_priorRcvNxtFlag = true;
        }
        m_priorRcvNxt = tcb->m_rxBuffer->NextRxSequence();
        m_ceState = true;
        tcb->m_ecnState = TcpSocketState::ECN_CE_RCVD;
    }

    void
    DefTcpDccs::CeState1to0(Ptr<TcpSocketState> tcb)
    {
        NS_LOG_FUNCTION(this << tcb);
        if (m_ceState && m_delayedAckReserved && m_priorRcvNxtFlag)
        {
            SequenceNumber32 tmpRcvNxt;
            /* Save current NextRxSequence. */
            tmpRcvNxt = tcb->m_rxBuffer->NextRxSequence();

            /* Generate previous ACK with ECE */
            tcb->m_rxBuffer->SetNextRxSequence(m_priorRcvNxt);
            tcb->m_sendEmptyPacketCallback(TcpHeader::ACK | TcpHeader::ECE);

            /* Recover current RcvNxt. */
            tcb->m_rxBuffer->SetNextRxSequence(tmpRcvNxt);
        }

        if (!m_priorRcvNxtFlag)
        {
            m_priorRcvNxtFlag = true;
        }
        m_priorRcvNxt = tcb->m_rxBuffer->NextRxSequence();
        m_ceState = false;

        if (tcb->m_ecnState.Get() == TcpSocketState::ECN_CE_RCVD ||
            tcb->m_ecnState.Get() == TcpSocketState::ECN_SENDING_ECE)
        {
            tcb->m_ecnState = TcpSocketState::ECN_IDLE;
        }
    }

    void
    DefTcpDccs::UpdateAckReserved(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
    {
        NS_LOG_FUNCTION(this << tcb << event);
        switch (event)
        {
            case TcpSocketState::CA_EVENT_DELAYED_ACK:
                if (!m_delayedAckReserved)
                {
                    m_delayedAckReserved = true;
                }
                break;
            case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
                if (m_delayedAckReserved)
                {
                    m_delayedAckReserved = false;
                }
                break;
            default:
                /* Don't care for the rest. */
                break;
        }
    }

    void
    DefTcpDccs::CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event)
    {
        NS_LOG_FUNCTION(this << tcb << event);
        switch (event)
        {
            case TcpSocketState::CA_EVENT_ECN_IS_CE:
                CeState0to1(tcb);
                break;
            case TcpSocketState::CA_EVENT_ECN_NO_CE:
                CeState1to0(tcb);
                break;
            case TcpSocketState::CA_EVENT_DELAYED_ACK:
            case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
                UpdateAckReserved(tcb, event);
                break;
            default:
                /* Don't care for the rest. */
                break;
        }
    }

} // namespace ns3

