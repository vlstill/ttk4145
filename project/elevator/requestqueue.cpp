#include <elevator/requestqueue.h>
#include <chrono>
#include <climits>

namespace elevator {

TimePoint RequestQueue::_earliestDeadline( const Guard &, TimePoint extra ) const {
    return _queue.empty() ? extra : std::min( _queue.top().deadline(), extra );
}

wibble::Maybe< Request > RequestQueue::waitForEarliestDeadline( MillisecondTime timeout ) {
    TimePoint d0 = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeout );
    Guard g{ _lock };
    return _waitForEarliestDeadline( g, d0 );
}

wibble::Maybe< Request > RequestQueue::_waitForEarliestDeadline( Guard &g, TimePoint d0 ) {
    TimePoint deadline = _earliestDeadline( g, d0 );
    TimePoint now;
    std::cv_status cvs = std::cv_status::no_timeout;
    while ( cvs == std::cv_status::no_timeout ) {
        bool cleaned;
        // we need to to do what was only flagged by ackRequest
        do {
            cleaned = true;
            // clean done requests
            while ( !_queue.empty() && _queue.top().type == RequestType::Done ) {
                cleaned = false;
                _queue.pop();
            }
            // reschedule request if required
            while ( !_queue.empty() && !_queue.top()._tweakedDeadline.isNothing() ) {
                // we need to extract request and re-push it, because we cannot
                // change deadline on request in queue
                auto req = _queue.top();
                _queue.pop();
                req._deadline = req._tweakedDeadline.value();
                req._tweakedDeadline = wibble::Maybe< TimePoint >::Nothing();
                _queue.push( req );
                cleaned = false;
            }
        } while ( !cleaned );
        // get earliest deadline
        deadline = _earliestDeadline( g, d0 );
        // wait if necessary
        if ( deadline <= (now = std::chrono::steady_clock::now()) )
            break;
        cvs = _signal.wait_until( g, deadline );
    }

    if ( !_queue.empty() && _queue.top().deadline() <= now ) {
        auto val = _queue.top();
        _queue.pop();
        assert( val._tweakedDeadline.isNothing(), "invalid deadline" );
        return wibble::Maybe< Request >::Just( val );
    }
    return wibble::Maybe< Request >::Nothing();
}

void RequestQueue::push( Request r ) {
    Guard g{ _lock };
    TimePoint oldDeadline;
    if ( !_queue.empty() )
        oldDeadline = _queue.top().deadline();
    _queue.push( r );
    // we need to recalculate deadline at waiter
    if ( _queue.size() == 1 || (_queue.size() > 1 && r.deadline() < oldDeadline) )
        _signal.notify_all();
}

void RequestQueue::ackRequest( StateChange change, MillisecondTime newDeadlineDelta ) {
    auto newDeadline = newDeadlineDelta == 0
        ? wibble::Maybe< TimePoint >::Nothing()
        : wibble::Maybe< TimePoint >::Just( std::chrono::steady_clock::now()
                + std::chrono::milliseconds( newDeadlineDelta ) );

    Guard g{ _lock };
    for ( auto &req : _queue ) {
        if ( req.elevatorId() == change.state.id
                && req.triggerType() == change.changeType
                && req.triggerFloor() == change.changeFloor )
        {
            if ( req.type == RequestType::NotAcknowledged )
                req.type = RequestType::NotDone;
            else if ( req.type == RequestType::NotDone )
                req.type = RequestType::Done;
            if ( !newDeadline.isNothing() ) {
                req._tweakedDeadline = newDeadline;
                req._deadlineDelta = newDeadlineDelta;
            }
            req.repeated = 0;
        }
    }
    // note: it might be that we ack-ed the very first item and someone was
    // waiting for its deadline. But that is not a problem, because the next
    // item can have only later deadline and we will ignore done item in
    // waitForEarliestDeadline
}

int RequestQueue::size() { Guard g{ _lock }; return _queue.size(); };

} // namespace elevator
