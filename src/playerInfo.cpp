#include <i18n.h>
#include "playerInfo.h"
#include "random.h"
#include "menus/luaConsole.h"
#include "screens/mainScreen.h"
#include "screens/crewStationScreen.h"

#include "screens/crew6/helmsScreen.h"
#include "screens/crew6/weaponsScreen.h"
#include "screens/crew6/engineeringScreen.h"
#include "screens/crew6/scienceScreen.h"
#include "screens/crew6/relayScreen.h"

#include "screens/crew4/tacticalScreen.h"
#include "screens/crew4/engineeringAdvancedScreen.h"
#include "screens/crew4/operationsScreen.h"

#include "screens/crew1/singlePilotScreen.h"

#include "screens/extra/damcon.h"
#include "screens/extra/powerManagement.h"
#include "screens/extra/databaseScreen.h"
#include "screens/extra/commsScreen.h"
#include "screens/extra/shipLogScreen.h"

#include "screenComponents/mainScreenControls.h"
#include "screenComponents/selfDestructEntry.h"

#include "components/internalrooms.h"

#include "components/collision.h"
#include "components/impulse.h"
#include "components/hull.h"
#include "components/customshipfunction.h"
#include "components/shiplog.h"
#include "components/probe.h"
#include "components/reactor.h"
#include "components/coolant.h"
#include "components/beamweapon.h"
#include "components/warpdrive.h"
#include "components/jumpdrive.h"
#include "components/shields.h"
#include "components/target.h"
#include "components/missiletubes.h"
#include "components/maneuveringthrusters.h"
#include "components/selfdestruct.h"
#include "components/hacking.h"
#include "components/scanning.h"
#include "components/radar.h"
#include "components/internalrooms.h"
#include "components/moveto.h"
#include "systems/jumpsystem.h"
#include "systems/docking.h"
#include "systems/missilesystem.h"
#include "systems/selfdestruct.h"
#include "systems/comms.h"
#include "systems/scanning.h"

//Ship commands
static const uint16_t CMD_TARGET_ROTATION = 0x0001;
static const uint16_t CMD_IMPULSE = 0x0002;
static const uint16_t CMD_WARP = 0x0003;
static const uint16_t CMD_JUMP = 0x0004;
static const uint16_t CMD_SET_TARGET = 0x0005;
static const uint16_t CMD_LOAD_TUBE = 0x0006;
static const uint16_t CMD_UNLOAD_TUBE = 0x0007;
static const uint16_t CMD_FIRE_TUBE = 0x0008;
static const uint16_t CMD_SET_SHIELDS = 0x0009;
static const uint16_t CMD_SET_MAIN_SCREEN_SETTING = 0x000A; // Overlay is 0x0027
static const uint16_t CMD_SCAN_OBJECT = 0x000B;
static const uint16_t CMD_SCAN_DONE = 0x000C;
static const uint16_t CMD_SCAN_CANCEL = 0x000D;
static const uint16_t CMD_SET_SYSTEM_POWER_REQUEST = 0x000E;
static const uint16_t CMD_SET_SYSTEM_COOLANT_REQUEST = 0x000F;
static const uint16_t CMD_DOCK = 0x0010;
static const uint16_t CMD_UNDOCK = 0x0011;
static const uint16_t CMD_OPEN_TEXT_COMM = 0x0012; //TEXT communication
static const uint16_t CMD_CLOSE_TEXT_COMM = 0x0013;
static const uint16_t CMD_SEND_TEXT_COMM = 0x0014;
static const uint16_t CMD_SEND_TEXT_COMM_PLAYER = 0x0015;
static const uint16_t CMD_ANSWER_COMM_HAIL = 0x0016;
static const uint16_t CMD_SET_AUTO_REPAIR = 0x0017;
static const uint16_t CMD_SET_BEAM_FREQUENCY = 0x0018;
static const uint16_t CMD_SET_BEAM_SYSTEM_TARGET = 0x0019;
static const uint16_t CMD_SET_SHIELD_FREQUENCY = 0x001A;
static const uint16_t CMD_ADD_WAYPOINT = 0x001B;
static const uint16_t CMD_REMOVE_WAYPOINT = 0x001C;
static const uint16_t CMD_MOVE_WAYPOINT = 0x001D;
static const uint16_t CMD_ACTIVATE_SELF_DESTRUCT = 0x001E;
static const uint16_t CMD_CANCEL_SELF_DESTRUCT = 0x001F;
static const uint16_t CMD_CONFIRM_SELF_DESTRUCT = 0x0020;
static const uint16_t CMD_COMBAT_MANEUVER_BOOST = 0x0021;
static const uint16_t CMD_COMBAT_MANEUVER_STRAFE = 0x0022;
static const uint16_t CMD_LAUNCH_PROBE = 0x0023;
static const uint16_t CMD_SET_ALERT_LEVEL = 0x0024;
static const uint16_t CMD_SET_SCIENCE_LINK = 0x0025;
static const uint16_t CMD_ABORT_DOCK = 0x0026;
static const uint16_t CMD_SET_MAIN_SCREEN_OVERLAY = 0x0027;
static const uint16_t CMD_HACKING_FINISHED = 0x0028;
static const uint16_t CMD_CUSTOM_FUNCTION = 0x0029;
static const uint16_t CMD_TURN_SPEED = 0x002A;
static const uint16_t CMD_CREW_SET_TARGET = 0x002B;

