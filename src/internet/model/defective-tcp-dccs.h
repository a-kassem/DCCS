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

#ifndef DEF_TCP_DCCS_H
#define DEF_TCP_DCCS_H

#include "tcp-congestion-ops.h"
#include "tcp-linux-reno.h"

#include "ns3/traced-callback.h"

namespace ns3
{

    class DefTcpDccs : public TcpNewReno
    {
    public:
        static TypeId GetTypeId();

        DefTcpDccs();

        DefTcpDccs(const DefTcpDccs& sock);

        ~DefTcpDccs() override;

        // Documented in base class
        std::string GetName() const override;

        void Init(Ptr<TcpSocketState> tcb) override;

        typedef void (*CongestionEstimateTracedCallback)(uint32_t bytesAcked,
                                                         uint32_t bytesMarked,
                                                         double alpha);

        // Documented in base class
        uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
        Ptr<TcpCongestionOps> Fork() override;
        void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
        void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override; //NEW
        void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event) override;

    private:
        void CeState0to1(Ptr<TcpSocketState> tcb);

        void CeState1to0(Ptr<TcpSocketState> tcb);

        void UpdateAckReserved(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);

        void Reset(Ptr<TcpSocketState> tcb);

        void InitializeEcnAlpha(double alpha); //CHANGED
        void InitializeDelayAlpha(double alpha); //NEW
        void InitializeNQAvg(double nq); //NEW

        uint64_t m_capacity;
        uint32_t m_flow_num;
        uint32_t m_ackedBytesEcn; //CHANGED
        uint32_t m_ackedBytesDelayed; //NEW
        uint32_t m_ackedBytesTotal;
        uint64_t m_drain_cwnd; //NEW
        uint64_t m_drain_cycle; //NEW
        uint64_t m_count; //NEW
        SequenceNumber32 m_priorRcvNxt;
        bool m_priorRcvNxtFlag;
        double m_alpha_ecn; //CHANGED
        double m_alpha_d; //NEW
        double m_nq_w; //NEW
        double m_nq_avg; //NEW
        SequenceNumber32 m_nextSeq;
        bool m_nextSeqFlag;
        bool m_ceState;
        bool m_delayedAckReserved;
        double m_g;
        double m_gamma;
        uint32_t m_k;
        Time m_baseRtt;
        Time m_minRtt;
        uint32_t m_cntRtt;
        bool m_useEct0;
        bool m_initialized;
        bool m_drain_updated; //NEW
        TracedCallback<uint32_t, uint32_t, double> m_traceCongestionEstimate;
    };

} // namespace ns3

#endif /* DEF_TCP_DCCS_H */
