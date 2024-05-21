#include "black-queue-disc.h"

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BLACKQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(BLACKQueueDisc);

TypeId
BLACKQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BLACKQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<BLACKQueueDisc>()
            .AddAttribute("MeanPktSize",
                          "Average of packet size",
                          UintegerValue(500),
                          MakeUintegerAccessor(&BLACKQueueDisc::m_meanPktSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("IdlePktSize",
                          "Average packet size used during idle times. Used when m_cautions = 3",
                          UintegerValue(0),
                          MakeUintegerAccessor(&BLACKQueueDisc::m_idlePktSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("Wait",
                          "True for waiting between dropped packets",
                          BooleanValue(true),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isWait),
                          MakeBooleanChecker())
            .AddAttribute("Gentle",
                          "True to increases dropping probability slowly when average queue "
                          "exceeds maxthresh",
                          BooleanValue(true),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isGentle),
                          MakeBooleanChecker())
            .AddAttribute("ABLACK",
                          "True to enable ABLACK",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isABLACK),
                          MakeBooleanChecker())
            .AddAttribute("AdaptMaxP",
                          "True to adapt m_curMaxP",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isAdaptMaxP),
                          MakeBooleanChecker())
            .AddAttribute("FengAdaptive",
                          "True to enable Feng's Adaptive BLACK",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isFengAdaptive),
                          MakeBooleanChecker())
            .AddAttribute("NLBLACK",
                          "True to enable Nonlinear BLACK",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isNonlinear),
                          MakeBooleanChecker())
            .AddAttribute("MinTh",
                          "Minimum average length threshold in packets/bytes",
                          DoubleValue(5),
                          MakeDoubleAccessor(&BLACKQueueDisc::m_minTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxTh",
                          "Maximum average length threshold in packets/bytes",
                          DoubleValue(15),
                          MakeDoubleAccessor(&BLACKQueueDisc::m_maxTh),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxSize",
                          "The maximum number of packets accepted by this queue disc",
                          QueueSizeValue(QueueSize("25p")),
                          MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                          MakeQueueSizeChecker())
            .AddAttribute("QW",
                          "Queue weight related to the exponential weighted moving average (EWMA)",
                          DoubleValue(0.002),
                          MakeDoubleAccessor(&BLACKQueueDisc::m_qW),
                          MakeDoubleChecker<double>())
            .AddAttribute("LInterm",
                          "The maximum probability of dropping a packet",
                          DoubleValue(50),
                          MakeDoubleAccessor(&BLACKQueueDisc::m_lInterm),
                          MakeDoubleChecker<double>())
            .AddAttribute("TargetDelay",
                          "Target average queuing delay in ABLACK",
                          TimeValue(Seconds(0.005)),
                          MakeTimeAccessor(&BLACKQueueDisc::m_targetDelay),
                          MakeTimeChecker())
            .AddAttribute("Interval",
                          "Time interval to update m_curMaxP",
                          TimeValue(Seconds(0.5)),
                          MakeTimeAccessor(&BLACKQueueDisc::m_interval),
                          MakeTimeChecker())
            .AddAttribute("Top",
                          "Upper bound for m_curMaxP in ABLACK",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&BLACKQueueDisc::m_top),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("Bottom",
                          "Lower bound for m_curMaxP in ABLACK",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&BLACKQueueDisc::m_bottom),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("Alpha",
                          "Increment parameter for m_curMaxP in ABLACK",
                          DoubleValue(0.01),
                          MakeDoubleAccessor(&BLACKQueueDisc::SetABLACKAlpha),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("Beta",
                          "Decrement parameter for m_curMaxP in ABLACK",
                          DoubleValue(0.9),
                          MakeDoubleAccessor(&BLACKQueueDisc::SetABLACKBeta),
                          MakeDoubleChecker<double>(0, 1))
            .AddAttribute("FengAlpha",
                          "Decrement parameter for m_curMaxP in Feng's Adaptive BLACK",
                          DoubleValue(3.0),
                          MakeDoubleAccessor(&BLACKQueueDisc::SetFengAdaptiveA),
                          MakeDoubleChecker<double>())
            .AddAttribute("FengBeta",
                          "Increment parameter for m_curMaxP in Feng's Adaptive BLACK",
                          DoubleValue(2.0),
                          MakeDoubleAccessor(&BLACKQueueDisc::SetFengAdaptiveB),
                          MakeDoubleChecker<double>())
            .AddAttribute("LastSet",
                          "Store the last time m_curMaxP was updated",
                          TimeValue(Seconds(0.0)),
                          MakeTimeAccessor(&BLACKQueueDisc::m_lastSet),
                          MakeTimeChecker())
            .AddAttribute("Rtt",
                          "Round Trip Time to be consideBLACK while automatically setting m_bottom",
                          TimeValue(Seconds(0.1)),
                          MakeTimeAccessor(&BLACKQueueDisc::m_rtt),
                          MakeTimeChecker())
            .AddAttribute("Ns1Compat",
                          "NS-1 compatibility",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_isNs1Compat),
                          MakeBooleanChecker())
            .AddAttribute("LinkBandwidth",
                          "The BLACK link bandwidth",
                          DataRateValue(DataRate("1.5Mbps")),
                          MakeDataRateAccessor(&BLACKQueueDisc::m_linkBandwidth),
                          MakeDataRateChecker())
            .AddAttribute("LinkDelay",
                          "The BLACK link delay",
                          TimeValue(MilliSeconds(20)),
                          MakeTimeAccessor(&BLACKQueueDisc::m_linkDelay),
                          MakeTimeChecker())
            .AddAttribute("UseEcn",
                          "True to use ECN (packets are marked instead of being dropped)",
                          BooleanValue(false),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_useEcn),
                          MakeBooleanChecker())
            .AddAttribute("UseHardDrop",
                          "True to always drop packets above max threshold",
                          BooleanValue(true),
                          MakeBooleanAccessor(&BLACKQueueDisc::m_useHardDrop),
                          MakeBooleanChecker());

    return tid;
}

