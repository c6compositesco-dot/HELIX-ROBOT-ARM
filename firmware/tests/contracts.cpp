#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "helix/joints/transmission.hpp"
#include "helix/feedback/sample.hpp"
#include "helix/motion/waypoint_queue.hpp"
#include "helix/runtime/execution_gate.hpp"
#include "helix/protocol/admission.hpp"
#include "helix/drives/drive.hpp"
#include "helix/kinematics/solver.hpp"
#include "helix/tasks/program.hpp"
#include "helix/platform/clock.hpp"

using namespace helix;
static int checks = 0;
static void check(bool passed, const char* expression, int line) {
  ++checks;
  if (!passed) { std::fprintf(stderr, "FAIL line %d: %s\n", line, expression); std::exit(1); }
}
#define CHECK(...) check((__VA_ARGS__), #__VA_ARGS__, __LINE__)
static bool near(double a, double b) { return std::abs(a - b) < 1e-10; }
static RobotLimits limits() {
  RobotLimits value{};
  for (auto& joint : value) joint = {-2.0, 2.0, 1.0};
  return value;
}
static JointWaypoint point(TimeUs time) { JointWaypoint p{}; p.apply_at_us = time; return p; }
static ArmChecks qualified() { return {true, true, true, true, true}; }

static void transmission() {
  FixedRatioTransmission t{};
  CHECK(!t.valid()); CHECK(!t.to_joint(0));
  t = {10.0, -1, 1.0, 0.25}; // Synthetic values, not Helix configuration.
  CHECK(t.valid());
  CHECK(near(*t.to_joint(11.0), -0.75));
  CHECK(near(*t.to_motor(-0.75), 11.0));
  for (int i = -10; i <= 10; ++i) CHECK(near(*t.to_joint(*t.to_motor(i / 10.0)), i / 10.0));
  CHECK(!t.to_joint(std::numeric_limits<double>::infinity()));
  CHECK(!t.to_motor(std::numeric_limits<double>::quiet_NaN()));
  CHECK(!t.to_motor(std::numeric_limits<double>::max()));
  t.direction = 0; CHECK(!t.valid());
  t.direction = 1; t.motor_per_joint = -1; CHECK(!t.valid());
}
static void feedback() {
  JointFeedback f{};
  CHECK(!has_fresh_output(f, 100, 10));
  f.step_estimate = {0.2, 90, SampleSource::StepEstimate, true, true};
  CHECK(fresh(f.step_estimate, 100, 10));
  CHECK(!has_fresh_output(f, 100, 10));
  f.motor_position = {1.0, 95, SampleSource::MotorEncoder, true, true};
  CHECK(!has_fresh_output(f, 100, 10));
  f.output_position = {0.1, 95, SampleSource::OutputEncoder, true, true};
  CHECK(has_fresh_output(f, 100, 10));
  CHECK(!has_fresh_output(f, 106, 10));
  CHECK(!has_fresh_output(f, 94, 10));
  f.output_position.referenced = false; CHECK(!has_fresh_output(f, 100, 10));
  f.output_position.referenced = true; f.output_position.valid = false;
  CHECK(!has_fresh_output(f, 100, 10));
  f.output_position.valid = true; f.output_position.radians = std::numeric_limits<double>::quiet_NaN();
  CHECK(!has_fresh_output(f, 100, 10));
  f.output_position = f.step_estimate; CHECK(!has_fresh_output(f, 100, 10));
}
static void motion() {
  WaypointQueue<2> q(limits());
  CHECK(!q.pop()); CHECK(q.size() == 0);
  CHECK(q.push(point(10), 10) == Status::Stale);
  CHECK(q.push(point(20), 10) == Status::Ok);
  CHECK(q.push(point(20), 10) == Status::Stale);
  CHECK(q.push(point(30), 10) == Status::Ok);
  CHECK(q.push(point(40), 10) == Status::Full);
  CHECK(q.pop()->apply_at_us == 20);
  CHECK(q.push(point(40), 10) == Status::Ok);
  CHECK(q.pop()->apply_at_us == 30); CHECK(q.pop()->apply_at_us == 40);
  CHECK(q.push(point(35), 10) == Status::Stale);
  q.clear();
  auto bad = point(50); bad.position_rad[2] = 2.01;
  CHECK(q.push(bad, 0) == Status::OutOfRange);
  bad = point(50); bad.velocity_rad_s[5] = -1.01;
  CHECK(q.push(bad, 0) == Status::OutOfRange);
  bad = point(50); bad.position_rad[0] = std::numeric_limits<double>::quiet_NaN();
  CHECK(q.push(bad, 0) == Status::Invalid); CHECK(q.size() == 0);
  bad = point(50); bad.velocity_rad_s[0] = std::numeric_limits<double>::infinity();
  CHECK(q.push(bad, 0) == Status::Invalid);
  auto edge = point(50); edge.position_rad[1] = -2; edge.velocity_rad_s[4] = 1;
  CHECK(q.push(edge, 0) == Status::Ok); CHECK(q.pop().has_value());
  for (TimeUs t = 51; t < 100; ++t) { CHECK(q.push(point(t), 0) == Status::Ok); CHECK(q.pop()->apply_at_us == t); }
  WaypointQueue<1> invalid(RobotLimits{});
  CHECK(invalid.push(point(10), 0) == Status::Invalid);
}
static void runtime() {
  ExecutionGate gate(100);
  CHECK(gate.state() == ExecutionState::Disarmed);
  CHECK(gate.start(0) == Status::NotReady);
  CHECK(gate.arm({}, 0) == Status::NotReady);
  CHECK(gate.arm(qualified(), 10) == Status::Ok);
  CHECK(gate.arm(qualified(), 10) == Status::NotReady);
  CHECK(gate.start(11) == Status::Ok);
  CHECK(gate.heartbeat(50) == Status::Ok);
  gate.poll(149); CHECK(gate.state() == ExecutionState::Running);
  CHECK(gate.heartbeat(150) == Status::NotReady);
  CHECK(gate.state() == ExecutionState::Faulted);
  CHECK(gate.fault() == Fault::CommunicationsLost);
  gate.revoke_permission(); CHECK(gate.state() == ExecutionState::Faulted);
  gate.trip(Fault::Drive); CHECK(gate.fault() == Fault::CommunicationsLost);
  CHECK(gate.arm(qualified(), 151) == Status::Faulted);
  CHECK(gate.acknowledge_fault(false) == Status::NotReady);
  CHECK(gate.acknowledge_fault(true) == Status::Ok);
  CHECK(gate.state() == ExecutionState::Disarmed);
  CHECK(gate.heartbeat(152) == Status::NotReady);
  CHECK(gate.start(152) == Status::NotReady);
  CHECK(gate.arm(qualified(), 200) == Status::Ok);
  gate.poll(220); gate.poll(219); CHECK(gate.fault() == Fault::ClockRegression);
  ExecutionGate disabled(0); CHECK(disabled.arm(qualified(), 0) == Status::NotReady);
  for (int i = 0; i < 5; ++i) {
    auto c = qualified();
    if (i == 0) c.board_qualified = false;
    if (i == 1) c.model_valid = false;
    if (i == 2) c.references_valid = false;
    if (i == 3) c.required_feedback_valid = false;
    if (i == 4) c.interlocks_ok = false;
    ExecutionGate denied(100); CHECK(denied.arm(c, 0) == Status::NotReady);
  }
}
static void protocol() {
  CommandAdmission admission(7, 9);
  CommandEnvelope e{1, 7, 1, 9, 100};
  CHECK(admission.admit(e, 10) == Status::Ok);
  CHECK(admission.admit(e, 10) == Status::Stale);
  e.sequence = 2; e.major_version = 2;
  CHECK(admission.admit(e, 10) == Status::Unsupported);
  e.major_version = 1; e.session_id = 8; CHECK(admission.admit(e, 10) == Status::Stale);
  e.session_id = 7; e.model_revision = 8; CHECK(admission.admit(e, 10) == Status::ModelMismatch);
  e.model_revision = 9; CHECK(admission.admit(e, 100) == Status::Stale);
  CHECK(admission.admit(e, 99) == Status::Ok); // Rejected packets did not consume sequence.
  CommandAdmission reconnect(8, 9); CHECK(reconnect.admit(e, 99) == Status::Stale);
  CommandAdmission invalid(0, 9); CHECK(invalid.admit(e, 99) == Status::NotReady);
  CommandAdmission zero(7, 9); e.sequence = 0; CHECK(zero.admit(e, 99) == Status::Stale);
}
static void drives() {
  UnavailableDrive drive;
  CHECK(!drive.capabilities().operational);
  CHECK(drive.enable() == Status::NotImplemented);
  CHECK(drive.submit({}, 0) == Status::NotImplemented);
  CHECK(!has_fresh_output(drive.feedback(), 0, 10));
  JointCommand c{};
  DriveCapabilities step{}; step.operational = true; step.position = true;
  CHECK(validate_command(c, step, 0) == Status::Ok);
  c.mode = DriveMode::Torque; CHECK(validate_command(c, step, 0) == Status::Unsupported);
  c.mode = DriveMode::Velocity; CHECK(validate_command(c, step, 0) == Status::Unsupported);
  c.mode = DriveMode::Position; c.apply_at_us = 10;
  CHECK(validate_command(c, step, 0) == Status::Unsupported);
  step.timestamped_commands = true; CHECK(validate_command(c, step, 0) == Status::Ok);
  CHECK(validate_command(c, step, 10) == Status::Stale);
  c.position_rad = std::numeric_limits<double>::quiet_NaN();
  CHECK(validate_command(c, step, 0) == Status::Invalid);
  c = {}; c.mode = static_cast<DriveMode>(99); CHECK(validate_command(c, step, 0) == Status::Invalid);
  CHECK(validate_command({}, {}, 0) == Status::NotReady);
}
static void kinematics() {
  UnavailableKinematics solver;
  auto result = solver.inverse({});
  CHECK(result.status == Status::NotImplemented);
  CHECK(result.status != Status::Ok);
  CHECK(result.iterations == 0);
}
static void tasks() {
  TaskProgram<3> task;
  CHECK(!task.complete());
  CHECK(task.append({TaskOp::WaitInput, 1, 0}) == Status::Invalid);
  CHECK(task.append({TaskOp::MoveTrajectory, 0, 100}) == Status::Invalid);
  CHECK(task.append({TaskOp::MoveTrajectory, 1, 100}) == Status::Ok);
  CHECK(task.append({TaskOp::WaitInput, 2, 100}) == Status::Ok);
  CHECK(!task.complete());
  CHECK(task.append({TaskOp::End, 0, 0}) == Status::Ok);
  CHECK(task.complete()); CHECK(task.size() == 3);
  CHECK(task.append({TaskOp::SetTool, 1, 100}) == Status::Invalid);
  TaskProgram<1> full;
  CHECK(full.append({TaskOp::SetTool, 1, 100}) == Status::Ok);
  CHECK(full.append({TaskOp::End, 0, 0}) == Status::Full);
  CHECK(!full.complete());
  TaskProgram<1> bad;
  CHECK(bad.append({static_cast<TaskOp>(99), 1, 10}) == Status::Invalid);
}
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  struct Suite { const char* name; void (*run)(); };
  const Suite suites[] = {{"transmission", transmission}, {"feedback", feedback},
    {"motion", motion}, {"runtime", runtime}, {"protocol", protocol},
    {"drives", drives}, {"kinematics", kinematics}, {"tasks", tasks}};
  for (const auto& suite : suites) {
    if (std::strcmp(argv[1], suite.name) == 0) {
      suite.run(); std::printf("PASS %s: %d checks\n", suite.name, checks); return 0;
    }
  }
  std::fprintf(stderr, "Unknown suite: %s\n", argv[1]); return 2;
}
