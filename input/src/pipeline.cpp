#include <actcraft/input/pipeline.h>

namespace actcraft::input {

bool DuplicateFilter::accept(const Sample& sample) noexcept
{
    if (sample.phase == Phase::Move && m_hasLast && sample.x == m_last.x && sample.y == m_last.y &&
        sample.pressure == m_last.pressure) {
        return false;
    }
    m_last = sample;
    m_hasLast = true;
    return true;
}

void DuplicateFilter::reset() noexcept
{
    m_hasLast = false;
}

Pipeline::Pipeline(SampleSink& sink) : m_sink(sink) {}

void Pipeline::setProbe(core::InkProbe* probe) noexcept
{
    m_probe = probe;
}

void Pipeline::resetSequence(std::uint64_t next) noexcept
{
    m_next = next;
}

std::uint64_t Pipeline::nextSequence() const noexcept
{
    return m_next;
}

std::uint64_t Pipeline::push(Sample sample)
{
    sample.seq = m_next++;
    if (m_probe) {
        m_probe->sampleReceived(sample.seq, sample.timestampNs);
    }
    if (m_filter.accept(sample)) {
        m_sink.onSample(m_predictor.predict(sample));
    }
    return sample.seq;
}

} // namespace actcraft::input