BLACKQueueDisc::BLACKQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
    NS_LOG_FUNCTION(this);
    m_uv = CreateObject<UniformRandomVariable>();
}

BLACKQueueDisc::~BLACKQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
BLACKQueueDisc::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_uv = nullptr;
    QueueDisc::DoDispose();
}

void
BLACKQueueDisc::SetABLACKAlpha(double alpha)
{
    NS_LOG_FUNCTION(this << alpha);
    m_alpha = alpha;

    if (m_alpha > 0.01)
    {
        NS_LOG_WARN("Alpha value is above the recommended bound!");
    }
}

double
BLACKQueueDisc::GetABLACKAlpha()
{
    NS_LOG_FUNCTION(this);
    return m_alpha;
}

void
BLACKQueueDisc::SetABLACKBeta(double beta)
{
    NS_LOG_FUNCTION(this << beta);
    m_beta = beta;

    if (m_beta < 0.83)
    {
        NS_LOG_WARN("Beta value is below the recommended bound!");
    }
}

double
BLACKQueueDisc::GetABLACKBeta()
{
    NS_LOG_FUNCTION(this);
    return m_beta;
}

void
BLACKQueueDisc::SetFengAdaptiveA(double a)
{
    NS_LOG_FUNCTION(this << a);
    m_a = a;

    if (m_a != 3)
    {
        NS_LOG_WARN("Alpha value does not follow the recommendations!");
    }
}

double
BLACKQueueDisc::GetFengAdaptiveA()
{
    NS_LOG_FUNCTION(this);
    return m_a;
}

void
BLACKQueueDisc::SetFengAdaptiveB(double b)
{
    NS_LOG_FUNCTION(this << b);
    m_b = b;

    if (m_b != 2)
    {
        NS_LOG_WARN("Beta value does not follow the recommendations!");
    }
}