//Pre-ship commands
static const uint16_t CMD_UPDATE_CREW_POSITION = 0x0101;
static const uint16_t CMD_UPDATE_SHIP_ID = 0x0102;
static const uint16_t CMD_UPDATE_MAIN_SCREEN = 0x0103;
static const uint16_t CMD_UPDATE_MAIN_SCREEN_CONTROL = 0x0104;
static const uint16_t CMD_UPDATE_NAME = 0x0105;

P<PlayerInfo> my_player_info;
sp::ecs::Entity my_spaceship;
PVector<PlayerInfo> player_info_list;

REGISTER_MULTIPLAYER_CLASS(PlayerInfo, "PlayerInfo");
PlayerInfo::PlayerInfo()
: MultiplayerObject("PlayerInfo")
{
    client_id = -1;
    main_screen_control = 0;
    last_ship_password = "";
    registerMemberReplication(&client_id);

    for(int n=0; n<max_crew_positions; n++)
    {
        crew_position[n] = 0;
        registerMemberReplication(&crew_position[n]);
    }
    registerMemberReplication(&ship);
    registerMemberReplication(&name);
    registerMemberReplication(&main_screen);
    registerMemberReplication(&main_screen_control);

    player_info_list.push_back(this);
}

void PlayerInfo::reset()
{
    ship = {};
    main_screen_control = 0;
    last_ship_password = "";

    for(int n=0; n<max_crew_positions; n++)
        crew_position[n] = 0;
}

bool PlayerInfo::isOnlyMainScreen(int monitor_index)
{
    if (!(main_screen & (1 << monitor_index)))
        return false;
    for(int n=0; n<max_crew_positions; n++)
        if (crew_position[n] & (1 << monitor_index))
            return false;
    return true;
}

// Client-side functions to send a command to the server.
void PlayerInfo::commandTargetRotation(float target)
{
    sp::io::DataBuffer packet;
    packet << CMD_TARGET_ROTATION << target;
    sendClientCommand(packet);
}

void PlayerInfo::commandTurnSpeed(float turnSpeed)
{
    sp::io::DataBuffer packet;
    packet << CMD_TURN_SPEED << turnSpeed;
    sendClientCommand(packet);
}

void PlayerInfo::commandImpulse(float target)
{
    sp::io::DataBuffer packet;
    packet << CMD_IMPULSE << target;
    sendClientCommand(packet);
}

void PlayerInfo::commandWarp(int8_t target)
{
    sp::io::DataBuffer packet;
    packet << CMD_WARP << target;
    sendClientCommand(packet);
}

void PlayerInfo::commandJump(float distance)
{
    sp::io::DataBuffer packet;
    packet << CMD_JUMP << distance;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetTarget(sp::ecs::Entity target)
{
    sp::io::DataBuffer packet;
    if (target)
        packet << CMD_SET_TARGET << target;
    else
        packet << CMD_SET_TARGET << sp::ecs::Entity();
    sendClientCommand(packet);
}

void PlayerInfo::commandLoadTube(uint32_t tubeNumber, EMissileWeapons missileType)
{
    sp::io::DataBuffer packet;
    packet << CMD_LOAD_TUBE << tubeNumber << missileType;
    sendClientCommand(packet);
}

void PlayerInfo::commandUnloadTube(uint32_t tubeNumber)
{
    sp::io::DataBuffer packet;
    packet << CMD_UNLOAD_TUBE << tubeNumber;
    sendClientCommand(packet);
}

void PlayerInfo::commandFireTube(uint32_t tubeNumber, float missile_target_angle)
{
    sp::io::DataBuffer packet;
    packet << CMD_FIRE_TUBE << tubeNumber << missile_target_angle;
    sendClientCommand(packet);
}

void PlayerInfo::commandFireTubeAtTarget(uint32_t tubeNumber, sp::ecs::Entity target)
{
    float targetAngle = 0.0;
    auto missiletubes = my_spaceship.getComponent<MissileTubes>();

    if (!target || !missiletubes || tubeNumber >= missiletubes->mounts.size())
        return;

    targetAngle = MissileSystem::calculateFiringSolution(my_spaceship, missiletubes->mounts[tubeNumber], target);
    if (targetAngle == std::numeric_limits<float>::infinity()) {
        if (auto transform = my_spaceship.getComponent<sp::Transform>())
            targetAngle = transform->getRotation() + missiletubes->mounts[tubeNumber].direction;
    }

    commandFireTube(tubeNumber, targetAngle);
}

void PlayerInfo::commandSetShields(bool enabled)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_SHIELDS << enabled;
    sendClientCommand(packet);
}

void PlayerInfo::commandMainScreenSetting(MainScreenSetting mainScreen)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_MAIN_SCREEN_SETTING << mainScreen;
    sendClientCommand(packet);
}

void PlayerInfo::commandMainScreenOverlay(MainScreenOverlay mainScreen)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_MAIN_SCREEN_OVERLAY << mainScreen;
    sendClientCommand(packet);
}

