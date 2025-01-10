/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
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
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */
 
#ifndef TCPDCVEGAS_H
#define TCPDCVEGAS_H
 
#include "tcp-congestion-ops.h"
 
namespace ns3
{
 
class TcpSocketState;
 
class TcpDcVegas : public TcpNewReno
{
  public:
    static TypeId GetTypeId();
 
    TcpDcVegas();
 
    TcpDcVegas(const TcpDcVegas& sock);
    ~TcpDcVegas() override;
 
    std::string GetName() const override;
 
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
 
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;
 
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
 
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
 
    Ptr<TcpCongestionOps> Fork() override;
 
  protected:
  private:
    void EnableVegas(Ptr<TcpSocketState> tcb);
 
    void DisableVegas();
 
  private:
    uint32_t m_ackedBytesDelayed;
    uint32_t m_ackedBytesTotal;
    double m_alpha;             
    uint32_t m_k;              
    uint32_t m_gamma;
    double m_g;             
    Time m_baseRtt;               
    Time m_minRtt;                
    uint32_t m_cntRtt;            
    bool m_doingVegasNow;         
    SequenceNumber32 m_begSndNxt; 
};
 
} // namespace ns3
 
#endif // TCPDCVEGAS_H