double
BLACKQueueDisc::GetFengAdaptiveB()
{
    NS_LOG_FUNCTION(this);
    return m_b;
}

void
BLACKQueueDisc::SetTh(double minTh, double maxTh)
{
    NS_LOG_FUNCTION(this << minTh << maxTh);
    NS_ASSERT(minTh <= maxTh);
    m_minTh = minTh;
    m_maxTh = maxTh;
}

int64_t
BLACKQueueDisc::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

bool
BLACKQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    uint32_t nQueued = GetInternalQueue(0)->GetCurrentSize().GetValue();

    // simulate number of packets arrival during idle period
    uint32_t m = 0;

    if (m_idle == 1)
    {
        NS_LOG_DEBUG("BLACK Queue Disc is idle.");
        Time now = Simulator::Now();

        if (m_cautious == 3)
        {
            double ptc = m_ptc * m_meanPktSize / m_idlePktSize;
            m = uint32_t(ptc * (now - m_idleTime).GetSeconds());
        }
        else
        {
            m = uint32_t(m_ptc * (now - m_idleTime).GetSeconds());
        }

        m_idle = 0;
    }

    m_qAvg = Estimator(nQueued, m + 1, m_qAvg, m_qW);

    NS_LOG_DEBUG("\t bytesInQueue  " << GetInternalQueue(0)->GetNBytes() << "\tQavg " << m_qAvg);
    NS_LOG_DEBUG("\t packetsInQueue  " << GetInternalQueue(0)->GetNPackets() << "\tQavg "
                                       << m_qAvg);

    m_count++;
    m_countBytes += item->GetSize();

    uint32_t dropType = DTYPE_NONE;
    if (m_qAvg >= m_minTh && nQueued > 1)
    {
        if ((!m_isGentle && m_qAvg >= m_maxTh) || (m_isGentle && m_qAvg >= 2 * m_maxTh))
        {
            NS_LOG_DEBUG("adding DROP FORCED MARK");
            dropType = DTYPE_FORCED;
        }
        else if (m_old == 0)
        {
            /*
             * The average queue size has just crossed the
             * threshold from below to above m_minTh, or
             * from above m_minTh with an empty queue to
             * above m_minTh with a nonempty queue.
             */
            m_count = 1;
            m_countBytes = item->GetSize();
            m_old = 1;
        }
        else if (DropEarly(item, nQueued))
        {
            NS_LOG_LOGIC("DropEarly returns 1");
            dropType = DTYPE_UNFORCED;
        }
    }
    else
    {
        // No packets are being dropped
        m_vProb = 0.0;
        m_old = 0;
    }

    if (dropType == DTYPE_UNFORCED)
    {
        if (!m_useEcn || !Mark(item, UNFORCED_MARK))
        {
            NS_LOG_DEBUG("\t Dropping due to Prob Mark " << m_qAvg);
            DropBeforeEnqueue(item, UNFORCED_DROP);
            return false;
        }
        NS_LOG_DEBUG("\t Marking due to Prob Mark " << m_qAvg);
    }
    else if (dropType == DTYPE_FORCED)
    {
        if (m_useHardDrop || !m_useEcn || !Mark(item, FORCED_MARK))
        {
            NS_LOG_DEBUG("\t Dropping due to Hard Mark " << m_qAvg);
            DropBeforeEnqueue(item, FORCED_DROP);
            if (m_isNs1Compat)
            {
                m_count = 0;
                m_countBytes = 0;
            }
            return false;
        }
        NS_LOG_DEBUG("\t Marking due to Hard Mark " << m_qAvg);
    }

    bool retval = GetInternalQueue(0)->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
    // internal queue because QueueDisc::AddInternalQueue sets the trace callback

    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return retval;
}

/*
 * Note: if the link bandwidth changes in the course of the
 * simulation, the bandwidth-dependent BLACK parameters do not change.
 * This should be fixed, but it would require some extra parameters,
 * and didn't seem worth the trouble...
 */