void PlayerInfo::commandScan(sp::ecs::Entity object)
{
    sp::io::DataBuffer packet;
    packet << CMD_SCAN_OBJECT << object;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetSystemPowerRequest(ShipSystem::Type system, float power_request)
{
    sp::io::DataBuffer packet;
    auto sys = ShipSystem::get(my_spaceship, system);
    if (sys) sys->power_request = power_request;
    packet << CMD_SET_SYSTEM_POWER_REQUEST << system << power_request;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetSystemCoolantRequest(ShipSystem::Type system, float coolant_request)
{
    sp::io::DataBuffer packet;
    auto sys = ShipSystem::get(my_spaceship, system);
    if (sys) sys->coolant_request = coolant_request;
    packet << CMD_SET_SYSTEM_COOLANT_REQUEST << system << coolant_request;
    sendClientCommand(packet);
}

void PlayerInfo::commandDock(sp::ecs::Entity object)
{
    if (!object) return;
    sp::io::DataBuffer packet;
    packet << CMD_DOCK << object;
    sendClientCommand(packet);
}

void PlayerInfo::commandUndock()
{
    sp::io::DataBuffer packet;
    packet << CMD_UNDOCK;
    sendClientCommand(packet);
}

void PlayerInfo::commandAbortDock()
{
    sp::io::DataBuffer packet;
    packet << CMD_ABORT_DOCK;
    sendClientCommand(packet);
}

void PlayerInfo::commandOpenTextComm(sp::ecs::Entity obj)
{
    if (!obj) return;
    sp::io::DataBuffer packet;
    packet << CMD_OPEN_TEXT_COMM << obj;
    sendClientCommand(packet);
}

void PlayerInfo::commandCloseTextComm()
{
    sp::io::DataBuffer packet;
    packet << CMD_CLOSE_TEXT_COMM;
    sendClientCommand(packet);
}

void PlayerInfo::commandAnswerCommHail(bool awnser)
{
    sp::io::DataBuffer packet;
    packet << CMD_ANSWER_COMM_HAIL << awnser;
    sendClientCommand(packet);
}

void PlayerInfo::commandSendComm(uint8_t index)
{
    sp::io::DataBuffer packet;
    packet << CMD_SEND_TEXT_COMM << index;
    sendClientCommand(packet);
}

void PlayerInfo::commandSendCommPlayer(string message)
{
    sp::io::DataBuffer packet;
    packet << CMD_SEND_TEXT_COMM_PLAYER << message;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetAutoRepair(bool enabled)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_AUTO_REPAIR << enabled;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetBeamFrequency(int32_t frequency)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_BEAM_FREQUENCY << frequency;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetBeamSystemTarget(ShipSystem::Type system)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_BEAM_SYSTEM_TARGET << system;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetShieldFrequency(int32_t frequency)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_SHIELD_FREQUENCY << frequency;
    sendClientCommand(packet);
}

void PlayerInfo::commandAddWaypoint(glm::vec2 position)
{
    sp::io::DataBuffer packet;
    packet << CMD_ADD_WAYPOINT << position;
    sendClientCommand(packet);
}

void PlayerInfo::commandRemoveWaypoint(int32_t index)
{
    sp::io::DataBuffer packet;
    packet << CMD_REMOVE_WAYPOINT << index;
    sendClientCommand(packet);
}

void PlayerInfo::commandMoveWaypoint(int32_t index, glm::vec2 position)
{
    sp::io::DataBuffer packet;
    packet << CMD_MOVE_WAYPOINT << index << position;
    sendClientCommand(packet);
}

void PlayerInfo::commandActivateSelfDestruct()
{
    sp::io::DataBuffer packet;
    packet << CMD_ACTIVATE_SELF_DESTRUCT;
    sendClientCommand(packet);
}

void PlayerInfo::commandCancelSelfDestruct()
{
    sp::io::DataBuffer packet;
    packet << CMD_CANCEL_SELF_DESTRUCT;
    sendClientCommand(packet);
}

void PlayerInfo::commandConfirmDestructCode(int8_t index, uint32_t code)
{
    sp::io::DataBuffer packet;
    packet << CMD_CONFIRM_SELF_DESTRUCT << index << code;
    sendClientCommand(packet);
}

void PlayerInfo::commandCombatManeuverBoost(float amount)
{
    auto combat = my_spaceship.getComponent<CombatManeuveringThrusters>();
    if (!combat) return;
    combat->boost.request = amount;
    sp::io::DataBuffer packet;
    packet << CMD_COMBAT_MANEUVER_BOOST << amount;
    sendClientCommand(packet);
}

void PlayerInfo::commandCombatManeuverStrafe(float amount)
{
    auto combat = my_spaceship.getComponent<CombatManeuveringThrusters>();
    if (!combat) return;
    combat->strafe.request = amount;
    sp::io::DataBuffer packet;
    packet << CMD_COMBAT_MANEUVER_STRAFE << amount;
    sendClientCommand(packet);
}

void PlayerInfo::commandLaunchProbe(glm::vec2 target_position)
{
    sp::io::DataBuffer packet;
    packet << CMD_LAUNCH_PROBE << target_position;
    sendClientCommand(packet);
}

void PlayerInfo::commandScanDone()
{
    sp::io::DataBuffer packet;
    packet << CMD_SCAN_DONE;
    sendClientCommand(packet);
}

void PlayerInfo::commandScanCancel()
{
    sp::io::DataBuffer packet;
    packet << CMD_SCAN_CANCEL;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetAlertLevel(AlertLevel level)
{
    sp::io::DataBuffer packet;
    packet << CMD_SET_ALERT_LEVEL;
    packet << level;
    sendClientCommand(packet);
}

void PlayerInfo::commandHackingFinished(sp::ecs::Entity target, ShipSystem::Type target_system)
{
    sp::io::DataBuffer packet;
    packet << CMD_HACKING_FINISHED;
    packet << target;
    packet << target_system;
    sendClientCommand(packet);
}

void PlayerInfo::commandCustomFunction(string name)
{
    sp::io::DataBuffer packet;
    packet << CMD_CUSTOM_FUNCTION;
    packet << name;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetScienceLink(sp::ecs::Entity probe)
{
    sp::io::DataBuffer packet;

    // Pass the probe's multiplayer ID if the probe isn't nullptr.
    if (probe)
    {
        packet << CMD_SET_SCIENCE_LINK;
        packet << probe;
        sendClientCommand(packet);
    }
    // Otherwise, it's invalid. Warn and do nothing.
    else
    {
        LOG(WARNING) << "commandSetScienceLink received a null or invalid ScanProbe, so no command was sent.";
    }
}

void PlayerInfo::commandClearScienceLink()
{
    sp::io::DataBuffer packet;

    packet << CMD_SET_SCIENCE_LINK;
    packet << sp::ecs::Entity{};
    sendClientCommand(packet);
}

void PlayerInfo::commandSetCrewPosition(int monitor_index, ECrewPosition position, bool active)
{
    sp::io::DataBuffer packet;
    packet << CMD_UPDATE_CREW_POSITION << uint32_t(monitor_index) << int32_t(position) << active;
    sendClientCommand(packet);

    if (active)
        crew_position[position] |= (1 << monitor_index);
    else
        crew_position[position] &=~(1 << monitor_index);
}

void PlayerInfo::commandSetShip(sp::ecs::Entity entity)
{
    sp::io::DataBuffer packet;
    packet << CMD_UPDATE_SHIP_ID << entity;
    sendClientCommand(packet);
}

void PlayerInfo::commandSetMainScreen(int monitor_index, bool enabled)
{
    sp::io::DataBuffer packet;
    packet << CMD_UPDATE_MAIN_SCREEN << uint32_t(monitor_index) << enabled;
    sendClientCommand(packet);

    if (enabled)
        main_screen |= (1 << monitor_index);
    else
        main_screen &=~(1 << monitor_index);
}

void PlayerInfo::commandSetMainScreenControl(int monitor_index, bool control)
{
    sp::io::DataBuffer packet;
    packet << CMD_UPDATE_MAIN_SCREEN_CONTROL << uint32_t(monitor_index) << control;
    sendClientCommand(packet);

    if (control)
        main_screen_control |= (1 << monitor_index);
    else
        main_screen_control &=~(1 << monitor_index);
}

void PlayerInfo::commandSetName(const string& name)
{
    sp::io::DataBuffer packet;
    packet << CMD_UPDATE_NAME << name;
    sendClientCommand(packet);

    this->name = name;
}

void PlayerInfo::commandCrewSetTargetPosition(sp::ecs::Entity crew, glm::ivec2 position)
{
    sp::io::DataBuffer packet;
    packet << CMD_CREW_SET_TARGET << crew << position;
    sendClientCommand(packet);
}

void PlayerInfo::onReceiveClientCommand(int32_t client_id, sp::io::DataBuffer& packet)
{
    if (client_id != this->client_id) return;
    uint16_t command;
    uint32_t monitor_index;
    bool active;
    packet >> command;
    switch(command)
    {
    case CMD_TARGET_ROTATION:{
        float f;
        packet >> f;
        auto thrusters = my_spaceship.getComponent<ManeuveringThrusters>();
        if (thrusters) { thrusters->stop(); thrusters->target = f; }
        }break;
    case CMD_TURN_SPEED:{
        float f;
        packet >> f;
        auto thrusters = my_spaceship.getComponent<ManeuveringThrusters>();
        if (thrusters) { thrusters->stop(); thrusters->rotation_request = f; }
        }break;
    case CMD_IMPULSE:{
        auto engine = my_spaceship.getComponent<ImpulseEngine>();
        if (engine)
            packet >> engine->request;
        else {
            float f;
            packet >> f;
        }
        } break;
    case CMD_WARP:{
        auto warp = my_spaceship.getComponent<WarpDrive>();
        if (warp)
            packet >> warp->request;
        else {
            uint8_t i;
            packet >> i;
        }
        } break;
    case CMD_JUMP:
        {
            float distance;
            packet >> distance;
            JumpSystem::initializeJump(my_spaceship, distance);
        }
        break;
    case CMD_SET_TARGET:
        {
            sp::ecs::Entity target;
            packet >> target;
            my_spaceship.getOrAddComponent<Target>().entity = target;
        }
        break;
    case CMD_LOAD_TUBE:
        {
            uint32_t tube_nr;
            EMissileWeapons type;
            packet >> tube_nr >> type;

            auto missiletubes = my_spaceship.getComponent<MissileTubes>();
            if (missiletubes && tube_nr < missiletubes->mounts.size())
                MissileSystem::startLoad(my_spaceship, missiletubes->mounts[tube_nr], type);
        }
        break;
    case CMD_UNLOAD_TUBE:
        {
            uint32_t tube_nr;
            packet >> tube_nr;

            auto missiletubes = my_spaceship.getComponent<MissileTubes>();
            if (missiletubes && tube_nr < missiletubes->mounts.size())
                MissileSystem::startUnload(my_spaceship, missiletubes->mounts[tube_nr]);
        }
        break;
    case CMD_FIRE_TUBE:
        {
            uint32_t tube_nr;
            float missile_target_angle;
            packet >> tube_nr >> missile_target_angle;

            auto missiletubes = my_spaceship.getComponent<MissileTubes>();
            if (missiletubes && tube_nr < missiletubes->mounts.size()) {
                sp::ecs::Entity target;
                if (auto t = my_spaceship.getComponent<Target>())
                    target = t->entity;
                MissileSystem::fire(my_spaceship, missiletubes->mounts[tube_nr], missile_target_angle, target);
            }
        }
        break;
    case CMD_SET_SHIELDS:
        {
            bool active;
            packet >> active;

            auto shields = my_spaceship.getComponent<Shields>();
            if (shields) {
                if (shields->calibration_delay <= 0.0f && active != shields->active)
                {
                    shields->active = active;
                    /*TODO if (active)
                        playSoundOnMainScreen("sfx/shield_up.wav");
                    else
                        playSoundOnMainScreen("sfx/shield_down.wav");
                    */
                }
            }
        }
        break;
    case CMD_SET_MAIN_SCREEN_SETTING:{
        MainScreenSetting mss;
        packet >> mss;
        if (auto pc = my_spaceship.getComponent<PlayerControl>())
            pc->main_screen_setting = mss;
        }break;
    case CMD_SET_MAIN_SCREEN_OVERLAY:{
        MainScreenOverlay mso;
        packet >> mso;
        if (auto pc = my_spaceship.getComponent<PlayerControl>())
            pc->main_screen_overlay = mso;
        }break;
        break;
    case CMD_SCAN_OBJECT:
        {
            sp::ecs::Entity e;
            packet >> e;

            if (auto scanner = my_spaceship.getComponent<ScienceScanner>())
            {
                scanner->delay = scanner->max_scanning_delay;
                scanner->target = e;
            }
        }
        break;
    case CMD_SCAN_DONE:
        ScanningSystem::scanningFinished(my_spaceship);
        break;
    case CMD_SCAN_CANCEL:
        if (auto ss = my_spaceship.getComponent<ScienceScanner>()) {
            ss->target = {};
        }
        break;
    case CMD_SET_SYSTEM_POWER_REQUEST:
        {
            ShipSystem::Type system;
            float request;
            packet >> system >> request;
            auto sys = ShipSystem::get(my_spaceship, system);
            if (sys && request >= 0.0f && request <= 3.0f)
                sys->power_request = request;
        }
        break;
    case CMD_SET_SYSTEM_COOLANT_REQUEST:
        {
            ShipSystem::Type system;
            float request;
            packet >> system >> request;

            auto coolant = my_spaceship.getComponent<Coolant>();
            if (coolant) {
                request = std::clamp(request, 0.0f, std::min(coolant->max_coolant_per_system, coolant->max));
                auto sys = ShipSystem::get(my_spaceship, system);
                if (sys)
                    sys->coolant_request = request;
            }
        }
        break;
    case CMD_DOCK:
        {
            int32_t id;
            sp::ecs::Entity target;
            packet >> id >> target;
            if (target)
                DockingSystem::requestDock(my_spaceship, target);
        }
        break;
    case CMD_UNDOCK:
        DockingSystem::requestUndock(my_spaceship);
        break;
    case CMD_ABORT_DOCK:
        DockingSystem::abortDock(my_spaceship);
        break;
    case CMD_OPEN_TEXT_COMM:
        {
            sp::ecs::Entity target;
            packet >> target;
            CommsSystem::openTo(my_spaceship, target);
        }
        break;
    case CMD_CLOSE_TEXT_COMM:
        CommsSystem::close(my_spaceship);
        break;
    case CMD_ANSWER_COMM_HAIL: 
        {
            bool answer = false;
            packet >> answer;
            CommsSystem::answer(my_spaceship, answer);
        }
        break;
    case CMD_SEND_TEXT_COMM:
        {
            uint8_t index;
            packet >> index;
            CommsSystem::selectScriptReply(my_spaceship, index);
        }
        break;
    case CMD_SEND_TEXT_COMM_PLAYER:
        {
            string message;
            packet >> message;

            CommsSystem::textReply(my_spaceship, message);
        }
        break;
    case CMD_SET_AUTO_REPAIR:
        {
            bool auto_repair_enabled = false;
            packet >> auto_repair_enabled;
        }
        break;
    case CMD_SET_BEAM_FREQUENCY:
        {
            int32_t new_frequency;
            packet >> new_frequency;
            auto beamweapons = my_spaceship.getComponent<BeamWeaponSys>();
            if (beamweapons)
                beamweapons->frequency = std::clamp(new_frequency, 0, BeamWeaponSys::max_frequency);
        }
        break;
    case CMD_SET_BEAM_SYSTEM_TARGET:
        {
            ShipSystem::Type system;
            packet >> system;
            auto beamweapons = my_spaceship.getComponent<BeamWeaponSys>();
            if (beamweapons)
                beamweapons->system_target = (ShipSystem::Type)std::clamp((int)system, 0, (int)(ShipSystem::COUNT - 1));
        }
        break;
    case CMD_SET_SHIELD_FREQUENCY:
        {
            int32_t new_frequency;
            packet >> new_frequency;
            auto shields = my_spaceship.getComponent<Shields>();
            if (shields && shields->calibration_delay <= 0.0f && new_frequency != shields->frequency)
            {
                shields->frequency = new_frequency;
                shields->calibration_delay = shields->calibration_time;
                shields->active = false;
                if (shields->frequency < 0)
                    shields->frequency = 0;
                if (shields->frequency > BeamWeaponSys::max_frequency)
                    shields->frequency = BeamWeaponSys::max_frequency;
            }
        }
        break;
    case CMD_ADD_WAYPOINT:
        {
            glm::vec2 position{};
            packet >> position;
            auto lrr = my_spaceship.getComponent<LongRangeRadar>();
            if (lrr && lrr->waypoints.size() < 9)
                lrr->waypoints.push_back(position);
        }
        break;
    case CMD_REMOVE_WAYPOINT:
        {
            int32_t index;
            packet >> index;
            auto lrr = my_spaceship.getComponent<LongRangeRadar>();
            if (lrr && index >= 0 && index < int(lrr->waypoints.size()))
                lrr->waypoints.erase(lrr->waypoints.begin() + index);
        }
        break;
    case CMD_MOVE_WAYPOINT:
        {
            int32_t index;
            glm::vec2 position{};
            packet >> index >> position;
            auto lrr = my_spaceship.getComponent<LongRangeRadar>();
            if (lrr && index >= 0 && index < int(lrr->waypoints.size()))
                lrr->waypoints[index] = position;
        }
        break;
    case CMD_ACTIVATE_SELF_DESTRUCT:
        SelfDestructSystem::activate(my_spaceship);
        break;
    case CMD_CANCEL_SELF_DESTRUCT:
        if (auto self_destruct = my_spaceship.getComponent<SelfDestruct>()) {
            if (self_destruct->countdown <= 0.0f) {
                self_destruct->active = false;
            }
        }
        break;
    case CMD_CONFIRM_SELF_DESTRUCT:
        {
            int8_t index;
            uint32_t code;
            packet >> index >> code;
            if (auto self_destruct = my_spaceship.getComponent<SelfDestruct>()) {
                if (index >= 0 && index < SelfDestruct::max_codes && self_destruct->code[index] == code && self_destruct->active)
                    self_destruct->confirmed[index] = true;
            }
        }
        break;
    case CMD_COMBAT_MANEUVER_BOOST:
        {
            float request_amount;
            packet >> request_amount;
            auto combat = my_spaceship.getComponent<CombatManeuveringThrusters>();
            if (combat)
                combat->boost.request = request_amount;
        }
        break;
    case CMD_COMBAT_MANEUVER_STRAFE:
        {
            float request_amount;
            packet >> request_amount;
            auto combat = my_spaceship.getComponent<CombatManeuveringThrusters>();
            if (combat)
                combat->strafe.request = request_amount;
        }
        break;
    case CMD_LAUNCH_PROBE:
        if (auto spl = my_spaceship.getComponent<ScanProbeLauncher>())
        {
            auto t = my_spaceship.getComponent<sp::Transform>();
            if (t && spl->stock > 0) {
                glm::vec2 target{};
                packet >> target;

                auto p = sp::ecs::Entity::create();
                p.addComponent<sp::Transform>(*t);
                p.addComponent<LifeTime>().lifetime = 60*10;
                if (auto faction = my_spaceship.getComponent<Faction>())
                    p.addComponent<Faction>() = *faction;
                auto& mt = p.addComponent<MoveTo>();
                mt.target = target;
                mt.speed = 1000;
                p.addComponent<AllowRadarLink>().owner = my_spaceship;
                //TODO: setRadarSignatureInfo(0.0, 0.2, 0.0);
                //TODO: setCallSign(string(getMultiplayerId()) + "P");
                auto& trace = p.addComponent<RadarTrace>();
                trace.icon = "radar/probe.png";
                trace.min_size = 10.0;
                trace.max_size = 10.0;
                trace.color = {96, 192, 128, 255};
                trace.flags = RadarTrace::LongRange;
                auto& hull = p.addComponent<Hull>();
                hull.current = hull.max = 1;
                p.addComponent<ShareShortRangeRadar>();
                auto model = "SensorBuoyMKI";
                auto idx = irandom(1, 3);
                if (idx == 2) model = "SensorBuoyMKII";
                if (idx == 3) model = "SensorBuoyMKIII";
                /*TODO 3D model
                for k, v in pairs(__model_data[model]) do
                    if string.sub(1, 2) ~= "__" then
                        e[k] = table.deepcopy(v)
                    end
                end
                */
                if (spl->on_launch)
                    LuaConsole::checkResult(spl->on_launch.call<void>(my_spaceship, p));
                spl->stock--;
            }
        }
        break;
    case CMD_SET_ALERT_LEVEL:{
        AlertLevel al;
        packet >> al;
        if (auto ps = my_spaceship.getComponent<PlayerControl>())
            ps->alert_level = al;
        }break;
    case CMD_SET_SCIENCE_LINK:
        {
            // Capture previously linked probe, if there is one.
            sp::ecs::Entity target;
            packet >> target;

            // TODO: Check if this probe is ours
            if (auto lrr = my_spaceship.getComponent<LongRangeRadar>()) {
                auto old = lrr->radar_view_linked_entity;
                if (lrr->on_probe_link && target)
                    LuaConsole::checkResult(lrr->on_probe_link.call<void>(my_spaceship, target));
                lrr->radar_view_linked_entity = target;
                if (lrr->on_probe_unlink && old)
                    LuaConsole::checkResult(lrr->on_probe_unlink.call<void>(my_spaceship, old));
            }
        }
        break;
    case CMD_HACKING_FINISHED:
        {
            sp::ecs::Entity target;
            ShipSystem::Type target_system;
            packet >> target >> target_system;
            auto sys = ShipSystem::get(target, target_system);
            if (sys)
                sys->hacked_level = std::min(1.0f, sys->hacked_level + 0.5f);
        }
        break;
    case CMD_CUSTOM_FUNCTION:
        {
            string name;
            packet >> name;
            if (auto csf = my_spaceship.getComponent<CustomShipFunctions>()) {
                for(auto& f : csf->functions)
                {
                    if (f.name == name)
                    {
                        if (f.type == CustomShipFunctions::Function::Type::Button)
                        {
                            auto cb = f.callback;
                            cb.call<void>();
                        }
                        else if (f.type == CustomShipFunctions::Function::Type::Message)
                        {
                            auto cb = f.callback;
                            cb.call<void>();
                            csf->functions.erase(std::remove_if(csf->functions.begin(), csf->functions.end(), [name](const CustomShipFunctions::Function& f) { return f.name == name; }), csf->functions.end());
                        }
                        break;
                    }
                }
            }
        }
        break;

    case CMD_UPDATE_CREW_POSITION:
        {
            int32_t position;
            packet >> monitor_index >> position >> active;
            if (active)
                crew_position[position] |= (1 << monitor_index);
            else
                crew_position[position] &=~(1 << monitor_index);
        }
        break;
    case CMD_UPDATE_SHIP_ID:
        packet >> ship;
        break;
    case CMD_UPDATE_MAIN_SCREEN:
        packet >> monitor_index >> active;
        if (active)
            main_screen |= (1 << monitor_index);
        else
            main_screen &=~(1 << monitor_index);
        break;
    case CMD_UPDATE_MAIN_SCREEN_CONTROL:
        packet >> monitor_index >> active;
        if (active)
            main_screen_control |= (1 << monitor_index);
        else
            main_screen_control &=~(1 << monitor_index);
        break;
    case CMD_UPDATE_NAME:
        packet >> name;
        break;

    case CMD_CREW_SET_TARGET:{
            auto [crew, position] = packet.read<sp::ecs::Entity, glm::ivec2>();
            if (auto ic = crew.getComponent<InternalCrew>())
                ic->target_position = position;
        }break;
    }
}

void PlayerInfo::spawnUI(int monitor_index, RenderLayer* render_layer)
{
    if (my_player_info->isOnlyMainScreen(monitor_index))
    {
        new ScreenMainScreen(render_layer);
    }
    else
    {
        CrewStationScreen* screen = new CrewStationScreen(render_layer, bool(main_screen & (1 << monitor_index)));
        auto container = screen->getTabContainer();

        //Crew 6/5
        if (crew_position[helmsOfficer] & (1 << monitor_index))
            screen->addStationTab(new HelmsScreen(container), helmsOfficer, getCrewPositionName(helmsOfficer), getCrewPositionIcon(helmsOfficer));
        if (crew_position[weaponsOfficer] & (1 << monitor_index))
            screen->addStationTab(new WeaponsScreen(container), weaponsOfficer, getCrewPositionName(weaponsOfficer), getCrewPositionIcon(weaponsOfficer));
        if (crew_position[engineering] & (1 << monitor_index))
            screen->addStationTab(new EngineeringScreen(container), engineering, getCrewPositionName(engineering), getCrewPositionIcon(engineering));
        if (crew_position[scienceOfficer] & (1 << monitor_index))
            screen->addStationTab(new ScienceScreen(container), scienceOfficer, getCrewPositionName(scienceOfficer), getCrewPositionIcon(scienceOfficer));
        if (crew_position[relayOfficer] & (1 << monitor_index))
            screen->addStationTab(new RelayScreen(container, true), relayOfficer, getCrewPositionName(relayOfficer), getCrewPositionIcon(relayOfficer));

        //Crew 4/3
        if (crew_position[tacticalOfficer] & (1 << monitor_index))
            screen->addStationTab(new TacticalScreen(container), tacticalOfficer, getCrewPositionName(tacticalOfficer), getCrewPositionIcon(tacticalOfficer));
        if (crew_position[engineeringAdvanced] & (1 << monitor_index))
            screen->addStationTab(new EngineeringAdvancedScreen(container), engineeringAdvanced, getCrewPositionName(engineeringAdvanced), getCrewPositionIcon(engineeringAdvanced));
        if (crew_position[operationsOfficer] & (1 << monitor_index))
            screen->addStationTab(new OperationScreen(container), operationsOfficer, getCrewPositionName(operationsOfficer), getCrewPositionIcon(operationsOfficer));

        //Crew 1
        if (crew_position[singlePilot] & (1 << monitor_index))
            screen->addStationTab(new SinglePilotScreen(container), singlePilot, getCrewPositionName(singlePilot), getCrewPositionIcon(singlePilot));

        //Extra
        if (crew_position[damageControl] & (1 << monitor_index))
            screen->addStationTab(new DamageControlScreen(container), damageControl, getCrewPositionName(damageControl), getCrewPositionIcon(damageControl));
        if (crew_position[powerManagement] & (1 << monitor_index))
            screen->addStationTab(new PowerManagementScreen(container), powerManagement, getCrewPositionName(powerManagement), getCrewPositionIcon(powerManagement));
        if (crew_position[databaseView] & (1 << monitor_index))
            screen->addStationTab(new DatabaseScreen(container), databaseView, getCrewPositionName(databaseView), getCrewPositionIcon(databaseView));
        if (crew_position[altRelay] & (1 << monitor_index))
            screen->addStationTab(new RelayScreen(container, false), altRelay, getCrewPositionName(altRelay), getCrewPositionIcon(altRelay));
        if (crew_position[commsOnly] & (1 << monitor_index))
            screen->addStationTab(new CommsScreen(container), commsOnly, getCrewPositionName(commsOnly), getCrewPositionIcon(commsOnly));
        if (crew_position[shipLog] & (1 << monitor_index))
            screen->addStationTab(new ShipLogScreen(container), shipLog, getCrewPositionName(shipLog), getCrewPositionIcon(shipLog));

        GuiSelfDestructEntry* sde = new GuiSelfDestructEntry(container, "SELF_DESTRUCT_ENTRY");
        for(int n=0; n<max_crew_positions; n++)
            if (crew_position[n] & (1 << monitor_index))
                sde->enablePosition(ECrewPosition(n));
        if (crew_position[tacticalOfficer] & (1 << monitor_index))
        {
            sde->enablePosition(weaponsOfficer);
            sde->enablePosition(helmsOfficer);
        }
        if (crew_position[engineeringAdvanced] & (1 << monitor_index))
        {
            sde->enablePosition(engineering);
        }
        if (crew_position[operationsOfficer] & (1 << monitor_index))
        {
            sde->enablePosition(scienceOfficer);
            sde->enablePosition(relayOfficer);
        }

        if (main_screen_control & (1 << monitor_index))
            new GuiMainScreenControls(container);

        screen->finishCreation();
    }
}

string getCrewPositionName(ECrewPosition position)
{
    switch(position)
    {
    case helmsOfficer: return tr("station","Helms");
    case weaponsOfficer: return tr("station","Weapons");
    case engineering: return tr("station","Engineering");
    case scienceOfficer: return tr("station","Science");
    case relayOfficer: return tr("station","Relay");
    case tacticalOfficer: return tr("station","Tactical");
    case engineeringAdvanced: return tr("station","Engineering+");
    case operationsOfficer: return tr("station","Operations");
    case singlePilot: return tr("station","Single Pilot");
    case damageControl: return tr("station","Damage Control");
    case powerManagement: return tr("station","Power Management");
    case databaseView: return tr("station","Database");
    case altRelay: return tr("station","Strategic Map");
    case commsOnly: return tr("station","Comms");
    case shipLog: return tr("station","Ship's Log");
    default: return "ErrUnk: " + string(position);
    }
}

string getCrewPositionIcon(ECrewPosition position)
{
    switch(position)
    {
    case helmsOfficer: return "gui/icons/station-helm";
    case weaponsOfficer: return "gui/icons/station-weapons";
    case engineering: return "gui/icons/station-engineering";
    case scienceOfficer: return "gui/icons/station-science";
    case relayOfficer: return "gui/icons/station-relay";
    case tacticalOfficer: return "";
    case engineeringAdvanced: return "";
    case operationsOfficer: return "";
    case singlePilot: return "";
    case damageControl: return "";
    case powerManagement: return "";
    case databaseView: return "";
    case altRelay: return "";
    case commsOnly: return "";
    case shipLog: return "";
    default: return "ErrUnk: " + string(position);
    }
}

/* Define script conversion function for the ECrewPosition enum. */
template<> void convert<ECrewPosition>::param(lua_State* L, int& idx, ECrewPosition& cp)
{
    string str = string(luaL_checkstring(L, idx++)).lower();

    //6/5 player crew
    if (str == "helms" || str == "helmsofficer")
        cp = helmsOfficer;
    else if (str == "weapons" || str == "weaponsofficer")
        cp = weaponsOfficer;
    else if (str == "engineering" || str == "engineeringsofficer")
        cp = engineering;
    else if (str == "science" || str == "scienceofficer")
        cp = scienceOfficer;
    else if (str == "relay" || str == "relayofficer")
        cp = relayOfficer;

    //4/3 player crew
    else if (str == "tactical" || str == "tacticalofficer")
        cp = tacticalOfficer;    //helms+weapons-shields
    else if (str == "engineering+" || str == "engineering+officer" || str == "engineeringadvanced" || str == "engineeringadvancedofficer")
        cp = engineeringAdvanced;//engineering+shields
    else if (str == "operations" || str == "operationsofficer")
        cp = operationsOfficer; //science+comms

    //1 player crew
    else if (str == "single" || str == "singlepilot")
        cp = singlePilot;

    //extras
    else if (str == "damagecontrol")
        cp = damageControl;
    else if (str == "powermanagement")
        cp = powerManagement;
    else if (str == "database" || str == "databaseview")
        cp = databaseView;
    else if (str == "altrelay")
        cp = altRelay;
    else if (str == "commsonly")
        cp = commsOnly;
    else if (str == "shiplog")
        cp = shipLog;
    else
        luaL_error(L, "Unknown value for crew position: %s", str.c_str());
}

bool PlayerInfo::hasPlayerAtPosition(sp::ecs::Entity entity, ECrewPosition position)
{
    foreach(PlayerInfo, i, player_info_list)
    {
        if (i->ship == entity && i->crew_position[position])
        {
            return true;
        }
    }
    return false;
}