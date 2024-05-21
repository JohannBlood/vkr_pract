/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#ifndef WFQ_QUEUE_DISC_H
#define WFQ_QUEUE_DISC_H

#include "queue-disc.h"

#include <array>

namespace ns3
{

/// Priority map
typedef std::array<uint16_t, 16> WFQmap;

class WFQQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief WFQQueueDisc constructor
     */
    WFQQueueDisc();

    ~WFQQueueDisc() override;

    /**
     * Set the band (class) assigned to packets with specified priority.
     *
     * \param prio the priority of packets (a value between 0 and 15).
     * \param band the band assigned to packets.
     */
    void SetBandForWFQrity(uint8_t prio, uint16_t band);

    /**
     * Get the band (class) assigned to packets with specified priority.
     *
     * \param prio the priority of packets (a value between 0 and 15).
     * \returns the band assigned to packets.
     */
    uint16_t GetBandForWFQrity(uint8_t prio) const;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;

    WFQmap m_prio2band; //!< Priority to band mapping
};

/**
 * Serialize the priomap to the given ostream
 *
 * \param os
 * \param priomap
 *
 * \return std::ostream
 */
std::ostream& operator<<(std::ostream& os, const WFQmap& priomap);

/**
 * Serialize from the given istream to this priomap.
 *
 * \param is
 * \param priomap
 *
 * \return std::istream
 */
std::istream& operator>>(std::istream& is, WFQmap& priomap);

ATTRIBUTE_HELPER_HEADER(WFQmap);

} // namespace ns3

#endif /* WFQ_QUEUE_DISC_H */