void
BLACKQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Initializing BLACK params.");

    m_cautious = 0;
    m_ptc = m_linkBandwidth.GetBitRate() / (8.0 * m_meanPktSize);

    if (m_isABLACK)
    {
        // Set m_minTh, m_maxTh and m_qW to zero for automatic setting
        m_minTh = 0;
        m_maxTh = 0;
        m_qW = 0;

        // Turn on m_isAdaptMaxP to adapt m_curMaxP
        m_isAdaptMaxP = true;
    }

    if (m_isFengAdaptive)
    {
        // Initialize m_fengStatus
        m_fengStatus = Above;
    }

    if (m_minTh == 0 && m_maxTh == 0)
    {
        m_minTh = 5.0;

        // set m_minTh to max(m_minTh, targetqueue/2.0) [Ref:
        // http://www.icir.org/floyd/papers/adaptiveBLACK.pdf]
        double targetqueue = m_targetDelay.GetSeconds() * m_ptc;

        if (m_minTh < targetqueue / 2.0)
        {
            m_minTh = targetqueue / 2.0;
        }
        if (GetMaxSize().GetUnit() == QueueSizeUnit::BYTES)
        {
            m_minTh = m_minTh * m_meanPktSize;
        }

        // set m_maxTh to three times m_minTh [Ref:
        // http://www.icir.org/floyd/papers/adaptiveBLACK.pdf]
        m_maxTh = 3 * m_minTh;
    }

    NS_ASSERT(m_minTh <= m_maxTh);

    m_qAvg = 0.0;
    m_count = 0;
    m_countBytes = 0;
    m_old = 0;
    m_idle = 1;

    double th_diff = (m_maxTh - m_minTh);
    if (th_diff == 0)
    {
        th_diff = 1.0;
    }
    m_vA = 1.0 / th_diff;
    m_curMaxP = 1.0 / m_lInterm;
    m_vB = -m_minTh / th_diff;

    if (m_isGentle)
    {
        m_vC = (1.0 - m_curMaxP) / m_maxTh;
        m_vD = 2.0 * m_curMaxP - 1.0;
    }
    m_idleTime = NanoSeconds(0);

    /*
     * If m_qW=0, set it to a reasonable value of 1-exp(-1/C)
     * This corresponds to choosing m_qW to be of that value for
     * which the packet time constant -1/ln(1-m)qW) per default RTT
     * of 100ms is an order of magnitude more than the link capacity, C.
     *
     * If m_qW=-1, then the queue weight is set to be a function of
     * the bandwidth and the link propagation delay.  In particular,
     * the default RTT is assumed to be three times the link delay and
     * transmission delay, if this gives a default RTT greater than 100 ms.
     *
     * If m_qW=-2, set it to a reasonable value of 1-exp(-10/C).
     */
    if (m_qW == 0.0)
    {
        m_qW = 1.0 - std::exp(-1.0 / m_ptc);
    }
    else if (m_qW == -1.0)
    {
        double rtt = 3.0 * (m_linkDelay.GetSeconds() + 1.0 / m_ptc);

        if (rtt < 0.1)
        {
            rtt = 0.1;
        }
        m_qW = 1.0 - std::exp(-1.0 / (10 * rtt * m_ptc));
    }
    else if (m_qW == -2.0)
    {
        m_qW = 1.0 - std::exp(-10.0 / m_ptc);
    }

    if (m_bottom == 0)
    {
        m_bottom = 0.01;
        // Set bottom to at most 1/W, where W is the delay-bandwidth
        // product in packets for a connection.
        // So W = m_linkBandwidth.GetBitRate () / (8.0 * m_meanPktSize * m_rtt.GetSeconds())
        double bottom1 = (8.0 * m_meanPktSize * m_rtt.GetSeconds()) / m_linkBandwidth.GetBitRate();
        if (bottom1 < m_bottom)
        {
            m_bottom = bottom1;
        }
    }

    NS_LOG_DEBUG("\tm_delay " << m_linkDelay.GetSeconds() << "; m_isWait " << m_isWait << "; m_qW "
                              << m_qW << "; m_ptc " << m_ptc << "; m_minTh " << m_minTh
                              << "; m_maxTh " << m_maxTh << "; m_isGentle " << m_isGentle
                              << "; th_diff " << th_diff << "; lInterm " << m_lInterm << "; va "
                              << m_vA << "; cur_max_p " << m_curMaxP << "; v_b " << m_vB
                              << "; m_vC " << m_vC << "; m_vD " << m_vD);
}

