#ifndef BLACK_QUEUE_DISC_H
#define BLACK_QUEUE_DISC_H

#include "queue-disc.h"

#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

class TraceContainer;

/**
 * \ingroup traffic-control
 *
 * \brief A BLACK packet queue disc
 */
class BLACKQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief BLACKQueueDisc Constructor
     *
     * Create a BLACK queue disc
     */
    BLACKQueueDisc();

    /**
     * \brief Destructor
     *
     * Destructor
     */
    ~BLACKQueueDisc() override;

    /**
     * \brief Used in Feng's Adaptive BLACK
     */
    enum FengStatus
    {
        Above,   //!< When m_qAvg > m_maxTh
        Between, //!< When m_maxTh < m_qAvg < m_minTh
        Below,   //!< When m_qAvg < m_minTh
    };

    /**
     * \brief Drop types
     */
    enum
    {
        DTYPE_NONE,     //!< Ok, no drop
        DTYPE_FORCED,   //!< A "forced" drop
        DTYPE_UNFORCED, //!< An "unforced" (random) drop
    };

    /**
     * \brief Set the alpha value to adapt m_curMaxP.
     *
     * \param alpha The value of alpha to adapt m_curMaxP.
     */
    void SetABLACKAlpha(double alpha);

    /**
     * \brief Get the alpha value to adapt m_curMaxP.
     *
     * \returns The alpha value to adapt m_curMaxP.
     */
    double GetABLACKAlpha();

    /**
     * \brief Set the beta value to adapt m_curMaxP.
     *
     * \param beta The value of beta to adapt m_curMaxP.
     */
    void SetABLACKBeta(double beta);

    /**
     * \brief Get the beta value to adapt m_curMaxP.
     *
     * \returns The beta value to adapt m_curMaxP.
     */
    double GetABLACKBeta();

    /**
     * \brief Set the alpha value to adapt m_curMaxP in Feng's Adaptive BLACK.
     *
     * \param a The value of alpha to adapt m_curMaxP in Feng's Adaptive BLACK.
     */
    void SetFengAdaptiveA(double a);

    /**
     * \brief Get the alpha value to adapt m_curMaxP in Feng's Adaptive BLACK.
     *
     * \returns The alpha value to adapt m_curMaxP in Feng's Adaptive BLACK.
     */
    double GetFengAdaptiveA();

    /**
     * \brief Set the beta value to adapt m_curMaxP in Feng's Adaptive BLACK.
     *
     * \param b The value of beta to adapt m_curMaxP in Feng's Adaptive BLACK.
     */
    void SetFengAdaptiveB(double b);

    /**
     * \brief Get the beta value to adapt m_curMaxP in Feng's Adaptive BLACK.
     *
     * \returns The beta value to adapt m_curMaxP in Feng's Adaptive BLACK.
     */
    double GetFengAdaptiveB();

    /**
     * \brief Set the thresh limits of BLACK.
     *
     * \param minTh Minimum thresh in bytes or packets.
     * \param maxTh Maximum thresh in bytes or packets.
     */
    void SetTh(double minTh, double maxTh);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

    // Reasons for dropping packets
    static constexpr const char* UNFORCED_DROP = "Unforced drop"; //!< Early probability drops
    static constexpr const char* FORCED_DROP = "Forced drop"; //!< Forced drops, m_qAvg > m_maxTh
    // Reasons for marking packets
    static constexpr const char* UNFORCED_MARK = "Unforced mark"; //!< Early probability marks
    static constexpr const char* FORCED_MARK = "Forced mark"; //!< Forced marks, m_qAvg > m_maxTh

  protected:
    /**
     * \brief Dispose of the object
     */
    void DoDispose() override;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;

    /**
     * \brief Initialize the queue parameters.
     *
     * Note: if the link bandwidth changes in the course of the
     * simulation, the bandwidth-dependent BLACK parameters do not change.
     * This should be fixed, but it would require some extra parameters,
     * and didn't seem worth the trouble...
     */
    void InitializeParams() override;
    /**
     * \brief Compute the average queue size
     * \param nQueued number of queued packets
     * \param m simulated number of packets arrival during idle period
     * \param qAvg average queue size
     * \param qW queue weight given to cur q size sample
     * \returns new average queue size
     */
    double Estimator(uint32_t nQueued, uint32_t m, double qAvg, double qW);
    /**
     * \brief Update m_curMaxP
     * \param newAve new average queue length
     */
    void UpdateMaxP(double newAve);
    /**
     * \brief Update m_curMaxP based on Feng's Adaptive BLACK
     * \param newAve new average queue length
     */
    void UpdateMaxPFeng(double newAve);
    /**
     * \brief Check if a packet needs to be dropped due to probability mark
     * \param item queue item
     * \param qSize queue size
     * \returns 0 for no drop/mark, 1 for drop
     */
    uint32_t DropEarly(Ptr<QueueDiscItem> item, uint32_t qSize);
    /**
     * \brief Returns a probability using these function parameters for the DropEarly function
     * \returns Prob. of packet drop before "count"
     */
    double CalculatePNew();
    /**
     * \brief Returns a probability using these function parameters for the DropEarly function
     * \param p Prob. of packet drop before "count"
     * \param size packet size
     * \returns Prob. of packet drop
     */
    double ModifyP(double p, uint32_t size);

    // ** Variables supplied by user
    uint32_t m_meanPktSize; //!< Avg pkt size
    uint32_t m_idlePktSize; //!< Avg pkt size used during idle times
    bool m_isWait;          //!< True for waiting between dropped packets
    bool m_isGentle;        //!< True to increase dropping prob. slowly when m_qAvg exceeds m_maxTh
    bool m_isABLACK;          //!< True to enable Adaptive BLACK
    bool m_isAdaptMaxP;     //!< True to adapt m_curMaxP
    double m_minTh;         //!< Minimum threshold for m_qAvg (bytes or packets)
    double m_maxTh;   //!< Maximum threshold for m_qAvg (bytes or packets), should be >= 2 * m_minTh
    double m_qW;      //!< Queue weight given to cur queue size sample
    double m_lInterm; //!< The max probability of dropping a packet
    Time m_targetDelay;       //!< Target average queuing delay in ABLACK
    Time m_interval;          //!< Time interval to update m_curMaxP
    double m_top;             //!< Upper bound for m_curMaxP in ABLACK
    double m_bottom;          //!< Lower bound for m_curMaxP in ABLACK
    double m_alpha;           //!< Increment parameter for m_curMaxP in ABLACK
    double m_beta;            //!< Decrement parameter for m_curMaxP in ABLACK
    Time m_rtt;               //!< Rtt to be consideBLACK while automatically setting m_bottom in ABLACK
    bool m_isFengAdaptive;    //!< True to enable Feng's Adaptive BLACK
    bool m_isNonlinear;       //!< True to enable Nonlinear BLACK
    double m_b;               //!< Increment parameter for m_curMaxP in Feng's Adaptive BLACK
    double m_a;               //!< Decrement parameter for m_curMaxP in Feng's Adaptive BLACK
    bool m_isNs1Compat;       //!< Ns-1 compatibility
    DataRate m_linkBandwidth; //!< Link bandwidth
    Time m_linkDelay;         //!< Link delay
    bool m_useEcn;            //!< True if ECN is used (packets are marked instead of being dropped)
    bool m_useHardDrop;       //!< True if packets are always dropped above max threshold

    // ** Variables maintained by BLACK
    double m_vA;             //!< 1.0 / (m_maxTh - m_minTh)
    double m_vB;             //!< -m_minTh / (m_maxTh - m_minTh)
    double m_vC;             //!< (1.0 - m_curMaxP) / m_maxTh - used in "gentle" mode
    double m_vD;             //!< 2.0 * m_curMaxP - 1.0 - used in "gentle" mode
    double m_curMaxP;        //!< Current max_p
    Time m_lastSet;          //!< Last time m_curMaxP was updated
    double m_vProb;          //!< Prob. of packet drop
    uint32_t m_countBytes;   //!< Number of bytes since last drop
    uint32_t m_old;          //!< 0 when average queue first exceeds threshold
    uint32_t m_idle;         //!< 0/1 idle status
    double m_ptc;            //!< packet time constant in packets/second
    double m_qAvg;           //!< Average queue length
    uint32_t m_count;        //!< Number of packets since last random number generation
    FengStatus m_fengStatus; //!< For use in Feng's Adaptive BLACK
    uint32_t m_cautious;
    Time m_idleTime; //!< Start of current idle period

    Ptr<UniformRandomVariable> m_uv; //!< rng stream
};

}; // namespace ns3

#endif // BLACK_QUEUE_DISC_H
