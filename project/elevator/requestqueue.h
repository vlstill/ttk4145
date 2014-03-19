#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

#include <wibble/maybe.h>

#include <elevator/test.h>
#include <elevator/time.h>
#include <elevator/state.h>
#include <elevator/command.h>

#ifndef ELEVATOR_REQUEST_QUEUE_H
#define ELEVATOR_REQUEST_QUEUE_H

namespace elevator {

using TimePoint = std::chrono::time_point< std::chrono::steady_clock >;

enum class RequestType { NotAcknowledged, NotDone, Done };

struct Request {
    Request( Command comm, MillisecondTime deadline ) :
        type( RequestType::NotAcknowledged ),
        deadline( deadline ),
        _deadline( std::chrono::steady_clock::now() + std::chrono::milliseconds( deadline ) ),
        repeated( 0 ),
        command( comm )
    { }
    void updateDeadline() {
        _deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( deadline );
    }
    void updateDeadline( MillisecondTime deadline ) {
        this->deadline = deadline;
        updateDeadline();
    }

    RequestType type;
    MillisecondTime deadline;
    TimePoint _deadline;
    int repeated;
    Command command;
    int elevatorId() const { return command.targetElevatorId; }
    ChangeType triggerType() const {
        switch ( command.commandType ) {
            case CommandType::CallToFloorAndGoUp:
                return type == RequestType::NotAcknowledged
                    ? ChangeType::GoingToServeUp
                    : ChangeType::ServedUp;
            case CommandType::CallToFloorAndGoDown:
                return type == RequestType::NotAcknowledged
                    ? ChangeType::GoingToServeDown
                    : ChangeType::ServedDown;
            default:
                return ChangeType::None;
        }
    };
    int triggerFloor() const { return command.targetFloor; }
};

/* comparator to make the request queue sorted in ascending order */
struct RequestCompare {
    bool operator()( const Request &a, const Request &b ) const {
        return a.deadline > b.deadline;
    }
};

struct RequestQueue {

    wibble::Maybe< Request > waitForEarliestDeadline( MillisecondTime timeout );
    void push( Request );
    void ackRequest( StateChange change );
    int size();

  private:
    struct QueueType :
        std::priority_queue< Request, std::vector< Request >, RequestCompare >
    {
        /* this is sort of hack C++ library, normally so called container adaptors
         * such as priority_queue does not allow access to underlying container
         * to avoid its breakage. Such an modification would in this
         * case be deleting some element or changing dealine (which affects ordering).
         * Despite that we need to enumerate over all requests to be able to
         * mark some of them as done. Therefore we allow iterators (which is possible
         * as underlying container is protected member). It is not completely safe,
         * if we modified dealine we would break container, but if we only
         * mark request as done, we cannot break container as that does not
         * effect ordering. But we must avoid exporting this to public interface. */
        using iterator = std::vector< Request >::iterator;

        iterator begin() { return c.begin(); }
        iterator end() { return c.end(); }
    };
    QueueType _queue;

    using Guard = std::unique_lock< std::mutex >;
    std::mutex _lock;
    std::condition_variable _signal;

    TimePoint _earliestDeadline( const Guard &, TimePoint ) const;
    wibble::Maybe< Request > _waitForEarliestDeadline( Guard &, TimePoint );
};

}

#endif // ELEVATOR_REQUEST_QUEUE_H