// Updating m_curMaxP, following the pseudocode
// from: A Self-Configuring BLACK Gateway, INFOCOMM '99.
// They recommend m_a = 3, and m_b = 2.
void
BLACKQueueDisc::UpdateMaxPFeng(double newAve)
{
    NS_LOG_FUNCTION(this << newAve);

    if (m_minTh < newAve && newAve < m_maxTh)
    {
        m_fengStatus = Between;
    }
    else if (newAve < m_minTh && m_fengStatus != Below)
    {
        m_fengStatus = Below;
        m_curMaxP = m_curMaxP / m_a;
    }
    else if (newAve > m_maxTh && m_fengStatus != Above)
    {
        m_fengStatus = Above;
        m_curMaxP = m_curMaxP * m_b;
    }
}

// Update m_curMaxP to keep the average queue length within the target range.
void
BLACKQueueDisc::UpdateMaxP(double newAve)
{
    NS_LOG_FUNCTION(this << newAve);

    Time now = Simulator::Now();
    double m_part = 0.4 * (m_maxTh - m_minTh);
    // AIMD rule to keep target Q~1/2(m_minTh + m_maxTh)
    if (newAve < m_minTh + m_part && m_curMaxP > m_bottom)
    {
        // we should increase the average queue size, so decrease m_curMaxP
        m_curMaxP = m_curMaxP * m_beta;
        m_lastSet = now;
    }
    else if (newAve > m_maxTh - m_part && m_top > m_curMaxP)
    {
        // we should decrease the average queue size, so increase m_curMaxP
        double alpha = m_alpha;
        if (alpha > 0.25 * m_curMaxP)
        {
            alpha = 0.25 * m_curMaxP;
        }
        m_curMaxP = m_curMaxP + alpha;
        m_lastSet = now;
    }
}

// Compute the average queue size
double
BLACKQueueDisc::Estimator(uint32_t nQueued, uint32_t m, double qAvg, double qW)
{
    NS_LOG_FUNCTION(this << nQueued << m << qAvg << qW);

    double newAve = qAvg * std::pow(1.0 - qW, m);
    newAve += qW * nQueued;

    Time now = Simulator::Now();
    if (m_isAdaptMaxP && now > m_lastSet + m_interval)
    {
        UpdateMaxP(newAve);
    }
    else if (m_isFengAdaptive)
    {
        UpdateMaxPFeng(newAve); // Update m_curMaxP in MIMD fashion.
    }

    return newAve;
}

// Check if packet p needs to be dropped due to probability mark
uint32_t
BLACKQueueDisc::DropEarly(Ptr<QueueDiscItem> item, uint32_t qSize)
{
    NS_LOG_FUNCTION(this << item << qSize);

    double prob1 = CalculatePNew();
    m_vProb = ModifyP(prob1, item->GetSize());

    // Drop probability is computed, pick random number and act
    if (m_cautious == 1)
    {
        /*
         * Don't drop/mark if the instantaneous queue is much below the average.
         * For experimental purposes only.
         * pkts: the number of packets arriving in 50 ms
         */
        double pkts = m_ptc * 0.05;
        double fraction = std::pow((1 - m_qW), pkts);

        if ((double)qSize < fraction * m_qAvg)
        {
            // Queue could have been empty for 0.05 seconds
            return 0;
        }
    }

    double u = m_uv->GetValue();

    if (m_cautious == 2)
    {
        /*
         * Decrease the drop probability if the instantaneous
         * queue is much below the average.
         * For experimental purposes only.
         * pkts: the number of packets arriving in 50 ms
         */
        double pkts = m_ptc * 0.05;
        double fraction = std::pow((1 - m_qW), pkts);
        double ratio = qSize / (fraction * m_qAvg);

        if (ratio < 1.0)
        {
            u *= 1.0 / ratio;
        }
    }

    if (u <= m_vProb)
    {
        NS_LOG_LOGIC("u <= m_vProb; u " << u << "; m_vProb " << m_vProb);

        // DROP or MARK
        m_count = 0;
        m_countBytes = 0;
        /// \todo Implement set bit to mark

        return 1; // drop
    }

    return 0; // no drop/mark
}

