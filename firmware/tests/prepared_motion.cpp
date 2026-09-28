#include <cstdio>
#include <cstdlib>
#include "helix/protocol/trajectory_inbox.hpp"
using namespace helix;
using namespace helix::protocol;
static unsigned checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(false)
struct Clock final : MonotonicClock {
  TimeUs before, after; mutable bool read{false};
  Clock(TimeUs a, TimeUs b) : before(a), after(b) {}
  TimeUs now_us() const noexcept override { if (!read) { read=true; return before; } return after; }
};
static InboxConfig config() {
  InboxConfig c{};
  for (auto& l:c.limits) l={-10,10,10,10,10};
  c.budget={8,12264}; c.continuity={0,0,0};
  c.max_future_us=10000; c.lease_us=5000; c.preparation_lead_us=100;
  return c; // Synthetic host fixture only, never a target timing specification.
}
using Inbox=TrajectoryInbox<2,2>;
static TrajectoryRequest prepare(Inbox& i, TimeUs start=1000) {
  auto key=i.new_session(42,0); CHECK(key);
  CHECK(i.arm(*key,{true,true,true,true,true},start,{},0)==Status::Ok);
  helix::motion::CubicSegment s{}; s.start_us=start; s.duration_us=1000;
  TrajectoryRequest r{1,*key,1,42,start,s};
  CHECK(i.submit(r,Clock{0,0}).code==AdmissionCode::Accepted);
  CHECK(!i.prepared_authorized(r,0)); // Queued is not prepared.
  auto got=i.take_for_preparation(0); CHECK(got && same_request(*got,r));
  return r;
}
int main() {
  auto c=config(); auto invalid=c; invalid.preparation_lead_us=0;
  CHECK(!invalid.valid()); invalid.preparation_lead_us=10001; CHECK(!invalid.valid());
  Inbox i(7,c); auto r=prepare(i);
  CHECK(i.prepared_authorized(r,1));
  auto forged=r; forged.segment.control_rad[0][0]=0.1;
  CHECK(!i.prepared_authorized(forged,2));
  forged=r; ++forged.model_revision; CHECK(!i.prepared_authorized(forged,3));
  forged=r; ++forged.sequence; CHECK(!i.prepared_authorized(forged,4));
  CHECK(!i.execution_authorized(r,999));
  CHECK(i.execution_authorized(r,1000));
  CHECK(i.execution_authorized(r,1999));
  CHECK(!i.execution_authorized(r,2000));

  Inbox disarmed(7,c); r=prepare(disarmed);
  disarmed.disarm(10); CHECK(!disarmed.prepared_authorized(r,10));
  CHECK(disarmed.arm(r.owner,{true,true,true,true,true},1000,{},11)==Status::NotReady);
  CHECK(disarmed.new_session(42,12)); CHECK(!disarmed.prepared_authorized(r,12));

  Inbox faulted(7,c); r=prepare(faulted);
  faulted.trip(Fault::Interlock); CHECK(!faulted.prepared_authorized(r,10));
  CHECK(faulted.acknowledge_fault(true)==Status::Ok);
  CHECK(!faulted.prepared_authorized(r,11));

  auto shortlease=c; shortlease.lease_us=100;
  Inbox lease(7,shortlease); r=prepare(lease);
  CHECK(lease.prepared_authorized(r,99));
  CHECK(!lease.prepared_authorized(r,100)); CHECK(lease.fault()==Fault::CommunicationsLost);
  Inbox backwards(7,c); r=prepare(backwards);
  CHECK(backwards.prepared_authorized(r,10));
  CHECK(!backwards.prepared_authorized(r,9)); CHECK(backwards.fault()==Fault::ClockRegression);

  // Receipt eviction cannot leave a detached copy with execution permission.
  Inbox evicted(7,c); r=prepare(evicted);
  auto next=r; next.sequence=2; next.segment.start_us=2000; next.expires_us=2000;
  CHECK(evicted.submit(next,Clock{1,1}).code==AdmissionCode::Accepted);
  CHECK(evicted.take_for_preparation(1));
  next.sequence=3; next.segment.start_us=3000; next.expires_us=3000;
  CHECK(evicted.submit(next,Clock{2,2}).code==AdmissionCode::Accepted);
  CHECK(!evicted.prepared_authorized(r,2));

  for (TimeUs t : {TimeUs{900},TimeUs{901},TimeUs{1000},TimeUs{1001}}) {
    Inbox boundary(7,c); auto key=boundary.new_session(42,0); CHECK(key);
    CHECK(boundary.arm(*key,{true,true,true,true,true},1000,{},0)==Status::Ok);
    helix::motion::CubicSegment s{}; s.start_us=1000; s.duration_us=1000;
    TrajectoryRequest q{1,*key,1,42,1000,s};
    CHECK(boundary.submit(q,Clock{0,0}).code==AdmissionCode::Accepted);
    CHECK(static_cast<bool>(boundary.take_for_preparation(t))==(t==900));
    if (t!=900) CHECK(boundary.fault()==Fault::MotionDeadline);
  }
  Inbox elapsed(7,c); auto key=elapsed.new_session(42,0); CHECK(key);
  CHECK(elapsed.arm(*key,{true,true,true,true,true},1000,{},0)==Status::Ok);
  helix::motion::CubicSegment s{}; s.start_us=1000; s.duration_us=1000;
  TrajectoryRequest q{1,*key,1,42,1000,s};
  CHECK(elapsed.submit(q,Clock{0,901}).code==AdmissionCode::PreparationLate);
  CHECK(elapsed.queued()==0);
  // A subsequent continuously-timed hold retains the same ID after a rejected insert.
  Inbox boundary(7,c); key=boundary.new_session(42,0); q.owner=*key;
  CHECK(boundary.arm(*key,{true,true,true,true,true},1000,{},0)==Status::Ok);
  CHECK(boundary.submit(q,Clock{900,900}).code==AdmissionCode::Accepted);
  CHECK(boundary.take_for_preparation(900));
  Inbox too_soon(7,c); key=too_soon.new_session(42,0); CHECK(key);
  CHECK(too_soon.arm(*key,{true,true,true,true,true},99,{},0)==Status::Invalid);
  CHECK(too_soon.arm(*key,{true,true,true,true,true},100,{},0)==Status::Ok);
  std::printf("Prepared authorization and lead-time boundaries: %u checks passed.\n",checks);
}
