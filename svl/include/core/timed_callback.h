
#ifndef ___TIME_TRIGGER__
#define ___TIME_TRIGGER__

#include <boost/timer.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/slot.hpp>


  /**
   * @brief timer class that invokes registered callback methods periodically.
   * @param interval_seconds interval in seconds
   * @param callback callback to be invoked periodically
   * \ingroup common
   */
  class q_time_trigger
  {
    public:
      typedef boost::function<void() > callback_type;
      /**
       * @brief timer class that calls a callback method periodically. Due to possible blocking calls, only one callback method can be registered per instance.
       * @param interval_seconds interval in seconds
       * @param callback callback to be invoked periodically
       */
      q_time_trigger (double interval_seconds, const callback_type& callback);
      /**
       * @brief timer class that calls a callback method periodically. Due to possible blocking calls, only one callback method can be registered per instance.
       * @param interval_seconds interval in seconds
       */
      q_time_trigger (double interval_seconds = 1.0);
      /**
       * @brief desctructor
       */
      ~q_time_trigger ();
      /**
       * @brief registeres a callback
       * @param callback callback function to the list of callbacks. signature has to be boost::function<void()>
       * @return connection the connection, which can be used to disable/enable and remove callback from list
       */
      boost::signals2::connection registerCallback (const callback_type& callback);
      /**
       * @brief resets the timer interval
       * @param interval_seconds interval in seconds
       */
      void setInterval (double interval_seconds);
      /**
       * @brief start the Trigger
       */
      void start ();
      /**
       * @brief stop the Trigger
       */
      void stop ();
    private:
      void thread_function ();
      boost::signals2::signal <void() > callbacks_;

      double interval_;

      bool quit_;
      bool running_;

      boost::thread timer_thread_;
      boost::condition_variable condition_;
      boost::mutex condition_mutex_;
  };


#endif