// Returns a probability using these function parameters for the DropEarly function
double
BLACKQueueDisc::CalculatePNew()
{
    NS_LOG_FUNCTION(this);
    double p;

    if (m_isGentle && m_qAvg >= m_maxTh)
    {
        // p ranges from m_curMaxP to 1 as the average queue
        // size ranges from m_maxTh to twice m_maxTh
        p = m_vC * m_qAvg + m_vD;
    }
    else if (!m_isGentle && m_qAvg >= m_maxTh)
    {
        /*
         * OLD: p continues to range linearly above m_curMaxP as
         * the average queue size ranges above m_maxTh.
         * NEW: p is set to 1.0
         */
        p = 1.0;
    }
    else
    {
        /*
         * p ranges from 0 to m_curMaxP as the average queue size ranges from
         * m_minTh to m_maxTh
         */
        p = m_vA * m_qAvg + m_vB;

        if (m_isNonlinear)
        {
            p *= p * 1.5;
        }

        p *= m_curMaxP;
    }

    if (p > 1.0)
    {
        p = 1.0;
    }

    return p;
}

// Returns a probability using these function parameters for the DropEarly function
double
BLACKQueueDisc::ModifyP(double p, uint32_t size)
{
    NS_LOG_FUNCTION(this << p << size);
    auto count1 = (double)m_count;

    if (GetMaxSize().GetUnit() == QueueSizeUnit::BYTES)
    {
        count1 = (double)(m_countBytes / m_meanPktSize);
    }

    if (m_isWait)
    {
        if (count1 * p < 1.0)
        {
            p = 0.0;
        }
        else if (count1 * p < 2.0)
        {
            p /= (2.0 - count1 * p);
        }
        else
        {
            p = 1.0;
        }
    }
    else
    {
        if (count1 * p < 1.0)
        {
            p /= (1.0 - count1 * p);
        }
        else
        {
            p = 1.0;
        }
    }

    if ((GetMaxSize().GetUnit() == QueueSizeUnit::BYTES) && (p < 1.0))
    {
        p = (p * size) / m_meanPktSize;
    }

    if (p > 1.0)
    {
        p = 1.0;
    }

    return p;
}

Ptr<QueueDiscItem>
BLACKQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        m_idle = 1;
        m_idleTime = Simulator::Now();

        return nullptr;
    }
    else
    {
        m_idle = 0;
        Ptr<QueueDiscItem> item = GetInternalQueue(0)->Dequeue();

        NS_LOG_LOGIC("Popped " << item);

        NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
        NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

        return item;
    }
}

Ptr<const QueueDiscItem>
BLACKQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);
    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    Ptr<const QueueDiscItem> item = GetInternalQueue(0)->Peek();

    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return item;
}

bool
BLACKQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("BLACKQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("BLACKQueueDisc cannot have packet filters");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // add a DropTail queue
        AddInternalQueue(
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>("MaxSize",
                                                                     QueueSizeValue(GetMaxSize())));
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("BLACKQueueDisc needs 1 internal queue");
        return false;
    }

    if ((m_isABLACK || m_isAdaptMaxP) && m_isFengAdaptive)
    {
        NS_LOG_ERROR("m_isAdaptMaxP and m_isFengAdaptive cannot be simultaneously true");
    }

    return true;
}

} // namespace ns3