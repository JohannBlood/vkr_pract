#include "wfq-queue-disc.h"

#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/socket.h"

#include <algorithm>
#include <iterator>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WFQQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(WFQQueueDisc);

ATTRIBUTE_HELPER_CPP(WFQmap);

std::ostream&
operator<<(std::ostream& os, const WFQmap& priomap)
{
    std::copy(priomap.begin(), priomap.end() - 1, std::ostream_iterator<uint16_t>(os, " "));
    os << priomap.back();
    return os;
}

std::istream&
operator>>(std::istream& is, WFQmap& priomap)
{
    for (int i = 0; i < 16; i++)
    {
        if (!(is >> priomap[i]))
        {
            NS_FATAL_ERROR("Incomplete priomap specification ("
                           << i << " values provided, 16 required)");
        }
    }
    return is;
}

TypeId
WFQQueueDisc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WFQQueueDisc")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<WFQQueueDisc>()
            .AddAttribute("WFQmap",
                          "The priority to band mapping.",
                          WFQmapValue(WFQmap{{1, 2, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}}),
                          MakeWFQmapAccessor(&WFQQueueDisc::m_prio2band),
                          MakeWFQmapChecker());
    return tid;
}

WFQQueueDisc::WFQQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::NO_LIMITS)
{
    NS_LOG_FUNCTION(this);
}

WFQQueueDisc::~WFQQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
WFQQueueDisc::SetBandForWFQrity(uint8_t prio, uint16_t band)
{
    NS_LOG_FUNCTION(this << prio << band);

    NS_ASSERT_MSG(prio < 16, "WFQrity must be a value between 0 and 15");

    m_prio2band[prio] = band;
}

uint16_t
WFQQueueDisc::GetBandForWFQrity(uint8_t prio) const
{
    NS_LOG_FUNCTION(this << prio);

    NS_ASSERT_MSG(prio < 16, "WFQrity must be a value between 0 and 15");

    return m_prio2band[prio];
}

bool
WFQQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    uint32_t band = m_prio2band[0];

    int32_t ret = Classify(item);

    if (ret == PacketFilter::PF_NO_MATCH)
    {
        NS_LOG_DEBUG("No filter has been able to classify this packet, using priomap.");

        SocketPriorityTag priorityTag;
        if (item->GetPacket()->PeekPacketTag(priorityTag))
        {
            band = m_prio2band[priorityTag.GetPriority() & 0x0f];
        }
    }
    else
    {
        NS_LOG_DEBUG("Packet filters returned " << ret);

        if (ret >= 0 && static_cast<uint32_t>(ret) < GetNQueueDiscClasses())
        {
            band = ret;
        }
    }

    NS_ASSERT_MSG(band < GetNQueueDiscClasses(), "Selected band out of range");
    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue disc
    // because QueueDisc::AddQueueDiscClass sets the drop callback

    NS_LOG_LOGIC("Number packets band " << band << ": "
                                        << GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets());

    return retval;
}

Ptr<QueueDiscItem>
WFQQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Dequeue()))
        {
            NS_LOG_LOGIC("Popped from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band "
                         << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem>
WFQQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++)
    {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Peek()))
        {
            NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC("Number packets band "
                         << i << ": " << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool
WFQQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNInternalQueues() > 0)
    {
        NS_LOG_ERROR("WFQQueueDisc cannot have internal queues");
        return false;
    }

    if (GetNQueueDiscClasses() == 0)
    {
        // create 3 fifo queue discs
        ObjectFactory factory;
        factory.SetTypeId("ns3::FifoQueueDisc");
        for (uint8_t i = 0; i < 2; i++)
        {
            Ptr<QueueDisc> qd = factory.Create<QueueDisc>();
            qd->Initialize();
            Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass>();
            c->SetQueueDisc(qd);
            AddQueueDiscClass(c);
        }
    }

    if (GetNQueueDiscClasses() < 2)
    {
        NS_LOG_ERROR("WFQQueueDisc needs at least 2 classes");
        return false;
    }

    return true;
}

void
WFQQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
