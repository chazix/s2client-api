#include "test_sendmapcommand.h"
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"

#include <iostream>

namespace sc2 {
    static const unsigned CommandStepCount = 500;

    struct CommandInfo {
        CommandInfo(SC2APIProtocol::RequestMapCommand::CommandChoiceCase choice, const std::string& cmdid = std::string()) :
            m_choice(choice), m_cmdid(cmdid) {
        }

        SC2APIProtocol::RequestMapCommand::CommandChoiceCase m_choice;
        std::string m_cmdid;
    };

    class MarineMicroBot : public Agent {
    public:
        MarineMicroBot(Coordinator& coord) : Agent(&coord), m_coordinator(coord) {
            m_commands = {
                CommandInfo(SC2APIProtocol::RequestMapCommand::kRestartGame),
            };
        }
        virtual void OnGameStart();
        virtual void OnStep() final;
        virtual void OnUnitDestroyed(const Unit* unit) override;
        virtual void OnGameEnd() override;

        int& GetNumMultiplayerRestarts() { return multiPlayerRestarts; }

    private:
        bool GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D& position);
        bool GetNearestZergling(const Point2D& from);

        const Unit* targeted_zergling_;
        bool move_back_;
        Point2D backup_target_;
        Point2D backup_start_;
        int multiPlayerRestarts{ 0 };
        unsigned m_numSteps{ 0 };
        unsigned m_commandIndex{ 0 };
        Coordinator& m_coordinator;
        std::vector<CommandInfo> m_commands;
    };

    void MarineMicroBot::OnGameStart() {
        move_back_ = false;
        targeted_zergling_ = 0;
    }

    void MarineMicroBot::OnStep() {
        const ObservationInterface* observation = Observation();
        ActionInterface* action = Actions();

        Point2D mp, zp;

        if (!GetPosition(UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, mp)) {
            return;
        }

        if (!GetPosition(UNIT_TYPEID::ZERG_ZERGLING, Unit::Alliance::Enemy, zp)) {
            return;
        }

        if (!GetNearestZergling(mp)) {
            return;
        }

        Units units = observation->GetUnits(Unit::Alliance::Self);
        for (const auto& u : units) {
            switch (static_cast<UNIT_TYPEID>(u->unit_type)) {
            case UNIT_TYPEID::TERRAN_MARINE: {
                if (!move_back_) {
                    action->UnitCommand(u, ABILITY_ID::ATTACK, targeted_zergling_);
                }
                else {
                    if (Distance2D(mp, backup_target_) < 1.5f) {
                        move_back_ = false;
                    }

                    action->UnitCommand(u, ABILITY_ID::SMART, backup_target_);
                }
                break;
            }
            default: {
                break;
            }
            }
        }

        if (++m_numSteps == CommandStepCount) {
            const CommandInfo& cmdinfo = m_commands[m_commandIndex];
            //const google::protobuf::Descriptor* mapCmdDiscriptor = SC2APIProtocol::RequestMapCommand::default_instance().GetDescriptor();
            //const std::string& commandChoiceName = mapCmdDiscriptor->oneof_decl(cmdinfo.m_choice)->field()->name();
            std::cout << "Sending MapCommand: (choice: " << "---" << "), (cmdid: " << cmdinfo.m_cmdid << ")" << std::endl;
            m_coordinator.SendMapCommand(cmdinfo.m_choice, cmdinfo.m_cmdid);
            m_commandIndex = (m_commandIndex + 1) % m_commands.size();
            m_numSteps = 0;
        }
    }

    void MarineMicroBot::OnUnitDestroyed(const Unit* unit) {
        if (unit == targeted_zergling_) {
            Point2D mp, zp;
            if (!GetPosition(UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, mp)) {
                return;
            }

            if (!GetPosition(UNIT_TYPEID::ZERG_ZERGLING, Unit::Alliance::Enemy, zp)) {
                return;
            }

            Vector2D diff = mp - zp;
            Normalize2D(diff);

            targeted_zergling_ = 0;
            move_back_ = true;
            backup_start_ = mp;
            backup_target_ = mp + diff * 3.0f;
        }
    }

    void MarineMicroBot::OnGameEnd() {
        //++multiPlayerRestarts;
        //std::cout << "Restart test: " << std::to_string(multiPlayerRestarts) << " of " <<
            //std::to_string(NumRestartsToTest) << " complete." << std::endl;
    }

    bool MarineMicroBot::GetPosition(UNIT_TYPEID unit_type, Unit::Alliance alliace, Point2D& position) {
        const ObservationInterface* observation = Observation();
        Units units = observation->GetUnits(alliace);

        if (units.empty()) {
            return false;
        }

        position = Point2D(0.0f, 0.0f);
        unsigned int count = 0;

        for (const auto& u : units) {
            if (u->unit_type == unit_type) {
                position += u->pos;
                ++count;
            }
        }

        position /= (float)count;

        return true;
    }

    bool MarineMicroBot::GetNearestZergling(const Point2D& from) {
        const ObservationInterface* observation = Observation();
        Units units = observation->GetUnits(Unit::Alliance::Enemy);

        if (units.empty()) {
            return false;
        }

        float distance = std::numeric_limits<float>::max();
        for (const auto& u : units) {
            if (u->unit_type == UNIT_TYPEID::ZERG_ZERGLING) {
                float d = DistanceSquared2D(u->pos, from);
                if (d < distance) {
                    distance = d;
                    targeted_zergling_ = u;
                }
            }
        }

        return true;
    }

    class SendCommandBot : public Agent {
    public:
        SendCommandBot(Coordinator& coordinator) : Agent(&coordinator), m_coordinator(coordinator) {
            m_commands = {
                CommandInfo(SC2APIProtocol::RequestMapCommand::kRestartGame),
            };
        }

        virtual void OnStep() {
            if (++m_numSteps == CommandStepCount) {
                const CommandInfo& cmdinfo = m_commands[m_commandIndex];
                //const google::protobuf::Descriptor* mapCmdDiscriptor = SC2APIProtocol::RequestMapCommand::default_instance().GetDescriptor();
                //const std::string& commandChoiceName = mapCmdDiscriptor->oneof_decl(cmdinfo.m_choice)->field()->name();
                std::cout << "Sending MapCommand: (choice: " << "---" << "), (cmdid: " << cmdinfo.m_cmdid << ")" << std::endl;
                m_coordinator.SendMapCommand(cmdinfo.m_choice, cmdinfo.m_cmdid);
                m_commandIndex = (m_commandIndex + 1) % m_commands.size();
                m_numSteps = 0;
            }
        }

        /*virtual void OnGameEnd() {
            std::cout << "Sending MapCommand: (choice: " << "---" << "), (cmdid: n/a)" << std::endl;
            m_coordinator.SendMapCommand(SC2APIProtocol::RequestMapCommand::kRestartGame);
        }*/

    private:
        unsigned m_numSteps{0};
        unsigned m_commandIndex{0};
        Coordinator& m_coordinator;
        std::vector<CommandInfo> m_commands;
    };

    void CoordinatorOnGameEnd(Coordinator* coordinator) {
        //const google::protobuf::Descriptor* mapCmdDiscriptor = SC2APIProtocol::RequestMapCommand::default_instance().GetDescriptor();
        //const std::string& commandChoiceName = mapCmdDiscriptor->oneof_decl(cmdinfo.m_choice)->field()->name();
        std::cout << "Sending MapCommand: (choice: " << "---" << "), (cmdid: n/a)" << std::endl;
        coordinator->SendMapCommand(SC2APIProtocol::RequestMapCommand::kRestartGame);
    }

    bool TestSendMapCommand(int argc, char** argv) {
        Coordinator coordinator;
        if (!coordinator.LoadSettings(argc, argv)) {
            return false;
        }

        coordinator.RegisterOnGameEndCallback(CoordinatorOnGameEnd);
        coordinator.SetRealtime(true);
        coordinator.SetMultithreaded(true);
        MarineMicroBot mapCommandBot(coordinator);
        Agent nothingBot(&coordinator);
        coordinator.SetParticipants({
            CreateParticipant(sc2::Race::Terran, &mapCommandBot),
            CreateParticipant(sc2::Race::Zerg, &nothingBot),
        });

        coordinator.LaunchStarcraft();
        coordinator.StartGame(sc2::kMapFastRestartMultiplayer);

        while (true) {
            coordinator.Update();
            continue;
        }

        return true;
    }
}