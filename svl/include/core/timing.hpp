#ifndef _TIMING_H_
#define _TIMING_H_

#include <cmath>
#include <string>
#include <chrono>
#include <iomanip>

namespace timing
{

    /** \brief chrono based timer. Reports time since last reset
      *
      */
    class timer
    {
    public:
        timer(const std::string& name) : m_start_time(0), m_count(0), m_sofar(0)
        {

        }

        static uint64_t getTime()
        {
            auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        }

		static double getTimeMsec()
        {
			return timer::getTime() / 1000000.0;
        }

        void reset()
        {
            m_start_time = m_count = m_sofar = 0;
        }
        void record_restart ()
        {
            stop();
            m_start_time = getTime();
        }

        void stop()
        {
            if (m_start_time)
            {
                uint64_t now = getTime();
                if (now > m_start_time)
                    m_sofar += (now - m_start_time);
                m_start_time = 0;
                ++m_count;
            }
        }

        uint64_t value() { stop(); return m_sofar; }
        double seconds() { return milliseconds() / 1000.0; }
        double milliseconds() { return value() / 1000000.0; }

        double ms_per_event() { return milliseconds() / count(); }
        double seconds_per_event() { return seconds() / count(); }

        uint64_t count() const { return m_count; }
        std::string get_name() const { return m_name; }



    protected:
        uint64_t m_start_time, m_count, m_sofar;
        std::string m_name;
    };




    std::ostream& operator<<(std::ostream& out, timer& timer)
    {


        double events_per_second_fl =
            static_cast<double>(timer.count() / timer.seconds());

        uint64_t events_per_second = static_cast<uint64_t>(events_per_second_fl);

        out << events_per_second << " " << timer.get_name() << " per second; ";

        std::string op_or_ops = (timer.count() == 1) ? "op" : "ops";

        out << std::setprecision(2) << std::fixed
            << timer.ms_per_event() << " ms/op"
            << " (" << timer.count() << " " << op_or_ops << " in "
            << timer.milliseconds() << " ms)";

        return out;
    }

    inline bool operator<(const timer& x, const timer& y)
    {
        return (x.get_name() < y.get_name());
    }

    inline bool operator==(const timer& x, const timer& y)
    {
        return (x.get_name() == y.get_name());
    }





    /// Executes code, only if secs are gone since last exec.
#ifndef DO_EVERY_TS
#define DO_EVERY_TS(secs, currentTime, code) \
if (1) {\
  static double s_lastDone_ = 0.0; \
  double s_now_ = (currentTime); \
  if (s_lastDone_ > s_now_) \
    s_lastDone_ = s_now_; \
  if ((s_now_ - s_lastDone_) > (secs)) {        \
    code; \
    s_lastDone_ = s_now_; \
      }\
} else \
  (void)0
#endif

    /// Executes code, only if secs are gone since last exec.
#ifndef DO_EVERY
#define DO_EVERY(secs, code) \
  DO_EVERY_TS(secs, timing::timer::getTime(), code)
#endif

}  // end namespace


#endif  



